#include "undo.hpp"

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

void
undo_buffer::add_undo(undo_type type, u32 const* data, mm len, u64 line, u64 index)
{
    // TODO: Simplify as it doesn't do anything more right now.
    add_undo_impl(type, data, len, line, index);
}

void
undo_buffer::undo()
{
    // TODO: Make sure it is fine here!
    if (g::prev_comm != prev_command_type::undo)
        redo_head = undo_head;

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
    constexpr mm dbg_buf_len = 1024;
    u8 dbg_buf[dbg_buf_len];

    auto[_, dest_reached] = utf32_to_utf8(data_head, data_head + len,
                                          dbg_buf, dbg_buf + (dbg_buf_len - 1));
    *dest_reached = '\0';

    switch (mdata->oper)
    {
        case undo_type::insert:
        {
            LOG_WARN("Undoing insertion of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, dbg_buf);

            add_undo_impl(undo_type::remove, data_head, len, mdata->line, mdata->index);
            auto[new_line, new_point] =
                g::buf_pt.buffer_ptr->apply_remove(data_head, len, mdata->line, mdata->index);
            g::buf_pt.curr_line = new_line;
            g::buf_pt.curr_idx = new_point;
        } break;

        case undo_type::insert_inplace:
        {
            LOG_WARN("Undoing insertion _inplace_ of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, dbg_buf);

            add_undo_impl(undo_type::remove_inplace, data_head, len, mdata->line, mdata->index);
            auto[new_line, new_point] =
                g::buf_pt.buffer_ptr->apply_remove(data_head, len, mdata->line, mdata->index);
            g::buf_pt.curr_line = new_line;
            g::buf_pt.curr_idx = new_point;
        } break;

        case undo_type::remove:
        {
            LOG_WARN("Undoing remove of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, dbg_buf);

            add_undo_impl(undo_type::insert, data_head, len, mdata->line, mdata->index);
            auto[new_line, new_point] =
                g::buf_pt.buffer_ptr->apply_insert(data_head, len, mdata->line, mdata->index);
            g::buf_pt.curr_line = new_line;
            g::buf_pt.curr_idx = new_point;
        } break;

        case undo_type::remove_inplace:
        {
            LOG_WARN("Undoing remove _inplace_ of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, dbg_buf);

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

        constexpr mm buf_len = 1024;
        // If the outputed text is longer that buffer len, this will truncate
        // the text.
        u8 utf8_buf[buf_len];
        u32* data_head = reinterpret_cast<u32*>(head + sizeof(undo_metadata));
        auto[_, dest_reached] = utf32_to_utf8(data_head, data_head + len,
                                              utf8_buf, utf8_buf + buf_len);

        if (dest_reached == utf8_buf + buf_len)
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
               utf8_buf[0] == '\n' ? reinterpret_cast<u8 const*>("EOL") : utf8_buf);
    }
}
