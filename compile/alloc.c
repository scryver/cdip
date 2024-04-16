#define xalloc(s, ctx)      malloc(s)

#define ALLOC_ALIGNMENT     16

typedef struct ArenaBlock
{
    struct ArenaBlock *next;
    byte *limit;
    byte *avail;
} ArenaBlock;

union ArenaAlign
{
    ArenaBlock b;
    byte align[ALLOC_ALIGNMENT];
};

global ArenaBlock gBaseArenas[] = {{null}, {null}, {null}};
global ArenaBlock *gAllocArena[] = {&gBaseArenas[0], &gBaseArenas[1], &gBaseArenas[2]};

global ArenaBlock *gFreeBlocks = 0;

func void *allocate(sze size, ArenaType arena)
{
    ArenaBlock *ap = gAllocArena[arena];
    sze needed = roundup(size, ALLOC_ALIGNMENT);
    while ((ap->avail + needed) > ap->limit)
    {
        ap->next = gFreeBlocks;
        if (ap->next != null) {
            gFreeBlocks = gFreeBlocks->next;
            ap = ap->next;
        } else {
            sze newSize = sizeof(ArenaAlign) + needed + kilobytes(10);
            ap->next = xalloc(newSize, 0);
            ap = ap->next;
            if (ap == null) {
                error("Out of memory!");
                exit(1);
            }
            ap->limit = (byte *)ap + newSize;
        }
        ap->avail = (byte *)((ArenaAlign *)ap + 1);
        ap->next = null;
        gAllocArena[arena] = ap;
    }
    void *result = ap->avail;
    ap->avail += needed;
    return result;
}

func void deallocate(ArenaType arena)
{
    gAllocArena[arena]->next = gFreeBlocks;
    gFreeBlocks = gBaseArenas[arena].next;
    gBaseArenas[arena].next = 0;
    gAllocArena[arena] = &gBaseArenas[arena];
}

func void *create_array(sze count, sze size, ArenaType arena)
{
    sze total = count * size;
    if (size && ((total / size) != count)) {
        error("Array size overflow!");
        exit(1);
    }
    void *result = allocate(total, arena);
    return result;
}
