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

#if 0
typedef struct OsWindow
{
    Display *display;
    Window window;
    b32 isOpen;
} OsWindow;
#endif

#include "x11helpers.h"
//#include <X11/Xft/Xft.h>
#include "drawing.c"
#include "fonts.c"
#include "ui.c"

int main(int argCount, char **arguments)
{
    Arena permArena = create_arena(16*1024*1024);
    Arena tempArena = create_arena(1024*1024);

    Arena winArena  = create_arena(1024*1024*1024);
    Arena windArena = winArena;

    OsFile output = open_file(cstr("stdout"), OsFile_Write, &permArena, tempArena);

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
    windowAttr.event_mask = (StructureNotifyMask | PropertyChangeMask |
                             KeyPressMask | KeyReleaseMask |
                             PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
    u64 attributeMask = CWBitGravity | CWBackPixel | CWColormap | CWEventMask;

    u32 borderWidth = 0;
    Window window = XCreateWindow(display, root, x, y, (u32)width, (u32)height, borderWidth, visInfo.depth, InputOutput,
                                  visInfo.visual, attributeMask, &windowAttr);

    if (!window)
    {
        write_str_to_file(&output, cstr("No window was created!\n"));
        exit(1);
    }

    XStoreName(display, window, "A X11 Window Test");
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

#if 0
    XftFont *font = XftFontOpenName(display, defaultScreen, "freesans:pixelsize=16");
    if (!font) {
        write_str_to_file(&output, cstr("No font found!\n"));
        exit(1);
    }
    XftColor fontColor;
    if(!XftColorAllocName(display, visInfo.visual, windowAttr.colormap, "#000000", &fontColor)) {
        write_str_to_file(&output, cstr("No font color allocated!\n"));
        exit(1);
    }
    XftDraw *fontDraw = XftDrawCreate(display, window, visInfo.visual, windowAttr.colormap);
#endif

    FontTexture fontTex = {};
    fontTex.memory = create_arena(32 * 1024 * 1024); // 32MB
    if (!font_load_texture(&fontTex, gFontBlobs[0].buffer)) {
        write_str_to_file(&output, cstr("Could not load font!\n"));
        exit(1);
    }
    GuiFont font = {};
    //font_setup(&font, &fontTex, 14.0f, 0.0f);
    font_setup(&font, &fontTex, 24.0f, 0.0f);

    DrawContext drawing;
    drawing.dim.w = width;
    drawing.dim.h = height;
    drawing.stride = width;
    drawing.pixels = (u32 *)windowMem;

    UiContext ui;
    ui.draw = &drawing;
    ui.mouse.pos = v2i_init(0, 0);
    ui.mouse.btnDown    = 0;

    f32 testSlider = 0.5f;

    b32 sizeChanged = false;
    b32 windowIsOpen = true;
    while (windowIsOpen)
    {
        inputStr.size = 0;
        ui.mouse.btnPress = 0;
        ui.mouse.btnRelease = 0;
        ui.mouse.scroll = 0;

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

                case MotionNotify: {
                    ui.mouse.pos.x = event.xmotion.x - 40;
                    ui.mouse.pos.y = event.xmotion.y - 40;
                } break;

                case ButtonPress: {
                    switch (event.xbutton.button)
                    {
                        case LINUX_LEFT_MOUSE:   { ui.mouse.btnPress |= Mouse_Left;   ui.mouse.btnDown |= Mouse_Left; } break;
                        case LINUX_RIGHT_MOUSE:  { ui.mouse.btnPress |= Mouse_Right;  ui.mouse.btnDown |= Mouse_Right; } break;
                        case LINUX_MIDDLE_MOUSE: { ui.mouse.btnPress |= Mouse_Middle; ui.mouse.btnDown |= Mouse_Middle; } break;
                        case LINUX_SCROLL_UP:    { ++ui.mouse.scroll; } break;
                        case LINUX_SCROLL_DOWN:  { --ui.mouse.scroll; } break;
                    }
                } break;

                case ButtonRelease: {
                    switch (event.xbutton.button)
                    {
                        case LINUX_LEFT_MOUSE:   { ui.mouse.btnRelease |= Mouse_Left;   ui.mouse.btnDown &= (u8)~Mouse_Left; } break;
                        case LINUX_RIGHT_MOUSE:  { ui.mouse.btnRelease |= Mouse_Right;  ui.mouse.btnDown &= (u8)~Mouse_Right; } break;
                        case LINUX_MIDDLE_MOUSE: { ui.mouse.btnRelease |= Mouse_Middle; ui.mouse.btnDown &= (u8)~Mouse_Middle; } break;
                    }
                } break;

                default: {} break;
            }
        }

        if (inputStr.size) {
            write_str_to_file(&output, cstr("String input: "));
            write_str_to_file(&output, inputStr);
            write_str_to_file(&output, cstr("\n"));
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
            drawing.dim.w = width - 80;
            drawing.dim.h = height - 80;
            drawing.stride = width;
            drawing.pixels = (u32 *)windowMem + width * 40 + 40;
        }

        i32 pitch = width * pixelBytes;
        for (i32 row = 0; row < height; ++row)
        {
            u8 *r = windowMem + pitch * row;
            for (i32 col = 0; col < width; ++col)
            {
                u32 *p = (u32 *)(r + col * pixelBytes);
                if ((col % 16) && (row % 16)) {
                    *p = 0xFFFFFFFF;
                } else {
                    *p = 0;
                }
            }
        }

        draw_rect(&drawing, 0, 0, drawing.dim.w, drawing.dim.h, 0xFF404040);

        // NOTE(michiel): Slider
        f32 linePos[] = {
            0.125f,
            0.25f,
            0.375f,
            0.5f,
            0.625f,
            0.75f,
            0.875f,
        };
        ui_slider_vert(&ui, 25, 25, 39, 432, &testSlider, sizeof(linePos)/sizeof(*linePos), linePos, 1);
#if 0
        draw_horz_line(&drawing,  25,  90, 39, 0xFF000000);
        draw_horz_line(&drawing,  25, 140, 39, 0xFF000000);
        draw_horz_line(&drawing,  25, 190, 39, 0xFF000000);
        draw_rect(&drawing,       25, 239, 39, 3, 0xFF000000);
        draw_horz_line(&drawing,  25, 290, 39, 0xFF000000);
        draw_horz_line(&drawing,  25, 340, 39, 0xFF000000);
        draw_horz_line(&drawing,  25, 390, 39, 0xFF000000);
        draw_round_rect(&drawing, 39, 42, 11, 400, 5, 0xFF606060);
        draw_round_rect(&drawing, 40, 40,  9, 400, 4, 0xFF000000);
        draw_round_rect(&drawing, 41, 43,  7, 396, 3, 0xFF202020);
        draw_circ(&drawing, 44, 40 + sliderAt + 3, 15, 0x80000000);
        draw_circ(&drawing, 44, 40 + sliderAt, 15, 0xFFF0F0F0);
        draw_circ_grad(&drawing, 44, 40 + sliderAt, 13, 0xFFE95A00, 0xFFAA562F);
#endif

        // NOTE(michiel): Meter
        i32 meterAt = clamp_i32(0, ui.mouse.pos.y - 40, 400);
        draw_round_rect(&drawing, 79, 42, 21, 400, 7, 0xFF606060);
        draw_round_rect(&drawing, 80, 40, 19, 400, 6, 0xFF000000);
        draw_round_rect(&drawing, 81, 43, 17, 396, 5, 0xFF202020);
        DrawContext meterFill = drawing;
        meterFill.pixels += (40 + meterAt) * drawing.stride;
        meterFill.dim.h  -= 40 + meterAt;
        draw_round_rect_grad(&meterFill, 81, 1 - meterAt, 17, 398, 5, 0xFFE95A00, 0xFFAA562F);

        // NOTE(michiel): Slider/Meter
        i32 sliderAt = clamp_i32(0, ui.mouse.pos.y - 40, 400);
        draw_horz_line(&drawing,  125,  90, 39, 0xFF000000);
        draw_horz_line(&drawing,  125, 140, 39, 0xFF000000);
        draw_horz_line(&drawing,  125, 190, 39, 0xFF000000);
        draw_rect(&drawing,       125, 239, 39, 3, 0xFF000000);
        draw_horz_line(&drawing,  125, 290, 39, 0xFF000000);
        draw_horz_line(&drawing,  125, 340, 39, 0xFF000000);
        draw_horz_line(&drawing,  125, 390, 39, 0xFF000000);
        draw_round_rect(&drawing, 139, 42, 11, 400, 5, 0xFF606060);
        draw_round_rect(&drawing, 140, 40,  9, 400, 4, 0xFF000000);
        draw_round_rect(&drawing, 141, 43,  7, 396, 3, 0xFF202020);
        DrawContext sliderFill = drawing;
        sliderFill.pixels += (40 + sliderAt) * drawing.stride;
        sliderFill.dim.h  -= (40 + sliderAt);
        draw_round_rect_grad(&sliderFill, 141, 1 - sliderAt, 7, 398, 3, 0xFFE95A00, 0xFFAA562F);
        draw_circ(&drawing, 144, (40 + sliderAt) + 3, 15, 0x80000000);
        draw_circ(&drawing, 144, (40 + sliderAt), 15, 0xFFF0F0F0);
        draw_circ_grad(&drawing, 144, (40 + sliderAt), 13, 0xFFE95A00, 0xFFAA562F);

        font_setup(&font, &fontTex, 18.0f, 0.0f);
        draw_rect(&drawing, 300, 100, 200, 24, 0xFFFF0000);
        draw_char(&drawing, &font, 300, 100, 'A', 0xFF000000);
        draw_char(&drawing, &font, 316, 100, 'a', 0xFF000000);
        draw_char(&drawing, &font, 328, 100, 'b', 0xFF000000);
        draw_char(&drawing, &font, 340, 100, 'c', 0xFF000000);
        draw_char(&drawing, &font, 352, 100, 'p', 0xFF000000);
        draw_char(&drawing, &font, 364, 100, 'q', 0xFF000000);
        draw_char(&drawing, &font, 376, 100, 'r', 0xFF000000);
        draw_char(&drawing, &font, 388, 100, 's', 0xFF000000);
        draw_char(&drawing, &font, 400, 100, 't', 0xFF000000);
        draw_char(&drawing, &font, 412, 100, 'u', 0xFF000000);
        draw_text(&drawing, &font, 300, 150, cstr("Aabcpqrstu"), 0xFF000000);
        draw_text(&drawing, &font, 300, 200, cstr("AWAKENING"), 0xFF000000);
#if 0
        font_setup(&font, &fontTex, 64.0f, 0.0f);
        draw_char(&drawing, &font, 300, 100, 'A', 0xFF000000);
        font_setup(&font, &fontTex, 128.0f, 0.0f);
        draw_char(&drawing, &font, 370, 100, 'A', 0xFF000000);
        font_setup(&font, &fontTex, 200.0f, 0.0f);
        draw_char(&drawing, &font, 500, 100, 'A', 0xFF000000);
        font_setup(&font, &fontTex, 32.0f, 0.0f);
        draw_char(&drawing, &font, 300, 320, 'A', 0xFF000000);
        font_setup(&font, &fontTex, 20.0f, 0.0f);
        draw_char(&drawing, &font, 340, 320, 'A', 0xFF000000);
        font_setup(&font, &fontTex, 18.0f, 0.0f);
        draw_char(&drawing, &font, 370, 320, 'A', 0xFF000000);
        font_setup(&font, &fontTex, 16.0f, 0.0f);
        draw_char(&drawing, &font, 390, 320, 'A', 0xFF000000);
#endif

#if 0
        draw_rect(&drawing, 40, 40, 80, 80, 0xFFFF0000);
        draw_rect(&drawing, -40, 160, 80, 80, 0xFF00FF00);
        draw_rect(&drawing, 160, -40, 80, 80, 0xFF0000FF);
        draw_rect(&drawing, drawing.dim.w - 40, drawing.dim.h - 40, 80, 80, 0xFFFFFF00);

        draw_round_rect(&drawing, drawing.dim.w - 40 - 80, 40, 80, 80, 10, 0xFFFF0000);
        draw_round_rect(&drawing, drawing.dim.w - 40, 160, 80, 80, 10, 0xFF00FF00);
        draw_round_rect(&drawing, drawing.dim.w - 160 - 80, -40, 80, 80, 10, 0xFF0000FF);
        draw_round_rect(&drawing, -40, drawing.dim.h - 40, 80, 80, 10, 0xFFFFFF00);

        draw_circ(&drawing, drawing.dim.w - 80, 80, 40, 0xFFFF0000);
        draw_circ(&drawing, drawing.dim.w, 200, 40, 0xFF00FF00);
        draw_circ(&drawing, drawing.dim.w - 160 - 40, 0, 40, 0xFF0000FF);
        draw_circ(&drawing, 0, drawing.dim.h, 40, 0xFFFFFF00);

        draw_circ_lt(&drawing, 303, 203, 80, 0xFF000000);
        draw_circ_rt(&drawing, 305, 203, 80, 0xFF000000);
        draw_circ_lb(&drawing, 303, 205, 80, 0xFF000000);
        draw_circ_rb(&drawing, 305, 205, 80, 0xFF000000);
#endif

        XPutImage(display, window, defaultGc, xWindowBuf, 0, 0, 0, 0, (u32)width, (u32)height);
    }

    return 0;
}
