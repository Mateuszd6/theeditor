#include "undo.hpp"

void
undo_buffer::break_undo_chain()
{
    redo_head = undo_head;
}

void
undo_buffer::add_undo_impl(undo_type type, u32 const* data, mm len, u64 line, u64 index)
{
    u8* new_head = undo_head;

    *(reinterpret_cast<undo_metadata*>(new_head)) = undo_metadata { type, line, index };
    new_head += sizeof(undo_metadata);

    u32* data_ptr = reinterpret_cast<u32*>(new_head);
    for(u32 const* p = data; p != data + len;)
        *data_ptr++ = *p++;

    new_head += (sizeof(u32) * (len % 2 == 1 ? len + 1 : len));
    *(reinterpret_cast<mm*>(new_head)) = len;
    new_head += sizeof(len);

    undo_head = new_head;
}

static inline std::tuple<undo_metadata*, u32*, mm*>
get_elem_under(u8* head)
{
    mm* len_ptr = reinterpret_cast<mm*>(head) - 1;
    u32* data_ptr = reinterpret_cast<u32*>(len_ptr) - ((*len_ptr % 2 == 0) ? (*len_ptr) : (*len_ptr + 1));
    undo_metadata* mdata_ptr = reinterpret_cast<undo_metadata*>(data_ptr) - 1;

    return { mdata_ptr, data_ptr, len_ptr };
}


void
undo_buffer::add_undo(undo_type type, u32 const* data, mm len, u64 line, u64 index)
{
    LOG_WARN("Curr state: %d, type: %d", curr_state, type);

    // Check if we are in the sequence (e.g. character insertion sequence).
    if(curr_state == undo_seq_state::insert && type == undo_type::insert)
    {
        auto[mdata_ptr, data_ptr, len_ptr] = get_elem_under(undo_head);
        LOG_INFO("Current head is { (%lu, %lu), %c, %ld}",
                 mdata_ptr->line, mdata_ptr->index,
                 *data_ptr < 255 ? static_cast<char>(*data_ptr) : '?',
                 *len_ptr);


        u32* last_ch_ptr = data_ptr + (*len_ptr - 1);
        bool endswith_newlines = (*last_ch_ptr == static_cast<u32>('\n'));
        bool endswith_spaces = (*last_ch_ptr == static_cast<u32>(' '));
        bool endswith_text = !endswith_newlines && !endswith_spaces;
        bool inserting_newlines = std::all_of(data, data + len, [](u32 i){ return i == '\n'; });
        bool inserting_spaces = std::all_of(data, data + len, [](u32 i){ return i == ' '; });
        bool inserting_text = !inserting_newlines && !inserting_spaces;

        if (mdata_ptr->line == line
            && mdata_ptr->index + *len_ptr == index
            && ((endswith_newlines && inserting_newlines)
                || (endswith_spaces && inserting_spaces)
                || (endswith_text && inserting_text)
                || (endswith_text && inserting_spaces)))
        {
            mm old_len = *len_ptr;
            mm new_len = old_len + len;
            last_ch_ptr++;
            for (mm i = 0; i < len; ++i)
                *last_ch_ptr++ = data[i];
            if (new_len % 2 == 1)
                last_ch_ptr++;

            *(reinterpret_cast<mm*>(last_ch_ptr)) = new_len;
            undo_head = reinterpret_cast<u8*>(last_ch_ptr) + sizeof(mm);

            return;
        }
    }

    // If we haven't hit any of the conditions before, just set this one to none.
    curr_state = undo_seq_state::none;

    add_undo_impl(type, data, len, line, index);

    // TODO: can break_undo_chain() be at the top?
    break_undo_chain();

    if(type == undo_type::insert)
    {
        curr_state = undo_seq_state::insert;
    }
}

void
undo_buffer::undo()
{
    if (redo_head == &(buffer[0]))
    {
        LOG_ERROR("No more undo information.");
        return;
    }

    u8* head = redo_head;

    mm len = *(reinterpret_cast<mm*>(head - sizeof(mm)));
    head -= ((len % 2 == 1 ? len + 1 : len) * sizeof(u32) + sizeof(mm) + sizeof(undo_metadata));
    u32* data_head = reinterpret_cast<u32*>(head + sizeof(undo_metadata));
    undo_metadata* mdata = reinterpret_cast<undo_metadata*>(head);

    // TODO: Make a fucntion utf32_to_utf8_allocate and call it here!
    u8 utf8_buf[1024];
    auto[_, dest_reached] = utf32_to_utf8(data_head, data_head + len,
                                          utf8_buf, utf8_buf + 1023);
    *dest_reached = '\0';

    switch (mdata->oper)
    {
        case undo_type::insert:
        {
            LOG_WARN("Undoing insertion of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, utf8_buf);

            add_undo_impl(undo_type::remove, data_head, len, mdata->line, mdata->index);
            auto[new_line, new_point] =
                g::buf_pt.buffer_ptr->apply_remove(data_head, len, mdata->line, mdata->index);
            g::buf_pt.curr_line = new_line;
            g::buf_pt.curr_idx = new_point;
        } break;

        case undo_type::insert_inplace:
        {
            LOG_WARN("Undoing insertion _inplace_ of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, utf8_buf);

            add_undo_impl(undo_type::remove_inplace, data_head, len, mdata->line, mdata->index);
            auto[new_line, new_point] =
                g::buf_pt.buffer_ptr->apply_remove(data_head, len, mdata->line, mdata->index);
            g::buf_pt.curr_line = new_line;
            g::buf_pt.curr_idx = new_point;
        } break;

        case undo_type::remove:
        {
            LOG_WARN("Undoing remove of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, utf8_buf);

            add_undo_impl(undo_type::insert, data_head, len, mdata->line, mdata->index);
            auto[new_line, new_point] =
                g::buf_pt.buffer_ptr->apply_insert(data_head, len, mdata->line, mdata->index);
            g::buf_pt.curr_line = new_line;
            g::buf_pt.curr_idx = new_point;
        } break;

        case undo_type::remove_inplace:
        {
            LOG_WARN("Undoing remove _inplace_ of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, utf8_buf);

            add_undo_impl(undo_type::insert_inplace, data_head, len, mdata->line, mdata->index);
            auto[new_line, new_point] =
                g::buf_pt.buffer_ptr->apply_insert(data_head, len, mdata->line, mdata->index);
            g::buf_pt.curr_line = new_line;
            g::buf_pt.curr_idx = new_point;

            for(int i = 0; i < len; ++i)
            {
                bool moved_left = g::buf_pt.character_left();
                ASSERT(moved_left);
            }
        } break;
    }

    redo_head = head;
}

inline void
undo_buffer::DEBUG_print_state()
{
    u8* head = undo_head;
    while(head != &buffer[0])
    {
        mm len = *(reinterpret_cast<mm*>(head - sizeof(mm)));
        head -= ((len % 2 == 1 ? len + 1 : len) * sizeof(u32)
                 + sizeof(mm) + sizeof(undo_metadata));

        // TODO: make 1024 a constant
        u8 utf32_buf[1024];
        u32* data_head = reinterpret_cast<u32*>(head + sizeof(undo_metadata));
        auto[_, dest_reached] = utf32_to_utf8(data_head, data_head + len,
                                              utf32_buf, utf32_buf + 1024);

        if (dest_reached == utf32_buf + 1024)
        {
            (*(dest_reached - 1)) = '\0';
            (*(dest_reached - 2)) = ']';
            (*(dest_reached - 3)) = '.';
            (*(dest_reached - 4)) = '.';
            (*(dest_reached - 5)) = '.';
            (*(dest_reached - 6)) = '[';
        }
        else
        {
            // If we fit in the buffer, just put 0 at the and to print it as asciiz
            *dest_reached = '\0';
        }

        printf("BUFFER: op %d, at:%lu:%lu characters[%lu]: '%s'\n",
               reinterpret_cast<undo_metadata*>(head)->oper,
               reinterpret_cast<undo_metadata*>(head)->line,
               reinterpret_cast<undo_metadata*>(head)->index,
               len,
               utf32_buf[0] == '\n' ? reinterpret_cast<u8 const*>("EOL") : utf32_buf);
    }
}
