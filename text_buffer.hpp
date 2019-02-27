#ifndef TEXT_BUFFER_HPP
#define TEXT_BUFFER_HPP

#include "gap_buffer.hpp"
#include "undo.hpp"
#include "utf.hpp"

#define NUMBER_OF_LINES_IN_BUFFER (256)

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

    bool insert_character(mm line, mm point, u32 character);
    bool insert_newline(mm line, mm point);

    bool delete_line(mm line);

    mm size() const;
    mm gap_size() const;
    gap_buffer* get_line(mm line) const;

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
    i64 last_line_idx;
    bool starting_from_top;


    bool insert_character_at_point(u32 character);
    bool insert_newline_at_point();

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
