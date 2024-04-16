typedef enum ArenaType
{
    ArenaType_Perm,
    ArenaType_Func,
} ArenaType;

func void *allocate(sze size, ArenaType arena);
func void  deallocate(ArenaType arena);

#define create(T, a)      (T *)allocate(sizeof(T), a)
#define create0(T, a)     (T *)memset(allocate(sizeof(T), a), 0, sizeof(T))

func void *create_array(sze count, sze size, ArenaType arena);
