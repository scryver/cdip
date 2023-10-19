typedef enum ArenaFlags
{
    Arena_NoClear  = 0x01,
    Arena_SoftFail = 0x02,
} ArenaFlags;

typedef struct Arena
{
    byte *begin;
    byte *end;
} Arena;

func Arena create_arena(sze capacity);

#define create(...)                     createx(__VA_ARGS__, create4, create3, create2)(__VA_ARGS__)
#define createx(a, b, c, d, e, ...)     e
#define create2(a, T)                   (T *)alloc(a, sizeof(T), alignof(T), 1, 0)
#define create3(a, T, c)                (T *)alloc(a, sizeof(T), alignof(T), c, 0)
#define create4(a, T, c, f)             (T *)alloc(a, sizeof(T), alignof(T), c, f)

// NOTE(michiel): Can implement your own out_of_memory handler
#ifndef out_of_memory
#define out_of_memory out_of_memory_
#endif
func void out_of_memory_(void)
{
    static const s8 msg = {(u8*)"Out of memory!\n", sizeof("Out of memory!\n")-1}; // cstr("Out of memory!\n");
    os_exit(msg, 1);
}

#if COMPILER_LLVM || COMPILER_GCC
__attribute((malloc, alloc_size(2, 4), alloc_align(3)))
#endif
func void *alloc(Arena *arena, sze size, sze align, sze count, u32 flags)
{
    assert(ispow2(align));
    sze avail = arena->end - arena->begin;
    sze padding = -(uptr)arena->begin & (align - 1);
    void *result = 0;
    if (count <= ((avail - padding) / size))
    {
        sze total = count * size;
        result = arena->begin + padding;
        arena->begin += padding + total;
        if ((flags & Arena_NoClear) == 0) {
            memset(result, 0, total);
        }
    }
    else if ((flags & Arena_SoftFail) == 0)
    {
        out_of_memory();
    }
    return result;
}

#if PLATFORM_WIN32
func Arena create_arena(sze capacity)
{
    Arena result = {0};
    result.begin = VirtualAlloc(0, capacity, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (result.begin) {
        result.end = result.begin + capacity;
    }
    return result;
}
#elif PLATFORM_LINUX
func Arena create_arena(sze capacity)
{
    Arena result = {0};
    result.begin = (byte *)mmap(0, capacity, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (result.begin != MAP_FAILED) {
        result.end = result.begin + capacity;
    } else {
        result.begin = 0;
    }
    return result;
}
#elif PLATFORM_APPLE
func Arena create_arena(sze capacity)
{
    Arena result = {0};
    kern_return_t allocResult = mach_vm_allocate(mach_task_self(), (mach_vm_address_t *)&result.begin, capacity, VM_FLAGS_ANYWHERE);
    if (allocResult == KERN_SUCCESS) {
        result.end = result.begin + capacity;
    } else {
        result.end = result.begin = 0;
    }
    return result;
}
#else
func Arena create_arena(sze capacity)
{
    Arena result = {0};
    result.begin = (byte *)malloc(capacity);
    if (result.begin) {
        result.end = result.begin + capacity;
    }
    return result;
}
#endif
