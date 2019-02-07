#ifndef EDITOR_BUFFER_HPP
#define EDITOR_BUFFER_HPP

#include "gap_buffer.hpp"

#define NUMBER_OF_LINES_IN_BUFFER (256)

struct buffer
{
    gap_buffer* lines;
    size_t capacity;
    size_t gap_start;
    size_t gap_end;

    void initialize();

    void move_gap_to_point(size_t point);
    void move_gap_to_buffer_end();

    bool insert_character(size_t line, size_t point, u8 character);
    bool insert_newline(size_t line);
    bool insert_newline_correct(size_t line, size_t point);

    bool delete_line(size_t line);

    size_t size() const;
    size_t gap_size() const;
    gap_buffer* get_line(size_t line) const;

    void DEBUG_print_state() const;
};

static buffer* create_buffer_from_file(char const* file_path);

struct buffer_point
{
    buffer* buffer_ptr;
    u64 first_line;
    u64 curr_line;
    u64 curr_idx;

    /// If the line is switched to the much shorter one, and the current
    /// index is truncated to the end, ths value holds the previous index
    /// until the cursor is moved left / right, something is inserted etc.
    /// After moving to the larger one, this index is tried to be restored.
    /// This value is ignored when it is equal to -1.
    i64 last_line_idx;
    bool starting_from_top;


    bool insert_character_at_point(u8 character);
    bool insert_newline_at_point();

    bool remove_character_backward();
    bool remove_character_forward();

    bool character_right();
    bool character_left();
    bool line_up();
    bool line_down();
    bool jump_up(u64 number_of_lines);
    bool jump_down(u64 number_of_lines);
    bool line_start();
    bool line_end();
    bool buffer_start();
    bool buffer_end();

    bool point_is_valid();
};

static buffer_point create_buffer_point(buffer* buffer_ptr);


#endif // EDITOR_BUFFER_HPP
