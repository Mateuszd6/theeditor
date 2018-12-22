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
    XSelectInput(dpy, win, StructureNotifyMask);
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
xwindow::draw_rect(int x, int y, int w, int h, int colorid)
{
    XftDrawRect (draw, scm[colorid], x, y, w, h);
}

void
xwindow::draw_text(int x, int y, int colorid, char const* txt)
{
    XftDrawStringUtf8(draw, scm[colorid], font,
                      x, y,
                      r_cast<FcChar8 const*>(txt),
                      s_cast<int32>(strlen(txt)));
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
