#include "xwindow.hpp"

#include "gap_buffer.hpp"
#include "buffer.hpp"
#include "strref.hpp"
#include "utf.hpp"
#include "internals.hpp"

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
        "#3A392F", // monokai-current-line (alt: 1E1F1C)
        "#057405", // NOTE - green
        "#F61010", // TOD0 - red (spelled this way to avoid grep todo)
    };

    static i32 font_ascent;
    static i32 font_descent;
    static i32 font_height;

    static buffer* file_buffer;
    static buffer_point buf_pt;

    static bool buffer_is_dirty = true;
}

static void
handle_event(xwindow* win)
{
    XEvent ev;
    XNextEvent(win->dpy, &ev);

    switch(ev.type)
    {
        case ConfigureNotify:
        {
            g::buffer_is_dirty = true;

            auto xce = ev.xconfigure;
            LOG_WARN("CONFIGURE NOTIFY %dx%d", xce.width, xce.height);

            // This event type is generated for a variety of happenings, so check
            // whether the window has been resized.
            if (xce.width != win->width || xce.height != win->height)
            {
                // TODO: Resizing height when buffer is displayed from the
                //       bottom is buggy. The buffers must be notified.

                win->resize(xce.width, xce.height);
                LOG_WARN("RESIZING: %dx%d", xce.width, xce.height);
            }
        } break;

        case UnmapNotify:
        {
            g::buffer_is_dirty = true;
            LOG_WARN("UNMAP NOTIFY");

            // TODO: Figure out this event.
        } break;

        case KeyPress:
        {
            g::buffer_is_dirty = true;

            // note: if you just want the XK_ keysym, then you can just use
            // XLookupKeysym(&ev.xkey, 0);
            // and ignore all the XIM / XIC / status etc stuff

            Status status;
            KeySym keysym = NoSymbol;
            char text[32] = {};

            LOG_WARN("Key press!");
#if 0
            // if you want to tell if this was a repeated key, this trick seems reliable.
            int is_repeat = (prev_ev.type == KeyRelease &&
                             prev_ev.xkey.time    == ev.xkey.time &&
                             prev_ev.xkey.keycode == ev.xkey.keycode);
#endif

            // TODO: This is the way to check modifiers! Make an internal represenation
#define CHECK_FOR_KEY(KEY_NAME, MASK_NAME)              \
            if(ev.xkey.state & MASK_NAME)               \
                LOG_WARN(#KEY_NAME " is pressed!")
            CHECK_FOR_KEY(Shift, ShiftMask);
            CHECK_FOR_KEY(Lock, LockMask);
            CHECK_FOR_KEY(Control, ControlMask);
            CHECK_FOR_KEY(Mod1, Mod1Mask);
            CHECK_FOR_KEY(Mod2, Mod2Mask);
            CHECK_FOR_KEY(Mod3, Mod3Mask);
            CHECK_FOR_KEY(Mod4, Mod4Mask);
            CHECK_FOR_KEY(Mod5, Mod5Mask);

            // you might want to remove the control modifier, since it makes
            // stuff return control codes
            ev.xkey.state &= ~ControlMask;

            // get text from the key.
            // it could be multiple characters in the case an IME is used.
            // if you only care about latin-1 input, you can use XLookupString instead
            // and skip all the XIM / XIC setup stuff

            Xutf8LookupString(win->input_xic, &ev.xkey,
                              text, sizeof(text) - 1,
                              &keysym, &status);

            if(status == XBufferOverflow)
            {
                // an IME was probably used, and wants to commit more than 32 chars.
                // ignore this fairly unlikely case for now
            }

            if(status == XLookupChars)
            {
                // some characters were returned without an associated key,
                // again probably the result of an IME
                LOG_INFO("Got chars: (%s)", text);
            }

            if(status == XLookupBoth)
            {
                // we got one or more characters with an associated keysym
                // (all the keysyms are listed in /usr/include/X11/keysymdef.h)

                char* sym_name = XKeysymToString(keysym);
                LOG_INFO("Got both: (%s), (%s)", text, sym_name);

                // TODO: This _must_ be incorrect. Investigate.
                switch(text[0])
                {
                    case 8:
                    {
                        g::buf_pt.remove_character_backward();

                    } break;

                    case 9:
                    {
                        for(auto i = 0; i < 4; ++i)
                            g::buf_pt.insert_character_at_point(' ');
                    } break;

                    case 10:
                        break;

                    case 13:
                    {
                        g::buf_pt.insert_newline_at_point();
                    } break;

                    case 127:
                    {
                        g::buf_pt.remove_character_forward();
                    } break;

                    default:
                    {
                        g::buf_pt.insert_character_at_point(text[0]);
                    } break;
                }
            }

            if(status == XLookupKeySym)
            {
                LOG_INFO("Got keysym: (%s) (0x%lx)", keycode_names[keysym & 0xFF], keysym);
            }

            // Switch on some keys that we handle in the special way.
            switch (keysym)
            {
                case XK_Escape:
                {
                    LOG_INFO("Escape was pressed!");
                } break;

                case XK_Up:
                {
                    if(!g::buf_pt.line_up())
                        LOG_WARN("Cannot move up!");
                } break;

                case XK_Down:
                {
                    if(!g::buf_pt.line_down())
                        LOG_WARN("Cannot move down!");
                } break;

                case XK_Left:
                {
                    if(!g::buf_pt.character_left())
                        LOG_WARN("Cannot move left!");
                } break;

                case XK_Right:
                {
                    if(!g::buf_pt.character_right())
                        LOG_WARN("Cannot move right!");
                } break;

                case XK_End:
                {
                    if(!g::buf_pt.line_end())
                        LOG_WARN("Cannot move to end!");
                } break;

                case XK_Home:
                {
                    if(!g::buf_pt.line_start())
                        LOG_WARN("Cannot move to home!");
                } break;

                default:
                {
                } break;
            }
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
// TODO: This should not be used anymore, becuase glyph_info has been removed
//       since it was flat 256 byte table and didn't work with unicode
//       characters.
#if 0
static inline void
blit_letter(xwindow* win, char ch,
            i32 basex, i32 basey,
            int* advance, int colorid)
{
    auto const& glyph_info = g::glyph_info[s_cast<int>(ch)];

    XftDrawString32 (win->draw, win->scm[colorid], win->font,
                    basex, basey,
                    r_cast<FcChar32 const*>(&ch), 1);

    *advance = basex + glyph_info.xOff;
}
#endif

static void
draw_textline_aux(xwindow& win, bool is_current,
                  i32 framex, i32 framew,
                  i32 basex, i32 basey,
                  strref* refs)
{
    // TODO: This horrible part is used to draw the current line highlight and
    //       the caret. Simplify!
    if (is_current)
    {
        win.draw_rect(framex, basey - g::font_height + g::font_descent,
                      framew, g::font_height,
                      10);

    }

    auto adv = 0;
    auto sref = strref{};

    for(auto j = 0; j < 2; ++j)
    {
        sref.first = refs[j].first;
        sref.last = refs[j].first;
    }

    auto col = 1; // Default foreground.
    win.draw_text(basex + adv, s_cast<i32>(basey), col++, refs[0], &adv);
    win.draw_text(basex + adv, s_cast<i32>(basey), col, refs[1], &adv);

    // The caret must be drawn after the text.
    if (is_current)
    {
        auto caret_x = basex;
        auto caret_adv = 0;
        if(g::buf_pt.curr_idx < refs[0].size())
        {
            XGlyphInfo extents;
            XftTextExtents32(win.dpy, win.font,
                             r_cast<FcChar32 const*>(refs[0].first),
                             s_cast<int>(g::buf_pt.curr_idx),
                             &extents);
            caret_adv = extents.xOff;
        }
        else
        {
            ASSERT(g::buf_pt.curr_idx <= refs[0].size() + refs[1].size());
            XGlyphInfo extents;
            XftTextExtents32(win.dpy, win.font,
                             r_cast<FcChar32 const*>(refs[0].first),
                             s_cast<int>(refs[0].size()),
                             &extents);
            caret_adv += extents.xOff;

            XftTextExtents32(win.dpy, win.font,
                             r_cast<FcChar32 const*>(refs[1].first),
                             s_cast<int>(g::buf_pt.curr_idx - refs[0].size()),
                             &extents);
            caret_adv += extents.xOff;
        }
        caret_x += caret_adv;

        win.draw_rect(caret_x, basey - g::font_height + g::font_descent,
                      1, g::font_height,
                      1);
    }
}

int
main()
{
    LOG_INFO("Using font: %s", g::fontname);
    g::file_buffer = create_buffer_from_file("./test");

#if 0 // TODO: Temporary stuff for testing the file + unicode api.
    for(umm i = 0; i < g::file_buffer->size(); ++i)
    {
        mm line_size = g::file_buffer->get_line(i)->size();
        i32 random_number = line_size == 0 ? 0 : std::random_device()() % line_size;
        g::file_buffer
            ->get_line(i)
            ->insert_at_point(random_number,
                              s_cast<u32>('a' + std::random_device()() % 10));

        g::file_buffer
            ->get_line(i)
            ->delete_char_forward(random_number);
    }

    save_buffer_utf8(g::file_buffer, "./saved_test");
    system("diff -s ./test ./saved_test");
    return 0;
#endif


    g::buf_pt = create_buffer_point(g::file_buffer);

    xwindow win{ 400, 500 };
    win.load_scheme(g::colornames, array_cnt(g::colornames));
    if (!win.load_font(g::fontname))
        PANIC("Cannot load font %s", g::fontname);

    // Load glyph metrics info:
    // TODO: Since they are now just aliases, possibly get rid of them?
    g::font_ascent = win.font->ascent;
    g::font_descent = win.font->descent;
    g::font_height = win.font->height;

    while(1)
    {
        while(XPending(win.dpy))
            handle_event(&win);

#if 0 // DEBUG: Use this to make sure the bug is not an issue with reusing the canvas
        win.canvas = XCreatePixmap(win.dpy, win.win, win.width, win.height, DefaultDepth(win.dpy, win.scr));
        win.draw = XftDrawCreate(win.dpy, win.canvas, win.vis, win.cmap);
#endif

        if(g::buffer_is_dirty)
        {
            auto start = chrono::system_clock::now();

            g::buffer_is_dirty = false;
            win.draw_rect(0, 0, win.width, win.height, 0);
            win.draw_rect(16 -1 , 16 - 1, win.width - 32 + 2, win.height - 32 + 2, 1);
            win.draw_rect(16, 16, win.width - 32, win.height - 32, 0);

            win.set_clamp_rect(16 -1 , 16 - 1 + 1,
                               s_cast<i16>(win.width - 32 + 1),
                               s_cast<i16>(win.height - 32));

            // TODO: Check if boundries are correct.
            auto no_lines = ((win.height - 32 + 1) / g::font_height) + 1;

            if(g::buf_pt.curr_line <= g::buf_pt.first_line)
            {
                g::buf_pt.first_line = g::buf_pt.curr_line;
                g::buf_pt.starting_from_top = true;
            }
            else if (s_cast<i64>(g::buf_pt.curr_line - g::buf_pt.first_line) >= no_lines - 1)
            {
                g::buf_pt.first_line = g::buf_pt.curr_line - (no_lines - 1);
                g::buf_pt.starting_from_top = false;
            }

            // TODO: Make sure we are not drawing out-of-bounds, because 1. We
            //       loose a lot of time when doing that, 2. It is bugprone.
            // TODO: Merge it somehow.
            if(g::buf_pt.starting_from_top)
                for(auto k = 0; k < no_lines; ++k)
                {
                    auto line_to_draw = k + g::buf_pt.first_line;
                    if(line_to_draw >= g::file_buffer->size())
                        break;

                    auto basex = 18;
                    auto basey = 16 + (k + 1) * g::font_height - g::font_descent;

                    strref refs[2];
                    g::file_buffer->get_line(line_to_draw)->to_str_refs(refs);
                    draw_textline_aux(win, g::buf_pt.curr_line == line_to_draw,
                                      16, s_cast<i16>(win.width - 32 + 1) - 1,
                                      basex, basey,
                                      refs);
                }
            else
                for(auto k = no_lines - 1; k >= 0; --k)
                {
                    auto line_to_draw = k + g::buf_pt.first_line;

                    // We can be starting from bot and not have a full buffer (check
                    // how sublime does it)
                    if(line_to_draw >= g::file_buffer->size())
                        continue;

                    auto basex = 18;
                    auto basey = 16 - 1 + win.height - 32 + 1 -
                        ((no_lines - 1) - k) * g::font_height - g::font_descent;

                    strref refs[2];
                    g::file_buffer->get_line(line_to_draw)->to_str_refs(refs);
                    draw_textline_aux(win, g::buf_pt.curr_line == line_to_draw,
                                      16, s_cast<i16>(win.width - 32 + 1) - 1,
                                      basex, basey,
                                      refs);
                }

            win.clear_clamp_rect();
            win.flush();

            auto elapsed = chrono::system_clock::now() - start;

            LOG_INFO("elapsed: %d",
                     s_cast<int>(chrono::dur_cast<chrono::milliseconds>(elapsed).count()));

            // This will give us about 16ms speed.
            std::this_thread::sleep_for(16ms - elapsed);
        }
    }

    // NOTE: Since the above loop should not be breaked the cleaup should take
    //       place in the global exit function
#if 0
    win.free_scheme();
    win.free_font();
#endif
}

#include "buffer.cpp"
#include "gap_buffer.cpp"
#include "xwindow.cpp"
#include "utf.cpp"
#include "internals.cpp"
