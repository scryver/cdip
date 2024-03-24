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
        XPutImage(display, window, defaultGc, xWindowBuf, 0, 0, 0, 0, (u32)width, (u32)height);
    }

    return 0;
}
