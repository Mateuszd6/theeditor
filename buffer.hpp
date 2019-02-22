#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "gap_buffer.hpp"
#include "utf.hpp"

#define NUMBER_OF_LINES_IN_BUFFER (256)

struct buffer
{
    gap_buffer* lines;
    umm capacity;
    umm gap_start;
    umm gap_end;

    void initialize();

    void move_gap_to_point(umm point);
    void move_gap_to_buffer_end();

    bool insert_character(umm line, umm point, u32 character);
    bool insert_newline(umm line);
    bool insert_newline_correct(umm line, umm point);

    bool delete_line(umm line);

    umm size() const;
    umm gap_size() const;
    gap_buffer* get_line(umm line) const;

    void DEBUG_print_state() const;
};

static buffer* create_buffer_from_file(char const* file_path);

// TODO: Error code on fail.
static void save_buffer(buffer* buf, char const* file_path, encoding enc);

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


    bool insert_character_at_point(u32 character);
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


#endif // BUFFER_HPP
