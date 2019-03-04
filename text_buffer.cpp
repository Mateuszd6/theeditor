#include "cstdio"

// TODO(Cleanup): Globals.
namespace g
{
static int number_of_buffers = 0;
static text_buffer buffers[256];
}

// Line 0 is valid.
void text_buffer::initialize()
{
    // prev_chunks_size = 0;
    lines = static_cast<gap_buffer*>(
        malloc(sizeof(gap_buffer) * TEXT_BUF_INITIAL_CAPACITY));
    memset(lines, 0, sizeof(gap_buffer) * TEXT_BUF_INITIAL_CAPACITY);

    gap_start = 1;
    gap_end = TEXT_BUF_INITIAL_CAPACITY;
    capacity = TEXT_BUF_INITIAL_CAPACITY;

    lines[0].initialize();
}

/// After this move point'th line is not valid and must be initialized before use.
void text_buffer::move_gap_to_point(mm point)
{
    if (point < gap_start)
    {
        mm diff = gap_start - point;
        auto dest_start = gap_end - diff;
        auto source_start = point;
        for (mm i = diff - 1; i >= 0; --i)
            move_gap_bufffer(&lines[source_start + i], &lines[dest_start + i]);

        gap_start -= diff;
        gap_end -= diff;
    }
    else if (point > gap_start)
    {
        mm diff = point - gap_start;
        auto dest_start = gap_start;
        auto source_start = gap_end;
        for (mm i = 0; i < diff; ++i)
            move_gap_bufffer(&lines[source_start + i], &lines[dest_start + i]);

        gap_start += diff;
        gap_end += diff;
    }
    else if (point == gap_start)
    {
    }
    else
        UNREACHABLE();
}

void text_buffer::move_gap_to_buffer_end()
{
    if (gap_end == capacity)
        LOG_WARN("Gap already at the end.");
    else
    {
        auto diff = capacity - gap_end;
        auto dest_start = gap_start;
        auto source_start = gap_end;
        for (mm i = 0; i < diff; ++i)
            move_gap_bufffer(&lines[source_start + i], &lines[dest_start + i]);

        gap_start += diff;
        gap_end += diff;
    }
}

void text_buffer::grow_gap()
{
    ASSERT(capacity > 0);
    mm old_capacity = capacity;

    move_gap_to_buffer_end();
    capacity *= 2;

    // TODO: Fix realloc. Check and abort on leaks.
    lines = static_cast<gap_buffer*>(realloc(lines, sizeof(gap_buffer) * capacity));

    // We need to default our new, uninitialized part of memory.
    for(mm i = gap_end; i < capacity; ++i)
        lines[i] = { 0, nullptr, nullptr, nullptr };

    gap_end = capacity;

    LOG_WARN("Buffer realloced: %ld -> %ld", old_capacity, capacity);
}

std::tuple<bool, mm, mm>
text_buffer::insert_character_impl(mm line, mm point, u32 character)
{
    // This should not be used to insert the newline. User should call
    // insert_newline instead.
    ASSERT(character != static_cast<u32>('\n'));
    ASSERT(line >= 0);
    ASSERT(point >= 0);

    LOG_DEBUG("Inserting character: %c",
              character < 255 ? static_cast<u8>(character) : '?');

    get_line(line)->insert(point++, character);
    return { true, line, point } ;
}

std::tuple<bool, mm, mm>
text_buffer::insert_character(mm line, mm point, u32 character)
{
    auto[succeeded, ret_line, ret_point] = insert_character_impl(line, point, character);
    if (succeeded)
    {
        undo_buf.add_undo(undo_type::insert, &character, 1, line, point);
    }

    return { succeeded, ret_line, ret_point };
}

std::tuple<bool, mm, mm>
text_buffer::insert_range_impl(mm line, mm point, u32* begin, u32* end)
{
    ASSERT(line >= 0);
    ASSERT(line < size());
    ASSERT(point >= 0);
    ASSERT(point <= get_line(line)->size());

    LOG_DEBUG("Inserting %ld characters", end - begin);

    for(u32* i = begin; i != end; ++i)
        ASSERT(*i != static_cast<u32>('\n'));

    get_line(line)->insert_range(point, begin, end);
    return { true, line, point + (end - begin) };
}

std::tuple<bool, mm, mm>
text_buffer::insert_range(mm line, mm point, u32* begin, u32* end)
{
    auto[succeeded, ret_line, ret_point] = insert_range_impl(line, point, begin, end);
    if (succeeded)
    {
        undo_buf.add_undo(undo_type::insert, begin, end - begin, line, point);
    }

    return { succeeded, ret_line, ret_point };
}

std::tuple<bool, mm, mm>
text_buffer::insert_newline_impl(mm line, mm point)
{
    ASSERT(line >= 0);
    ASSERT(line < size());
    ASSERT(point >= 0);
    ASSERT(point <= get_line(line)->size());

    LOG_DEBUG("Inserting newline");

    if(gap_size() <= 2) // TODO: Make constants.
    {
        grow_gap();
    }

    move_gap_to_point(line + 1);
    gap_start++;

    // TODO: Possibly reuse the alloced memory.
    if(lines[line + 1].buffer)
        free(lines[line + 1].buffer);
    lines[line + 1].initialize();

    auto edited_line = get_line(line);
    auto created_line = get_line(line + 1);
    for(auto i = point; i < edited_line->size(); ++i)
    {
        // TODO: Don't insert characters one-by-one here.
        created_line->insert(i - point, edited_line->get(i));
    }
    edited_line->del_to_end(point);

    return { true, line + 1, 0 };
}

std::tuple<bool, mm, mm>
text_buffer::insert_newline(mm line, mm point)
{
    auto[succeeded, ret_line, ret_point] = insert_newline_impl(line, point);
    if (succeeded)
    {
        u32 newline_ch = static_cast<u32>('\n');
        undo_buf.add_undo(undo_type::insert, &newline_ch, 1, line, point);
    }

    return { succeeded, ret_line, ret_point };
}

void
text_buffer::delete_line_impl(mm line)
{
    ASSERT(line > 0);
    ASSERT(line < size());
    move_gap_to_point(line + 1);

    gap_buffer* prev_line = get_line(line - 1);
    gap_buffer* removed_line = get_line(line);

    // The index where removed line characters will be inserted.
    mm prev_line_idx = prev_line->size();

    prev_line->insert_range(prev_line_idx,
                            removed_line->buffer,
                            removed_line->gap_start);

    prev_line->insert_range(prev_line_idx + (removed_line->gap_start - removed_line->buffer),
                            removed_line->gap_end,
                            removed_line->buffer + removed_line->capacity);

    free(lines[gap_start].buffer);
    lines[gap_start].buffer = nullptr;
    gap_start--;
}

u32
text_buffer::del_forward_char_impl(mm line, mm point)
{
    LOG_DEBUG("Deleting character forward");
    ASSERT(point < get_line(line)->size());

    u32 removed_ch = get_line(line)->get(point);
    bool deleted = get_line(line)->del_forward(point);
    ASSERT(deleted);

    return removed_ch;
}

u32
text_buffer::del_forward_newline_impl(mm line, mm point)
{
    LOG_DEBUG("Deleting newline forward");
    ASSERT(point == get_line(line)->size());
    ASSERT(line < size() - 1);

    u32 removed_ch = static_cast<u32>('\n');
    delete_line_impl(line + 1);

    return removed_ch;
}

u32
text_buffer::del_backward_char_impl(mm line, mm point)
{
    LOG_DEBUG("Deleting character backward");
    ASSERT(point > 0);

    u32 removed_ch = get_line(line)->get(point - 1);
    get_line(line)->del_backward(point--);

    return removed_ch;
}

u32
text_buffer::del_backward_newline_impl(mm line, mm point)
{
    LOG_DEBUG("Deleting newline backward");
    ASSERT(line > 0);
    ASSERT(point == 0);

    u32 removed_ch = static_cast<u32>('\n');
    delete_line_impl(line);

    return removed_ch;
}

std::tuple<bool, mm, mm>
text_buffer::del_forward(mm line, mm point)
{
    if (point < get_line(line)->size())
    {
        u32 removed_ch = del_forward_char_impl(line, point);
        undo_buf.add_undo(undo_type::remove_inplace, &removed_ch, 1, line, point);

        return { true, line, point };
    }
    else if (line < size() - 1)
    {
        u32 removed_ch = del_forward_newline_impl(line, point);
        undo_buf.add_undo(undo_type::remove_inplace, &removed_ch, 1, line, point);

        return { true, line, point };
    }
    else
    {
        LOG_WARN("Cannot delete forward");

        // TODO: Should break undo chain.
        return { false, line, point };
    }
}

std::tuple<bool, mm, mm>
text_buffer::del_backward(mm line, mm point)
{
    if (point > 0)
    {
        u32 removed_ch = del_backward_char_impl(line, point);
        undo_buf.add_undo(undo_type::remove, &removed_ch, 1, line, point - 1);

        return { true, line, point - 1 };
    }
    else if (line > 0)
    {
        mm prev_line_size = get_line(line - 1)->size();
        u32 removed_ch = del_backward_newline_impl(line, point);
        undo_buf.add_undo(undo_type::remove, &removed_ch, 1, line - 1, prev_line_size);

        return { true, line - 1, prev_line_size };
    }
    else
    {
        LOG_WARN("Cannot delete backward");

        // TODO: Should break undo chain.
        return { false, line, point };
    }
}

mm
text_buffer::size() const
{
    return capacity - gap_size();
}

mm
text_buffer::gap_size() const
{
    return gap_end - gap_start;
}

gap_buffer*
text_buffer::get_line(mm line) const
{
    ASSERT(line >= 0);
    ASSERT(line < size());

    if (line < gap_start)
        return &lines[line];
    else
        return &lines[gap_end + (line - gap_start)];
}

std::tuple<mm, mm>
text_buffer::apply_insert(u32* data, mm len, mm line, mm index)
{
    for(int i = 0; i < len; ++i)
    {
        if(data[i] == static_cast<u32>('\n'))
        {
            auto[succeeded, ret_line, ret_point] = insert_newline_impl(line, index);
            ASSERT(succeeded); // TODO: For now we assume success.
            line = ret_line;
            index = ret_point;
        }
        else
        {
            auto[succeeded, ret_line, ret_point] = insert_character_impl(line, index, data[i]);
            ASSERT(succeeded); // TODO: For now we assume success.
            line = ret_line;
            index = ret_point;
        }
    }

    return { line, index };
}

std::tuple<mm, mm>
text_buffer::apply_remove(u32* data, mm len, mm line, mm index)
{
    for(int i = 0; i < len; ++i)
    {
        if(data[i] == static_cast<u32>('\n'))
        {
            // Assert that we are at the end of the line.
            ASSERT(get_line(line)->size() == index);

            u32 line_removed = del_forward_newline_impl(line, index);
            ASSERT(line_removed == static_cast<u32>('\n'));
        }
        else
        {
            ASSERT(get_line(line)->get(index) == data[i]);

            u32 char_removed = del_forward_char_impl(line, index);
            ASSERT(char_removed == data[i]);
        }
    }

    return { line, index };
}

bool
buffer_point::insert(u32 character)
{
    // TODO: Encapsulate.
    last_line_idx = -1;

    ASSERT(character != 0);
    if (character == static_cast<u32>('\n'))
    {
        auto[succeeded, line, point] = buffer_ptr->insert_newline(curr_line, curr_idx);
        if (succeeded)
        {
            curr_line = line;
            curr_idx = point;
        }

        ASSERT(point_is_valid());
        return succeeded;
    }
    else
    {
        auto[succeeded, line, point] = buffer_ptr->insert_character(curr_line, curr_idx, character);
        if (succeeded)
        {
            curr_line = line;
            curr_idx = point;
        }

        ASSERT(point_is_valid());
        return succeeded;
    }
}

bool
buffer_point::insert(u32* begin, u32* end)
{
    // TODO: Encapsulate.
    last_line_idx = -1;

    for(u32* curr = begin; curr != end;)
    {
        while(curr != end && *curr != '\n')
        {
            ++curr;
        }

        {
            auto[succeeded, line, point] =
                buffer_ptr->insert_range(curr_line, curr_idx, begin, end);
            ASSERT(succeeded); // TODO: For now assume success.
            curr_line = line;
            curr_idx = point;
        }

        // EOL was reached, not end.
        if(curr != end)
        {
            auto[succeeded, line, point] = buffer_ptr->insert_newline(curr_line, curr_idx);
            ASSERT(succeeded); // TODO: For now assume success.
            curr_line = line;
            curr_idx = point;

            curr++;
        }
    }

    return true;
}

bool buffer_point::del_backward()
{
    // TODO: Encapsulate.
    last_line_idx = -1;

    auto[succeeded, line, point] = buffer_ptr->del_backward(curr_line, curr_idx);

    if(succeeded)
    {
        curr_line = line;
        curr_idx = point;
    }

    ASSERT(point_is_valid()); // We must still be in valid pos.
    return succeeded;
}

bool buffer_point::del_forward()
{
    // TODO: Encapsulate.
    last_line_idx = -1;

    auto[succeeded, line, point] = buffer_ptr->del_forward(curr_line, curr_idx);

    if(succeeded)
    {
        curr_line = line;
        curr_idx = point;
    }

    ASSERT(point_is_valid()); // We must still be in valid pos.
    return succeeded;
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

bool buffer_point::jump_up(mm number_of_lines)
{
    auto goal_idx = (last_line_idx == -1
        ? curr_idx
        : (curr_idx > last_line_idx ? curr_idx : last_line_idx));

    if(curr_line == 0)
        return false;
    else
    {
        curr_line = (curr_line > number_of_lines ? curr_line - number_of_lines : 0);
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

bool buffer_point::jump_down(mm number_of_lines)
{
    auto last_line_index = buffer_ptr->size() - 1;
    auto goal_idx = (last_line_idx == -1
        ? curr_idx
        : (curr_idx > static_cast<mm>(last_line_idx) ? curr_idx : last_line_idx));

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

#if 0
// TODO(Docs): Summary!
// TODO(Cleaup): Move to buffer file.
static buffer* create_new_buffer()
{
    auto result = g::buffers + g::number_of_buffers;
    g::number_of_buffers++;
    result->initialize();

    return result;
}
#endif

namespace detail
{

// TODO: Move to file api.
static inline std::pair<u8 const*, mm>
read_full_file(char const* filename)
{
    FILE* f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    mm fsize = ftell(f);
    fseek(f, 0, SEEK_SET); // same as rewind(f);

    u8* str = static_cast<u8*>(malloc(fsize));
    fread(str, fsize, 1, f);
    fclose(f);

    return { str, fsize };
}

// Write to buffer starting from line'th line, and idx'th character.
// Returns the pair (line + idx) at which the inserting has ended.
static std::pair<mm, mm>
write_to_buffer(text_buffer* buffer,
                mm line, mm idx,
                u32 const* begin, u32 const* end)
{
    // TODO: Possible different newline lineendings.
    u32 const* curr = begin;
    u32 const* prev = begin;

    while(curr != end)
    {
        // TODO: Support more newlines.
        while(curr != end && *curr != static_cast<u32>('\n'))
            ++curr;

        buffer->get_line(line)->insert_range(idx, prev, curr);
        idx += curr - prev;

        if(curr != end) // Different than end means newline character.
        {
#if 0
            buffer->insert_newline(line, buffer->get_line(line)->size());
#else
            // TODO: This is the same as insert_newline but does not record
            //       undo. Think how undo recording can be disabled/enabled.
            if (buffer->gap_size() <= 2)
                buffer->grow_gap();
            buffer->move_gap_to_point(line + 1);
            buffer->gap_start++;
#endif

            line++;
            idx = 0;
            ++curr;
            prev = curr;
        }
        else
            break;
    }

    return { line, idx };
}

// TODO: Move to the file api.
static inline void
load_file_into_buffer_utf8(text_buffer* buffer, char const* file_path)
{
    constexpr i32 chunk_size = 1024;
    u32 output_chunk[chunk_size];

    LOG_INFO("Loading the file: %s", file_path);
    auto[data_ptr, size] = read_full_file(file_path);

    if (data_ptr)
    {
        mm insert_at_line = 0;
        mm insert_at_idx = 0;

        u8 const* src_curr = data_ptr;

        while(src_curr != data_ptr + size)
        {
            auto[src_reached, dest_reached] = utf8_to_utf32(src_curr,
                                                            data_ptr + size,
                                                            output_chunk,
                                                            output_chunk + chunk_size);
            src_curr = src_reached;

            // This will split the lines, by searching for newline characters.
            auto[res_line, res_idx] =
                write_to_buffer(buffer, insert_at_line, insert_at_idx,
                                &(output_chunk[0]), dest_reached);
            insert_at_line = res_line;
            insert_at_idx = res_idx;
        }

        free(const_cast<void*>(static_cast<const void*>(data_ptr)));
    }
    else
    {
        free(const_cast<void*>(static_cast<const void*>(data_ptr)));
        PANIC("Could not read file, becuase it does not exists!");
    }
}
} // namespace detail

// TODO: Non-stupid file reader. Chunk by chunk. Do after unicode is implemented
//       in the gap buffer.
static text_buffer*
create_buffer_from_file(char const* file_path)
{
    auto result = g::buffers + g::number_of_buffers++;

    result->initialize();
    detail::load_file_into_buffer_utf8(result, file_path);

    return result;
}

// TODO: Benchmark this.
static void
save_buffer_utf8(text_buffer* buf, char const* file_path)
{
    constexpr i32 chunk_size = 1024;
    u8 output_chunk[chunk_size];
    u32 input_chunk[chunk_size];
    u32* input_idx = &(input_chunk[0]);

    FILE* file = fopen(file_path, "w");

    auto write_utf32_as_utf8_to_file =
        [&](u32 const* begin, u32 const* end)
        {
            while(begin != end)
            {
                auto[reached_src, reached_dst] =
                    utf32_to_utf8(begin, end, output_chunk, output_chunk + chunk_size);
                begin = reached_src;

                fwrite(output_chunk, 1, reached_dst - output_chunk, file);
            }
        };

    // 4 is more than enought to store any possible newline sequence. For now
    // only linuxish newlines are used.
    u32 newline[4];
    newline[0] = static_cast<u32>('\n');
    newline[1] = 0;

    strref refs[3];
    refs[2] = strref{ newline, newline + 1 };

    for(mm i = 0; i < buf->size(); ++i)
    {
        auto line_as_refs = buf->get_line(i)->to_str_refs_();
        refs[0] = line_as_refs[0];
        refs[1] = line_as_refs[1];

        // For the last line, we dont append a newline character.
        for(i32 j = 0; j < (i + 1 == buf->size() ? 2 : 3); ++j)
        {
            for(auto c : refs[j])
            {
                *input_idx++ = c;

                if(input_idx == input_chunk + chunk_size)
                {
#if 0
                    fprintf(stdout, "Stopped at:\n");
                    buf->get_line(i)->DEBUG_print_state();
#endif

                    write_utf32_as_utf8_to_file(input_chunk, input_chunk + chunk_size);
                    input_idx = &(input_chunk[0]);
                }
            }
        }
    }

    write_utf32_as_utf8_to_file(input_chunk, input_idx);
    fclose(file);
}

static void
save_buffer(text_buffer* buf, char const* file_path, encoding enc)
{
    if (enc == encoding::utf8)
        save_buffer_utf8(buf, file_path);
    else
        PANIC("Saving the buffer in this encoding is not supported.");
}

static buffer_point create_buffer_point(text_buffer* buffer_ptr)
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
