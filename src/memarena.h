typedef struct Arena
{
    byte *begin;
    byte *end;
} Arena;

#define create(...)                     createx(__VA_ARGS__, create4, create3, create2)(__VA_ARGS__)
#define createx(a, b, c, d, e, ...)     e
#define create2(a, T)                   (T *)alloc(a, sizeof(T), alignof(T), 1, 0)
#define create3(a, T, c)                (T *)alloc(a, sizeof(T), alignof(T), c, 0)
#define create4(a, T, c, f)             (T *)alloc(a, sizeof(T), alignof(T), c, f)

func Arena create_arena(sze capacity);

#if COMPILER_LLVM || COMPILER_GCC
__attribute__((malloc, alloc_size(2, 4), alloc_align(3)))
#endif
func void *alloc(Arena *arena, sze size, sze align, sze count, u32 flags);

//#if MEMARENA_IMPLEMENTATION

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
        if ((flags & Alloc_NoClear) == 0) {
            memset(result, 0, total);
        }
    }
    else if ((flags & Alloc_SoftFail) == 0)
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
    result.begin = mmap(0, capacity, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
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
    result.begin = malloc(capacity);
    if (result.begin) {
        result.end = result.begin + capacity;
    }
    return result;
}
#endif
