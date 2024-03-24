// NOTE(michiel): For keysym see:  /usr/include/X11/keysymdef.h
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<X11/Xatom.h>

enum
{
    NET_WM_STATE_REMOVE    = 0,
    NET_WM_STATE_ADD       = 1,
    NET_WM_STATE_TOGGLE    = 2
};

func void window_set_size_hints(Display *display, Window window, i32 minWidth, i32 minHeight, i32 maxWidth, i32 maxHeight)
{
    XSizeHints hints = {0};
    if ((minWidth > 0) && (minHeight > 0)) { hints.flags |= PMinSize; }
    if ((maxWidth > 0) && (maxHeight > 0)) { hints.flags |= PMaxSize; }
    hints.min_width = minWidth;
    hints.min_height = minHeight;
    hints.max_width = maxWidth;
    hints.max_height = maxHeight;
    XSetWMNormalHints(display, window, &hints);
}

func Status window_toggle_maximize(Display *display, Window window)
{
    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom maxHorz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    Atom maxVert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

    Status result = 0;
    if (wmState != None)
    {
        XClientMessageEvent ev = {0};
        ev.type = ClientMessage;
        ev.format = 32;
        ev.window = window;
        ev.message_type = wmState;
        ev.data.l[0] = NET_WM_STATE_TOGGLE;
        ev.data.l[1] = (i64)maxHorz;
        ev.data.l[2] = (i64)maxVert;
        ev.data.l[3] = 1;
        result = XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, (XEvent *)&ev);
    }
    return result;
}

func XIC window_setup_utf8_input(Display *display, Window window, OsFile *output)
{
    XIC result = 0;

    XIM xInputMethod = XOpenIM(display, 0, 0, 0);
    if (xInputMethod)
    {
        XIMStyles *styles = 0;
        if ((XGetIMValues(xInputMethod, XNQueryInputStyle, &styles, NULL) == 0) && styles)
        {
            XIMStyle bestMatchStyle = 0;
            for (i32 idx = 0; idx < styles->count_styles; ++idx)
            {
                XIMStyle testStyle = styles->supported_styles[idx];
                if (testStyle == (XIMPreeditNothing | XIMStatusNothing)) {
                    bestMatchStyle = testStyle;
                    break;
                }
            }
            XFree(styles);

            if (bestMatchStyle)
            {
                result = XCreateIC(xInputMethod, XNInputStyle, bestMatchStyle, XNClientWindow, window, XNFocusWindow, window, NULL);
                if (!result && output)
                {
                    write_str_to_file(output, cstr("Input Context could not be created!\n"));
                }
            }
            else if (output)
            {
                write_str_to_file(output, cstr("No matching input style could be determined!\n"));
            }
        }
        else if (output)
        {
            write_str_to_file(output, cstr("Input Styles could not be retrieved!\n"));
        }
    }
    else if (output)
    {
        write_str_to_file(output, cstr("Input Method could not be opened!\n"));
    }

    return result;
}
