#ifndef XWINDOW_HPP
#define XWINDOW_HPP

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
    Region* clamp;

    Drawable canvas; // pixmap used for the double buffering.
    Window win;
    Colormap cmap;
    GC gc;

    int scr;
    int width, height;

    xcolorscm scm;

    xwindow(int width_, int heigth_);
    ~xwindow();

    // Flush the window (or part of the window) to the screen.
    void flush();
    void flush(int x, int y, int w, int h);

    // This must be called on a window after the window is resized. Drawables
    // are reallocated if the size has changed.
    void resize(int new_w, int new_h);

    void set_clamp_rect(int16 x, int16 y, uint16 w, uint16 h);
    void clear_clamp_rect();

    void draw_text(int x, int y, char const* txt, int colorid);
    void draw_rect(int x, int y, int w, int h, int colorid);

    // Load new [num_names] of colors as the scheme. [draw_*] fucntions will now
    // accept [colorid] value in range (0 ; num_names-1).
    void load_scheme(char const** names, mm num_names);

    // Clear the current scheme and the colors. No idea what happens when you
    // try to draw something unless you allocate new scheme.
    void free_scheme();

    // Load a new xft font.
    bool load_font(char const* fontname);

    // Clear the current font.
    void free_font();
};

#endif // XWINDOW_HPP
