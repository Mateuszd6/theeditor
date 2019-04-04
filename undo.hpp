#ifndef UNDO_HPP
#define UNDO_HPP

enum struct undo_type : i32
{
    // Buffer after undoing the remove (text: "This is"):
    // | more text.
    // This is| more text.
    insert,
    remove,

    // Buffer after undoing the remove_inplace (text: "This is"):
    // | more text.
    // |This is more text.
    insert_inplace,
    remove_inplace,
};

enum struct undo_seq_state : i32
{
    none,

    insert,
};

struct undo_metadata
{
    undo_type oper;
    u64 line;
    u64 index;
};

// NOTE: The buffer structure looks like this:
// LEN is the size of DATA on the left.
//
// [[METADATA][DATA][LEN]][[METADATA][DATA][LEN]][[METADATA][DATA][LEN]]
//                                               ^
// In order to move back (assuming not already at the beginning), do:
// * Move back sizeof(LEN) to get the size of the data (remember it).
// * Then skip the data + sizeof(METADATA)
//
// Result:
// [[METADATA][DATA][LEN]][[METADATA][DATA][LEN]][[METADATA][DATA][LEN]]
//                        ^
struct undo_buffer
{
    u8 buffer[1024 * 1024];
    u8* undo_head = &(buffer[0]);
    u8* redo_head = &(buffer[0]);
    undo_seq_state curr_state = undo_seq_state::none;

    void add_undo_impl(undo_type type, u32 const* data, mm len, u64 line, u64 index);
    void add_undo(undo_type type, u32 const* data, mm len, u64 line, u64 index);

    void undo();

    void DEBUG_print_state();
};

#endif // UNDO_HPP
