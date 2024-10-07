typedef struct fmt_buf
{
    byte   *data;
    sze     size;
    sze     cap;
    OsFile *file;
    b32     error;
} fmt_buf;
#define fmt_buf(sz, d)          (fmt_buf){d, 0, sz, 0, 0}
#define fmt_buf_fd(sz, d, fd)   (fmt_buf){d, 0, sz, fd, 0}

void flush(fmt_buf *f)
{
    f->error |= (f->file == 0);
    if (!f->error && f->size) {
        sze written = write_to_file(f->file, buf(f->size, f->data));
        f->error |= ((written != f->size) || (f->file->error != OsFile_NoError));
        f->size = 0;
    }
}

void append(fmt_buf *f, byte *src, sze len)
{
    byte *end = src + len;
    while (!f->error && (src < end))
    {
        sze left = end - src;
        sze avail = f->cap - f->size;
        sze amount = (avail < left) ? avail : left;
        for (sze idx = 0; idx < amount; ++idx) {
            f->data[f->size + idx] = src[idx];
        }
        f->size += amount;
        src += amount;

        if (amount < left) {
            flush(f);
        }
    }
}

#define append_cstr(f, s) append(f, (byte *)s, lengthof(s))
#define append_s8(f, s)   append(f, (byte *)s.data, s.size)
#define append_buf(f, b)  append(f, b.data, b.size)

void append_byte(fmt_buf *f, byte b)
{
    append(f, &b, 1);
}

void append_i64(fmt_buf *f, i64 x)
{
    byte temp[64];
    byte *end = temp + sizeof(temp);
    byte *beg = end;
    i64 t = (x < 0) ? x : -x;
    do {
        *--beg = (byte)('0' - (char)(t % 10));
        t /= 10;
    } while (t);
    if (x < 0) {
        *--beg = '-';
    }
    append(f, beg, end - beg);
}

void append_u64(fmt_buf *f, u64 x)
{
    byte temp[64];
    byte *end = temp + sizeof(temp);
    byte *beg = end;
    do {
        *--beg = (byte)('0' + (char)(x % 10));
        x /= 10;
    } while (x);
    append(f, beg, end - beg);
}

void append_hex64(fmt_buf *f, u64 x)
{
    append_cstr(f, "0x");
    for (sze idx = 2 * sizeof(x) - 1; idx >= 0; --idx) {
        append_byte(f, (byte)("0123456789ABCDEF"[(x >> (4*idx)) & 0xF]));
    }
}

void append_hex32(fmt_buf *f, u32 x)
{
    append_cstr(f, "0x");
    for (sze idx = 2 * sizeof(x) - 1; idx >= 0; --idx) {
        append_byte(f, (byte)("0123456789ABCDEF"[(x >> (4*idx)) & 0xF]));
    }
}

void append_ptr(fmt_buf *f, void *p)
{
    append_hex64(f, (uptr)p);
}

void append_fast_f64(fmt_buf *f, f64 x)
{
    i64 prec = 1000000;

    if (x < 0) {
        append_byte(f, '-');
        x = -x;
    }

    x += 0.5 / (f64)prec;
    if (x >= (f64)I64_MAX) {
        append_cstr(f, "inf");
    } else {
        i64 integral = (i64)x;
        i64 fractional = (i64)((x - (f64)integral) * (f64)prec);
        append_i64(f, integral);
        append_byte(f, '.');
        for (i64 i = (prec / 10); i > 1; i /= 10) {
            if (i > fractional) {
                append_byte(f, '0');
            }
        }
        append_i64(f, fractional);
    }
}
