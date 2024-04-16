typedef enum Scope
{
    Scope_Const,
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
    i8  int8;
    i16 int16;
    i32 int32;
    i64 int64;
    u8  uint8;
    u16 uint16;
    u32 uint32;
    u64 uint64;
    f32 flt32;
    f64 flt64;
    void *ptr;
} Value;

typedef struct Symbol
{
    s8 name;
    i32 scope;
    Origin src;
    struct Symbol *parent;
    List *uses;
    Storage sClass;
    // symbol flags [p50]
    Type *type;
    f32 ref;
    union {
        struct {
            i32 label;
            struct Symbol *equatedTo;
        } l; // label
        // struct [p65]
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