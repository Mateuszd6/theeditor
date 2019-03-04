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

// TODO: think about it, because this looks usless.
void
undo_buffer::add_undo(undo_type type, u32 const* data, mm len, u64 line, u64 index)
{
    add_undo_impl(type, data, len, line, index);
    break_undo_chain();
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

    // TODO: What happend if we index out of buffer?
    u8 utf32_buf[1024];
    auto[_, dest_reached] = utf32_to_utf8(data_head, data_head + len,
                                          utf32_buf, utf32_buf + 1024);
    *dest_reached = '\0';

    switch (mdata->oper)
    {
        case undo_type::insert:
        {
            LOG_WARN("Undoing insertion of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, utf32_buf);

            add_undo_impl(undo_type::remove, data_head, len, mdata->line, mdata->index);
            auto[new_line, new_point] =
                g::buf_pt.buffer_ptr->apply_remove(data_head, len, mdata->line, mdata->index);
            g::buf_pt.curr_line = new_line;
            g::buf_pt.curr_idx = new_point;
        } break;

        case undo_type::insert_inplace:
        {
            LOG_WARN("Undoing insertion _inplace_ of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, utf32_buf);

            add_undo_impl(undo_type::remove_inplace, data_head, len, mdata->line, mdata->index);
            auto[new_line, new_point] =
                g::buf_pt.buffer_ptr->apply_remove(data_head, len, mdata->line, mdata->index);
            g::buf_pt.curr_line = new_line;
            g::buf_pt.curr_idx = new_point;
        } break;

        case undo_type::remove:
        {
            LOG_WARN("Undoing remove of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, utf32_buf);

            add_undo_impl(undo_type::insert, data_head, len, mdata->line, mdata->index);
            auto[new_line, new_point] =
                g::buf_pt.buffer_ptr->apply_insert(data_head, len, mdata->line, mdata->index);
            g::buf_pt.curr_line = new_line;
            g::buf_pt.curr_idx = new_point;
        } break;

        case undo_type::remove_inplace:
        {
            LOG_WARN("Undoing remove _inplace_ of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, utf32_buf);

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

        u8 utf32_buf[1024];
        u32* data_head = reinterpret_cast<u32*>(head + sizeof(undo_metadata));
        auto[_, dest_reached] = utf32_to_utf8(data_head, data_head + len,
                                              utf32_buf, utf32_buf + 1024);
        *dest_reached = '\0';

        printf("BUFFER: op %d, at:%lu:%lu characters[%lu]: '%s'\n",
               reinterpret_cast<undo_metadata*>(head)->oper,
               reinterpret_cast<undo_metadata*>(head)->line,
               reinterpret_cast<undo_metadata*>(head)->index,
               len,
               utf32_buf[0] == '\n' ? reinterpret_cast<u8 const*>("EOL") : utf32_buf);
    }
}
