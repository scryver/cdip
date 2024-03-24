typedef struct BoundBlock
{
    usze last      : 1;
    usze used      : 1;
    usze prevUsed  : 1;
    usze padding   : 5;
    usze size      : (MEM_SIZE_BITS - 8);
} BoundBlock;

typedef struct BoundFooter
{
    usze padding   : 8;
    usze size      : (MEM_SIZE_BITS - 8);
    BoundBlock *prev;
    BoundBlock *next;
} BoundFooter;

typedef struct BoundMemory
{
    BoundBlock *blocks[16];
} BoundMemory;

#if MEM_SIZE_BITS == 32
#define BOUND_MIN_SIZE    8
#elif MEM_SIZE_BITS == 64
#define BOUND_MIN_SIZE    16
#endif
#define BOUND_SIZE_MASK   ((IMM_USIZE(1) << (MEM_SIZE_BITS - 8)) - 1)

compile_assert(offsetof(BoundFooter, prev) == sizeof(BoundBlock));

#define bound_create(...)                     bound_createx(__VA_ARGS__, bound_create4, bound_create3, bound_create2)(__VA_ARGS__)
#define bound_createx(a, b, c, d, e, ...)     e
#define bound_create2(b, T)                   (T *)bound_alloc(b, sizeof(T), alignof(T), 1, 0)
#define bound_create3(b, T, c)                (T *)bound_alloc(b, sizeof(T), alignof(T), c, 0)
#define bound_create4(b, T, c, f)             (T *)bound_alloc(b, sizeof(T), alignof(T), c, f)

func BoundMemory create_bound_memory(sze capacity);

func void *bound_dealloc(BoundMemory *memory, void *ptr, sze size);
#if COMPILER_LLVM || COMPILER_GCC
//__attribute__((malloc, malloc(bound_dealloc, 2), alloc_size(2, 4), alloc_align(3)))
__attribute__((malloc, alloc_size(2, 4), alloc_align(3)))
#endif
func void *bound_alloc(BoundMemory *memory, sze size, sze align, sze count, u32 flags);
func void *bound_realloc(BoundMemory *memory, void *ptr, sze oldSize, sze newSize, sze align, u32 flags);

func BoundBlock *bound_next_block(BoundBlock *block)
{
    BoundBlock *result = 0;
    if (!block->last) {
        result = (BoundBlock *)((byte *)block + sizeof(BoundBlock) + block->size);
    }
    return result;
}

func BoundBlock *bound_prev_free_block(BoundBlock *block)
{
    BoundBlock *result = 0;
    if (!block->prevUsed) {
        BoundFooter *footer = (BoundFooter *)block - 1;
        result = (BoundBlock *)((byte *)block - (sizeof(BoundBlock) + footer->size));
    }
    return result;
}

func BoundBlock *bound_split_block(BoundBlock *block, sze size)
{
    BoundBlock *result = 0;

    sze remaining = (sze)block->size - size;
    if (remaining >= sizeof(BoundFooter))
    {
        result = (BoundBlock *)((byte *)block + sizeof(BoundBlock) + size);
        result->last = block->last;
        result->used = false;
        result->prevUsed = block->used;
        result->size = (usze)(remaining - sizeof(BoundBlock)) & BOUND_SIZE_MASK;

        BoundBlock *nextBlock = bound_next_block(result);
        if (nextBlock) {
            nextBlock->prevUsed = result->used;
        }

        block->last = false;
        block->size = (usze)size & BOUND_SIZE_MASK;
    }

    return result;
}

func BoundBlock *bound_merge_blocks(BoundBlock *left, BoundBlock *right)
{
    usze extraSize = sizeof(BoundBlock) + right->size;
    left->size = (left->size + extraSize) & BOUND_SIZE_MASK;
    left->last = right->last;

    BoundBlock *nextBlock = bound_next_block(left);
    if (nextBlock) {
        nextBlock->prevUsed = left->used;
    }

    return left;
}

func BoundBlock *bound_next_linked_block(BoundBlock *block)
{
    BoundFooter *footer = (BoundFooter *)((byte *)block + sizeof(BoundBlock) + block->size) - 1;
    return footer->next;
}

func BoundBlock *bound_prev_linked_block(BoundBlock *block)
{
    BoundFooter *footer = (BoundFooter *)((byte *)block + sizeof(BoundBlock) + block->size) - 1;
    return footer->prev;
}

func void bound_set_linked_blocks(BoundBlock *block, BoundBlock *prev, BoundBlock *next)
{
    BoundFooter *footer = (BoundFooter *)((byte *)block + sizeof(BoundBlock) + block->size) - 1;
    footer->size = block->size;
    footer->prev = prev;
    footer->next = next;
}

func BoundBlock *bound_find_best_fit_block(BoundBlock *list, sze size)
{
    assert(size >= 0);
    BoundBlock *result = 0;
    usze bestSize = USZE_MAX;

    while (list && (bestSize != (usze)size))
    {
        if ((list->size >= (usze)size) && (list->size < bestSize))
        {
            result = list;
            bestSize = list->size;
        }
        list = bound_next_linked_block(list);
    }

    return result;
}

func i32 bound_size_index(sze size)
{
    i32 result = 0;
#if MEM_SIZE_BITS == 32
    if (size < 64) { result = (i32)((size - 8) >> 2); }
    else if (size < 256) { result = 14; }
#else
    if (size < 128) { result = (i32)((size - 16) >> 3); }
    else if (size < 1024) { result = 14; }
#endif
    else { result = 15; }

    return result;
}

func void bound_remove_linked_block(BoundMemory *memory, BoundBlock *block)
{
    i32 index = bound_size_index(block->size);
    BoundBlock *prev = bound_prev_linked_block(block);
    BoundBlock *next = bound_next_linked_block(block);

    if (prev) { bound_set_linked_blocks(prev, bound_prev_linked_block(prev), next); }
    if (next) { bound_set_linked_blocks(next, prev, bound_next_linked_block(next)); }

    if (memory->blocks[index] == block) {
        memory->blocks[index] = next;
    }
}

func void bound_prepend_block(BoundMemory *memory, BoundBlock *block)
{
    i32 index = bound_size_index(block->size);

    BoundBlock *first = memory->blocks[index];
    bound_set_linked_blocks(block, 0, first);
    if (first) { bound_set_linked_blocks(first, block, bound_next_linked_block(first)); }

    memory->blocks[index] = block;
}

func sze bound_round_size(sze size)
{
    assert((size >= 0) && ((usze)size <= BOUND_SIZE_MASK));
    sze result = 0;
    if (size > 0) {
#if MEM_SIZE_BITS == 32
        result = (size < 8) ? 8 : ((size + 3) & ~3);
#else
        result = (size < 16) ? 16 : ((size + 7) & ~7);
#endif
    }
    return result;
}

func void *bound_alloc(BoundMemory *memory, sze size, sze align, sze count, u32 flags)
{
    assert(ispow2(align));
    void *result = 0;

    sze totalSize = size * count;
    if ((totalSize > 0) && ((totalSize / size) == count))
    {
        totalSize = bound_round_size(totalSize);
        i32 index = bound_size_index(totalSize);

        BoundBlock *block = 0;
        for (; index < 16; ++index) {
            BoundBlock *list = memory->blocks[index];
            block = (index < 14) ? list : bound_find_best_fit_block(list, totalSize);
            if (block) {
                break;
            }
        }

        if (block)
        {
            bound_remove_linked_block(memory, block);
            BoundBlock *split = bound_split_block(block, totalSize);
            if (split) {
                bound_prepend_block(memory, split);
            }

            BoundBlock *next = bound_next_block(block);
            block->used = true;
            if (next) {
                next->prevUsed = true;
            }

            result = block + 1;
            // TODO(michiel): Alignment
            assert((-(sze)(uptr)result & (align - 1)) == 0);
        }
        else if ((flags & Alloc_SoftFail) == 0)
        {
            out_of_memory();
        }
    }

    return result;
}

func void *bound_dealloc(BoundMemory *memory, void *ptr, sze size)
{
    void *result = 0;
    if (ptr)
    {
        BoundBlock *block = (BoundBlock *)ptr - 1;
        assert((usze)size <= block->size);

        BoundBlock *prev = bound_prev_free_block(block);
        BoundBlock *next = bound_next_block(block);

        block->used = false;
        if (next) {
            next->prevUsed = false;
        }

        if (next && !next->used)
        {
            bound_remove_linked_block(memory, next);
            block = bound_merge_blocks(block, next);
        }
        if (prev && !prev->used)
        {
            bound_remove_linked_block(memory, prev);
            block = bound_merge_blocks(prev, block);
        }

        bound_prepend_block(memory, block);
    }

    return result;
}

func void *bound_realloc(BoundMemory *memory, void *ptr, sze oldSize, sze newSize, sze align, u32 flags)
{
    void *result = 0;
    if (ptr)
    {
        BoundBlock *block = (BoundBlock *)ptr - 1;
        oldSize = bound_round_size(oldSize);
        newSize = bound_round_size(newSize);

        if (newSize <= oldSize)
        {
            assert((-(sze)(uptr)ptr & (align - 1)) == 0);

            BoundBlock *split = bound_split_block(block, newSize);
            if (split)
            {
                BoundBlock *next = bound_next_block(split);
                if (next && !next->used) {
                    bound_remove_linked_block(memory, next);
                    bound_merge_blocks(split, next);
                }
                bound_prepend_block(memory, split);
            }
            result = ptr;
        }
        else
        {
            BoundBlock *next = bound_next_block(block);
            sze extraNeeded = newSize - oldSize - sizeof(BoundBlock);
            if (next && !next->used && (extraNeeded > 0) && (next->size >= (usze)extraNeeded))
            {
                assert((-(sze)(uptr)ptr & (align - 1)) == 0);

                bound_remove_linked_block(memory, next);
                BoundBlock *split = bound_split_block(next, extraNeeded);
                if (split) {
                    bound_prepend_block(memory, split);
                }
                block = bound_merge_blocks(block, next);

                result = ptr;
            }
            else
            {
                result = bound_alloc(memory, newSize, align, 1, flags);
                if (result)
                {
                    if (oldSize > 0) {
                        memcpy(result, ptr, (usze)oldSize);
                    }
                    bound_dealloc(memory, ptr, oldSize);
                }
            }
        }
    }
    else
    {
        result = bound_alloc(memory, newSize, align, 1, flags);
    }

    return result;
}

#if PLATFORM_WIN32
func BoundMemory create_bound_memory(sze capacity)
{
    assert((capacity > 0) && ((usze)capacity <= BOUND_SIZE_MASK));
    BoundMemory result = {0};
    BoundBlock *base = VirtualAlloc(0, (usze)capacity, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (base) {
        base->last = true;
        base->used = false;
        base->prevUsed = true;
        base->size = (usze)(capacity - sizeof(BoundBlock)) & BOUND_SIZE_MASK;
        bound_set_linked_blocks(base, 0, 0);

        result.blocks[bound_size_index(base->size)] = base;
    }
    return result;
}
#elif PLATFORM_LINUX
func BoundMemory create_bound_memory(sze capacity)
{
    assert((capacity > 0) && ((usze)capacity <= BOUND_SIZE_MASK));
    BoundMemory result = {0};
    BoundBlock *base = mmap(0, (usze)capacity, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (base != MAP_FAILED) {
        base->last = true;
        base->used = false;
        base->prevUsed = true;
        base->size = (usze)(capacity - sizeof(BoundBlock)) & BOUND_SIZE_MASK;
        bound_set_linked_blocks(base, 0, 0);

        result.blocks[bound_size_index(base->size)] = base;
    }
    return result;
}
#elif PLATFORM_APPLE
func BoundMemory create_bound_memory(sze capacity)
{
    assert((capacity > 0) && ((usze)capacity <= BOUND_SIZE_MASK));
    BoundMemory result = {0};
    BoundBlock *base = 0;
    kern_return_t allocResult = mach_vm_allocate(mach_task_self(), (mach_vm_address_t *)&base, (usze)capacity, VM_FLAGS_ANYWHERE);
    if (allocResult == KERN_SUCCESS) {
        base->last = true;
        base->used = false;
        base->prevUsed = true;
        base->size = (usze)(capacity - sizeof(BoundBlock)) & BOUND_SIZE_MASK;
        bound_set_linked_blocks(base, 0, 0);

        result.blocks[bound_size_index(base->size)] = base;
    } else {
        result.end = result.begin = 0;
    }
    return result;
}
#else
func BoundMemory create_bound_memory(sze capacity)
{
    assert((capacity > 0) && ((usze)capacity <= BOUND_SIZE_MASK));
    BoundMemory result = {0};
    BoundBlock *base = malloc((usze)capacity);
    if (base) {
        base->last = true;
        base->used = false;
        base->prevUsed = true;
        base->size = (usze)(capacity - sizeof(BoundBlock)) & BOUND_SIZE_MASK;
        bound_set_linked_blocks(base, 0, 0);

        result.blocks[bound_size_index(base->size)] = base;
    }
    return result;
}
#endif
