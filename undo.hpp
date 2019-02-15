#ifndef UNDO_HPP
#define UNDO_HPP

enum struct undo_type
{
    insert,
    remove,
};

struct undo_metadata
{
    undo_type oper;
    u64 line;
    u64 index;
};

static void break_undo_chain();

static void add_undo(undo_type type, u32 const* data, mm len, u64 line, u64 index);

static void undo();

static void DEBUG_print_state();

#endif // UNDO_HPP
