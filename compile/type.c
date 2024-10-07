typedef struct TypeEntry
{
    Type type;
    struct TypeEntry *link;
} TypeEntry;

global TypeEntry *gTypeTable[256];
#define TYPE_HASH_SIZE  countof(gTypeTable)

global Symbol *gPointerSym;
global i32 gMaxLevel;

func Type *get_type(TypeOp op, Type *type, sze size, sze align, void *sym)
{
    uptr hashIdx = (op ^ ((uptr)type >> 4)) & (TYPE_HASH_SIZE - 1);

    Type *result = 0;
    if ((op != TypeFunction) && ((op != TypeArray) || (size > 0))) {
        for (TypeEntry *test = gTypeTable[hashIdx]; test; test = test->link) {
            if ((test->type.op == op) && (test->type.type == type) &&
                (test->type.size == size) && (test->type.align == align) &&
                (test->type.u.sym == sym)) {
                result = &test->type;
                break;
            }
        }
    }

    if (!result)
    {
        TypeEntry *entry = create0(TypeEntry, ArenaType_Perm);
        entry->type.op = op;
        entry->type.type = type;
        entry->type.size = size;
        entry->type.align = align;
        entry->type.u.sym = sym;
        memset(&entry->type.x, 0, sizeof(entry->type.x));
        entry->link = gTypeTable[hashIdx];
        gTypeTable[hashIdx] = entry;
        result = &entry->type;
    }

    return result;
}

func Type *get_pointer(Type *type)
{
    Type *result = get_type(TypePointer, type, gIR->pointerMetric.size, gIR->pointerMetric.align, gPointerSym);
    return result;
}

func Type *get_array(Type *type, sze count, sze align)
{
    Type *result = 0;
    if (isfunction(type))
    {
        error("illegal type 'array of %t'\n", type);
        result = get_array(gIntType, count, 0);
    }
    else
    {
        if ((gLevel > Scope_Global) && isarray(type) && (type->size == 0)) {
            error("missing array size\n");
        }

        if (type->size == 0) {
            if (unqual(type) == gVoidType) {
                error("illegal type 'array of %t'\n", type);
            } else if (gAFlag >= 2) {
                warning("declaring type 'array of %t' is undefined\n", type);
            }
        } else if (count > (I64_MAX / type->size)) {
            error("size of 'array of %t' exceeds %ld bytes\n", type, I64_MAX);
            count = 1;
        }

        result = get_type(TypeArray, type, count * type->size, align ? align : type->align, 0);
    }
    return result;
}

func Type *get_function(Type *type, Type **proto, i32 style)
{
    if (type && (isarray(type) || isfunction(type))) {
        error("illegal return type: '%t'\n", type);
    }
    Type *result = get_type(TypeFunction, type, 0, 0, 0);
    result->u.f.proto = proto;
    result->u.f.oldStyle = style;
    return result;
}

func Type *dereference(Type *type)
{
    if (ispointer(type)) {
        type = type->type;
    } else {
        error("type error: %s\n", "pointer expected");
    }
    return isenum(type) ? unqual(type)->type : type;
}

func Type *pointer_decay(Type *type)
{
    if (isarray(type)) {
        type = type->type;
    } else {
        error("type error: %s\n", "array expected");
    }
    return get_pointer(type);
}

func Type *function_return(Type *type)
{
    if (isfunction(type)) {
        type = type->type;
    } else {
        error("type error: %s\n", "function expected");
        type = gIntType;
    }
    return type;
}

func Type *qual(TypeOp op, Type *type)
{
    if (isarray(type)) {
        type = get_type(TypeArray, qual(op, type->type), type->size, type->align, 0);
    } else if (isfunction(type)) {
        warning("qualified function type ignored\n");
    } else if ((isconst(type) && (op == TypeConst)) ||
               (isvolatile(type) && (op == TypeVolatile))) {
        error("illegal type '%k %t'\n", op, type);
    } else {
        if (isqual(type)) {
            op += type->op;
            type = type->type;
        }
        type = get_type(op, type, type->size, type->align, 0);
    }
    return type;
}

func b32 variadic(Type *type)
{
    b32 result = false;
    if (isfunction(type) && type->u.f.proto)
    {
        i32 idx;
        for (idx = 0; type->u.f.proto[idx]; ++idx) {}
        result = (idx > 1) && (type->u.f.proto[idx - 1] == gVoidType);
    }
    return result;
}

func void type_init(void)
{
#define xx(v, name, op, metrics) { \
Symbol *p = install(string(name), &gTypes, Scope_Global, ArenaType_Perm); \
v = get_type(op, 0, gIR->metrics.size, gIR->metrics.align, p); \
p->type = v; \
p->addressed = gIR->metrics.outofline; \
}
    xx(gCharType,          cstr("char"),           TypeChar,     charMetric);
    xx(gSignedCharType,    cstr("signed char"),    TypeChar,     charMetric);
    xx(gShortType,         cstr("short"),          TypeShort,    shortMetric);
    xx(gIntType,           cstr("int"),            TypeInt,      intMetric);
    xx(gLongType,          cstr("long"),           TypeLong,     longMetric);
    xx(gUnsignedCharType,  cstr("unsigned char"),  TypeChar,     charMetric);
    xx(gUnsignedShortType, cstr("unsigned short"), TypeShort,    shortMetric);
    xx(gUnsignedType,      cstr("unsigned"),       TypeUnsigned, intMetric);
    xx(gUnsignedLongType,  cstr("unsigned long"),  TypeUnsigned, longMetric);
    xx(gFloatType,         cstr("float"),          TypeFloat,    floatMetric);
    xx(gDoubleType,        cstr("double"),         TypeDouble,   doubleMetric);
#undef xx
    {
        Symbol *p = install(string(cstr("void")), &gTypes, Scope_Global, ArenaType_Perm);
        gVoidType = get_type(TypeVoid, 0, 0, 0, p);
        p->type = gVoidType;
    }
    gPointerSym = install(string(cstr("T*")), &gTypes, Scope_Global, ArenaType_Perm);
    gPointerSym->addressed = gIR->pointerMetric.outofline;
    gVoidPType = get_pointer(gVoidType);
}

func void remove_types(i32 level)
{
    if (gMaxLevel >= level)
    {
        gMaxLevel = 0;
        for (i32 idx = 0; idx < TYPE_HASH_SIZE; ++idx) {
            TypeEntry **testChain = &gTypeTable[idx];
            TypeEntry *test = *testChain;
            while (test) {
                if (test->type.op == TypeFunction) {
                    testChain = &test->link;
                } else if (test->type.u.sym && (test->type.u.sym->scope >= level)) {
                    *testChain = test->link;
                } else {
                    if (test->type.u.sym && (gMaxLevel < test->type.u.sym->scope)) {
                        gMaxLevel = test->type.u.sym->scope;
                    }
                    testChain = &test->link;
                }
                test = *testChain;
            }
        }
    }
}
