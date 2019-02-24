#include "internals.hpp"

namespace g
{
    static std::pair<key, std::string> keyboard_shortcuts[] = {
        { key{ u32('z'), 0, ctrl }, "Undo" },
        { key{ u32('c'), 0, ctrl }, "Copy" },
        { key{ u32('v'), 0, ctrl }, "Paste" },
        { key{ u32('x'), 0, ctrl }, "Kill" },
        { key{ 0, Escape, ctrl }, "Quit" },
        { key{ 0, keycode_values::Tab, ctrl }, "Reindent" },
        { key{ u32(' '), 0, ctrl }, "Set Mark" },
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
