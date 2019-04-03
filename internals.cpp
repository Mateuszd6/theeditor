#include "internals.hpp"

namespace g
{
    static std::pair<key, std::string> keyboard_shortcuts[] = {
        { key{ u32('z'), 0, ctrl }, "Undo" },
        { key{ u32('c'), 0, ctrl }, "Copy" },
        { key{ u32('v'), 0, ctrl }, "Paste" },
        { key{ u32('x'), 0, ctrl }, "Kill" },
        { key{ u32('s'), 0, ctrl }, "Save" },
        { key{ 0, Escape, ctrl }, "Quit" },
        { key{ 0, keycode_values::Tab, ctrl }, "Reindent" },
        { key{ u32(' '), 0, ctrl }, "Set Mark" },

        { key{ 0, Escape, 0 }, "Debug Print Undo" },
        { key{ 0, BackSpace, 0 }, "Delete Backward" },
        { key{ 0, Delete, 0 }, "Delete Forward" },
        { key{ 0, Tab, 0 }, "Insert Tab" },
        { key{ 0, Return, 0 }, "Insert Newline" },
        { key{ 0, Up, 0 }, "Move Up" },
        { key{ 0, Down, 0 }, "Move Down" },
        { key{ 0, Left, 0 }, "Move Left" },
        { key{ 0, Right, 0 }, "Move Right" },
        { key{ 0, End, 0 }, "Move End" },
        { key{ 0, Home, 0 }, "Move Home" },
    };
}

static inline bool
is_special_key(KeySym ksm)
{
    // Special keys have their values in range from 0xFF00 to 0xFFFF.
    return ((ksm & 0xFF00) == 0xFF00 && keycode_names[ksm & 0xFF][0] != 0);
}

static inline std::string const*
is_shortcut(key const& k)
{
    for(auto const& kvp: g::keyboard_shortcuts)
    {
        if(k.codept == kvp.first.codept
           && k.keycode == kvp.first.keycode
           && (kvp.first.mod_mask & k.mod_mask) == kvp.first.mod_mask)
        {
            return &(kvp.second);
        }
    }

    return nullptr;
}
