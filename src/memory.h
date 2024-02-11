typedef enum AllocFlags
{
    Alloc_NoClear  = 0x01,
    Alloc_SoftFail = 0x02,
} AllocFlags;

// NOTE(michiel): Can implement your own out_of_memory handler
#ifndef out_of_memory
#define out_of_memory out_of_memory_
#endif
func void out_of_memory_(void)
{
    //static const s8 msg = {(u8*)"Out of memory!\n", sizeof("Out of memory!\n")-1}; // cstr("Out of memory!\n");
    static const s8 msg = cstr("Out of memory!\n");
    os_exit(msg, 1);
}
