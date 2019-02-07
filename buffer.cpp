#include "cstdio"

// TODO(Cleanup): Try to use as less globals as possible...
namespace g
{
    static int number_of_buffers = 0;
    static buffer buffers[256];
}

// Line 0 is valid.
void buffer::initialize()
{
    // prev_chunks_size = 0;
    lines = s_cast<gap_buffer*>(
        malloc(sizeof(gap_buffer) * NUMBER_OF_LINES_IN_BUFFER));
    gap_start = 1;
    gap_end = NUMBER_OF_LINES_IN_BUFFER;
    capacity = NUMBER_OF_LINES_IN_BUFFER;

    lines[0].initialize();
}

/// After this move point'th line is not valid and must be initialized before use.
void buffer::move_gap_to_point(size_t point)
{
    if (point < gap_start)
    {
        auto diff = gap_start - point;
        auto dest_start = gap_end - diff;
        auto source_start = point;
        for (auto i = s_cast<i64>(diff - 1); i >= 0; --i)
            move_gap_bufffer(&lines[source_start + i], &lines[dest_start + i]);

        gap_start -= diff;
        gap_end -= diff;
    }
    else if (point > gap_start)
    {
        auto diff = point - gap_start;
        auto dest_start = gap_start;
        auto source_start = gap_end;
        for (auto i = 0_u64; i < diff; ++i)
            move_gap_bufffer(&lines[source_start + i], &lines[dest_start + i]);

        gap_start += diff;
        gap_end += diff;
    }
    else if (point == gap_start) { }
    else
        UNREACHABLE();
}

void buffer::move_gap_to_buffer_end()
{
    if (gap_end == capacity)
        LOG_WARN("Gap already at the end.");
    else
    {
        auto diff = capacity - gap_end;
        auto dest_start = gap_start;
        auto source_start = gap_end;
        for (auto i = 0_u64; i < diff; ++i)
            move_gap_bufffer(&lines[source_start + i], &lines[dest_start + i]);

        gap_start += diff;
        gap_end += diff;
    }
}

// TODO: Dont use it. Use: get_line()->insert....
bool buffer::insert_character(size_t line, size_t point, u8 character)
{
    get_line(line)->insert_at_point(point, character);

    // TODO: Change for normal assertion (if it is really necesarry, becasuse it
    //       looks usless).
#if 0
    if (get_line(line)->gap_start == nullptr)
        PANIC("Nooooooo");
#endif

    return true;
}

/// line - number of line after we insert newline.
bool buffer::insert_newline(size_t line)
{
    if(gap_size() <= 2) // TODO: Make constants.
    {
        move_gap_to_buffer_end();
        capacity *= 2;

        // TODO: Fix realloc.
        lines = s_cast<gap_buffer*>(realloc(lines, sizeof(gap_buffer) * capacity));
        gap_end = capacity;
    }

    move_gap_to_point(line + 1);
    gap_start++;
    lines[line + 1].initialize();

    // TODO: Case when there is not enought memory.
    return true;
}

bool buffer::insert_newline_correct(size_t line, size_t point)
{
    ASSERT(line < size());

    insert_newline(line);
    auto edited_line = get_line(line);
    auto created_line = get_line(line + 1);
    ASSERT(point <= edited_line->size());

    for(auto i = point; i < edited_line->size(); ++i)
        created_line->insert_at_point(i - point, edited_line->get(i));

    edited_line->delete_to_the_end_of_line(point);
    return true;
}

bool buffer::delete_line(size_t line)
{
    ASSERT(line > 0);
    ASSERT(line < size());
    move_gap_to_point(line + 1); // TODO(NEXT): What about it!

    auto prev_line = get_line(line - 1);
    auto removed_line = get_line(line);
    auto prev_line_size = prev_line->size();
    auto removed_line_size = removed_line->size();
    auto prev_line_idx = prev_line_size;

    // TODO: Don't use the constant. Think about optimal value here!
    prev_line->reserve_gap(removed_line_size + 4);
    for(auto i = 0_u64; i < removed_line_size; ++i)
        prev_line->insert_at_point(prev_line_idx++, removed_line->get(i));


    // TODO: Free the memory allocted it the line, just removed.
    gap_start--;
    return true;
}

size_t buffer::size() const
{
    return capacity - gap_size();
}

size_t buffer::gap_size() const
{
    return gap_end - gap_start;
}

gap_buffer* buffer::get_line(size_t line) const
{
    ASSERT(line < size());

    if (line < gap_start)
        return &lines[line];
    else
        return &lines[gap_end + (line - gap_start)];
}

void buffer::DEBUG_print_state() const
{
    auto in_gap = false;
    printf("Gap: %ld-%ld\n", gap_start, gap_end);
    for (auto i = 0_u64; i < capacity; ++i)
    {
        if (i == gap_start)
            in_gap = true;

        if (i == gap_end)
            in_gap = false;

        if (!in_gap)
        {
            printf("%3ld:[%3ld]  ", i, lines[i].size());
#if 0
            for(auto _ : lines[i])
                printf("%c", _);
#else
            auto line = lines[i].to_c_str();
            printf("%s", line);
            free(s_cast<void*>(line));
#endif
            printf("\n");
        }
        else if (in_gap && i - gap_start < 2)
            printf("%3ld:  ???\n", i);
        else if (in_gap && i == gap_end - 1)
            printf("      ...\n%3ld:  ???\n", i);
    }
}

bool buffer_point::insert_character_at_point(u8 character)
{
    buffer_ptr->get_line(curr_line)->insert_at_point(curr_idx++, character);
    last_line_idx = -1;

    // For now we assume that the inserting character cannot fail.
    return true;
}

bool buffer_point::insert_newline_at_point()
{
    buffer_ptr->insert_newline_correct(curr_line, curr_idx);

    curr_line++;
    curr_idx = 0;
    last_line_idx = -1;

    // For now we assume that the inserting character cannot fail.
    return true;
}

bool buffer_point::remove_character_backward()
{
    if(curr_idx > 0)
    {
        buffer_ptr->get_line(curr_line)->delete_char_backward(curr_idx--);
        last_line_idx = -1;

        return true;
    }
    else if(curr_line > 0)
    {
        auto line_size = buffer_ptr->get_line(curr_line - 1)->size();

        buffer_ptr->delete_line(curr_line);
        curr_line--;
        curr_idx = line_size;
        last_line_idx = -1;

        return true;
    }
    else
        return false;
}

bool buffer_point::remove_character_forward()
{
    if(curr_idx < buffer_ptr->get_line(curr_line)->size())
    {
        buffer_ptr->get_line(curr_line)->delete_char_forward(curr_idx);
        last_line_idx = -1;

        return true;
    }
    else if(curr_line < buffer_ptr->size() - 1)
    {
        curr_line++;

        // TODO(Cleaup): This is copypaste.
        auto line_size = buffer_ptr->get_line(curr_line - 1)->size();

        buffer_ptr->delete_line(curr_line);
        curr_line--;
        curr_idx = line_size;
        last_line_idx = -1;

        return true;
    }
    else
        return false;
}

bool buffer_point::character_right()
{
    auto line = buffer_ptr->get_line(curr_line);
    if(curr_idx < line->size())
    {
        // Pointer can be at 'line->size()' as we might append to the current line.
        ASSERT(curr_idx < line->size());
        curr_idx++;
        last_line_idx = -1;

        return true;
    }
    else if(curr_line < buffer_ptr->size() - 1)
    {
        curr_line++;
        curr_idx = 0;
        last_line_idx = -1;

        return true;
    }
    else
        return false;
}

bool buffer_point::character_left()
{
    if(curr_idx > 0)
    {
        ASSERT(curr_idx > 0);
        curr_idx--;
        last_line_idx = -1;

        return true;
    }
    else if(curr_line > 0)
    {
        curr_line--;
        curr_idx = buffer_ptr->get_line(curr_line)->size();
        last_line_idx = -1;

        return true;
    }
    else
        return false;
}

bool buffer_point::line_up()
{
    if(curr_line > 0)
    {
        curr_line--;
        if (last_line_idx >= 0)
        {
            LOG_DEBUG("Last line idx is %ld. Jumping:", last_line_idx);
            curr_idx = last_line_idx;
            last_line_idx = -1;
        }

        auto curr_line_size = buffer_ptr->get_line(curr_line)->size();
        if (curr_idx > curr_line_size)
        {
            last_line_idx = curr_idx;
            curr_idx = curr_line_size;
            LOG_DEBUG("Truncating the cursor. (Saved: %ld)", last_line_idx);
        }

        return true;
    }
    else
        return false;
}

bool buffer_point::line_down()
{
    if(curr_line < buffer_ptr->size() - 1)
    {
        curr_line++;
        if (last_line_idx >= 0)
        {
            LOG_DEBUG("Last line idx is %ld. Jumping:", last_line_idx);
            curr_idx = last_line_idx;
            last_line_idx = -1;
        }

        auto curr_line_size = buffer_ptr->get_line(curr_line)->size();
        if (curr_idx > curr_line_size)
        {
            last_line_idx = curr_idx;
            curr_idx = curr_line_size;
            LOG_DEBUG("Truncating the cursor. (Saved: %ld)", last_line_idx);
        }

        return true;
    }
    else
        return false;
}

bool buffer_point::jump_up(u64 number_of_lines)
{
    auto goal_idx = (last_line_idx == -1
                     ? curr_idx
                     : (curr_idx > s_cast<u64>(last_line_idx) ? curr_idx : last_line_idx));

    if(curr_line == 0)
        return false;
    else
    {
        curr_line = curr_line > number_of_lines ? curr_line - number_of_lines : 0;
        first_line = curr_line;
        starting_from_top = true;

        auto line_len = buffer_ptr->get_line(curr_line)->size();
        if(goal_idx > line_len)
        {
            last_line_idx = goal_idx;
            curr_idx = line_len;
            LOG_WARN("Done this! goal idx is: %ld\n", goal_idx);
        }
        else
            last_line_idx = -1;

        return true;
    }
}

bool buffer_point::jump_down(u64 number_of_lines)
{
    auto last_line_index = buffer_ptr->size() - 1;
    auto goal_idx = (last_line_idx == -1
                     ? curr_idx
                     : (curr_idx > s_cast<u64>(last_line_idx) ? curr_idx : last_line_idx));

    if(curr_line == last_line_index)
        return false;
    else
    {
        curr_line = (curr_line + number_of_lines > last_line_index
                     ? last_line_index
                     : curr_line + number_of_lines);
        first_line = curr_line;
        starting_from_top = true;

        auto line_len = buffer_ptr->get_line(curr_line)->size();
        if(goal_idx > line_len)
        {
            last_line_idx = goal_idx;
            curr_idx = line_len;
            LOG_WARN("Done this! goal idx is: %ld\n", goal_idx);
        }
        else
            last_line_idx = -1;

        return true;
    }
}

bool buffer_point::line_start()
{
    curr_idx = 0;
    last_line_idx = -1;

    return true;
}

bool buffer_point::line_end()
{
    curr_idx = buffer_ptr->get_line(curr_line)->size();
    last_line_idx = -1;

    return true;
}

bool buffer_point::buffer_start()
{
    curr_line = 0;
    line_start();

    return true;
}

bool buffer_point::buffer_end()
{
    curr_line = buffer_ptr->size() - 1;
    line_end();

    return true;
}

bool buffer_point::point_is_valid()
{
    return (buffer_ptr->size() > curr_line && buffer_ptr->get_line(curr_line)->size() >= curr_idx);
}

// TODO(Docs): Summary!
// TODO(Cleaup): Move to buffer file.
static buffer* CreateNewBuffer()
{
    auto result = g::buffers + g::number_of_buffers;
    g::number_of_buffers++;
    result->initialize();

    return result;
}

// TODO: Non-stupid file reader. Chunk by chunk. Do after unicode is implemented
//       in the gap buffer.
static buffer* create_buffer_from_file(char const* file_path)
{
    auto result = g::buffers + g::number_of_buffers;
    g::number_of_buffers++;
    result->initialize();

    auto file = fopen(file_path, "r");
    auto first_line_inserted = false;
    auto line_idx = -1_i64;
    auto line_capacity = 128_u64;
    auto line_size = 0_u64;
    auto line = s_cast<i8*>(malloc(sizeof(i8) * line_capacity));

    line[0] = 0_u8;
    auto c = EOF;
    do
    {
        c = fgetc(file);
        if(c == EOF && line_size == 0)
            break;

        if(c == '\n' || c == EOF)
        {
            // TODO: Im not sure if it is correct. Hope it fixed a bug with
            // adding an empty line after loading the buffer.

            if(!first_line_inserted)
                first_line_inserted = true;
            else
                result->insert_newline(line_idx);

            for(auto i = 0_u64; i < line_size; ++i)
                result->get_line(line_idx + 1)->insert_at_point(i, line[i]);

            line_idx++;
            line_size = 0;
            continue;
        }

        if(c == EOF)
            break;

        if (line_size + 1 >= line_capacity)
        {
            line_capacity *= 2;
            // TODO: Fix common realloc mistake!
            line = s_cast<i8*>(realloc(line, sizeof(i8) * line_capacity));
            ASSERT(line);
        }

        line[line_size++] = s_cast<u8>(c);
    }
    while(c != EOF);

    // may check feof here to make a difference between eof and io failure
    // -- network timeout for instance
    fclose(file);
    free(line);

    LOG_DEBUG("DONE");

    return result;
}

static buffer_point create_buffer_point(buffer* buffer_ptr)
{
    auto result = buffer_point {};
    result.buffer_ptr = buffer_ptr;
    result.first_line = 0;
    result.curr_line = 0;
    result.curr_idx = 0;
    result.last_line_idx = -1;
    result.starting_from_top = true;

    return result;
}
