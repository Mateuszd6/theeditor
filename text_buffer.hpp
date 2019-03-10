#ifndef TEXT_BUFFER_HPP
#define TEXT_BUFFER_HPP

#include "gap_buffer.hpp"
#include "undo.hpp"
#include "utf.hpp"

#define TEXT_BUF_INITIAL_CAPACITY (256)

struct text_buffer
{
    gap_buffer* lines;
    mm capacity;
    mm gap_start;
    mm gap_end;

    undo_buffer undo_buf;

    void initialize();

    void move_gap_to_point(mm point);
    void move_gap_to_buffer_end();

    void grow_gap();

    // *_impl fucntions do the same as fucntions without the suffix, but does
    // *not record undo state. Thus should only be used in the internal
    // *API. Regular fucntions call these, and then if it succeeded, store info
    // *about the operation in the undo buffer.
    std::tuple<bool, mm, mm> insert_character_impl(mm line, mm point, u32 character);
    std::tuple<bool, mm, mm> insert_range_impl(mm line, mm point, u32* begin, u32* end);
    std::tuple<bool, mm, mm> insert_newline_impl(mm line, mm point);

    // These fucntions are called by del_forward/del_backward and return the
    // deleted character. They assume very special scenarios so never call them.
    u32 del_forward_char_impl(mm line, mm point);
    u32 del_forward_newline_impl(mm line, mm point);
    u32 del_backward_char_impl(mm line, mm point);
    u32 del_backward_newline_impl(mm line, mm point);

    // All contents of the current line goes to the previous line. Current line
    // is removed.
    void delete_line_impl(mm line);

    std::tuple<bool, mm, mm> insert(mm line, mm point, u32* begin, u32* end);

    std::tuple<bool, mm, mm> del_forward(mm line, mm point);
    std::tuple<bool, mm, mm> del_backward(mm line, mm point);

    mm size() const;
    mm gap_size() const;
    gap_buffer* get_line(mm line) const;

    // TODO: They are horrible.
    std::tuple<mm, mm> apply_insert(u32* data, mm len, mm line, mm index);
    std::tuple<mm, mm> apply_remove(u32* data, mm len, mm line, mm index);

    void DEBUG_print_state() const;
};

// TODO: Error code on fail.
static text_buffer* create_buffer_from_file(char const* file_path);

// TODO: Error code on fail.
static void save_buffer(text_buffer* buf, char const* file_path, encoding enc);

struct buffer_point
{
    text_buffer* buffer_ptr;
    mm first_line;
    mm curr_line;
    mm curr_idx;

    /// If the line is switched to the much shorter one, and the current
    /// index is truncated to the end, ths value holds the previous index
    /// until the cursor is moved left / right, something is inserted etc.
    /// After moving to the larger one, this index is tried to be restored.
    /// This value is ignored when it is equal to -1.
    mm last_line_idx;
    bool starting_from_top;

    bool insert(u32 character);
    bool insert(u32* begin, u32* end);

    bool del_backward();
    bool del_forward();

    bool character_right();
    bool character_left();
    bool line_up();
    bool line_down();
    bool jump_up(mm number_of_lines);
    bool jump_down(mm number_of_lines);
    bool line_start();
    bool line_end();
    bool buffer_start();
    bool buffer_end();

    bool point_is_valid();
};

static buffer_point create_buffer_point(text_buffer* buffer_ptr);


#endif // TEXT_BUFFER_HPP
