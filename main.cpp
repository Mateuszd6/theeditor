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

    static XGlyphInfo glyph_info[256];
    static int32 font_ascent;
    static int32 font_descent;
    static int32 font_height;
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

/*
  NOTE: This comes from Xft tutorial.
  top = y - glyphInfo.y;
  left = x - glyphInfo.x;
  bottom = top + glyphInfo.height;
  right = left + glyphInfo.width;

  And to compute the normal location for the next glyph:
  x = x + glyphInfo.xOff;
  y = y + glyphInfo.yOff;
*/
// TODO: This is horriebly slow compared to window::draw_text.
static inline void
blit_letter(xwindow* win, char ch,
            int32 basex, int32 basey,
            int* advance, int colorid)
{
    auto const& glyph_info = g::glyph_info[s_cast<int>(ch)];

    XftDrawString8 (win->draw, win->scm[colorid], win->font,
                    basex, basey,
                    r_cast<FcChar8 const*>(&ch), 1);

    *advance = basex + glyph_info.xOff;
}

int
main()
{
    LOG_INFO("Using font: %s", g::fontname);


    xwindow win{ 400, 500 };
    win.load_scheme(g::colornames, array_cnt(g::colornames));
    if (!win.load_font(g::fontname))
        PANIC("Cannot load font %s", g::fontname);

    // Load glyph metrics info:
    {
        for(uint8 i = 0; i < 127; ++i)
        {
            char txt[2];
            txt[0] = i;
            txt[1] = 0;

            XftTextExtents8(win.dpy, win.font,
                            r_cast<FcChar8 const*>(txt), 1,
                            g::glyph_info + i);
        }

        g::font_ascent = win.font->ascent;
        g::font_descent = win.font->descent;
        g::font_height = win.font->height;
    }

    while(1)
    {
        auto start = chrono::system_clock::now();
        while(XPending(win.dpy))
            handle_event(&win);

        // Drawing:
        {
            win.draw_rect(0, 0, win.width, win.height, 0);
            win.draw_rect(16 -1 , 16 - 1, 512 + 2, 512 + 2, 1);
            win.draw_rect(16, 16, 512, 512, 0);

            // win.set_clamp_rect(16, 16, 512, 512);
            char const* lines[] = {
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"abcdefghijklmnoprstuwxyzABCDEFGHIJKLMNOPRSTUWXYZ01234~!@#$%^&*()__+=-][{}{}|\';'/.,,.././.,-*-/-*/-*+]",
"}",
            };

            static auto yoffset = 0;
            static auto advance = 0;
            for(auto i = 0; i < array_cnt(lines); ++i)
            {
                auto adv = 18;
                auto next_line = 32 + yoffset + i * g::font_height;
#if 0
                for(auto p = lines[i]; *p && adv <= 512 + 16; ++p)
                    blit_letter(&win, *p, adv, next_line, &adv, (i * 191) % 5 + 1);
#else
                win.draw_text(adv, next_line, lines[i], (i * 191) % 10 + 1);
#endif
            }

            yoffset += advance;
            if(32 + yoffset + array_cnt(lines) * g::font_height >= 512)
                advance *= -1;
            if(32 + yoffset <= 16)
                advance *= -1;

            win.clear_clamp_rect();
            win.flush();
        }

        auto elapsed = chrono::system_clock::now() - start;

        LOG_INFO("elapsed: %d",
                 s_cast<int>(chrono::dur_cast<chrono::milliseconds>(elapsed).count()));

        // This will give us about 16ms speed.
        std::this_thread::sleep_for(16ms - elapsed);
    }

    win.free_scheme();
    win.free_font();

    return 0;
}

#include "xwindow.cpp"
