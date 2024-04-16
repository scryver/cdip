#define TABLE_HASH_SIZE  countof(((Table *)0)->buckets)

global Table gTableConst = {Scope_Const};
global Table gTableExtrn = {Scope_Global};
global Table gTableIds   = {Scope_Global};
global Table gTableTypes = {Scope_Global};

global Origin gOrigin;

gConstants   = &gTableConst;
gExternals   = &gTableExtrn;
gGlobals     = &gTableIds;
gIdentifiers = &gTableIds;
gTypes       = &gTableTypes;

gLevel       = Scope_Global;

func Table *table(Table *prev, i32 level)
{
    Table *newTable = create0(Table, ArenaType_Func);
    newTable->level = level;;
    newTable->prev = prev;
    if (prev) {
        newTable->all = prev->all;
    }
    return newTable;
}

#define FOREACH_SYMBOL(name)    void name(Symbol *sym, void *ctx)
typedef FOREACH_SYMBOL(ForeachSymbol);

func void foreach(Table *t, i32 level, ForeachSymbol *apply, void *ctx)
{
    while (t && (t->level > level)) {
        t = t->prev;
    }

    if (t && (t->level == level))
    {
        Origin sav = gOrigin;
        for (Symbol *s = t->all; s && (s->scope == level); s = s->parent) {
            gOrigin = s->src;
            apply(s, ctx);
        }
        gOrigin = sav;
    }
}

func void enter_scope(void)
{
    ++gLevel;
}

func void exit_scope(void)
{
    rm_types(gLevel);
    if (gTypes->level == gLevel) {
        gTypes = gTypes->prev;
    }
    if (gIdentifiers->level == gLevel) {
        // warn if > 127
        gIdentifiers = gIdentifiers->prev;
    }
    --gLevel;
}

func uptr table_hash(s8 name)
{
    uptr hashIdx = ((uptr)name.data >> 4) & (TABLE_HASH_SIZE - 1);
    return hashIdx;
}

func Symbol *install(s8 name, Table **tp, i32 level, ArenaType arena)
{
    Table *t = *tp;
    uptr hashIdx = table_hash(name);

    if ((level > 0) && (t->level < level)) {
        t = *tp = table(t, level);
    }
    TableEntry *entry = create0(TableEntry, arena);
    entry->sym.name = name;
    entry->sym.scope = level;
    entry->sym.up = t->all;
    t->all = &entry->sym;
    entry->link = t->buckets[hashIdx];
    t->buckets[hashIdx] = entry;
    Symbol *result = &entry->sym;
    return result;
}

func Symbol *lookup(s8 name, Table *t)
{
    uptr hashIdx = table_hash(name);

    Symbol *result = null;
    do {
        for (TableEntry *entry = t->buckets[hashIdx]; entry; entry = entry->link) {
            if (s8eq(entry->sym.name, name)) {
                result = &entry->sym;
            }
        }
        t = t->prev;
    } while ((result == null) && (t != null));
    return result;
}

func i32 gen_label(i32 n)
{
    persist i32 label = 1;
    i32 result = label;
    label += n;
    return result;
}

func Symbol *find_label(i32 label)
{
    uptr hashIdx = (uptr)label & (TABLE_HASH_SIZE - 1);

    Symbol *result = 0;
    for (TableEntry *test = gLabels->buckets[hashIdx]; test; test = test->link)
    {
        if (label == test->sym.u.l.label) {
            result = &test->sym;
            break;
        }
    }

    if (!result)
    {
        TableEntry *entry = create0(TableEntry, ArenaType_Func);
        entry->link = gLabels->buckets[hashIdx];
        gLabels->buckets[hashIdx] = entry;

        result = &entry->sym;
        result->name = stringd(label);
        result->scope = Scope_Labels;
        result->generated = true;
        result->u.l.label = label;

        result->parent = gLabels->all;
        gLabels->all = result;
        gIR->def_symbol(result);
    }

    return result;
}

func Symbol *constant(Type *type, Value v)
{
    uptr hashIdx = (uptr)v.uint64 & (TABLE_HASH_SIZE - 1);
    type = unqual(type);

    Symbol *result = 0;
    for (TableEntry *test = gLabels->buckets[hashIdx]; test; test = test->link)
    {
        if (type_eq(type, test->sym.type, true)) {
#define val_eq(x)    (v.x == test->sym.u.c.v.x)
            switch (type->op) {
                case Type_i8 : { if (val_eq(int8))   { result = &test->sym; } } break;
                case Type_i16: { if (val_eq(int16))  { result = &test->sym; } } break;
                case Type_i32: { if (val_eq(int32))  { result = &test->sym; } } break;
                case Type_i64: { if (val_eq(int64))  { result = &test->sym; } } break;
                case Type_u8 : { if (val_eq(uint8))  { result = &test->sym; } } break;
                case Type_u16: { if (val_eq(uint16)) { result = &test->sym; } } break;
                case Type_u32: { if (val_eq(uint32)) { result = &test->sym; } } break;
                case Type_u64: { if (val_eq(uint64)) { result = &test->sym; } } break;
                case Type_f32: { if (val_eq(flt32))  { result = &test->sym; } } break;
                case Type_f64: { if (val_eq(flt64))  { result = &test->sym; } } break;
                case Type_Array:
                case Type_Function:
                case Type_Pointer: { if (val_eq(ptr)) { result = &test->sym; } } break;
                invalid_default;
            }
#undef val_eq

            if (result) {
                break;
            }
        }
    }

    if (!result)
    {
        TableEntry *entry = create0(TableEntry, ArenaType_Perm);
        entry->link = gConstants->buckets[hashIdx];
        gConstants->buckets[hashIdx] = entry;

        result = &entry->sym;
        result->name = string_from_value(type, v);
        result->scope = Scope_Const;
        result->type = type;
        result->sClass = Storage_Static;
        result->u.c.v = v;

        result->parent = gConstants->all;
        gConstants->all = result;

        if (type->u.sym && !type->u.sym->addressed) {
            gIR->def_symbol(result);
        }

        result->defined = true;
    }

    return result;
}

func Symbol *int_const(i64 n)
{
    Value v;
    v.int64 = n;
    return constant(gIntType, v);
}
