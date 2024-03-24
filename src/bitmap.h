#define BITMAP_FILE_WIN_BMP      0x4D42
#define BITMAP_FILE_OS2_ARRAY    0x4142
#define BITMAP_FILE_OS2_CICON    0x4943
#define BITMAP_FILE_OS2_CPTR     0x5043
#define BITMAP_FILE_OS2_ICON     0x4349
#define BITMAP_FILE_OS2_PTR      0x5450

typedef enum BitmapCompression
{
    BitmapCompression_None      = 0,
    BitmapCompression_RLE8      = 1,
    BitmapCompression_RLE4      = 2,
    BitmapCompression_BitFields = 3,
    BitmapCompression_Jpeg      = 4,
    BitmapCompression_PNG       = 5,
    BitmapCompression_AlphaBit  = 6,
    BitmapCompression_CMYK      = 11,
    BitmapCompression_CMYK_RLE8 = 12,
    BitmapCompression_CMYK_RLE4 = 13,
} BitmapCompression;

#pragma pack(push, 1)
typedef struct BitmapFileHeader
{
    // NOTE(michiel): BM: win bmp, BA: OS/2 bitmap array, CI: OS/2 color icon, CP: OS/2 color pointer, IC: OS/2 icon, PT: OS/2 pointer
    u16 fileType;
    u32 fileSize;
    u16 reserved1;
    u16 reserved2;
    u32 pixelOffset; // NOTE(michiel): Offset from start of this struct to the pixel data
} BitmapFileHeader;

typedef struct BitmapInfoHeader {
    u32 size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bitsPerPixel;
    u32 compression;
    u32 sizeOfBitmap;  // NOTE(michiel): The size of the raw bitmap data, 0 can be given for non-compressed bitmaps.
    i32 horzResolution; // NOTE(michiel): In pixels-per-meter
    i32 vertResolution;
    u32 colorsUsed;
    u32 colorsImportant;
    // V2
    u32 redMask;
    u32 greenMask;
    u32 blueMask;
    // V3
    u32 alphaMask;
    // V4
    u32 colorSpaceType;
    u32 endpoints[9];
    u32 gammaRed;
    u32 gammaGreen;
    u32 gammaBlue;
    // V5
    u32 intent;
    u32 profileData;
    u32 profileSize;
    u32 reserved;
} BitmapInfoHeader;
#pragma pack(pop)

#define BITMAP_INFO_HDR_V1_SIZE      offsetof(BitmapInfoHeader, redMask)
#define BITMAP_INFO_HDR_V2_SIZE      offsetof(BitmapInfoHeader, alphaMask)
#define BITMAP_INFO_HDR_V3_SIZE      offsetof(BitmapInfoHeader, colorSpaceType)
#define BITMAP_INFO_HDR_V4_SIZE      offsetof(BitmapInfoHeader, intent)
#define BITMAP_INFO_HDR_V5_SIZE      sizeof(BitmapInfoHeader)

func Image bitmap_load(buf bitmapBuf, Arena *perm)
{
    Image result = {0};
    if (bitmapBuf.size > (sizeof(BitmapFileHeader) + 4))
    {
        BitmapFileHeader *header = (BitmapFileHeader *)bitmapBuf.data;
        if ((header->fileType == BITMAP_FILE_WIN_BMP) &&
            (header->fileSize == bitmapBuf.size) &&
            (header->pixelOffset < bitmapBuf.size))
        {
            u32 *pixelsAt = (u32 *)(bitmapBuf.data + header->pixelOffset);

            BitmapInfoHeader *infoHeader = (BitmapInfoHeader *)(bitmapBuf.data + sizeof(BitmapFileHeader));
            if ((infoHeader->width > 0) &&
                (infoHeader->planes == 1) &&
                (infoHeader->bitsPerPixel == 32) &&
                ((infoHeader->compression == BitmapCompression_None) ||
                 (infoHeader->compression == BitmapCompression_BitFields) ||
                 (infoHeader->compression == BitmapCompression_AlphaBit)))
            {
                u32 redMask   = IMAGE_RED_MASK;
                u32 greenMask = IMAGE_GREEN_MASK;
                u32 blueMask  = IMAGE_BLUE_MASK;
                if ((infoHeader->size >= BITMAP_INFO_HDR_V2_SIZE) &&
                    ((infoHeader->compression == BitmapCompression_BitFields) ||
                     (infoHeader->compression == BitmapCompression_AlphaBit)))
                {
                    redMask = infoHeader->redMask;
                    greenMask = infoHeader->greenMask;
                    blueMask = infoHeader->blueMask;
                }
                u32 alphaMask = ~(redMask | greenMask | blueMask);
                if (((infoHeader->size >= BITMAP_INFO_HDR_V3_SIZE) && infoHeader->alphaMask) ||
                    (infoHeader->compression == BitmapCompression_AlphaBit)) {
                    alphaMask = infoHeader->alphaMask;
                }

                BitScan rScan = find_lsb32(redMask);
                BitScan gScan = find_lsb32(greenMask);
                BitScan bScan = find_lsb32(blueMask);
                BitScan aScan = find_lsb32(alphaMask);

                if (rScan.found && gScan.found && bScan.found && aScan.found)
                {
                    i32 rShift = rScan.index;
                    i32 gShift = gScan.index;
                    i32 bShift = bScan.index;
                    i32 aShift = aScan.index;

                    b32 flippedHeight = (infoHeader->height < 0);

                    result = create_image(infoHeader->width, infoHeader->height, perm);
                    if (result.pixels)
                    {
                        u32 *src = pixelsAt;
                        u32 *dst = result.pixels;
                        sze height = result.height < 0 ? -result.height : result.height;
                        sze stride = result.stride;

                        if (!flippedHeight) {
                            dst = result.pixels + stride * (height - 1);
                            stride = -stride;
                        }

                        for (sze rowIdx = 0; rowIdx < height; ++rowIdx) {
                            u32 *p = dst;
                            for (sze colIdx = 0; colIdx < result.width; ++colIdx) {
                                u32 color = *src++;
                                *p++ = ((((color & alphaMask) >> aShift) << IMAGE_ALPHA_SHIFT) |
                                        (((color & redMask)   >> rShift) << IMAGE_RED_SHIFT) |
                                        (((color & greenMask) >> gShift) << IMAGE_GREEN_SHIFT) |
                                        (((color & blueMask)  >> bShift) << IMAGE_BLUE_SHIFT));
                            }
                            dst += stride;
                        }
                    }
                }
            }
        }
    }

    return result;
}

func b32 bitmap_save(Image *image, s8 filename, Arena scratch)
{
    sze byteSize = image_pixel_count(image) * sizeof(u32);
    BitmapFileHeader *fileHeader = create(&scratch, BitmapFileHeader);
    fileHeader->fileType = BITMAP_FILE_WIN_BMP;
    fileHeader->fileSize = (u32)(sizeof(BitmapFileHeader) + BITMAP_INFO_HDR_V5_SIZE + byteSize);
    fileHeader->pixelOffset = sizeof(BitmapFileHeader) + BITMAP_INFO_HDR_V5_SIZE;
    BitmapInfoHeader *infoHeader = create(&scratch, BitmapInfoHeader);
    infoHeader->size = BITMAP_INFO_HDR_V5_SIZE;
    infoHeader->width = (i32)image->width;
    infoHeader->height = (i32)image->height;
    infoHeader->planes = 1;
    infoHeader->bitsPerPixel = 32;
    infoHeader->compression = BitmapCompression_BitFields;
    infoHeader->sizeOfBitmap = (u32)byteSize;
    infoHeader->horzResolution = 11811; // NOTE(michiel): 300ppi -> 300 * 100 / 2.54
    infoHeader->vertResolution = 11811;
    infoHeader->redMask   = IMAGE_RED_MASK;
    infoHeader->greenMask = IMAGE_GREEN_MASK;
    infoHeader->blueMask  = IMAGE_BLUE_MASK;
    infoHeader->alphaMask = IMAGE_ALPHA_MASK;

    OsFile outFile = open_file(filename, OsFile_Write, 0, scratch);
    sze result = write_to_file(&outFile, buf(sizeof(BitmapFileHeader), fileHeader));
    result += write_to_file(&outFile, buf(sizeof(BitmapInfoHeader), infoHeader));

    sze stride = image->stride;
    sze height = (image->height < 0) ? -image->height : image->height;
    buf writeBuf = buf(image->width * sizeof(u32), image->pixels + stride * (height - 1));
    stride = -stride * sizeof(u32);
    for (i32 yLine = 0; yLine < height; ++yLine)
    {
        result += write_to_file(&outFile, writeBuf);
        writeBuf.data += stride;
    }
    close_file(&outFile);
    return (result == (sze)fileHeader->fileSize);
}
