#include "config.hpp"

#include "xwindow.hpp"

xwindow::xwindow(int width_, int heigth_)
{
    width = width_;
    height = heigth_;

    // TODO: CHECK IF THEY HAVE SUCCEEDED.
    dpy = XOpenDisplay(nullptr);
    scr = DefaultScreen(dpy);
    vis = DefaultVisual(dpy, 0);
    cmap = DefaultColormap(dpy, scr);

    if (!dpy)
        PANIC("Cannot open display!");

#if 0 // TODO: How on earth do i check the member named 'class' in c++?
    if(visual->class != TrueColor)
        PANIC("Cannot handle non true color visual");
#endif

    XSetWindowAttributes wa;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = (KeyPressMask
                     | KeyReleaseMask
                     | StructureNotifyMask
                     | FocusChangeMask
                     | ExposureMask);

    // TODO: Figure CWBackPixmap out! This makes resizing horriebly slow
    win = XCreateWindow(dpy, DefaultRootWindow(dpy),
                        200, 100, width, height, 0, // TODO: window coords
                        DefaultDepth(dpy, scr),
                        InputOutput,
                        DefaultVisual(dpy, scr),
                        CWEventMask,
                        &wa);

    canvas = XCreatePixmap(dpy, win, width, height, DefaultDepth(dpy, scr));
    gc = XCreateGC(dpy, win, 0, nullptr);
    draw = XftDrawCreate(dpy, canvas, vis, cmap);

    XSetGraphicsExposures(dpy, gc, True);
    XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);
    XClearWindow(dpy, win);
    XChangeWindowAttributes(dpy, win, 0, &wa);

    char const* progname = "user@editor";
    char const* classname = "TheEditor";

    // set the titlebar name
    XStoreName(dpy, win, progname);

    // set the name and class hints for the window manager to use
    XClassHint* classHint = XAllocClassHint();
    if (classHint) {
        classHint->res_name = const_cast<char*>(progname);
        classHint->res_class = const_cast<char*>(classname);
    }
    XSetClassHint(dpy, win, classHint);
    XFree(classHint);

    // Input sutff:
    // loads the XMODIFIERS environment variable to see what IME to use
    XSetLocaleModifiers("");

    targets_atom = XInternAtom(dpy, "TARGETS", 0);
    text_atom = XInternAtom(dpy, "TEXT", 0);
    UTF8 = XInternAtom(dpy, "UTF8_STRING", 1);
    if (UTF8 == None)
        UTF8 = XA_STRING;
    ASSERT(UTF8 != None);

    input_xim = XOpenIM(dpy, nullptr, nullptr, nullptr);
    if(!input_xim)
    {
        // fallback to internal input method
        XSetLocaleModifiers("@im=none");
        input_xim = XOpenIM(dpy, nullptr, nullptr, nullptr);
    }

    input_xic = XCreateIC(input_xim,
                          XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                          XNClientWindow, win,
                          XNFocusWindow, win,
                          nullptr);

    XSetICFocus(input_xic);
    XSelectInput(dpy, win, (KeyPressMask
                            | KeyReleaseMask
                            | StructureNotifyMask
                            | FocusChangeMask
                            | ExposureMask));
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
    XftDrawSetClip(draw, nullptr);
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
xwindow::draw_text(int x, int y, int colorid, u32 const* ptr, mm len, int* adv)
{
    if(adv)
    {
        XGlyphInfo extents;
        XftTextExtents32(dpy, font,
                           reinterpret_cast<FcChar32 const*>(ptr),
                           static_cast<int>(len),
                           &extents);

        *adv = extents.xOff;
    }

    XftDrawString32(draw, scm[colorid], font,
                      x, y,
                      reinterpret_cast<FcChar32 const*>(ptr),
                      static_cast<int>(len));
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

std::pair<char*, i32>
xwindow::load_clipboard_contents()
{
    XEvent event;
    int format;
    unsigned long N, size = 0;
    char *data;
    char *s = nullptr;
    Atom target,
         CLIPBOARD = XInternAtom(dpy, "CLIPBOARD", 0),
         XSEL_DATA = XInternAtom(dpy, "XSEL_DATA", 0);

    XConvertSelection(dpy, CLIPBOARD, UTF8, XSEL_DATA, win, CurrentTime);
    XSync(dpy, 0);

    while(1)
    {
        XNextEvent(dpy, &event);
        switch (event.type)
        {
            case SelectionNotify:
            {
                if (event.xselection.selection != CLIPBOARD)
                    break;

                if (event.xselection.property)
                {
                    XGetWindowProperty(event.xselection.display,
                                       event.xselection.requestor,
                                       event.xselection.property,
                                       0L, (~0L), 0,
                                       AnyPropertyType,
                                       &target,
                                       &format,
                                       &size,
                                       &N,
                                       reinterpret_cast<unsigned char**>(&data));

                    if (target == UTF8 || target == XA_STRING)
                    {
                        s = strndup(data, size);
                        XFree(data);
                    }

                    XDeleteProperty(event.xselection.display,
                                    event.xselection.requestor,
                                    event.xselection.property);
                }
            } goto done;

            case SelectionRequest:
            {
                LOG_WARN("I probobly own the selection so I'll send the request to myself!");

                // TODO: Investigate it there is any reason to send the even to
                //       myself.
                handle_selection_request(&event.xselectionrequest);
            } break;

            // TODO: We should probobly skip every other even, not just die when
            //       it happens.
            default:
            {
                PANIC("This event was not expected here!");
            } break;
        }
    }

done:
    if(clip_text)
        free(clip_text);
    clip_text = s;
    clip_size = size;

    return { clip_text, clip_size };
}

void
xwindow::handle_selection_request(XSelectionRequestEvent* xsr)
{
#if 0 // TODO: Investigate this!
    auto selection = XInternAtom(win->dpy, "CLIPBOARD", 0);
    if (xsr->selection != selection)
        return;
#endif

    XSelectionEvent select_ev{ };
    int R = 0;

    select_ev.type = SelectionNotify;
    select_ev.display = xsr->display;
    select_ev.requestor = xsr->requestor;
    select_ev.selection = xsr->selection;
    select_ev.time = xsr->time;
    select_ev.target = xsr->target;
    select_ev.property = xsr->property;

    if (select_ev.target == targets_atom)
    {
        R = XChangeProperty(select_ev.display,
                            select_ev.requestor,
                            select_ev.property,
                            XA_ATOM,
                            32,
                            PropModeReplace,
                            reinterpret_cast<unsigned char*>(&UTF8),
                            1);
    }
    else if (select_ev.target == XA_STRING || select_ev.target == text_atom)
    {
        R = XChangeProperty(select_ev.display,
                            select_ev.requestor,
                            select_ev.property,
                            XA_STRING,
                            8,
                            PropModeReplace,
                            reinterpret_cast<u8 const*>(clip_text),
                            static_cast<i32>(clip_size));
    }
    else if (select_ev.target == UTF8)
    {
        R = XChangeProperty(select_ev.display,
                            select_ev.requestor,
                            select_ev.property,
                            UTF8,
                            8,
                            PropModeReplace,
                            reinterpret_cast<u8 const*>(clip_text),
                            static_cast<i32>(clip_size));
    }
    else
    {
        select_ev.property = None;
    }

    if ((R & 2) == 0)
        XSendEvent(dpy, select_ev.requestor, 0, 0, reinterpret_cast<XEvent*>(&select_ev));
}

void
xwindow::set_clipboard_contents(unsigned char* text, int size)
{
    if(clip_text)
    {
        free(clip_text);
        clip_text = nullptr;
        clip_size = 0;
    }

    // TODO: Investigate if this is worth/needed to store in the class.
    Atom selection = XInternAtom(dpy, "CLIPBOARD", 0);

    clip_text = static_cast<char*>(malloc(size));
    clip_size = size;
    memcpy(clip_text, text, size);

    XSetSelectionOwner(dpy, selection, win, 0);
    if (XGetSelectionOwner(dpy, selection) != win)
        return;
}
