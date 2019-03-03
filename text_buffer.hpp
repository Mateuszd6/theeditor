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

    // Character must not be '\n'. Use insert_newline to do it.
    std::tuple<bool, mm, mm> insert_character(mm line, mm point, u32 character);

    // Should be the same as calling end - begin times insert_charater, but
    // otpimized. The range cannot have '\n' characters in it!!
    std::tuple<bool, mm, mm> insert_range(mm line, mm point, u32* begin, u32* end);

    // Insert newline character at given point.
    std::tuple<bool, mm, mm> insert_newline(mm line, mm point);

    // All contents of the current line goes to the previous line. Current line
    // is removed. TODO: This is private API.
    void delete_line(mm line);

    std::tuple<bool, mm, mm> del_forward(mm line, mm point);
    std::tuple<bool, mm, mm> del_backward(mm line, mm point);

    mm size() const;
    mm gap_size() const;
    gap_buffer* get_line(mm line) const;

    // TODO: They are horrible.
    void apply_insert(u32* data, mm len, u64 line, u64 index);
    void apply_remove(u32* data, mm len, u64 line, u64 index);

    void DEBUG_print_state() const;
};

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

    bool remove_character_backward();
    bool remove_character_forward();

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
