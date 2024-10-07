#define isqual(t)         ((t)->op >= TypeConst)
#define unqual(t)         (isqual(t) ? (t)->type : (t))

#define isvolatile(t)     (((t)->op == TypeVolatile) || ((t)->op == (TypeVolatile + TypeConst)))
#define isconst(t)        (((t)->op == TypeConst)    || ((t)->op == (TypeVolatile + TypeConst)))
#define isarray(t)        (unqual(t)->op == TypeArray)
#define isstruct(t)       ((unqual(t)->op == TypeStruct) || (unqual(t)->op == TypeUnion))
#define isunion(t)        (unqual(t)->op == TypeUnion)
#define isfunction(t)     (unqual(t)->op == TypeFunction)
#define ispointer(t)      (unqual(t)->op == TypePointer)
#define ischar(t)         (unqual(t)->op == TypeChar)
#define isint(t)          ((unqual(t)->op >= TypeChar) && (unqual(t)->op <= TypeUnsigned))
#define isfloat(t)        (unqual(t)->op <= TypeDouble)
#define isarith(t)        (unqual(t)->op <= TypeUnsigned)
#define isunigned(t)      (unqual(t)->op == TypeUnsigned)
#define isscalar(t)       ((unqual(t)->op <= TypePointer) || (unqual(t)->op == TypeEnum))
#define isenum(t)         (unqual(t)->op == TypeEnum)


typedef struct TypeOp
{
#define xx(a, b, c, d, e, f, g) Type##a = b,
#define yy(a, b, c, d, e, f, g)
#include "tokens.h"
} TypeOp;

typedef struct Type
{
    TypeOp op;
    struct Type *type;
    sze align;
    sze size;
    union {
        Symbol *sym; // types with names or tags
        struct {
            u32 oldStyle : 1;
            Type **proto;
        } f;
    } u;
    XType *x;
} Type;

typedef struct Field
{
    s8 name;
    Type *type;
    sze offset;
    i16 bitSize;
    i16 lsb;
    struct Field *link;
} Field;

#define fieldsize(p)       (p)->bitSize
#define fieldright(p)      ((p)>lsb - 1)
#define fieldleft(p)       ((8 * (p)->type->size) - fieldsize(p) - fieldright(p))
#define fieldmask(p)       (~(~0U << fieldsize(p)))

global Type *gCharType;
global Type *gSignedCharType;
global Type *gShortType;
global Type *gIntType;
global Type *gLongType;
global Type *gUnsignedCharType;
global Type *gUnsignedShortType;
global Type *gUnsignedType;
global Type *gUnsignedLongType;
global Type *gFloatType;
global Type *gDoubleType;
global Type *gVoidPType;
global Type *gVoidType;
