#ifndef XWINDOW_HPP
#define XWINDOW_HPP

#include "config.hpp"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include "strref.hpp"

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

static constexpr Atom XA_ATOM = 4;
static constexpr Atom XA_STRING = 31;

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

    XIC input_xic;
    XIM input_xim;

    int scr;
    int width, height;

    xcolorscm scm;

    // TODO: This is part of the clipboard api, and should be separated.
    char* clip_text = nullptr;
    mm clip_size = 0;

    // Clipboard atoms. TODO: Find a way not to store them here.
    Atom UTF8;
    Atom text_atom;
    Atom targets_atom;
    Atom __selection;

    xwindow(int width_, int heigth_);
    ~xwindow();

    // Flush the window (or part of the window) to the screen.
    void flush();
    void flush(int x, int y, int w, int h);

    // This must be called on a window after the window is resized. Drawables
    // are reallocated if the size has changed.
    void resize(int new_w, int new_h);

    void set_clamp_rect(i16 x, i16 y, u16 w, u16 h);
    void clear_clamp_rect();

    void draw_rect(int x, int y, int w, int h, int colorid);

    void draw_text(int x, int y, int colorid, u32 const* ptr, mm len, int* adv);
    void draw_text(int x, int y, int colorid, strref strref, int* adv);

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

    // Load the xclipboard contents. This returns it, but also stores it in the
    // xwindow object, because we have to give it back, when we are notified
    // with XSelectionRequest event. So don't modify the contents.
    std::pair<char*, i32> load_clipboard_contents();

    // If someone wants our selection we send it to them.
    void handle_selection_request(XSelectionRequestEvent* xsr);

    // Set the xclipboard contents.
    void set_clipboard_contents(unsigned char* text, int size);
};

#endif // XWINDOW_HPP
