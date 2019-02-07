#include "config.hpp"

#include "xwindow.hpp"

xwindow::xwindow(int width_, int heigth_)
{
    width = width_;
    height = heigth_;

    // TODO: CHECK IF THEY HAVE SUCCEEDED.
    dpy = XOpenDisplay(0);
    scr = DefaultScreen(dpy);
    vis = DefaultVisual(dpy, 0);
    cmap = DefaultColormap(dpy, scr);

#if 0 // TODO: How on earth do i check the member named 'class'?
    if(visual->class != TrueColor)
        PANIC("Cannot handle non true color visual");
#endif

    if (!dpy)
        PANIC("Cannot open display!");

    XSetWindowAttributes wa;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ExposureMask | KeyPressMask | VisibilityChangeMask;

    // TODO: Figure CWBackPixmap out! This makes resizing horriebly slow
    win = XCreateWindow(dpy, DefaultRootWindow(dpy),
                        200, 100, width, height, 0, // TODO: window coords
                        DefaultDepth(dpy, scr),
                        InputOutput,
                        DefaultVisual(dpy, scr),
                        CWEventMask,
                        &wa);

    canvas = XCreatePixmap(dpy, win, width, height, DefaultDepth(dpy, scr));
    gc = XCreateGC(dpy, win, 0, 0);
    draw = XftDrawCreate(dpy, canvas, vis, cmap);

    XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);
    XClearWindow(dpy, win);
    XChangeWindowAttributes(dpy, win, 0, &wa);

    // Input sutff:
    // loads the XMODIFIERS environment variable to see what IME to use
    XSetLocaleModifiers("");

    input_xim = XOpenIM(dpy, 0, 0, 0);
    if(!input_xim)
    {
        // fallback to internal input method
        XSetLocaleModifiers("@im=none");
        input_xim = XOpenIM(dpy, 0, 0, 0);
    }

    input_xic = XCreateIC(input_xim,
                          XNInputStyle,   XIMPreeditNothing | XIMStatusNothing,
                          XNClientWindow, win,
                          XNFocusWindow,  win,
                          NULL);

    XSetICFocus(input_xic);
    XSelectInput(dpy, win, KeyPressMask | KeyReleaseMask | StructureNotifyMask);
    XMapWindow(dpy, win);
}

xwindow::~xwindow()
{
    if(canvas)
    {
        XftDrawDestroy(draw);
        XFreePixmap(dpy, canvas);
    }

    XDestroyWindow(dpy, win);
    XFreeGC(dpy, gc);
    XCloseDisplay(dpy);
}

void
xwindow::flush()
{
    flush(0, 0, width, height);
}

void
xwindow::flush(int x, int y, int w, int h)
{
    ASSERT(x + width <= w);
    ASSERT(y + height <= h);

    XCopyArea(dpy, canvas, win, gc, x, y, w, h, x, y);

    // TODO: Figure out whether XSync or XFlush are better: dmenu uses xsync,
    //       but xsync blocks so possibly it would be better to use just xflush?
#if 0
    XSync(dpy, False);
#else
    XFlush(dpy);
#endif
}

void
xwindow::resize(int new_w, int new_h)
{
    width = new_w;
    height = new_h;

    if(canvas)
    {
        XftDrawDestroy(draw);
        XFreePixmap(dpy, canvas);
    }

    canvas = XCreatePixmap(dpy, win, width, height, DefaultDepth(dpy, scr));
    draw = XftDrawCreate(dpy, canvas, vis, cmap);
}


void
xwindow::set_clamp_rect(i16 x, i16 y, u16 w, u16 h)
{
    auto boundrect = XRectangle{x, y, w, h};
    auto r = XCreateRegion();
    XUnionRectWithRegion(&boundrect, r, r);
    XftDrawSetClip(draw, r);
    XDestroyRegion(r);
}

void
xwindow::clear_clamp_rect()
{
    XftDrawSetClip(draw, 0);
}

void
xwindow::draw_rect(int x, int y, int w, int h, int colorid)
{
#if 1
    XftDrawRect (draw, scm[colorid], x, y, w, h);
#else
    XFillRectangle(dpy, canvas, gc, x, y, w, h);
#endif
}

void
xwindow::draw_text(int x, int y, int colorid, char const* ptr, size_t len, int* adv)
{
    if(adv)
    {
        XGlyphInfo extents;
        XftTextExtentsUtf8(dpy, font,
                           r_cast<FcChar8 const*>(ptr),
                           s_cast<int>(len),
                           &extents);

        *adv = extents.xOff;
    }

    XftDrawStringUtf8(draw, scm[colorid], font,
                      x, y,
                      r_cast<FcChar8 const*>(ptr),
                      s_cast<int>(len));
}

void
xwindow::draw_text(int x, int y, int colorid, char const* txt, int* adv)
{
    draw_text(x, y, colorid,
              txt,
              s_cast<i32>(strlen(txt)),
              adv);
}

void
xwindow::draw_text(int x, int y, int colorid, strref strref, int* adv)
{
    draw_text(x, y, colorid,
              strref.first,
              strref.size(),
              adv);
}

void
xwindow::load_scheme(char const** names, mm num_names)
{
    ASSERT(num_names <= xcolorscm::max_n_colors);

    scm.size = num_names;
    for(mm i = 0; i < num_names; ++i)
        if (!XftColorAllocName(dpy, vis, cmap, names[i], scm.xftcolor + i))
        {
            // TODO: Handle errors better. Possibly use some defaults.
            PANIC("Could not allocate color %s", names[i]);
        }
}

void
xwindow::free_scheme()
{
    for(mm i = scm.size - 1; i >= 0; --i)
        XftColorFree(dpy, vis, cmap, scm.xftcolor + i);

    scm.size = 0;
}

bool
xwindow::load_font(char const* fontname)
{
    font = XftFontOpenName(dpy, scr, fontname);
    return font ? true : false;
}

void
xwindow::free_font()
{
    XftFontClose(dpy, font);
}
