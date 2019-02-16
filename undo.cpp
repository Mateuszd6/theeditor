#include "undo.hpp"

namespace g
{
    // NOTE: The buffer structure looks like this:
    // LEN is the size of DATA on the left.
    //
    // [[METADATA][DATA][LEN]][[METADATA][DATA][LEN]][[METADATA][DATA][LEN]]
    //                                               ^
    // In order to move back (assuming not already at the beginning), do:
    // * Move back sizeof(LEN) to get the size of the data (remember it).
    // * Then skip the data + sizeof(METADATA)
    //
    // Result:
    // [[METADATA][DATA][LEN]][[METADATA][DATA][LEN]][[METADATA][DATA][LEN]]
    //                        ^
    static u8 undo_buffer[1024 * 1024];
    static u8* undo_head = &(undo_buffer[0]);
    static u8* redo_head = &(undo_buffer[0]);
}

static void break_undo_chain()
{
    g::redo_head = g::undo_head;
}

static void
add_undo_impl(undo_type type, u32 const* data, mm len, u64 line, u64 index)
{
    u8* new_head = g::undo_head;

    *(r_cast<undo_metadata*>(new_head)) = undo_metadata { type, line, index };
    new_head += sizeof(undo_metadata);

    u32* data_ptr = r_cast<u32*>(new_head);
    for(u32 const* p = data; p != data + len;)
        *data_ptr++ = *p++;

    new_head += (sizeof(u32) * (len % 2 == 1 ? len + 1 : len));
    *(r_cast<mm*>(new_head)) = len;
    new_head += sizeof(len);

    g::undo_head = new_head;
}

static void
add_undo(undo_type type, u32 const* data, mm len, u64 line, u64 index)
{
    add_undo_impl(type, data, len, line, index);
    break_undo_chain();
}

static void
apply_remove(u32* data_head, mm len, u64 line, u64 index)
{
    // TODO: This should reset last_line_idx. Make a safe ctor.
    g::buf_pt.curr_line = line;
    g::buf_pt.curr_idx = index;

    for(int i = 0; i < len; ++i)
    {
        ASSERT(g::buf_pt.point_is_valid());

        if(data_head[i] == '\n')
        {
            // Assert that we are at the end of the line.
            ASSERT(g::buf_pt.buffer_ptr
                   ->get_line(g::buf_pt.curr_line)
                   ->size() == g::buf_pt.curr_idx);

            bool line_removed = g::buf_pt.remove_character_forward();
            ASSERT(line_removed);
        }
        else
        {
            ASSERT(g::buf_pt.buffer_ptr
                   ->get_line(g::buf_pt.curr_line)
                   ->get(g::buf_pt.curr_idx) == data_head[i]);

            bool removed = g::buf_pt.remove_character_forward();
            ASSERT(removed);
        }
    }
}

static void
apply_insert(u32* data_head, mm len, u64 line, u64 index)
{
    // TODO: This should reset last_line_idx. Make a safe ctor.
    g::buf_pt.curr_line = line;
    g::buf_pt.curr_idx = index;

    for(int i = 0; i < len; ++i)
    {
        ASSERT(g::buf_pt.point_is_valid());

        if(data_head[i] == static_cast<u32>('\n'))
        {
            bool line_added = g::buf_pt.insert_newline_at_point();
            ASSERT(line_added);
        }
        else
        {
            bool inserted = g::buf_pt.insert_character_at_point(data_head[i]);
            ASSERT(inserted);
        }
    }
}

static void
undo()
{
    if (g::redo_head == &(g::undo_buffer[0]))
    {
        LOG_ERROR("No more undo information.");
        return;
    }

    u8* head = g::redo_head;

    mm len = *(r_cast<mm*>(head - sizeof(mm)));
    head -= (len % 2 == 1 ? len + 1 : len) * sizeof(u32) + sizeof(mm) + sizeof(undo_metadata);
    u32* data_head = r_cast<u32*>(head + sizeof(undo_metadata));
    undo_metadata* mdata = r_cast<undo_metadata*>(head);

    u8 buffer[1024];
    auto[_, dest_reached] = utf32_to_utf8(data_head, data_head + len, buffer, buffer + 1024);
    *dest_reached = '\0';

    switch (mdata->oper)
    {
        case undo_type::insert:
        {
            LOG_WARN("Undoing insertion of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, buffer);

            add_undo_impl(undo_type::remove, data_head, len, mdata->line, mdata->index);
            apply_remove(data_head, len, mdata->line, mdata->index);
        } break;

        case undo_type::insert_inplace:
        {
            LOG_WARN("Undoing insertion _inplace_ of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, buffer);

            add_undo_impl(undo_type::remove_inplace, data_head, len, mdata->line, mdata->index);
            apply_remove(data_head, len, mdata->line, mdata->index);
        } break;

        case undo_type::remove:
        {
            LOG_WARN("Undoing remove of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, buffer);

            add_undo_impl(undo_type::insert, data_head, len, mdata->line, mdata->index);
            apply_insert(data_head, len, mdata->line, mdata->index);
        } break;

        case undo_type::remove_inplace:
        {
            LOG_WARN("Undoing remove _inplace_ of [%ld] characters at %lu:%lu: %s",
                     len, mdata->line, mdata->index, buffer);

            add_undo_impl(undo_type::insert_inplace, data_head, len, mdata->line, mdata->index);
            apply_insert(data_head, len, mdata->line, mdata->index);

            for(int i = 0; i < len; ++i)
            {
                bool moved_left = g::buf_pt.character_left();
                ASSERT(moved_left);
            }
        } break;
    }

    g::redo_head = head;
}

static inline void
DEBUG_print_state()
{
    u8* head = g::undo_head;
    while(head != &g::undo_buffer[0])
    {
        mm len = *(r_cast<mm*>(head - sizeof(mm)));
        head -= (len % 2 == 1 ? len + 1 : len) * sizeof(u32) + sizeof(mm) + sizeof(undo_metadata);

        u8 buffer[1024];
        u32* data_head = r_cast<u32*>(head + sizeof(undo_metadata));
        auto[_, dest_reached] = utf32_to_utf8(data_head, data_head + len,
                                              buffer, buffer + 1024);
        *dest_reached = '\0';

        printf("BUFFER: op %d, at:%lu:%lu characters[%lu]: '%s'\n",
               r_cast<undo_metadata*>(head)->oper,
               r_cast<undo_metadata*>(head)->line,
               r_cast<undo_metadata*>(head)->index,
               len,
               buffer);
    }
}