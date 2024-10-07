typedef struct s8
{
    u8 *data;
    sze size;
} s8;
#define cstr(s)   (s8){(u8 *)s, lengthof(s)}
#define s8(sz, d) (s8){(u8 *)d, sz}

func b32 s8eq(s8 a, s8 b)
{
    b32 result = (a.size == b.size) && ((a.size == 0) || (a.data == b.data) || (memcmp(a.data, b.data, (usze)a.size) == 0));
    return result;
}

func sze s8len(char *c)
{
    sze result = 0;
    if (c) {
        while (c[result]) {
            ++result;
        }
    }
    return result;
}

func s8 string(s8 str);
func s8 stringd(i64 d);

