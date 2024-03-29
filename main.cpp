#include "xwindow.hpp"
#include "gap_buffer.hpp"
#include "text_buffer.hpp"
#include "strref.hpp"
#include "utf.hpp"
#include "internals.hpp"
#include "undo.hpp"

#include <thread>

// TODO: Handle for multiple buffers.
enum struct prev_command_type : i32
{
    none,

    insert,
    remove, // Remove with backspace
    remove_inplace, // Remove with delete
    paste, // Paste or rotate the killring
    undo,
};

namespace g
{

// Global font props.
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
    "#057405", // N0TE - green
    "#F61010", // T0DO - red
};
static i32 font_ascent;
static i32 font_descent;
static i32 font_height;

// Global buffer props.
static buffer_point buf_pt;
static bool buffer_is_dirty = true;

static xwindow* tmp_window_hndl;

static prev_command_type prev_comm = prev_command_type::none;

} // namespace g

static void
handle_key(key pressed_key)
{
    std::string const* shortcut_name;

    if ((shortcut_name = is_shortcut(pressed_key)) != nullptr)
    {
        LOG_WARN("Shortcut: %s", shortcut_name->c_str());

        // TODO: Checking the name makes no sense, but it is temporary anyways.
        if (*shortcut_name == "Undo")
        {
            g::buf_pt.buffer_ptr->undo_buf.undo();
            g::prev_comm = prev_command_type::undo;
            return;
        }
        else
        {
            g::prev_comm = prev_command_type::none;
        }

        if (*shortcut_name == "Copy")
        {
            char text[] = "Mateusz";
            g::tmp_window_hndl->set_clipboard_contents(
                reinterpret_cast<u8*>(text),
                static_cast<u32>(array_cnt("Mateusz") - 1));

            g::prev_comm = prev_command_type::none;
        }
        else if (*shortcut_name == "Quit")
        {
            // This is some save exitting code, to make sure we don't leak much.
            LOG_INFO("Exitting...");

            g::tmp_window_hndl->free_scheme();
            g::tmp_window_hndl->free_font();
            g::tmp_window_hndl->~xwindow();

            g::prev_comm = prev_command_type::none;
            exit(0);
        }
        else if (*shortcut_name == "Paste")
        {
            // TODO: Change so that nicer api is called here!
            g::tmp_window_hndl->load_clipboard_contents();
            if (g::tmp_window_hndl->clip_text)
            {
                auto text = reinterpret_cast<u8*>(g::tmp_window_hndl->clip_text);
                auto[utf32_data, utf32_size] =
                    utf8_to_utf32_allocate(text, text + g::tmp_window_hndl->clip_size);

                ASSERT(g::buf_pt.point_is_valid());
                auto[succeeded, ret_line, ret_idx] =
                    g::buf_pt.buffer_ptr->insert(g::buf_pt.curr_line,
                                                 g::buf_pt.curr_idx,
                                                 utf32_data,
                                                 utf32_data + utf32_size);

                ASSERT(succeeded); // TODO: Assume success for now.
                g::buf_pt.curr_line =  ret_line;
                g::buf_pt.curr_idx = ret_idx;
                ASSERT(g::buf_pt.point_is_valid());

                g::prev_comm = prev_command_type::paste;
            }
            else
            {
                LOG_ERROR("The clipboard is empty!");
                g::prev_comm = prev_command_type::none;
            }
        }
        else if (*shortcut_name == "Save")
        {
            LOG_INFO("Saving buffer as './saved_test'...saved_test");
            save_buffer(g::buf_pt.buffer_ptr, "./saved_test", encoding::utf8);
            g::prev_comm = prev_command_type::none;
        }
        else if (*shortcut_name == "Debug Print Undo")
        {
            LOG_INFO("Printing undo state: ");
            g::buf_pt.buffer_ptr->undo_buf.DEBUG_print_state();
            g::prev_comm = prev_command_type::none;
        }
        else if (*shortcut_name == "Delete Backward")
        {
            g::buf_pt.del_backward();
            g::prev_comm = prev_command_type::remove;
        }
        else if (*shortcut_name == "Delete Forward")
        {
            g::buf_pt.del_forward();
            g::prev_comm = prev_command_type::remove_inplace;
        }
        else if (*shortcut_name == "Insert Tab")
        {
            u32 buffer[] = { ' ', ' ', ' ', ' ', };
            g::buf_pt.insert(buffer, buffer + 4);

            g::prev_comm = prev_command_type::insert;
        }
        else if (*shortcut_name == "Insert Newline")
        {
            u32 newline_ch = static_cast<u32>('\n');
            g::buf_pt.insert(newline_ch);

            // TODO: possible differenciate between insert and insert_newline?
            g::prev_comm = prev_command_type::insert;
        }
        else if (*shortcut_name == "Move Up")
        {
            if(!g::buf_pt.line_up())
                LOG_WARN("Cannot move up!");

            g::prev_comm = prev_command_type::none;
        }
        else if (*shortcut_name == "Move Down")
        {
            if(!g::buf_pt.line_down())
                LOG_WARN("Cannot move down!");

            g::prev_comm = prev_command_type::none;
        }
        else if (*shortcut_name == "Move Left")
        {
            if(!g::buf_pt.character_left())
                LOG_WARN("Cannot move left!");

            g::prev_comm = prev_command_type::none;
        }
        else if (*shortcut_name == "Move Right")
        {
            if(!g::buf_pt.character_right())
                LOG_WARN("Cannot move right!");

            g::prev_comm = prev_command_type::none;
        }
        else if (*shortcut_name == "Move End")
        {
            if(!g::buf_pt.line_end())
                LOG_WARN("Cannot move to end!");

            g::prev_comm = prev_command_type::none;
        }
        else if (*shortcut_name == "Move Home")
        {
            if(!g::buf_pt.line_start())
                LOG_WARN("Cannot move to home!");

            g::prev_comm = prev_command_type::none;
        }
        else
        {
            LOG_WARN("Not supported shortcut: %s", shortcut_name->c_str());
            g::prev_comm = prev_command_type::none;
        }
    }
    else if (pressed_key.codept == 0)
    {
        LOG_INFO("[%s] is a special key with no shortcut assigned to it. Ignored.",
                 keycode_names[pressed_key.keycode]);

        g::prev_comm = prev_command_type::none;
    }
    else if (pressed_key.codept != 0)
    {
        LOG_INFO("0x%x is regular utf32 key", pressed_key.codept);
        g::buf_pt.insert(pressed_key.codept);

        g::prev_comm = prev_command_type::insert;
    }
}

static void
handle_event(xwindow* win)
{
    XEvent ev;
    XNextEvent(win->dpy, &ev); // NOTE: could use XNextEventTimed

    switch(ev.type)
    {
        case ConfigureNotify:
        {
            g::buffer_is_dirty = true;

            auto xce = ev.xconfigure;
            LOG_WARN("__ ConfigureNotify __ %dx%d", xce.width, xce.height);

            // This event type is generated for a variety of happenings, so check
            // whether the window has been resized.
            if (xce.width != win->width || xce.height != win->height)
            {
                win->resize(xce.width, xce.height);
                LOG_WARN("RESIZING: %dx%d", xce.width, xce.height);
            }
        } break;

        case VisibilityNotify:
        {
            LOG_WARN("__ VisibilityNotify __");

            // TODO: Figure out this event.
        } break;

        case MapNotify:
        {
            LOG_WARN("__ MapNotify __");

            // TODO: Figure out this event.
        } break;

        case UnmapNotify:
        {
            LOG_WARN("__ UnmapNotify __");

            g::buffer_is_dirty = true;
            // TODO: Figure out this event.
        } break;

        // TODO: Figure out the difference between Expose and FocusIn/FocusOut
#if 0
        case FocusIn:
        {
            LOG_WARN("__ FocusIn __");
        } break;

        case FocusOut:
        {
            LOG_WARN("__ FocusOut __");
        } break;
#endif

        case Expose:
        {
            LOG_WARN("__ Expose __");

            g::buffer_is_dirty = true;
        } break;

        case GraphicsExpose:
        {
            LOG_WARN("__ GraphicsExpose  __");
        } break;

        case NoExpose:
        {
            LOG_WARN("__ NoExpose  __");
        } break;

        case KeyPress:
        {
            LOG_WARN("__ KeyPress __");

            g::buffer_is_dirty = true;

            Status status;
            KeySym keysym = NoSymbol;
            char text[32] = {};

            // TODO: This should be unrolled in the optimized build.
            u32 xmodif_masks[] = {
                ShiftMask,
                LockMask,
                ControlMask,
                Mod1Mask,
                Mod2Mask,
                Mod3Mask,
                Mod4Mask,
                Mod5Mask,
            };
            u16 mod_mask = 0;
            for(int i = 0; i < array_cnt(xmodif_masks); ++i)
                if(ev.xkey.state & xmodif_masks[i])
                    mod_mask |= (1 << i);

            // you might want to remove the control modifier, since it makes
            // stuff return control codes
            ev.xkey.state &= ~ControlMask;

            // get text from the key.
            // TODO: It could be multiple characters in the case an IME is
            //       used. I guess it is not supported yet.
            Xutf8LookupString(win->input_xic, &ev.xkey,
                              text, sizeof(text) - 1,
                              &keysym, &status);

            if(status == XBufferOverflow || status == XLookupChars)
            {
                PANIC("Probably an IME. This is not supported yet.");
                return;
            }

            if(status == XLookupBoth || status == XLookupKeySym)
            {
                key pressed_key = { 0, 0, mod_mask };

                // Although we have text values for some keys (like escape or
                // return) we just treat them as special keys.
                if(is_special_key(keysym))
                {
                    u16 internal_keycode = (keysym & 0xFF);
                    pressed_key.keycode = internal_keycode;
                }
                else if(text[0])
                {
                    auto [_, rdest] = utf8_to_utf32(reinterpret_cast<u8*>(&text[0]),
                                                    reinterpret_cast<u8*>(&text[0]) + sizeof(text),
                                                    &(pressed_key.codept),
                                                    &(pressed_key.codept) + 1);

                    // Assert that we have parsed exacly one codept.
                    ASSERT(rdest == &(pressed_key.codept) + 1);
                }
                else
                {
                    LOG_ERROR("Unsupported key: 0x%lx", keysym);
                    break;
                }

                // The key struct is build and now we can do something with it:
                handle_key(pressed_key);
            }
        } break;

        // Clipboard stuff:
        case SelectionRequest:
        {
            LOG_INFO("__ SelectionRequest __");

            win->handle_selection_request(&ev.xselectionrequest);
        } break;

        case SelectionClear:
        {
            LOG_INFO("__ SelectionClear __");

            // Probobly not really necesarry, but I want to be sure I can do it
            // like this.
            win->clip_text = nullptr;
            win->clip_size = 0;
        } break;

        default:
        {
            LOG_WARN("__ UnknowEvnet: %d __", ev.type);
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
    auto const& glyph_info = g::glyph_info[static_cast<int>(ch)];

    XftDrawString32 (win->draw, win->scm[colorid], win->font,
                    basex, basey,
                    reinterpret_cast<FcChar32 const*>(&ch), 1);

    *advance = basex + glyph_info.xOff;
}
#endif

static void
draw_textline_aux(xwindow& win, bool is_current,
                  i32 framex, i32 framew,
                  i32 basex, i32 basey,
                  strref* refs)
{
    if (is_current)
    {
        win.draw_rect(framex, basey - g::font_height + g::font_descent,
                      framew, g::font_height, 10);

    }

    auto adv = 0;
    auto sref = strref{};

    for(auto j = 0; j < 2; ++j)
    {
        sref.first = refs[j].first;
        sref.last = refs[j].first;
    }

    auto col = 1; // Default foreground.
    win.draw_text(basex + adv, static_cast<i32>(basey), col++, refs[0], &adv);
    win.draw_text(basex + adv, static_cast<i32>(basey), col, refs[1], &adv);

    // The caret must be drawn after the text.
    if (is_current)
    {
        auto caret_x = basex;
        auto caret_adv = 0;
        if(g::buf_pt.curr_idx < refs[0].size())
        {
            XGlyphInfo extents;
            XftTextExtents32(win.dpy, win.font,
                             reinterpret_cast<FcChar32 const*>(refs[0].first),
                             static_cast<int>(g::buf_pt.curr_idx),
                             &extents);
            caret_adv = extents.xOff;
        }
        else
        {
            ASSERT(g::buf_pt.curr_idx <= refs[0].size() + refs[1].size());
            XGlyphInfo extents;
            XftTextExtents32(win.dpy, win.font,
                             reinterpret_cast<FcChar32 const*>(refs[0].first),
                             static_cast<int>(refs[0].size()),
                             &extents);
            caret_adv += extents.xOff;

            XftTextExtents32(win.dpy, win.font,
                             reinterpret_cast<FcChar32 const*>(refs[1].first),
                             static_cast<int>(g::buf_pt.curr_idx - refs[0].size()),
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
    auto init_start = chrono::system_clock::now();

#if 0 // Filesystem basic api tests.
    {
        fs::path p{  };
        LOG_INFO("CWD: %s", p.get_name());

        {
            auto dirs = p.get_contents();
            for (auto&& i : dirs)
                LOG_WARN("\t%s", reinterpret_cast<char const*>(i.name));
        }

        char next_dir[] = ".git";
        ASSERT(p.push_dir(reinterpret_cast<u8*>(next_dir)));
        LOG_INFO("NOW: %s", p.get_name());

        ASSERT(p.pop_dir());
        LOG_INFO("NOW: %s", p.get_name());

        ASSERT(p.push_dir(reinterpret_cast<u8*>(next_dir)));
        LOG_INFO("NOW: %s", p.get_name());

        ASSERT(p.pop_dir());
        LOG_INFO("NOW: %s", p.get_name());

        ASSERT(p.pop_dir());
        LOG_INFO("NOW: %s", p.get_name());

        ASSERT(p.pop_dir());
        LOG_INFO("NOW: %s", p.get_name());

        ASSERT(p.pop_dir());
        LOG_INFO("NOW: %s", p.get_name());

        ASSERT(p.pop_dir());
        LOG_INFO("NOW: %s", p.get_name());

        ASSERT(!p.pop_dir());
        LOG_INFO("NOW: %s", p.get_name());

        char next_dir2[] = "usr/";
        ASSERT(p.push_dir(reinterpret_cast<u8*>(next_dir2)));
        LOG_INFO("NOW: %s", p.get_name());

        char next_dir3[] = "share";
        ASSERT(p.push_dir(reinterpret_cast<u8*>(next_dir3)));
        LOG_INFO("NOW: %s", p.get_name());

        return 0;
    }
#endif

    LOG_INFO("Using font: %s", g::fontname);
    g::buf_pt.buffer_ptr = create_buffer_from_file("./test");

#if 0 // TODO: Temporary stuff for testing the file + unicode api.
    for(umm i = 0; i < g::buf_pt.buffer_ptr->size(); ++i)
    {
        mm line_size = g::buf_pt.buffer_ptr->get_line(i)->size();
        i32 random_number = line_size == 0 ? 0 : std::random_device()() % line_size;
        g::buf_pt.buffer_ptr
            ->get_line(i)
            ->insert(random_number,
                     static_cast<u32>('a' + std::random_device()() % 10));

        g::buf_pt.buffer_ptr
            ->get_line(i)
            ->del_forward(random_number);
    }

    save_buffer(g::buf_pt.buffer_ptr, "./saved_test", encoding::utf8);
    system("diff -s ./test ./saved_test");
    return 0;
#endif

    g::buf_pt = create_buffer_point(g::buf_pt.buffer_ptr);

    xwindow win{ 400, 500 };
    g::tmp_window_hndl = &win;
    win.load_scheme(g::colornames, array_cnt(g::colornames));
    if (!win.load_font(g::fontname))
        PANIC("Cannot load font %s", g::fontname);

    // Load glyph metrics info:
    // TODO: Since they are now just aliases, possibly get rid of them?
    g::font_ascent = win.font->ascent;
    g::font_descent = win.font->descent;
    g::font_height = win.font->height;

    auto init_elapsed = chrono::system_clock::now() - init_start;
    LOG_INFO("Init time: %dms",
             static_cast<int>(chrono::duration_cast<chrono::milliseconds>(init_elapsed).count()));

    while(1)
    {
        handle_event(&win);

        // NOTE(DEBUG): Use this to make sure the bug is not an issue with
        // reusing the canvas
#if 0
        win.canvas = XCreatePixmap(win.dpy, win.win,
                                   win.width, win.height,
                                   DefaultDepth(win.dpy, win.scr));
        win.draw = XftDrawCreate(win.dpy, win.canvas, win.vis, win.cmap);
#endif

        if(g::buffer_is_dirty)
        {
            auto start = chrono::system_clock::now();

            // This is the size of the main buffer relative to the screen.
            i16 marg_left = 5;
            i16 marg_right = 5;
            i16 marg_top = 5;
            i32 marg_bottom = g::font_height + 2;
            i32 buf_width = win.width - (marg_left) - (marg_right);
            i32 buf_heigth = win.height - (marg_top) - (marg_bottom);

            g::buffer_is_dirty = false;
            win.draw_rect(0, 0, win.width, win.height, 0);
            win.draw_rect(marg_left, marg_top, buf_width, buf_heigth, 2);
            win.draw_rect(marg_left + 1, marg_top + 1, buf_width - 2, buf_heigth - 2, 0);
            win.set_clamp_rect(marg_left + 1, marg_top + 1, buf_width - 2, buf_heigth - 2);

            // TODO: simplify & test this horrible thing:
            i32 no_lines = buf_heigth / g::font_height + 1;
            if (no_lines != g::buf_pt.visible_lines)
            {
                mm diff = no_lines - g::buf_pt.visible_lines;
                if (diff > 0 && !g::buf_pt.starting_from_top && g::buf_pt.visible_lines != 0)
                {
                    g::buf_pt.first_line -= diff;
                    if (g::buf_pt.first_line <= 0)
                    {
                        g::buf_pt.first_line = 0;
                        g::buf_pt.starting_from_top = true;
                    }
                }

                g::buf_pt.visible_lines = no_lines;
            }

            // Happens when only last line is visible and it is deleted with backspace.
            if (g::buf_pt.first_line >= g::buf_pt.buffer_ptr->size())
                g::buf_pt.first_line = g::buf_pt.buffer_ptr->size() - 1;

            ASSERT(0 <= g::buf_pt.first_line && g::buf_pt.first_line < g::buf_pt.buffer_ptr->size());
            ASSERT(0 <= g::buf_pt.curr_line && g::buf_pt.curr_line < g::buf_pt.buffer_ptr->size());

            if (g::buf_pt.starting_from_top)
            {
                if (g::buf_pt.curr_line - g::buf_pt.first_line >= no_lines - 1)
                {
                    g::buf_pt.first_line = g::buf_pt.curr_line - (no_lines - 1);
                    g::buf_pt.starting_from_top = false;
                }
                else if (g::buf_pt.curr_line < g::buf_pt.first_line)
                {
                    g::buf_pt.first_line = g::buf_pt.curr_line;
                }
            }
            else
            {
                if (g::buf_pt.curr_line <= g::buf_pt.first_line)
                {
                    g::buf_pt.first_line = g::buf_pt.curr_line;
                    g::buf_pt.starting_from_top = true;
                }
                else if (g::buf_pt.curr_line - g::buf_pt.first_line > no_lines - 1)
                {
                    g::buf_pt.first_line = g::buf_pt.curr_line - (no_lines - 1);
                }
            }

            // Assert that we didn't break anything.
            ASSERT(0 <= g::buf_pt.first_line && g::buf_pt.first_line < g::buf_pt.buffer_ptr->size());
            ASSERT(0 <= g::buf_pt.curr_line && g::buf_pt.curr_line < g::buf_pt.buffer_ptr->size());

            // TODO: Make sure we are not drawing out-of-bounds, because 1. We
            //       loose a lot of time when doing that, 2. It is bugprone.
            //       ^ Check how it works for long lines (longer than a line)
            // TODO: Merge it somehow.
            if (g::buf_pt.starting_from_top)
            {
                for(auto k = 0; k < no_lines; ++k)
                {
                    auto line_to_draw = k + g::buf_pt.first_line;
                    if(line_to_draw >= g::buf_pt.buffer_ptr->size())
                        break;

                    // NOTE: +2 because (+1) we don't want to draw on the
                    //       marigin, (+1) for additional space.
                    auto basex = marg_left + 2;
                    auto basey = marg_top + 2 + (k + 1) * g::font_height - g::font_descent;

                    auto refs = g::buf_pt.buffer_ptr->get_line(line_to_draw)->to_str_refs_();
                    draw_textline_aux(win, g::buf_pt.curr_line == line_to_draw,
                                      marg_left, buf_width - 1,
                                      basex, basey,
                                      refs.data());
                }
            }
            else
            {
                for(auto k = no_lines - 1; k >= 0; --k)
                {
                    auto line_to_draw = k + g::buf_pt.first_line;

                    // We can be starting from bot and not have a full buffer.
                    // TODO: But why? This is super-bugprone!!!
                    if(line_to_draw >= g::buf_pt.buffer_ptr->size())
                        continue;

                    // NOTE: x is +2 because same reason as above. y is + 1
                    //       becuase of the offset and each marigin adds 1
                    //       because we do not draw on them.
                    auto basex = marg_left + 2;
                    auto basey = buf_heigth + 3 -
                        ((no_lines - 1) - k) * g::font_height - g::font_descent;

                    auto refs = g::buf_pt.buffer_ptr->get_line(line_to_draw)->to_str_refs_();
                    draw_textline_aux(win, g::buf_pt.curr_line == line_to_draw,
                                      marg_left, buf_width - 1,
                                      basex, basey,
                                      refs.data());
                }
            }

            win.clear_clamp_rect();

            // Display previous command on the screen:
            {
                u32 buf[64];
                mm len = 0;
                switch(g::prev_comm)
                {
                    case prev_command_type::none:
                    {
                        char const txt_[] = "none";
                        u8 const* txt = reinterpret_cast<u8 const*>(&(txt_[0]));
                        len = array_cnt(txt_);
                        utf8_to_utf32(txt, txt + len, buf, buf+64);
                    } break;

                    case prev_command_type::insert:
                    {
                        char const txt_[] = "insert";
                        u8 const* txt = reinterpret_cast<u8 const*>(&(txt_[0]));
                        len = array_cnt(txt_);
                        utf8_to_utf32(txt, txt + len, buf, buf+64);
                    } break;

                    case prev_command_type::paste:
                    {
                        char const txt_[] = "paste";
                        u8 const* txt = reinterpret_cast<u8 const*>(&(txt_[0]));
                        len = array_cnt(txt_);
                        utf8_to_utf32(txt, txt + len, buf, buf+64);
                    } break;

                    case prev_command_type::remove:
                    {
                        char const txt_[] = "remove";
                        u8 const* txt = reinterpret_cast<u8 const*>(&(txt_[0]));
                        len = array_cnt(txt_);
                        utf8_to_utf32(txt, txt + len, buf, buf+64);
                    } break;

                    case prev_command_type::remove_inplace:
                    {
                        char const txt_[] = "remove_inplace";
                        u8 const* txt = reinterpret_cast<u8 const*>(&(txt_[0]));
                        len = array_cnt(txt_);
                        utf8_to_utf32(txt, txt + len, buf, buf+64);
                    } break;

                    case prev_command_type::undo:
                    {
                        char const txt_[] = "undo";
                        u8 const* txt = reinterpret_cast<u8 const*>(&(txt_[0]));
                        len = array_cnt(txt_);
                        utf8_to_utf32(txt, txt + array_cnt(txt_), buf, buf+64);
                    } break;
                }

                win.draw_text(marg_left, win.height - g::font_descent - 1, 3, buf, len - 1, nullptr);
            }

            win.flush();

            auto elapsed = chrono::system_clock::now() - start;

            LOG_INFO("elapsed: %d",
                     static_cast<int>(chrono::duration_cast<chrono::milliseconds>(elapsed).count()));

            // This will give us about 16ms speed.
            if (elapsed <  16ms)
                std::this_thread::sleep_for(16ms - elapsed);
        }
    }
}

#include "text_buffer.cpp"
#include "gap_buffer.cpp"
#include "xwindow.cpp"
#include "utf.cpp"
#include "internals.cpp"
#include "undo.cpp"
