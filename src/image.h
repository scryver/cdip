
#define IMAGE_ALPHA_MASK   0xFF000000
#define IMAGE_RED_MASK     0x00FF0000
#define IMAGE_GREEN_MASK   0x0000FF00
#define IMAGE_BLUE_MASK    0x000000FF

#define IMAGE_ALPHA_SHIFT  24
#define IMAGE_RED_SHIFT    16
#define IMAGE_GREEN_SHIFT   8
#define IMAGE_BLUE_SHIFT    0

#if 0
typedef enum ImageFlags
{
    ImageFlag_1bpc      = 0x00,
    ImageFlag_8bpc      = 0x00,
    ImageFlag_16bpc     = 0x01,
    ImageFlag_24bpc     = 0x02,
    ImageFlag_32bpc     = 0x04,
    ImageFlag_bpcMask   = 0x07,
    ImageFlag_Greyscale = 1 << 3,
    ImageFlag_NoAlpha   = 1 << 4,
} ImageFlags;

typedef struct Image
{
    flag32(ImageFlags) flags;
    i32 width;
    i32 height;
    i32 stride;
    byte *pixels;
} Image;

func sze calc_image_byte_size(i32 width, i32 height, flag32(ImageFlags) flags)
{
    sze w = (sze)width;
    sze h = (sze)height;
    w = (w < 0) ? -w : w;
    h = (h < 0) ? -h : h;
    sze pixelCount = w * h;
    sze colourSize = pixelCount;
    u32 bpc = (flags & ImageFlag_bpcMask);
    switch (bpc) {
        case ImageFlag_1bpc:  { colourSize = (colourSize + 7) / 8; } break;
        case ImageFlag_8bpc:  { } break;
        case ImageFlag_16bpc: { colourSize *= 2; } break;
        case ImageFlag_24bpc: { colourSize *= 3; } break;
        case ImageFlag_32bpc: { colourSize *= 4; } break;
        invalid_default;
    }

    sze totalSize;
    if ((flags & ImageFlag_Greyscale) == 0) {
        totalSize = colourSize * 3;
    }
    if ((flags & ImageFlag_NoAlpha) == 0) {
        totalSize += colourSize;
    }

    return totalSize;
}

func sze image_byte_size(Image image)
{
    sze size = calc_image_byte_size(image.stride, image.height, image.flags);
    return size;
}

func Image create_image(i32 width, i32 height, flag32(ImageFlags) flags, Arena *memory)
{
    sze size = calc_image_byte_size(width, height, flags);
    Image result;
    result.flags = flags;
    result.width = result.stride = width;
    result.height = height;
    result.pixels = create(memory, byte, size);
    return result;
}
#endif

typedef struct Image
{
    sze width;
    sze height;
    sze stride;
    u32 *pixels;
} Image;

func sze calc_image_pixel_count(sze width, sze height)
{
    sze w = (sze)width;
    sze h = (sze)height;
    w = (w < 0) ? -w : w;
    h = (h < 0) ? -h : h;
    sze result = w * h;
    if ((h == 0) || (w != (result / h))) {
        result = 0;
    }
    return result;
}

func sze image_pixel_count(Image *image)
{
    sze size = calc_image_pixel_count(image->width, image->height);
    return size;
}

func Image create_image(i32 width, i32 height, Arena *memory)
{
    sze size = calc_image_pixel_count(width, height);
    Image result;
    result.width = result.stride = width;
    result.height = height;
    result.pixels = create(memory, u32, size);
    return result;
}
