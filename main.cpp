#include "xwindow.hpp"

#include <thread>

namespace g
{
    // TODO: For now we'll use only one font.
    static constexpr char const* fontname = "DejaVu Sans Mono:size=11:antialias=true";
    static char const* colornames[] = {
        "#272822", // monokai-background
        "#F8F8F2", // monokai-foreground
        "#F92672", // monokai-red
        "#66D9EF", // monokai-blue
        "#A6E22E", // monokai-green
        "#FD971F", // monokai-orange
        "#E6DB74", // monokai-yellow
        "#AE81FF", // monokai-violet
        "#75715E", // monokai-comments
        "#8F908A", // monokai-line-number
        "#057405", // NOTE - green
        "#F61010", // TODO - red
    };
}

static void
handle_event(xwindow* win)
{
    XEvent e;
    XNextEvent(win->dpy, &e);
    switch(e.type)
    {
        case ConfigureNotify:
        {
            auto xce = e.xconfigure;
            LOG_WARN("CONFIGURE NOTIFY %dx%d", xce.width, xce.height);

            // This event type is generated for a variety of happenings, so check
            // whether the window has been resized.
            if (xce.width != win->width || xce.height != win->height)
            {
                win->resize(xce.width, xce.height);
                LOG_WARN("RESIZING: %dx%d", xce.width, xce.height);
            }
        } break;

        case UnmapNotify:
        {
            LOG_WARN("UNMAP NOTIFY");

            // TODO: Figure out this event as it is also send then I force
            // redrawing the window.
        } break;

        default:
        {
        } break;
    }
}

int
main()
{
    LOG_INFO("Using font: %s", g::fontname);

    xwindow win{ 400, 500 };
    win.load_scheme(g::colornames, array_cnt(g::colornames));
    if (!win.load_font(g::fontname))
        PANIC("Cannot load font %s", g::fontname);

    while(1)
    {
        auto start = chrono::system_clock::now();
        while(XPending(win.dpy))
            handle_event(&win);

        // Drawing:
        {
            win.draw_rect(0, 0, win.width, win.height, 0);
            win.draw_rect(32, 32, 128, 128, 1);
            win.draw_text(64 + 128, 64, 3, "Hello world!");

            win.flush();
        }

        auto elapsed = chrono::system_clock::now() - start;

#ifdef COMPILER_CLANG
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Weverything"
#endif
        {
            LOG_INFO("elapsed: %d",
                     s_cast<int>(chrono::dur_cast<chrono::milliseconds>(elapsed).count()));
        }
#ifdef COMPILER_CLANG
#  pragma clang diagnostic pop
#endif

        // This will give us about 16ms speed.
        std::this_thread::sleep_for(1s - elapsed);
    }

    win.free_scheme();
    win.free_font();

    return 0;
}

#include "xwindow.cpp"
