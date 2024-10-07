#include <zlib.h>
#include "../unicode/utf_encoding.h"

typedef struct FontBlob
{
    s8 name;
    buf buffer;
} FontBlob;

#include "font_blob.h"

typedef struct Glyph
{
    v2 posMin;
    v2 posMax;
    v2 uvMin;
    v2 uvMax;
    v2i glyphOffset;        // TODO(michiel): Glyph offset from base origin
    v2i glyphSize;
    i32 xBaseAdvance;       // NOTE(michiel): Based on the SDF size in load_font_texture, in 26.6 size
} Glyph;

#pragma pack(push, 1)
typedef struct FontFile
{
    u64 totalSize;
    u32 magic;
    i32 texSize; // NOTE(michiel): Texture is assumed to be square
    f32 baseScale;
    i32 unicodeMapCount;
    i32 maxGlyphCount;
    i32 glyphCount;
    i32 ascenderHeight;
    i32 descenderHeight;
    i32 lineAdvance;
    // u8 texture[texSize * texSize];
    // u16 unicodeMap[unicodeMapCount];
    // Glyph glyphs[maxGlyphCount];
    // i32 kerning[maxGlyphCount * maxGlyphCount];
} FontFile;
#pragma pack(pop)
#define font_file_magic()                    (((u32)'g' << 0) | ((u32)'a' << 8) | ((u32)'f' << 16) | ((u32)'f' << 24))
#define font_file_texture_offset(f)          sizeof(*(f))
#define font_file_unicode_offset(f)          (font_file_texture_offset(f) + ((f)->texSize * (f)->texSize))
#define font_file_glyph_offset(f)            (font_file_unicode_offset(f) + ((f)->unicodeMapCount * sizeof(u16)))
#define font_file_kerning_offset(f)          (font_file_glyph_offset(f) + ((f)->maxGlyphCount * sizeof(Glyph)))

typedef struct FontTexture
{
    Arena memory;
    Arena usedMem;
    i32 texSize;
    u8 *texture;         // [texSize * texSize]

    f32 baseScale;

    i32 unicodeMapCount;
    i32 maxGlyphCount;
    i32 glyphCount;
    u16 *unicodeMap;     // [unicodeMapCount]
    Glyph *glyphs;       // [maxGlyphCount]
    i32 *kerning;        // [maxGlyphCount * maxGlyphCount]

    i32 ascenderHeight;
    i32 descenderHeight;
    i32 lineAdvance;
} FontTexture;

typedef struct GuiFont
{
    FontTexture *texture;
    f32 pixelSize;
    f32 fontScale;
    f32 charScale;

    f32 letterSpacing;
    f32 ascenderHeight;
    f32 descenderHeight;
    f32 lineGap;
    f32 lineAdvance;
} GuiFont;

func buf
unzip_buffer(Arena *memory, buf zippedFile)
{
    u32 uncompressedSize = *(u32 *)(zippedFile.data + zippedFile.size - 4);

    buf result = {};

    z_stream zStream = {};
    zStream.zalloc = Z_NULL;
    zStream.zfree = Z_NULL;
    zStream.opaque = Z_NULL;
    zStream.next_in = (u8 *)zippedFile.data;
    zStream.avail_in = 0;

    // NOTE(michiel): See http://zlib.net/manual.html for the exact meanings.
    i32 zFlags = 32 | 15; // NOTE(michiel): ENABLE_ZLIB_GZIP | WINDOW_BITS
    i32 status = inflateInit2(&zStream, zFlags);
    if (status >= 0)
    {
        result.size = uncompressedSize;
        result.data = create(memory, byte, uncompressedSize);

        zStream.avail_in  = (u32)zippedFile.size;
        zStream.next_in   = (u8 *)zippedFile.data;
        zStream.avail_out = (u32)result.size;
        zStream.next_out  = (u8 *)result.data;

        if (zippedFile.size && result.size)
        {
            i32 zlibStatus = inflate(&zStream, Z_FINISH);
            if (zlibStatus != Z_STREAM_END) {
                result.size = 0;
                result.data = 0;
                //fprintf(stderr, "Couldn't unzip the file!\n");
            }
        }

        inflateEnd(&zStream);
    }
    else
    {
        //fprintf(stderr, "Could not initialize zlib to deflate.");
    }

    return result;
}

func u8 *get_texture_mipmap(u8 *data, i32 size)
{
    u8 *result = data; //data + size * (size / 2);
    return result;
}

func void downsample_texture(u8 *oldData, i32 oldSize, u8 *newData, i32 newSize)
{
    // expect oldSize == 2 * newSize
    for (i32 y = 0; y < newSize; ++y)
    {
        for (i32 x = 0;  x < newSize; ++x) {
            u8 pixX0Y0 = oldData[(y * 2) * oldSize + x * 2];
            u8 pixX1Y0 = oldData[(y * 2) * oldSize + x * 2 + 1];
            u8 pixX0Y1 = oldData[(y * 2 + 1) * oldSize + x * 2];
            u8 pixX1Y1 = oldData[(y * 2 + 1) * oldSize + x * 2 + 1];
            u32 newPix = (u32)pixX0Y0 + pixX1Y0 + pixX0Y1 + pixX1Y1;
            *newData++ = (u8)(newPix / 4);
        }
    }
}

func b32 font_load_texture(FontTexture *destFont, buf fontBuf)
{
    destFont->usedMem = destFont->memory;
    destFont->texSize = 0;
    destFont->texture = 0;
    destFont->baseScale = 0.0f;
    destFont->unicodeMapCount = 0;
    destFont->maxGlyphCount = 0;
    destFont->glyphCount = 0;
    destFont->unicodeMap = 0;
    destFont->glyphs = 0;
    destFont->kerning = 0;
    destFont->ascenderHeight = 0;
    destFont->descenderHeight = 0;
    destFont->lineAdvance = 0;

    b32 result = false;

    if (fontBuf.size)
    {
        // NOTE(michiel): GZIP'ed meta data
        if ((fontBuf.size > 10) && (fontBuf.data[0] == 0x1F) && (fontBuf.data[1] == 0x8B)) {
            fontBuf = unzip_buffer(&destFont->usedMem, fontBuf);
        }

        if (fontBuf.size >= sizeof(FontFile))
        {
            FontFile *fontFile = (FontFile *)fontBuf.data;
            if ((fontFile->totalSize == (u32)fontBuf.size) && (fontFile->magic == font_file_magic()))
            {
                destFont->baseScale = fontFile->baseScale;
                destFont->ascenderHeight = fontFile->ascenderHeight;
                destFont->descenderHeight = fontFile->descenderHeight;
                destFont->lineAdvance = fontFile->lineAdvance;
                destFont->unicodeMapCount = fontFile->unicodeMapCount;
                destFont->maxGlyphCount = fontFile->maxGlyphCount;
                destFont->glyphCount = fontFile->glyphCount;
                destFont->unicodeMap = (u16 *)(fontBuf.data + font_file_unicode_offset(fontFile));
                destFont->glyphs = (Glyph *)(fontBuf.data + font_file_glyph_offset(fontFile));
                destFont->kerning = (i32 *)(fontBuf.data + font_file_kerning_offset(fontFile));

                i32 texSize = fontFile->texSize;
                destFont->texSize = texSize;
                u8 *texture = (u8 *)fontBuf.data + font_file_texture_offset(fontFile);
                destFont->texture = create(&destFont->usedMem, u8, (texSize + texSize / 2) * texSize);
                memcpy(get_texture_mipmap(destFont->texture, texSize), texture, (usze)(texSize * texSize));
                while (texSize > 1) {
                    i32 newSize = texSize / 2;
                    downsample_texture(get_texture_mipmap(destFont->texture, texSize), texSize,
                                       get_texture_mipmap(destFont->texture, newSize), newSize);
                    texSize = newSize;
                }

                result = true;
            }
        }
    }

    return result;
}

func void font_setup(GuiFont *destFont, FontTexture *fontTexture, f32 pixelSize, f32 letterSpacing)
{
    destFont->texture = fontTexture;

    destFont->pixelSize = pixelSize;
    destFont->fontScale = pixelSize * fontTexture->baseScale;
    destFont->charScale = 64.0f * destFont->fontScale;

    destFont->letterSpacing   = letterSpacing;
    destFont->ascenderHeight  = destFont->fontScale * (f32)fontTexture->ascenderHeight;
    destFont->descenderHeight = destFont->fontScale * (f32)fontTexture->descenderHeight;
    destFont->lineAdvance     = (i32)(destFont->fontScale * (f32)fontTexture->lineAdvance + 1.0f);
    destFont->lineGap = destFont->lineAdvance - (destFont->ascenderHeight - destFont->descenderHeight);
}

func i32 font_get_kerning(FontTexture *font, i32 glyphIndex, i32 prevIndex)
{
    i32 result = 0;
    if ((glyphIndex < font->maxGlyphCount) && (prevIndex < font->maxGlyphCount)) {
        result = font->kerning[prevIndex * font->maxGlyphCount + glyphIndex];
    }
    return result;
}

typedef struct GlyphResult
{
    i32 glyphIndex;
    Glyph *glyph;
} GlyphResult;

func GlyphResult glyph_from_codepoint(FontTexture *font, u32 codepoint)
{
    u32 notFoundCodepoint = (0xFFFD >= font->unicodeMapCount) ? '?' : 0xFFFD;
    if (codepoint >= (u32)font->unicodeMapCount) { codepoint = notFoundCodepoint; }

    i32 glyphIndex = font->unicodeMap[codepoint];
    if (glyphIndex >= font->maxGlyphCount) { glyphIndex = font->unicodeMap[notFoundCodepoint]; }

    Glyph *glyph = font->glyphs + glyphIndex;
    if (glyph->xBaseAdvance == 0) { glyph = font->glyphs + font->unicodeMap[notFoundCodepoint]; }

    GlyphResult result = { glyphIndex, glyph };
    return result;
}

func f32 smoothstep_f32(f32 min, f32 t, f32 max)
{
    t = clamp_f32(0.0f, (t - min) / (max - min), 1.0f);
    f32 tSqr = t * t;
    f32 result = 3.0f * tSqr - 2.0f * tSqr * t;
    return result;
}

func f32 text_contour(f32 dist, f32 nudge)
{
    f32 result = smoothstep_f32(0.5f - nudge, dist, 0.5f + nudge);
    return result;
}

func f32 text_sample(u8 *texture, i32 texSize, v2 uvMin, v2 uvMax, v2 at)
{
    at.x = clamp_f32(0.0f, at.x, 1.0f);
    at.y = clamp_f32(0.0f, at.y, 1.0f);
    f32 sampleX = uvMin.x + at.x * (uvMax.x - uvMin.x);
    sampleX *= (f32)texSize;
    i32 minX = clamp_i32(0, (i32)sampleX, texSize - 1);
    i32 maxX = clamp_i32(0, (i32)sampleX + 1, texSize - 1);
    f32 tX = sampleX - (f32)(i32)sampleX;

    f32 sampleY = uvMin.y + at.y * (uvMax.y - uvMin.y);
    sampleY *= (f32)texSize;
    i32 minY = clamp_i32(0, (i32)sampleY, texSize - 1);
    i32 maxY = clamp_i32(0, (i32)sampleY + 1, texSize - 1);
    f32 tY = sampleY - (f32)(i32)sampleY;

    f32 sampleA = (f32)(texture[minY * texSize + minX]) / 255.0f;
    f32 sampleB = (f32)(texture[minY * texSize + maxX]) / 255.0f;
    f32 sampleC = (f32)(texture[maxY * texSize + minX]) / 255.0f;
    f32 sampleD = (f32)(texture[maxY * texSize + maxX]) / 255.0f;

    f32 sampleAB = lerp_f32(sampleA, tX, sampleB);
    f32 sampleCD = lerp_f32(sampleC, tX, sampleD);
    f32 result   = lerp_f32(sampleAB, tY, sampleCD);
    return result;
}

func void draw_char(DrawContext *ctx, GuiFont *font, f32 x, f32 y, u32 codepoint, u32 color)
{
    GlyphResult g = glyph_from_codepoint(font->texture, codepoint);
    Glyph *glyph = g.glyph;

    v2 offset = glyph->posMin;
    offset.x *= font->charScale;
    offset.y *= font->charScale;
    offset.y  = font->ascenderHeight - offset.y;
    v2 size   = v2_init((glyph->posMax.x - glyph->posMin.x) * font->charScale,
                        (glyph->posMax.y - glyph->posMin.y) * font->charScale);

    v2 p = v2_init(x + offset.x, y + offset.y);
    v2 pAt = v2_init(p.x - (f32)(i32)p.x, p.y - (f32)(i32)p.y);

    u32 *dst = ctx->pixels + (i32)(p.y + ((pAt.y >= 0.5f) ? 0.5f : 0.0f)) * ctx->stride + (i32)(p.x - 0.5f);

    f32 smoothing = 0.03f / font->charScale;
    v4 txtCol = unpack_color(color);

    i32 calcSize = (font->charScale < 1.0f ? (i32)(font->charScale * (f32)font->texture->texSize) : font->texture->texSize);
    BitScan msbBit = find_msb32((u32)calcSize);
    i32 texSize  = (msbBit.found ? (1 << msbBit.index) : 1);
    i32 texSize2 = texSize * 2;
    if (texSize2 <= font->texture->texSize)
    {
        u8 *texture0 = get_texture_mipmap(font->texture->texture, texSize);
        u8 *texture1 = get_texture_mipmap(font->texture->texture, texSize2);
        f32 texT = (f32)(calcSize - texSize) / (f32)texSize;
        for (i32 yAt = 0; yAt < (i32)(size.y + 1.0f); ++yAt)
        {
            u32 *d = dst;
            for (i32 xAt = 0; xAt < (i32)(size.x + 1.0f); ++xAt)
            {
                // TODO(michiel): Multisample??
                f32 dist0 = text_sample(texture0, texSize, glyph->uvMin, glyph->uvMax, v2_init(pAt.x / size.x, pAt.y / size.y));
                f32 dist1 = text_sample(texture1, texSize2, glyph->uvMin, glyph->uvMax, v2_init(pAt.x / size.x, pAt.y / size.y));
                f32 dist  = lerp_f32(dist0, texT, dist1);
                f32 alpha = text_contour(dist, smoothing);
                v4 dstCol = txtCol;
                dstCol.a *= alpha;
                v4 srcCol = unpack_color(*d);
                *d++ = pack_color(alpha_blend(srcCol, dstCol));
                pAt.x += 1.0f;
            }
            dst += ctx->stride;
            pAt.x = p.x - (f32)(i32)p.x;
            pAt.y += 1.0f;
        }
    }
    else
    {
        u8 *texture = get_texture_mipmap(font->texture->texture, texSize);
        for (i32 yAt = 0; yAt < (i32)(size.y + 1.0f); ++yAt)
        {
            u32 *d = dst;
            for (i32 xAt = 0; xAt < (i32)(size.x + 1.0f); ++xAt)
            {
                // TODO(michiel): Multisample??
                f32 dist  = text_sample(texture, texSize, glyph->uvMin, glyph->uvMax, v2_init((f32)xAt / size.x, (f32)yAt / size.y));
                f32 alpha = text_contour(dist, smoothing);
                v4 dstCol = txtCol;
                dstCol.a *= alpha;
                v4 srcCol = unpack_color(*d);
                *d++ = pack_color(alpha_blend(srcCol, dstCol));
            }
            dst += ctx->stride;
        }
    }
}

func void draw_text(DrawContext *ctx, GuiFont *font, f32 x, f32 y, s8 text, u32 color)
{
    b32 firstChar = true;
    s8 textAt = text;
    i32 prevIndex = 0;
    f32 atX = x;
    f32 atY = y + ((font->ascenderHeight - (f32)(i32)font->ascenderHeight));

    while (textAt.size)
    {
        u32 codepoint = utf8_decode(&textAt);
        if (codepoint != U32_MAX)
        {
            if (codepoint == '\n')
            {
                atX = x;
                atY += font->lineAdvance;
                prevIndex = 0;
                firstChar = true;
            }
            else
            {
                GlyphResult foundGlyph = glyph_from_codepoint(font->texture, codepoint);
                i32 glyphIndex = foundGlyph.glyphIndex;
                Glyph *glyph = foundGlyph.glyph;

                i32 kerning = font_get_kerning(font->texture, glyphIndex, prevIndex);
                atX += (200.0f * font->fontScale * (f32)kerning);
                if (firstChar) {
                    atX -= (f32)glyph->glyphOffset.x * font->fontScale;
                }

                draw_char(ctx, font, atX, atY, codepoint, color);
                atX += font->fontScale * (f32)glyph->xBaseAdvance + font->letterSpacing;
                prevIndex = glyphIndex;
                firstChar = false;
            }
        }
        else
        {
            //fprintf(stderr, "Malformed UTF-8 codepoint '%#02X'.\n", (u8)textAt.data[0]);
            textAt = s8adv(textAt, 1);
            prevIndex = 0;
        }
    }
}
