
typedef union v2
{
    struct {
        f32 x;
        f32 y;
    };
    struct {
        f32 w;
        f32 h;
    };
} v2;

typedef union v2i
{
    struct {
        i32 x;
        i32 y;
    };
    struct {
        i32 w;
        i32 h;
    };
} v2i;

typedef union v4
{
    struct {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };
    struct {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    };
} v4;

typedef struct Rect
{
    v2 org;
    v2 dim;
} Rect;

typedef struct RectI
{
    v2i org;
    v2i dim;
} RectI;

typedef struct DrawContext
{
    v2i dim;
    i32 stride;
    u32 *pixels;
} DrawContext;

func f32 clamp_f32(f32 min, f32 val, f32 max) { return (val < min) ? min : ((val > max) ? max : val); }
func i32 clamp_i32(i32 min, i32 val, i32 max) { return (val < min) ? min : ((val > max) ? max : val); }

func f32 abs32(f32 x)     { return x < 0.0f ? -x : x; }

func f32 lerp_f32(f32 a, f32 t, f32 b) { return (1.0f - t) * a + t * b; }

func v2i v2i_init(i32 x, i32 y) { v2i r; r.x = x; r.y = y; return r; }
func v2  v2_init (f32 x, f32 y) { v2 r;  r.x = x; r.y = y; return r; }

func v2i v2i_from_v2(v2 v)  { v2i r; r.x = (i32)v.x; r.y = (i32)v.y; return r; }
func v2  v2_from_v2i(v2i v) { v2  r; r.x = (f32)v.x; r.y = (f32)v.y; return r; }

func v4  v4_init (f32 x, f32 y, f32 z, f32 w) { v4 r; r.x = x; r.y = y; r.z = z; r.w = w; return r; }

func v4  lerp_v4(v4 a, f32 t, v4 b)
{
    v4 result;
    result.x = lerp_f32(a.x, t, b.x);
    result.y = lerp_f32(a.y, t, b.y);
    result.z = lerp_f32(a.z, t, b.z);
    result.w = lerp_f32(a.w, t, b.w);
    return result;
}

func v4  unpack_color(u32 c)
{
    v4 result;
    result.r = (f32)((c >> 16) & 0xFF) * 1.0f/255.0f;
    result.g = (f32)((c >>  8) & 0xFF) * 1.0f/255.0f;
    result.b = (f32)((c >>  0) & 0xFF) * 1.0f/255.0f;
    result.a = (f32)((c >> 24) & 0xFF) * 1.0f/255.0f;
    return result;
}

func u32 pack_color(v4 c)
{
    u32 result = (((u32)(c.a * 255.0f) << 24) |
                  ((u32)(c.r * 255.0f) << 16) |
                  ((u32)(c.g * 255.0f) <<  8) |
                  ((u32)(c.b * 255.0f) <<  0));
    return result;
}

func v4  alpha_blend(v4 src, v4 overlay)
{
    v4 result;
    result.a = overlay.a + (1.0f - overlay.a) * src.a;
    result.r = overlay.r * overlay.a + (1.0f - overlay.a) * src.r;
    result.g = overlay.g * overlay.a + (1.0f - overlay.a) * src.g;
    result.b = overlay.b * overlay.a + (1.0f - overlay.a) * src.b;
    return result;
}

func RectI rect_init(v2i origin, v2i dimension)
{
    RectI result;
    result.org = origin;
    result.dim = dimension;
    return result;
}

func b32 rect_is_inside(RectI rect, v2i point)
{
    b32 result = ((rect.org.x < point.x) &&
                  (rect.org.y < point.y) &&
                  ((rect.org.x + rect.dim.w) > point.x) &&
                  ((rect.org.y + rect.dim.h) > point.y));
    return result;
}

#if 0
func i32 int_part(f32 x)  { return (i32)x; }
func f32 round(f32 x)     { return (f32)int_part(x + 0.5f); }
func f32 frac_part(f32 x) { return x - (f32)int_part(x); }
func f32 repr_frac(f32 x) { return 1.0f - frac_part(x); }

typedef enum ClippedPoint
{
    Clip_Inside = 0,
    Clip_Left   = 1 << 0,
    Clip_Top    = 1 << 1,
    Clip_Right  = 1 << 2,
    Clip_Bottom = 1 << 3,
} ClippedPoint;

func u32 is_point_clipped(DrawContext *ctx, v2 p)
{
    u32 result = (((p.x < 0.0f)             ? Clip_Left   : 0) |
                  ((p.y < 0.0f)             ? Clip_Top    : 0) |
                  ((p.x >= (f32)ctx->dim.w) ? Clip_Right  : 0) |
                  ((p.y >= (f32)ctx->dim,h) ? Clip_Bottom : 0));
    return result;
}

func v2 clip_point(DrawContext *ctx, v2 start, v2 end, u32 clip)
{
    v2 result;
    v2 s = start;
    v2 e = end;

    if (clip & Clip_Top) {
        result.x = s.x + ((e.x - s.x) / (e.y - s.y)) * -s.y;
        result.y = 0.0f;
    } else if (clip & Clip_Bottom) {
        result.x = s.x + ((e.x - s.x) / (e.y - s.y)) * ((f32)ctx->dim.h - s.y);
        result.y = 0.0f;
    } else if (clip & Clip_Left) {
        result.x = 0.0f;
        result.y = s.y + ((e.y - s.y) / (e.x - s.x)) * -s.x;
    } else if (clip & Clip_Right) {
        result.x = 0.0f;
        result.y = s.y + ((e.y - s.y) / (e.x - s.x)) * ((f32)ctx->dim.w - s.x);
    }
    return result;
}

func void draw_line(DrawContext *ctx, i32 xStart, i32 yStart, i32 xEnd, i32 yEnd, u32 color)
{
    v2 start = v2_init((f32)xStart, (f32)yStart);
    v2 end   = v2_init((f32)xEnd,   (f32)yEnd);
    u32 startClip = is_point_clipped(ctx, start);
    u32 endClip   = is_point_clipped(ctx, end);

    b32 drawLine = false;
    do {
        drawLine = (startClip | endClip) == Clip_Inside;
        if (drawLine || ((startClip & endClip) != 0)) {
            break;
        }
        if (startClip != Clip_Inside) {
            start = clip_point(ctx, start, end, startClip);
            startClip = is_point_clipped(ctx, start);
        } else {
            end = clip_point(ctx, start, end, endClip);
            endClip = is_point_clipped(ctx, end);
        }
    } while (true);

    if (drawLine)
    {
        b32 isSteep = abs32(end.y - start.y) > abs32(end.x - start.x);
        if (isSteep) {
            f32 temp = start.x;
            start.x = start.y;
            start.y = temp;
            temp = end.x;
            end.x = end.y;
            end.y = temp;
        }
        if (start.x > end.x) {
            v2 temp = start;
            start = end;
            end = temp;
        }

        f32 dx = end.x - start.x;
        f32 dy = end.y - start.y;

        f32 gradient = (dx == 0.0f) ? 1.0f : (dy / dx);

    }
}
#endif

func void draw_horz_line(DrawContext *ctx, i32 x, i32 y, i32 w, u32 color)
{
    if (x < 0) { w += x; x = 0; }
    if ((x + w) >= ctx->dim.w) { w = ctx->dim.w - x; }
    if ((w > 0) && (y > 0) && (y < ctx->dim.h))
    {
        u32 *p = ctx->pixels + (y * ctx->stride) + x;
        for (i32 c = 0; c < w; ++c) {
            *p++ = color;
        }
    }
}

func void draw_vert_line(DrawContext *ctx, i32 x, i32 y, i32 h, u32 color)
{
    if (y < 0) { h += y; y = 0; }
    if ((y + h) >= ctx->dim.h) { h = ctx->dim.h - y; }
    if ((h > 0) && (x > 0) && (x < ctx->dim.w))
    {
        u32 *p = ctx->pixels + (y * ctx->stride) + x;
        for (i32 r = 0; r < h; ++r) {
            *p = color;
            p += ctx->stride;
        }
    }
}

func void draw_rect(DrawContext *ctx, i32 x, i32 y, i32 w, i32 h, u32 color)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if ((x + w) >= ctx->dim.w) { w = ctx->dim.w - x; }
    if ((y + h) >= ctx->dim.h) { h = ctx->dim.h - y; }
    if ((w > 0) && (h > 0))
    {
        u32 *pAt = ctx->pixels + (y * ctx->stride) + x;
        for (i32 r = 0; r < h; ++r) {
            u32 *p = pAt;
            for (i32 c = 0; c < w; ++c) {
                *p++ = color;
            }
            pAt += ctx->stride;
        }
    }
}

func void draw_rect_horz_grad(DrawContext *ctx, i32 x, i32 y, i32 w, i32 h, u32 leftColor, u32 rightColor)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if ((x + w) >= ctx->dim.w) { w = ctx->dim.w - x; }
    if ((y + h) >= ctx->dim.h) { h = ctx->dim.h - y; }
    if ((w > 0) && (h > 0))
    {
        u32 *pAt = ctx->pixels + (y * ctx->stride) + x;
        for (i32 r = 0; r < h; ++r) {
            u32 *p = pAt;
            for (i32 c = 0; c < w; ++c) {
                v4 lc = unpack_color(leftColor);
                v4 rc = unpack_color(rightColor);
                *p++ = pack_color(lerp_v4(lc, (f32)c / (f32)(w - 1), rc));
            }
            pAt += ctx->stride;
        }
    }
}

func void draw_rect_vert_grad(DrawContext *ctx, i32 x, i32 y, i32 w, i32 h, u32 topColor, u32 bottomColor)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if ((x + w) >= ctx->dim.w) { w = ctx->dim.w - x; }
    if ((y + h) >= ctx->dim.h) { h = ctx->dim.h - y; }
    if ((w > 0) && (h > 0))
    {
        u32 *pAt = ctx->pixels + (y * ctx->stride) + x;
        for (i32 r = 0; r < h; ++r) {
            u32 *p = pAt;
            v4 tc = unpack_color(topColor);
            v4 bc = unpack_color(bottomColor);
            u32 color = pack_color(lerp_v4(tc, (f32)r / (f32)(h - 1), bc));

            for (i32 c = 0; c < w; ++c) {
                *p++ = color;
            }
            pAt += ctx->stride;
        }
    }
}

func void draw_circ_(DrawContext *ctx, i32 xc, i32 yc, i32 r, v2i min, v2i max, u32 color)
{
    f32 maxDistSqr  = ((f32)r - 0.5f) * ((f32)r - 0.5f);
    f32 edgeDistSqr = ((f32)r + 0.5f) * ((f32)r + 0.5f);
    f32 edgeDiff    = 1.0f / (edgeDistSqr - maxDistSqr);

    u32 *pAt = ctx->pixels + (min.y * ctx->stride) + min.x;
    for (i32 y = min.y; y < max.y; ++y)
    {
        f32 yf = (f32)(y - yc);
        f32 yfSqr = yf * yf;
        u32 *p = pAt;
        for (i32 x = min.x; x < max.x; ++x)
        {
            f32 xf = (f32)(x - xc);
            f32 distSqr = (xf * xf) + yfSqr;
            if (distSqr <= edgeDistSqr)
            {
                v4 c = unpack_color(color);
                if (distSqr > maxDistSqr) {
                    c.a = clamp_f32(0.0f, c.a - (distSqr - maxDistSqr) * edgeDiff, 1.0f);
                }
                *p = pack_color(alpha_blend(unpack_color(*p), c));
            }
            ++p;
        }
        pAt += ctx->stride;
    }
}

func void draw_circ(DrawContext *ctx, i32 xc, i32 yc, i32 r, u32 color)
{
    i32 xMin = xc - r;
    i32 yMin = yc - r;
    i32 xMax = xc + r + 1;
    i32 yMax = yc + r + 1;
    xMin = clamp_i32(0, xMin, ctx->dim.w);
    yMin = clamp_i32(0, yMin, ctx->dim.h);
    xMax = clamp_i32(0, xMax, ctx->dim.w);
    yMax = clamp_i32(0, yMax, ctx->dim.h);

    draw_circ_(ctx, xc, yc, r, v2i_init(xMin, yMin), v2i_init(xMax, yMax), color);
}

func void draw_circ_lt(DrawContext *ctx, i32 xc, i32 yc, i32 r, u32 color)
{
    i32 xMin = xc - r;
    i32 yMin = yc - r;
    i32 xMax = xc + 1;
    i32 yMax = yc + 1;
    xMin = clamp_i32(0, xMin, ctx->dim.w);
    yMin = clamp_i32(0, yMin, ctx->dim.h);
    xMax = clamp_i32(0, xMax, ctx->dim.w);
    yMax = clamp_i32(0, yMax, ctx->dim.h);

    draw_circ_(ctx, xc, yc, r, v2i_init(xMin, yMin), v2i_init(xMax, yMax), color);
}

func void draw_circ_rt(DrawContext *ctx, i32 xc, i32 yc, i32 r, u32 color)
{
    i32 xMin = xc;
    i32 yMin = yc - r;
    i32 xMax = xc + r + 1;
    i32 yMax = yc + 1;
    xMin = clamp_i32(0, xMin, ctx->dim.w);
    yMin = clamp_i32(0, yMin, ctx->dim.h);
    xMax = clamp_i32(0, xMax, ctx->dim.w);
    yMax = clamp_i32(0, yMax, ctx->dim.h);

    draw_circ_(ctx, xc, yc, r, v2i_init(xMin, yMin), v2i_init(xMax, yMax), color);
}

func void draw_circ_lb(DrawContext *ctx, i32 xc, i32 yc, i32 r, u32 color)
{
    i32 xMin = xc - r;
    i32 yMin = yc;
    i32 xMax = xc + 1;
    i32 yMax = yc + r + 1;
    xMin = clamp_i32(0, xMin, ctx->dim.w);
    yMin = clamp_i32(0, yMin, ctx->dim.h);
    xMax = clamp_i32(0, xMax, ctx->dim.w);
    yMax = clamp_i32(0, yMax, ctx->dim.h);

    draw_circ_(ctx, xc, yc, r, v2i_init(xMin, yMin), v2i_init(xMax, yMax), color);
}

func void draw_circ_rb(DrawContext *ctx, i32 xc, i32 yc, i32 r, u32 color)
{
    i32 xMin = xc;
    i32 yMin = yc;
    i32 xMax = xc + r + 1;
    i32 yMax = yc + r + 1;
    xMin = clamp_i32(0, xMin, ctx->dim.w);
    yMin = clamp_i32(0, yMin, ctx->dim.h);
    xMax = clamp_i32(0, xMax, ctx->dim.w);
    yMax = clamp_i32(0, yMax, ctx->dim.h);

    draw_circ_(ctx, xc, yc, r, v2i_init(xMin, yMin), v2i_init(xMax, yMax), color);
}

func void draw_circ_grad_(DrawContext *ctx, i32 xc, i32 yc, i32 r, v2i min, v2i max, u32 innerColor, u32 outerColor)
{
    f32 maxDistSqr  = ((f32)r - 0.5f) * ((f32)r - 0.5f);
    f32 edgeDistSqr = ((f32)r + 0.5f) * ((f32)r + 0.5f);
    f32 edgeDiff    = 1.0f / (edgeDistSqr - maxDistSqr);
    f32 lerpFactor  = 1.0f / maxDistSqr;

    u32 *pAt = ctx->pixels + (min.y * ctx->stride) + min.x;
    for (i32 y = min.y; y < max.y; ++y)
    {
        f32 yf = (f32)(y - yc);
        f32 yfSqr = yf * yf;
        u32 *p = pAt;
        for (i32 x = min.x; x < max.x; ++x)
        {
            f32 xf = (f32)(x - xc);
            f32 distSqr = (xf * xf) + yfSqr;
            if (distSqr <= edgeDistSqr)
            {
                f32 t = clamp_f32(0.0f, distSqr * lerpFactor, 1.0f);
                v4 ic = unpack_color(innerColor);
                v4 oc = unpack_color(outerColor);
                v4 c = lerp_v4(ic, t, oc);
                if (distSqr > maxDistSqr) {
                    c.a = clamp_f32(0.0f, c.a - (distSqr - maxDistSqr) * edgeDiff, 1.0f);
                }
                *p = pack_color(alpha_blend(unpack_color(*p), c));
            }
            ++p;
        }
        pAt += ctx->stride;
    }
}

func void draw_circ_grad(DrawContext *ctx, i32 xc, i32 yc, i32 r, u32 innerColor, u32 outerColor)
{
    i32 xMin = xc - r;
    i32 yMin = yc - r;
    i32 xMax = xc + r + 1;
    i32 yMax = yc + r + 1;
    xMin = clamp_i32(0, xMin, ctx->dim.w);
    yMin = clamp_i32(0, yMin, ctx->dim.h);
    xMax = clamp_i32(0, xMax, ctx->dim.w);
    yMax = clamp_i32(0, yMax, ctx->dim.h);

    draw_circ_grad_(ctx, xc, yc, r, v2i_init(xMin, yMin), v2i_init(xMax, yMax), innerColor, outerColor);
}

func void draw_circ_grad_lt(DrawContext *ctx, i32 xc, i32 yc, i32 r, u32 innerColor, u32 outerColor)
{
    i32 xMin = xc - r;
    i32 yMin = yc - r;
    i32 xMax = xc + 1;
    i32 yMax = yc + 1;
    xMin = clamp_i32(0, xMin, ctx->dim.w);
    yMin = clamp_i32(0, yMin, ctx->dim.h);
    xMax = clamp_i32(0, xMax, ctx->dim.w);
    yMax = clamp_i32(0, yMax, ctx->dim.h);

    draw_circ_grad_(ctx, xc, yc, r, v2i_init(xMin, yMin), v2i_init(xMax, yMax), innerColor, outerColor);
}

func void draw_circ_grad_rt(DrawContext *ctx, i32 xc, i32 yc, i32 r, u32 innerColor, u32 outerColor)
{
    i32 xMin = xc;
    i32 yMin = yc - r;
    i32 xMax = xc + r + 1;
    i32 yMax = yc + 1;
    xMin = clamp_i32(0, xMin, ctx->dim.w);
    yMin = clamp_i32(0, yMin, ctx->dim.h);
    xMax = clamp_i32(0, xMax, ctx->dim.w);
    yMax = clamp_i32(0, yMax, ctx->dim.h);

    draw_circ_grad_(ctx, xc, yc, r, v2i_init(xMin, yMin), v2i_init(xMax, yMax), innerColor, outerColor);
}

func void draw_circ_grad_lb(DrawContext *ctx, i32 xc, i32 yc, i32 r, u32 innerColor, u32 outerColor)
{
    i32 xMin = xc - r;
    i32 yMin = yc;
    i32 xMax = xc + 1;
    i32 yMax = yc + r + 1;
    xMin = clamp_i32(0, xMin, ctx->dim.w);
    yMin = clamp_i32(0, yMin, ctx->dim.h);
    xMax = clamp_i32(0, xMax, ctx->dim.w);
    yMax = clamp_i32(0, yMax, ctx->dim.h);

    draw_circ_grad_(ctx, xc, yc, r, v2i_init(xMin, yMin), v2i_init(xMax, yMax), innerColor, outerColor);
}

func void draw_circ_grad_rb(DrawContext *ctx, i32 xc, i32 yc, i32 r, u32 innerColor, u32 outerColor)
{
    i32 xMin = xc;
    i32 yMin = yc;
    i32 xMax = xc + r + 1;
    i32 yMax = yc + r + 1;
    xMin = clamp_i32(0, xMin, ctx->dim.w);
    yMin = clamp_i32(0, yMin, ctx->dim.h);
    xMax = clamp_i32(0, xMax, ctx->dim.w);
    yMax = clamp_i32(0, yMax, ctx->dim.h);

    draw_circ_grad_(ctx, xc, yc, r, v2i_init(xMin, yMin), v2i_init(xMax, yMax), innerColor, outerColor);
}

func void draw_round_rect(DrawContext *ctx, i32 x, i32 y, i32 w, i32 h, i32 r, u32 color)
{
    if (2 * r > w) { r = w / 2; }
    if (2 * r > h) { r = h / 2; }

    /* +---------+
*  |A|  B  |C|
*  +-+-----+-+
*  |         |
*  .    D    .
*  .         .
*  |         |
*  +-+-----+-+
*  |E|  F  |G|
*  +-+-----+-+
*/

    draw_rect   (ctx, x + r, y, w - 2 * r, r, color);          // B
    draw_rect   (ctx, x, y + r, w, h - 2 * r, color);          // D
    draw_rect   (ctx, x + r, y + h - r, w - 2 * r, r, color);  // F
    draw_circ_lt(ctx, x + r, y + r, r, color);                 // A
    draw_circ_rt(ctx, x + w - r - 1, y + r, r, color);         // C
    draw_circ_lb(ctx, x + r, y + h - r - 1, r, color);         // E
    draw_circ_rb(ctx, x + w - r - 1, y + h - r - 1, r, color); // G
}

func void draw_round_rect_grad(DrawContext *ctx, i32 x, i32 y, i32 w, i32 h, i32 r, u32 innerColor, u32 outerColor)
{
    if (2 * r > w) { r = w / 2; }
    if (2 * r > h) { r = h / 2; }

    /* +---------+
*  |A|  B  |C|
*  +-+-----+-+
*  | |     | |
*  .D.  E  .F.
*  . .     . .
*  | |     | |
*  +-+-----+-+
*  |G|  H  |I|
*  +-+-----+-+
*/

    draw_rect_vert_grad(ctx, x + r, y, w - 2 * r, r, outerColor, innerColor);         // B
    draw_rect_horz_grad(ctx, x, y + r, r, h - 2 * r, outerColor, innerColor);         // D
    draw_rect          (ctx, x + r, y + r, w - 2 * r, h - 2 * r, innerColor);         // E
    draw_rect_horz_grad(ctx, x + w - r, y + r, r, h - 2 * r, innerColor, outerColor); // F
    draw_rect_vert_grad(ctx, x + r, y + h - r, w - 2 * r, r, innerColor, outerColor); // H
    draw_circ_grad_lt(ctx, x + r, y + r, r, innerColor, outerColor);                  // A
    draw_circ_grad_rt(ctx, x + w - r - 1, y + r, r, innerColor, outerColor);          // C
    draw_circ_grad_lb(ctx, x + r, y + h - r - 1, r, innerColor, outerColor);          // E
    draw_circ_grad_rb(ctx, x + w - r - 1, y + h - r - 1, r, innerColor, outerColor);  // G
}

