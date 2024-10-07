typedef enum Scope
{
    Scope_Const = 1,
    Scope_Labels,
    Scope_Global,
    Scope_Param,
    Scope_Local,
} Scope;

typedef enum Storage
{
    Storage_None,
    Storage_Auto,
    Storage_Register,
    Storage_Static,
    Storage_Extern,
    Storage_Typedef,
    Storage_Enum,
} Storage;

typedef struct Origin
{
    s8 file;
    u32 line;
    u32 column;
} Origin;

typedef union Value
{
    i8    sc;
    i16   ss;
    i32   i;
    i64   sl;
    u8    uc;
    u16   us;
    u32   u;
    u64   ul;
    f32   f;
    f64   d;
    void *p;
} Value;

typedef struct Symbol
{
    s8 name;
    i32 scope;
    Origin src;
    struct Symbol *parent;
    List *uses;
    Storage sClass;
    u32 temporary : 1;
    u32 generated : 1;
    u32 defined   : 1;
    Type *type;
    f32 ref;
    union {
        struct {
            i32 label;
            struct Symbol *equatedTo;
        } l; // label
        struct {
            u32 cFields : 1;
            u32 vFields : 1;
            Field *fList;
        } s; // struct
        // enum const  [p69]
        // enum types  [p68]
        struct {
            Value v;
            struct Symbol *loc;
        } c; // constants
        // func sym    [p290]
        // global [p265]
        // temps  [p346]
    } u;
    XSymbol *x;
    // debugger extension
} Symbol;

typedef struct TableEntry
{
    Symbol sym;
    struct TableEntry *link;
} TableEntry;

typedef struct Table
{
    i32 level;
    Table *prev;
    TableEntry *buckets[256];
    Symbol *all;
} Table;
global *Table gConstants;
global *Table gExternals;
global *Table gGlobals;
global *Table gIdentifiers;
global *Table gLabels;
global *Table gTypes;

global i32 gLevel;
