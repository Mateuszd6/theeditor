#ifndef XWINDOW_H
#define XWINDOW_H
#include "config.hpp"

  #include <X11/Xlib.h>
  #include <X11/Xutil.h>
  #include <X11/Xft/Xft.h>

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

    xwindow(int width_, int heigth_);
    ~xwindow();

    void flush();
    void flush(int x, int y, int w, int h);
    void resize(int new_w, int new_h);

    void draw_text(int x, int y, int colorid, char const* txt);
    void draw_rect(int x, int y, int w, int h, int colorid);

    void load_scheme(char const** names, mm num_names);
    void free_scheme();

    bool load_font(char const* fontname);
    void free_font();

};

#endif //XWINDOW_H
