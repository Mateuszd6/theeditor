#include "config.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
  #include <X11/Xlib.h>
  #include <X11/Xutil.h>
  #include <X11/Xft/Xft.h>
#pragma clang diagnostic pop

namespace g
{
    // TODO: For now we'll use only one font.
    static constexpr char const* fontname = "DejaVu Sans Mono:size=11:antialias=true";
}

struct xcolorscm
{
    static constexpr mm max_n_colors = 16;

    XftColor xftcolor[max_n_colors];
    mm size;

    inline XftColor* operator[](mm idx)
    {
        ASSERT(idx < max_n_colors);
        return (&xftcolor[idx]);
    }
};

struct xwindow
{
    Display* dpy;
    Visual* vis;
    XftFont* font; // TODO: Dont use only one font.
    XftDraw* draw; // Xft wraps pixmap `canvas` into this.
    Drawable canvas; // pixmap used for the double buffering.
    Window win;
    Colormap cmap;
    GC gc;

    int scr;
    int width, height;

    xcolorscm scm;

    xwindow(int width_, int heigth_)
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

        font = XftFontOpenName(dpy, scr, g::fontname);
        if (!font)
            PANIC("cannot load font %s", g::fontname);
    }

    ~xwindow()
    {
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
    }

    void
    flush(int x, int y, int w, int h)
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
    flush()
    {
        flush(0, 0, width, height);
    }

    void
    resize(int new_w, int new_h)
    {
        width = new_w;
        height = new_h;

        if(canvas)
            XFreePixmap(dpy, canvas);
        canvas = XCreatePixmap(dpy, win, width, height, DefaultDepth(dpy, scr));
        draw = XftDrawCreate(dpy, canvas, vis, cmap);
    }

    void
    fill_rect(int x, int y, int w, int h, int colorid)
    {
        XftDrawRect (draw, scm[colorid], x, y, w, h);
    }

    void
    draw_text(int x, int y, int colorid, char const* txt)
    {
        XftDrawStringUtf8(draw, scm[colorid], font,
                          x, y,
                          r_cast<FcChar8 const*>(txt),
                          strlen(txt));
    }

    void
    free_scheme()
    {
        for(mm i = scm.size - 1; i >= 0; --i)
            XftColorFree(dpy, vis, cmap, scm.xftcolor + i);

        scm.size = 0;
    }

    void
    make_scheme(char const** names, mm num_names)
    {
        ASSERT(num_names <= max_n_colors);

        scm.size = num_names;
        for(mm i = 0; i < num_names; ++i)
        {
            if (!XftColorAllocName(dpy, vis, cmap, names[i], scm.xftcolor + i))
            {
                // TODO: Handle errors better. Possibly use some defaults.
                PANIC("Could not allocate color %s", names[i]);
            }
        }
    }
};
