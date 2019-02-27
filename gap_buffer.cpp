#include "config.hpp"

#include <cstdlib>
#include <cstring>

#include "gap_buffer.hpp"

// TODO: Figure out if memcpy can be used instead of memmove. At least in some
//       places.

void gap_buffer::initialize()
{
    capacity = GAP_BUF_DEFAULT_CAPACITY;
    buffer = static_cast<u32*>(std::malloc(sizeof(u32) * capacity));

    if (!buffer)
        PANIC("Allocating memory failed.");

    gap_start = buffer;
    gap_end = buffer + capacity;
}

void gap_buffer::move_gap_to_point(mm point)
{
    ASSERT(point <= size());
    if (buffer + point == gap_start)
    {
    }
    else if (buffer + point < gap_start)
    {
        auto diff = gap_start - (buffer + point);
        memmove(gap_end - diff, buffer + point, sizeof(u32) * diff);
        gap_start -= diff;
        gap_end -= diff;
    }
    else if (buffer + point > gap_start)
    {
        auto diff = point - (gap_start - buffer);
        memmove(gap_start, gap_end, sizeof(u32) * diff);
        gap_start += diff;
        gap_end += diff;
    }
    else
        UNREACHABLE();
}

void gap_buffer::move_gap_to_buffer_end()
{
    if (gap_end == buffer + capacity)
        LOG_WARN("Gap already at the end.");
    else
    {
        auto move_from_the_end = (buffer + capacity) - gap_end;
        memmove(gap_start, gap_end, sizeof(u32) * move_from_the_end);
        gap_start += move_from_the_end;
        gap_end += move_from_the_end;
    }
}

void gap_buffer::reserve_gap(mm n)
{
    LOG_INFO("Reserve: %ld", n);

    if (gap_size() < n)
    {
        move_gap_to_buffer_end();
        LOG_DEBUG("Doing realloc.");

        mm gap_start_idx = gap_start - buffer;
        mm new_gap_size = n - gap_size();
        mm new_size = capacity + new_gap_size;

#if 0
        // TODO: realloc.
        auto new_ptr = static_cast<u32*>(malloc(sizeof(u32) * new_size));
        memmove(new_ptr, buffer, sizeof(u32) * capacity);
        free(buffer);

        if (new_ptr)
            buffer = new_ptr;
        else
            PANIC("Realloc has failed.");
#else
        buffer = static_cast<u32*>(realloc(buffer, sizeof(u32) * new_size));
#endif

        capacity = new_size;
        gap_start = buffer + gap_start_idx;
        gap_end = buffer + capacity;

        LOG_WARN("Realloced the gap. New size: %ld", gap_size());
    }
    else
        LOG_INFO("Gap untouched, still: %ld", gap_size());
}

void gap_buffer::insert(mm point, u32 character)
{
    ASSERT(point <= size());
    move_gap_to_point(point);

    LOG_INFO("Inserting 1 character");

    // If after the insert the gap would be less that we permit, realloc.
    if (gap_size() - 1 <= GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND)
        reserve_gap(gap_size() + GAP_BUF_SIZE_AFTER_REALLOC + 1);
    *gap_start++ = character;

    ASSERT(gap_size() > GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND);
}

void gap_buffer::insert_range(mm point, u32 const* begin, u32 const* end)
{
    ASSERT(point <= size());
    move_gap_to_point(point);

    LOG_INFO("Inserting range of %ld characters", end - begin);

    mm check = gap_size() - (end - begin + 1);
    (void(check));

    // If after the insert the gap would be smaller then we permit, realloc.
    if (static_cast<mm>(gap_size()) - (end - begin + 1) <= GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND)
        reserve_gap(gap_size() + GAP_BUF_SIZE_AFTER_REALLOC + (end - begin + 1));

    while(begin != end)
    {
        *gap_start++ = *begin++;
        ASSERT(gap_size() > GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND);
    }

    ASSERT(gap_size() > GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND);
}

void gap_buffer::replace_at_point(mm point, u32 character)
{
    ASSERT(point < size());


    auto ptr = (point < (gap_start - buffer)
                ? buffer + point
                : gap_end + (point - (gap_start - buffer)));

    *ptr = character;
}

bool gap_buffer::delete_char_backward(mm point)
{
    ASSERT(0 < point && point <= size());

    auto curr_point = buffer + point;
    if (curr_point != buffer)
    {
        if (curr_point != gap_start)
            move_gap_to_point(point);

        gap_start--;

        // TODO(): This is the only place that the memory can be shrinked, if we
        //         want to shrink it on the deleteion which im not sure if this
        //         is good idea.
        return true;
    }
    else
        return false;
}

bool gap_buffer::delete_char_forward(mm point)
{
    ASSERT(point >= 0);

    if(point == size())
        return false;

    delete_char_backward(point + 1);
    return true;
}

bool gap_buffer::delete_to_the_end_of_line(mm point)
{
    ASSERT(point >= 0);
    ASSERT(point <= size());

    move_gap_to_point(point);
    gap_end = buffer + capacity;

    return true;
}

mm gap_buffer::size() const
{
    return capacity - gap_size();
}

mm gap_buffer::gap_size() const
{
    return (gap_end - gap_start);
}

u32 gap_buffer::get(mm idx) const
{
    ASSERT(idx >= 0);

    if (static_cast<mm>(idx) < (gap_start - buffer))
        return *(buffer + idx);
    else
        return *(gap_end + (idx - (gap_start - buffer)));
}

u32 gap_buffer::operator [](mm idx) const
{
    ASSERT(idx >= 0);
    return get(idx);
}

#if 0
char* gap_buffer::to_c_str() const
{
#ifdef GAP_BUF_SSO
    if(sso_enabled())
    {
        auto result = static_cast<char*>(malloc(sizeof(u32) * GAP_BUF_SSO_CAP));
        memmove(result, data, GAP_BUF_SSO_GAP_START);
        memmove(result + GAP_BUF_SSO_GAP_START,
               data + GAP_BUF_SSO_GAP_END,
               GAP_BUF_SSO_CAP - GAP_BUF_SSO_GAP_END);

        result[GAP_BUF_SSO_GAP_START + (GAP_BUF_SSO_CAP - GAP_BUF_SSO_GAP_END)] = 0;
        return result;
    }
    else
    {
#endif
        // TODO: Alloc function.
        auto result = static_cast<char*>(std::malloc(sizeof(u32) * (capacity + 1)));
        auto resutl_idx = 0;
        auto in_gap = false;
        result[0] = '\0';

        for (auto i = 0_u64; i < capacity; ++i)
        {
            if (buffer + i == gap_start)
                in_gap = true;

            if (buffer + i == gap_end)
                in_gap = false;

            if (!in_gap)
            {
                result[resutl_idx++] = *(buffer + i);
                result[resutl_idx] = '\0';
            }
        }

        return result;
#ifdef GAP_BUF_SSO
    }
#endif
}
#endif

void gap_buffer::to_str_refs(strref* refs) const
{
    refs[0] = strref{ const_cast<u32*>(buffer),
                      const_cast<u32*>(gap_start) };
    refs[1] = strref{ const_cast<u32*>(gap_end),
                      const_cast<u32*>(buffer + capacity) };
}

void gap_buffer::DEBUG_print_state() const
{
    char print_buffer[capacity + 5];
    auto print_buffer_idx = 0LLU;
    auto in_gap = false;
    auto printed_chars = 0LLU;

    auto ptr = buffer;
    for (auto i = 0ULL; i < capacity; ++i)
    {
        if (ptr == gap_start)
            in_gap = true;

        if (ptr == gap_end)
            in_gap = false;

        print_buffer[print_buffer_idx++] = (in_gap ? '?' : static_cast<u8>(*ptr));
        printed_chars++;

        ptr++;
    }

    ASSERT(print_buffer_idx <= capacity); // TODO: Or < ????
    for (auto i = print_buffer_idx; i < capacity; ++i)
        print_buffer[print_buffer_idx++] = '_';
    print_buffer[print_buffer_idx++] = '\n';
    print_buffer[print_buffer_idx++] = '\0';
    printf("%s", print_buffer);

    print_buffer_idx = 0;
    ptr = buffer;
    for (auto i = 0ULL; i <= capacity; ++i)
    {
        if (ptr == gap_start)
            print_buffer[print_buffer_idx++] = 'b';
        else if (ptr == gap_end)
            print_buffer[print_buffer_idx++] = 'e';
        else
            print_buffer[print_buffer_idx++] = ' ';
        ptr++;
    }
    print_buffer[print_buffer_idx++] = '\n';
    print_buffer[print_buffer_idx++] = '\0';
    printf("%s", print_buffer);
}

bool gap_buffer::iterator::operator==(gap_buffer::iterator const& other)
{
    return curr == other.curr;
}

bool gap_buffer::iterator::operator!=(gap_buffer::iterator const& other)
{
    return curr != other.curr;
}

void gap_buffer::iterator::operator++()
{
    curr++;
    if(curr == gapb->gap_start)
        curr = gapb->gap_end;
}

u32 gap_buffer::iterator::operator*() const
{
    return *curr;
}

gap_buffer::iterator gap_buffer::begin()
{
    return iterator { this, buffer == gap_start ? gap_end : buffer };
}

gap_buffer::iterator gap_buffer::end()
{
    return iterator { this, buffer + capacity };
}

static void move_gap_bufffer(gap_buffer* from, gap_buffer* to)
{
    auto gap_start_offset = from->gap_start - from->buffer;
    auto gap_end_offset = from->gap_end - from->buffer;

    if(to->buffer)
        free(to->buffer);

    to->capacity = from->capacity;
    to->buffer = from->buffer;
    to->gap_start = (to->buffer + gap_start_offset);
    to->gap_end = (to->buffer + gap_end_offset);

    from->capacity = 0;
    from->buffer = nullptr;
    from->gap_start = nullptr;
    from->gap_end = nullptr;
}
