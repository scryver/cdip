#define TABLE_HASH_SIZE  countof(((Table *)0)->buckets)

global Table gTableConst = {Scope_Const};
global Table gTableExtrn = {Scope_Global};
global Table gTableIds   = {Scope_Global};
global Table gTableTypes = {Scope_Global};

global Origin gOrigin;
global i32 gTempId;
global List *gLocation;
global List *gSymbols;

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
    remove_types(gLevel);
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
    // NOTE(michiel): The name data pointer is coming from InternString which is allocated with a round on 16-byte boundary
    // so the lowest 5 bits are always the same.
    uptr hashIdx = ((uptr)name.data >> 5) & (TABLE_HASH_SIZE - 1);
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

func Symbol *relocate(s8 name, Table *src, Table *dst)
{
    uptr hashIdx = table_hash(name);
    TableEntry **q;
    for (q = &src->buckets[hashIdx]; *q; q = &(*q)->link)
    {
        if (name == (*q)->sym.name) {
            break;
        }
    }
    // TODO(michiel): assert(*q)

    // NOTE(michiel): Remove the entry from the src hash chain and from its list of symbols
    TableEntry *entry = *q;
    *q = (*q)->link;

    Symbol **r;
    for (r = &src->all; *r && (*r != &entry->sym); r = &(*r)->up) { }
    // TODO(michiel): assert(*r && *r == &entry->sym)
    *r = entry->sym.up;

    // NOTE(michiel): Insert the entry into dst hash chain and into the list of symbols.
    entry->sym.up = dst->all;
    dst->all = &entry->sym;
    entry->link = dst->buckets[hashIdx];
    dst->buckets[hashIdx] = entry;
    // NOTE(michiel): Return the symbol table entry
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
                case TypeChar    : { if (val_eq(uc)) { result = &test->sym; } } break;
                case TypeShort   : { if (val_eq(ss)) { result = &test->sym; } } break;
                case TypeInt     : { if (val_eq(i))  { result = &test->sym; } } break;
                case TypeLong    : { if (val_eq(sl)) { result = &test->sym; } } break;
                case TypeUnsigned: { if (val_eq(ul)) { result = &test->sym; } } break;
                case TypeFloat   : {
                    // TODO(michiel): Should we handle 0.0 and -0.0 the same?
                    if (v.f == 0.0f) {
                        f32 z1 = v.f; f32 z2 = test->sym.u.c.v.f;
                        char *b1 = (char *)&z1; char *b2 = (char *)&z2;
                        if (b1[sizeof(f32) - 1] == b2[sizeof(f32) - 1]) {
                            result = &test->sym;
                        }
                    } else if (val_eq(f)) {
                        result = &test->sym;
                    }
                } break;
                case TypeDouble: {
                    if (v.d == 0.0) {
                        f64 z1 = v.d; f64 z2 = test->sym.u.c.v.d;
                        char *b1 = (char *)&z1; char *b2 = (char *)&z2;
                        if (b1[sizeof(f64) - 1] == b2[sizeof(f64) - 1]) {
                            result = &test->sym;
                        }
                    } else if (val_eq(d)) {
                        result = &test->sym;
                    }
                } break;
                case TypeArray:
                case TypeFunction:
                case TypePointer: { if (val_eq(p)) { result = &test->sym; } } break;
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
    v.sl = n;
    return constant(gIntType, v);
}

func Symbol *gen_identifier(Storage sClass, Type *type, i32 level)
{
    Symbol *p = create0(Symbol, level >= Scope_Local ? ArenaType_Func : ArenaType_Perm);
    p->name = stringd(gen_label(1));
    p->scope = level;
    p->sClass = sClass;
    p->type = type;
    p->generated = true;
    if (level == Scope_Global) {
        gIr->def_symbol(p);
    }
    return p;
}

func Symbol *gen_temporary(Storage sClass, Type *type)
{
    Symbol *p = create0(Symbol, ArenaType_Func);
    p->name = stringd(++gTempId);
    p->scope = gLevel < Scope_Local ? Scope_Local : gLevel;
    p->sClass = sClass;
    p->type = type;
    p->generated = true;
    p->temporary = true;
    return p;
}

func Symbol *new_temporary(Storage sClass, i32 typeClass)
{
    Symbol *p = gen_temporary(sClass, backend_type(typeClass));
    gIr->local(p);
    p->defined = true;
    return p;
}

func Symbol *all_symbols(Table *t)
{
    return t->all;
}

func void locus(Table *t, Origin *o)
{
    gLocation = append(gLocation, o);
    gSymbols = append(gSymbols, all_symbols(t));
}

func void use(Symbol *p, Origin src) {
    Origin *o = create(Origin, ArenaType_Perm);
    *o = src;
	p->uses = append(o, p->uses);
}

func Symbol *find_type(Type *type)
{
    Table *t = gIdentifiers;
    // TODO(michiel): assert(t)
    Symbol *result = 0;
    do {
        for (i32 idx = 0; idx < TABLE_HASH_SIZE; ++idx) {
            for (TableEntry *entry = tp->buckets[idx]; entry; entry = entry->link) {
                if ((entry->sym.type == type) && (entry->sym.sClass == Storage_Typedef)) {
                    result = &entry->sym;
                    goto found;
                }
            }
        }
        t = tp->prev;
    } while (t);
    found:
    return result;
}

func Symbol *make_string_symbol(s8 str)
{
    Value v;
    v.p = str.data;
    Symbol *p = constant(get_array(gCharType, str.size + 1, 0), v);
    if (p->u.c.loc == 0) {
        p->u.c.loc = gen_identifier(Storage_Static, p->type, Scope_Global);
    }
    return p;
}

func Symbol *make_symbol(Storage sClass, s8 name, Type *type)
{
    Symbol *result;
    if (sClass == Storage_Extern) {
        result = install(string(name), &gGlobals, Scope_Global, ArenaType_Perm);
    } else {
        result = create0(Symbol, ArenaType_Perm);
        result->name = string(name);
        result->scope = Scope_Global;
    }
    result->sClass = sClass;
    result->type = type;
    gIr->def_symbol(result);
    result->defined = true;
    return result;
}

func s8 string_from_value(Type *type, Value v)
{
    s8 result;
    type = unqual(type);
    switch (type->op) {
        case TypeChar    : { result = stringd(v.uc); } break;
        case TypeShort   : { result = stringd(v.ss); } break;
        case TypeInt     : { result = stringd(v.i); } break;
        case TypeLong    : { result = stringd(v.l); } break;
        case TypeUnsigned: { result = stringf((v.ul & ~0x7FFF) ? "0x%X" : "%lu", v.ul); } break;
        case TypeFloat   : { result = stringf("%g", (f64)v.f); } break;
        case TypeDouble  : { result = stringf("%g", v.d); } break;
        case TypeArray   : {
            if ((type->type == gCharType) || (type->type == gSignedCharType) || (type->type == gUnsignedCharType)) {
                result = s8(s8len(v.p), v.p); // TODO(michiel): How do we know the length?
            } else {
                result = stringf("%p", v.p);
            }
        } break;
        case TypeFunction:
        case TypePointer: { result = stringf("%p", v.p); } break;
        invalid_default;
    }
    return result;
}
