#include "../src/defines.h"
#include <stdint.h>
#include <stddef.h>

#if PLATFORM_LINUX

#include <stdlib.h>

#include <unistd.h>
#include <time.h>

// For memory
#include <sys/mman.h>

// For files
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#else

#error Platform not supported for X11

#endif

#include "../src/types.h"
#include "../src/helpers.h"
#include "../src/helper_funcs.h"
#include "../src/memory.h"
#include "../src/memarena.h"
#include "../src/strings.h"
#include "../src/files.h"
#include "../src/image.h"

#include "x11helpers.h"

func u64 random_wyhash(u64 *state)
{
    u64 s = *state;
    s += IMM_U64(0x60BEE2BEE120FC15);
    *state = s;
#if COMPILER_MSVC
    u64 high;
    u64 low = _umul128(s, IMM_U64(0xA3B195354A39B70D), &high);
    u64 m1 = high ^ low;
    low = _umul128(m1, IMM_U64(0x1B03738712FAD5C9), &high);
    u64 m2 = high ^ low;
#else
    __uint128_t tmp;
    tmp = (__uint128_t)s * IMM_U64(0xA3B195354A39B70D);
    u64 m1 = (u64)(tmp >> 64) ^ (u64)tmp;
    tmp = (__uint128_t)m1 * IMM_U64(0x1B03738712FAD5C9);
    u64 m2 = (u64)(tmp >> 64) ^ (u64)tmp;
#endif
    return m2;
}

func sze
random_index(u64 *state, sze count)
{
    sze result = 0;

    if (ispow2(count))
    {
        result = (sze)(random_wyhash(state) & (u64)(count - 1));
    }
    else
    {
        // NOTE(michiel): Discard anything below the threshold so we get a fair result.
        u64 threshold = (u64)(-count) % (u64)count;

        // NOTE(michiel): Most of the time this loop will take a single iteration
        while (1)
        {
            u64 r = random_wyhash(state);
            if (r >= threshold) {
                result = (sze)(r % (u64)count);
                break;
            }
        }
    }

    return result;
}


typedef enum TileKind
{
    TileKind_Blank,
    TileKind_Up,
    TileKind_Right,
    TileKind_Down,
    TileKind_Left,
    TileKindCount,
} TileKind;

typedef enum TileOption
{
    TileOption_Blank  = 1 << TileKind_Blank,
    TileOption_Up     = 1 << TileKind_Up,
    TileOption_Right  = 1 << TileKind_Right,
    TileOption_Down   = 1 << TileKind_Down,
    TileOption_Left   = 1 << TileKind_Left,
    TileOption_All    = TileOption_Blank | TileOption_Up | TileOption_Right | TileOption_Down | TileOption_Left,
} TileOption;

#define PIXELS_PER_STEP       16
#define TILE_DIMENSION        (3*PIXELS_PER_STEP)
global Image gTiles[TileKindCount];
global flag32(TileOption) gTileNeighbours[TileKindCount][4];

typedef struct GridTile
{
    TileKind collapsed;
    flag32(TileOption) options;
} GridTile;

#define GRID_DIMENSION        10
global GridTile gGrid[GRID_DIMENSION * GRID_DIMENSION];
global GridTile gTempGrid[GRID_DIMENSION * GRID_DIMENSION];
global GridTile *gOptionGrid[GRID_DIMENSION * GRID_DIMENSION];
global sze gOptionIdx;

global b32 gRunCollapse;
global u64 gEntropy = IMM_U64(12742019437092147);

func void load_images(Arena *perm)
{
    gTiles[0] = create_image(TILE_DIMENSION, TILE_DIMENSION, perm);
    gTiles[1] = create_image(TILE_DIMENSION, TILE_DIMENSION, perm);
    gTiles[2] = create_image(TILE_DIMENSION, TILE_DIMENSION, perm);
    gTiles[3] = create_image(TILE_DIMENSION, TILE_DIMENSION, perm);
    gTiles[4] = create_image(TILE_DIMENSION, TILE_DIMENSION, perm);
    for (sze row = 0; row < TILE_DIMENSION; ++row)
    {
        for (sze col = 0; col < TILE_DIMENSION; ++col)
        {
            gTiles[0].pixels[row * gTiles[0].stride + col] = 0xFF808080;
            if (((row < PIXELS_PER_STEP) && (col >= PIXELS_PER_STEP) && (col < 2*PIXELS_PER_STEP)) ||
                ((row >= PIXELS_PER_STEP) && (row < 2*PIXELS_PER_STEP)))
            {
                gTiles[1].pixels[row * gTiles[1].stride + col] = 0xFF101010;
            }
            else
            {
                gTiles[1].pixels[row * gTiles[1].stride + col] = 0xFF808080;
            }
            if (((col >= PIXELS_PER_STEP) && (col < 2*PIXELS_PER_STEP)) ||
                ((row >= PIXELS_PER_STEP) && (row < 2*PIXELS_PER_STEP) && (col >= 2*PIXELS_PER_STEP)))
            {
                gTiles[2].pixels[row * gTiles[2].stride + col] = 0xFF101010;
            }
            else
            {
                gTiles[2].pixels[row * gTiles[2].stride + col] = 0xFF808080;
            }
            if (((row >= 2*PIXELS_PER_STEP) && (col >= PIXELS_PER_STEP) && (col < 2*PIXELS_PER_STEP)) ||
                ((row >= PIXELS_PER_STEP) && (row < 2*PIXELS_PER_STEP)))
            {
                gTiles[3].pixels[row * gTiles[3].stride + col] = 0xFF101010;
            }
            else
            {
                gTiles[3].pixels[row * gTiles[3].stride + col] = 0xFF808080;
            }
            if (((col >= PIXELS_PER_STEP) && (col < 2*PIXELS_PER_STEP)) ||
                ((row >= PIXELS_PER_STEP) && (row < 2*PIXELS_PER_STEP) && (col < PIXELS_PER_STEP)))
            {
                gTiles[4].pixels[row * gTiles[4].stride + col] = 0xFF101010;
            }
            else
            {
                gTiles[4].pixels[row * gTiles[4].stride + col] = 0xFF808080;
            }
        }
    }
}

func void init_grid(Arena *perm)
{
    gTileNeighbours[TileKind_Blank][0] = TileOption_Blank | TileOption_Up;
    gTileNeighbours[TileKind_Blank][1] = TileOption_Blank | TileOption_Right;
    gTileNeighbours[TileKind_Blank][2] = TileOption_Blank | TileOption_Down;
    gTileNeighbours[TileKind_Blank][3] = TileOption_Blank | TileOption_Left;

    gTileNeighbours[TileKind_Up][0] = TileOption_Right | TileOption_Down | TileOption_Left;
    gTileNeighbours[TileKind_Up][1] = TileOption_Up | TileOption_Down | TileOption_Left;
    gTileNeighbours[TileKind_Up][2] = TileOption_Blank | TileOption_Down;
    gTileNeighbours[TileKind_Up][3] = TileOption_Up | TileOption_Right | TileOption_Down;

    gTileNeighbours[TileKind_Right][0] = TileOption_Right | TileOption_Down | TileOption_Left;
    gTileNeighbours[TileKind_Right][1] = TileOption_Up | TileOption_Down | TileOption_Left;
    gTileNeighbours[TileKind_Right][2] = TileOption_Up | TileOption_Right | TileOption_Left;
    gTileNeighbours[TileKind_Right][3] = TileOption_Blank | TileOption_Left;

    gTileNeighbours[TileKind_Down][0] = TileOption_Blank | TileOption_Up;
    gTileNeighbours[TileKind_Down][1] = TileOption_Up | TileOption_Down | TileOption_Left;
    gTileNeighbours[TileKind_Down][2] = TileOption_Up | TileOption_Right | TileOption_Left;
    gTileNeighbours[TileKind_Down][3] = TileOption_Up | TileOption_Right | TileOption_Down;

    gTileNeighbours[TileKind_Left][0] = TileOption_Right | TileOption_Down | TileOption_Left;
    gTileNeighbours[TileKind_Left][1] = TileOption_Blank | TileOption_Right;
    gTileNeighbours[TileKind_Left][2] = TileOption_Up | TileOption_Right | TileOption_Left;
    gTileNeighbours[TileKind_Left][3] = TileOption_Up | TileOption_Right | TileOption_Down;

    for (sze row = 0; row < GRID_DIMENSION; ++row)
    {
        for (sze col = 0; col < GRID_DIMENSION; ++col)
        {
            gGrid[row * GRID_DIMENSION + col].collapsed = TileKindCount;
            gGrid[row * GRID_DIMENSION + col].options = TileOption_All;
        }
    }

    gRunCollapse = false;
}

func void find_lowest_entropy(void)
{
    u32 minOptionCount = U32_MAX;
    gOptionIdx = 0;
    for (sze row = 0; row < GRID_DIMENSION; ++row)
    {
        for (sze col = 0; col < GRID_DIMENSION; ++col)
        {
            GridTile *tile = gGrid + row * GRID_DIMENSION + col;
            if (tile->collapsed == TileKindCount)
            {
                u32 optionCount = popcount32(tile->options);
                if (optionCount == 0) {
                    debugbreak();
                } else if (optionCount == 1) {
                    BitScan scan = find_lsb32(tile->options);
                    if (!scan.found) {
                        debugbreak();
                    } else {
                        tile->collapsed = (TileKind)scan.index;
                    }
                    gOptionIdx = 0;
                    return;
                } else if (optionCount < minOptionCount) {
                    gOptionIdx = 0;
                    gOptionGrid[gOptionIdx++] = tile;
                    minOptionCount = optionCount;
                } else if (optionCount == minOptionCount) {
                    gOptionGrid[gOptionIdx++] = tile;
                }
            }
        }
    }
}

func void collapse_lowest_entropy(void)
{
    if (gOptionIdx)
    {
        sze randomIdx = random_index(&gEntropy, gOptionIdx);
        GridTile *useTile = gOptionGrid[randomIdx];
        u32 optionCount = popcount32(useTile->options);
        sze randomOption = random_index(&gEntropy, (sze)optionCount);
        for (TileKind option = TileKind_Blank; option < TileKindCount; ++option)
        {
            if (useTile->options & (1 << option)) {
                if (randomOption-- == 0) {
                    useTile->collapsed = option;
                    useTile->options = 1 << option;
                    break;
                }
            }
        }
    }
}

func void check_neighbours(void)
{
    for (sze row = 0; row < GRID_DIMENSION; ++row)
    {
        for (sze col = 0; col < GRID_DIMENSION; ++col)
        {
            GridTile *tile = gTempGrid + row * GRID_DIMENSION + col;
            *tile = gGrid[row * GRID_DIMENSION + col];
            if (tile->collapsed == TileKindCount)
            {
                tile->options = TileOption_All;
                if (row > 0) {
                    // NOTE(michiel): Upper neighbour
                    GridTile *neighbour = gGrid + (row - 1) * GRID_DIMENSION + col;
                    if (neighbour->collapsed < TileKindCount) {
                        tile->options &= gTileNeighbours[neighbour->collapsed][2];
                    }
                }
                if (col < (GRID_DIMENSION - 1)) {
                    // NOTE(michiel): Right neighbour
                    GridTile *neighbour = gGrid + row * GRID_DIMENSION + (col + 1);
                    if (neighbour->collapsed < TileKindCount) {
                        tile->options &= gTileNeighbours[neighbour->collapsed][3];
                    }
                }
                if (row < (GRID_DIMENSION - 1)) {
                    // NOTE(michiel): Lower neighbour
                    GridTile *neighbour = gGrid + (row + 1) * GRID_DIMENSION + col;
                    if (neighbour->collapsed < TileKindCount) {
                        tile->options &= gTileNeighbours[neighbour->collapsed][0];
                    }
                }
                if (col > 0) {
                    // NOTE(michiel): Left neighbour
                    GridTile *neighbour = gGrid + row * GRID_DIMENSION + (col - 1);
                    if (neighbour->collapsed < TileKindCount) {
                        tile->options &= gTileNeighbours[neighbour->collapsed][1];
                    }
                }

#if 0
                if (tile->options == 0) {
                    debugbreak();
                } else if (popcount32(tile->options) == 1) {
                    BitScan scan = find_lsb32(tile->options);
                    if (!scan.found) {
                        debugbreak();
                    } else {
                        tile->collapsed = scan.index;
                    }
                }
#endif
            }
        }
    }
    memcpy(gGrid, gTempGrid, sizeof(gGrid));
}

func void fill_screen(u8 *screen, i32 width, i32 height, i32 pixelBytes)
{
#if 0
    find_lowest_entropy();
    collapse_lowest_entropy();
    check_neighbours();
#endif

    i32 pitch = width * pixelBytes;
    for (i32 row = 0; row < height; ++row)
    {
        u8 *r = screen + pitch * row;
        for (i32 col = 0; col < width; ++col)
        {
            u32 *p = (u32 *)(r + col * pixelBytes);
            i32 x = (col / TILE_DIMENSION);
            i32 y = (row / TILE_DIMENSION);
            if (x >= GRID_DIMENSION) {
                x = y = GRID_DIMENSION;
            }
            i32 gridIdx = y * GRID_DIMENSION + x;
            i32 imageIdx = -1;
            if (gridIdx < countof(gGrid)) {
                if (gGrid[gridIdx].collapsed < TileKindCount) {
                    imageIdx = (i32)gGrid[gridIdx].collapsed;
                }
            }

            i32 pixelIdx = (row % TILE_DIMENSION) * (i32)gTiles[0].stride + (col % TILE_DIMENSION);
            switch (imageIdx)
            {
                case 0: { *p = gTiles[0].pixels[pixelIdx]; } break;
                case 1: { *p = gTiles[1].pixels[pixelIdx]; } break;
                case 2: { *p = gTiles[2].pixels[pixelIdx]; } break;
                case 3: { *p = gTiles[3].pixels[pixelIdx]; } break;
                case 4: { *p = gTiles[4].pixels[pixelIdx]; } break;
                default: {
                    if ((col % 16) && (row % 16)) {
                        *p = 0xFFFFFFFF;
                    } else {
                        *p = 0;
                    }
                } break;
            }
        }
    }
}

int main(int argCount, char **arguments)
{
    Arena permArena = create_arena(16*1024*1024);
    Arena tempArena = create_arena(1024*1024);

    Arena winArena  = create_arena(1024*1024*1024);
    Arena windArena = winArena;

    OsFile output = open_file(cstr("stdout"), OsFile_Write, &permArena, tempArena);

    gEntropy = (u64)(uptr)&output;
    gEntropy = 140737488346560;

    load_images(&permArena);
    init_grid(&permArena);

    i32 x = 200;
    i32 y = 500;
    i32 width = 800;
    i32 height = 600;

    Display *display = XOpenDisplay(0);
    if (!display)
    {
        write_str_to_file(&output, cstr("No display available!\n"));
        exit(1);
    }

    u64 root = DefaultRootWindow(display);
    i32 defaultScreen = DefaultScreen(display);

    i32 screenDepth = 24;
    XVisualInfo visInfo = {0};
    if (!XMatchVisualInfo(display, defaultScreen, screenDepth, TrueColor, &visInfo))
    {
        write_str_to_file(&output, cstr("No matching visual info found!\n"));
        exit(1);
    }

    XSetWindowAttributes windowAttr;
    windowAttr.bit_gravity = StaticGravity;
    windowAttr.background_pixel = 0;
    windowAttr.colormap = XCreateColormap(display, root, visInfo.visual, AllocNone);
    windowAttr.event_mask = StructureNotifyMask | PropertyChangeMask | KeyPressMask | KeyReleaseMask;
    u64 attributeMask = CWBitGravity | CWBackPixel | CWColormap | CWEventMask;

    u32 borderWidth = 0;
    Window window = XCreateWindow(display, root, x, y, (u32)width, (u32)height, borderWidth, visInfo.depth, InputOutput,
                                  visInfo.visual, attributeMask, &windowAttr);

    if (!window)
    {
        write_str_to_file(&output, cstr("No window was created!\n"));
        exit(1);
    }

    XStoreName(display, window, "Collapsing Wave Function");
    window_set_size_hints(display, window, 400, 300, 0, 0);

    XIC xInputContext = window_setup_utf8_input(display, window, &output);
    if (!xInputContext) {
        exit(1);
    }

    XMapWindow(display, window);
    XFlush(display);

    Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    if (!XSetWMProtocols(display, window, &wmDeleteWindow, 1))
    {
        write_str_to_file(&output, cstr("Couldn't register WM_DELETE_WINDOW property!\n"));
    }

    i32 pixelBits = 32;
    i32 pixelBytes = pixelBits / 8;
    i32 windowBufferSize = width * height * pixelBytes;
    u8 *windowMem = create(&windArena, u8, windowBufferSize, Alloc_NoClear);
    XImage *xWindowBuf = XCreateImage(display, visInfo.visual, (u32)visInfo.depth, ZPixmap, 0, (char *)windowMem, (u32)width, (u32)height, pixelBits, 0);
    GC defaultGc = DefaultGC(display, defaultScreen);

    s8 inputBuf = create_empty_s8(&permArena, 1024);
    s8 inputStr = inputBuf;

    b32 sizeChanged = false;
    b32 windowIsOpen = true;
    while (windowIsOpen)
    {
        inputStr.size = 0;
        XEvent event;
        while (XPending(display) > 0)
        {
            XNextEvent(display, &event);
            switch (event.type)
            {
                case DestroyNotify: {
                    XDestroyWindowEvent *e = (XDestroyWindowEvent *)&event;
                    if (e->window == window) {
                        windowIsOpen = false;
                    }
                } break;

                case ClientMessage: {
                    XClientMessageEvent *e = (XClientMessageEvent *)&event;
                    if ((Atom)e->data.l[0] == wmDeleteWindow) {
                        XDestroyWindow(display, window);
                        windowIsOpen = false;
                    }
                } break;

                case ConfigureNotify: {
                    XConfigureEvent *e = (XConfigureEvent *)&event;
                    width = e->width;
                    height = e->height;
                    sizeChanged = true;
                } break;

                case KeyPress: {
                    XKeyPressedEvent *e = (XKeyPressedEvent *)&event;
                    if (e->keycode == XKeysymToKeycode(display, XK_Left))  { write_str_to_file(&output, cstr("Left pressed\n")); }
                    if (e->keycode == XKeysymToKeycode(display, XK_Right)) { write_str_to_file(&output, cstr("Right pressed\n")); }
                    if (e->keycode == XKeysymToKeycode(display, XK_Up))    { write_str_to_file(&output, cstr("Up pressed\n")); }
                    if (e->keycode == XKeysymToKeycode(display, XK_Down))  { write_str_to_file(&output, cstr("Down pressed\n")); }

                    Status status = 0;
                    char *inputAt = (char *)(inputStr.data + inputStr.size);
                    i32 inputRem = (i32)(inputBuf.size - inputStr.size);
                    i32 byteCount = Xutf8LookupString(xInputContext, e, inputAt, inputRem, 0, &status);
                    if (status == XLookupChars) {
                        inputStr.size += byteCount;
                    }
                } break;

                case KeyRelease: {
                    XKeyPressedEvent *e = (XKeyPressedEvent *)&event;
                    if (e->keycode == XKeysymToKeycode(display, XK_Left))  { write_str_to_file(&output, cstr("Left released\n")); }
                    if (e->keycode == XKeysymToKeycode(display, XK_Right)) { write_str_to_file(&output, cstr("Right released\n")); }
                    if (e->keycode == XKeysymToKeycode(display, XK_Up))    { write_str_to_file(&output, cstr("Up released\n")); }
                    if (e->keycode == XKeysymToKeycode(display, XK_Down))  { write_str_to_file(&output, cstr("Down released\n")); }
                } break;

                default: {} break;
            }
        }

        if (inputStr.size)
        {
            if (inputStr.data[0] == 'r') {
                init_grid(&permArena);
            } else if (inputStr.data[0] == '1') {
                find_lowest_entropy();
            } else if (inputStr.data[0] == '2') {
                collapse_lowest_entropy();
            } else if (inputStr.data[0] == '3') {
                check_neighbours();
            } else if (inputStr.data[0] == 's') {
                find_lowest_entropy();
                collapse_lowest_entropy();
                check_neighbours();
            } else if (inputStr.data[0] == 'c') {
                gRunCollapse = true;
            } else if (inputStr.data[0] == 'n') {
                init_grid(&permArena);
                gRunCollapse = true;
            }
        }

        if (sizeChanged)
        {
            sizeChanged = false;
            xWindowBuf->data = 0;
            XDestroyImage(xWindowBuf);
            windowBufferSize = width * height * pixelBytes;
            windArena = winArena;
            windowMem = create(&windArena, u8, windowBufferSize, Alloc_NoClear);
            xWindowBuf = XCreateImage(display, visInfo.visual, (u32)visInfo.depth, ZPixmap, 0, (char *)windowMem, (u32)width, (u32)height, pixelBits, 0);
        }

        if (gRunCollapse) {
            find_lowest_entropy();
            collapse_lowest_entropy();
            check_neighbours();
        }

        fill_screen(windowMem, width, height, pixelBytes);

        XPutImage(display, window, defaultGc, xWindowBuf, 0, 0, 0, 0, (u32)width, (u32)height);
    }

    return 0;
}
