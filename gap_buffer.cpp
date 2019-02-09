#include "config.hpp"

#include <cstdlib>
#include <cstring>

#include "gap_buffer.hpp"

void gap_buffer::initialize()
{
#ifdef GAP_BUF_SSO
    GAP_BUF_SSO_GAP_START = 0;
    GAP_BUF_SSO_GAP_END = GAP_BUF_SSO_CAP;
#else
    capacity = GAP_BUF_DEFAULT_CAPACITY;
    buffer = static_cast<u32*>(std::malloc(sizeof(u32) * capacity));

    if (!buffer)
        PANIC("Allocating memory failed.");

    gap_start = buffer;
    gap_end = buffer + capacity;
#endif
}

void gap_buffer::move_gap_to_point(size_t point)
{
#ifdef GAP_BUF_SSO
    if(sso_enabled())
    {
        ASSERT(point <= size());
        if (point == GAP_BUF_SSO_GAP_START)
        {
        }
        else if (point < GAP_BUF_SSO_GAP_START)
        {
            auto diff = GAP_BUF_SSO_GAP_START - point;
            memcpy(data + GAP_BUF_SSO_GAP_END - diff, data + point, sizeof(u32) * diff);
            GAP_BUF_SSO_GAP_START -= diff;
            GAP_BUF_SSO_GAP_END -= diff;
        }
        else if (point > GAP_BUF_SSO_GAP_START)
        {
            auto diff = point - GAP_BUF_SSO_GAP_START;
            memcpy(data + GAP_BUF_SSO_GAP_START, data + GAP_BUF_SSO_GAP_END, sizeof(u32) * diff);
            GAP_BUF_SSO_GAP_START += diff;
            GAP_BUF_SSO_GAP_END += diff;
        }
        else
            UNREACHABLE();
    }
    else
    {
#endif
        ASSERT(point <= size());
        if (buffer + point == gap_start)
        {
        }
        else if (buffer + point < gap_start)
        {
            auto diff = gap_start - (buffer + point);
            memcpy(gap_end - diff, buffer + point, sizeof(u32) * diff);
            gap_start -= diff;
            gap_end -= diff;
        }
        else if (buffer + point > gap_start)
        {
            auto diff = point - (gap_start - buffer);
            memcpy(gap_start, gap_end, sizeof(u32) * diff);
            gap_start += diff;
            gap_end += diff;
        }
        else
            UNREACHABLE();
#ifdef GAP_BUF_SSO
    }
#endif
}

void gap_buffer::move_gap_to_buffer_end()
{
#ifdef GAP_BUF_SSO
    ASSERT(!sso_enabled());
#endif
    if (gap_end == buffer + capacity)
        LOG_WARN("Gap already at the end.");
    else
    {
        auto move_from_the_end = (buffer + capacity) - gap_end;
        memcpy(gap_start, gap_end, sizeof(u32) * move_from_the_end);
        gap_start += move_from_the_end;
        gap_end += move_from_the_end;
    }
}

void gap_buffer::reserve_gap(size_t n)
{
    // ASSERT(!sso_enabled());

#ifdef GAP_BUF_SSO
    if(sso_enabled())
    {
        if(gap_size() < n)
        {
            if(size() + n  >= GAP_BUF_SSO_CAP)
            {
                sso_move_from_sso_to_full();
            }
            else
            {
                // TODO?
            }
        }
    }
    else
    {
#endif
        if (gap_size() < n)
        {
            move_gap_to_buffer_end();
            LOG_DEBUG("Doing realloc.");

            auto gap_start_idx = gap_start - buffer;
            auto new_gap_size = n - gap_size();
            auto new_size = capacity + new_gap_size;

            // TODO(Testing): Test it with custom realloc that always moves the memory.
            auto new_ptr = static_cast<u32*>(malloc(sizeof(u32) * new_size));
            memcpy(new_ptr, buffer, sizeof(u32) * capacity);
            free(buffer);

            if (new_ptr)
                buffer = new_ptr;
            else
                PANIC("Realloc has failed.");

            capacity = new_size;
            gap_start = buffer + gap_start_idx;
            gap_end = buffer + capacity;
        }
#ifdef GAP_BUF_SSO
    }
#endif
}

// TODO(Testing): Battle-test this. Watch out for allocations!
void gap_buffer::insert_at_point(size_t point, u32 character) // LATIN2 characters only.
{
    ASSERT(point <= size());

#ifdef GAP_BUF_SSO
    if(sso_enabled())
    {
        move_gap_to_point(point);

        data[GAP_BUF_SSO_GAP_START] = character;
        GAP_BUF_SSO_GAP_START++;

        // Expand memory if needed:
        if (sso_gap_size() <= 4)
        {
            sso_move_from_sso_to_full();
            LOG_WARN("Full gap buffer allocated.");
        }
    }
    else
    {
#endif
        move_gap_to_point(point);

        *gap_start++ = character;

        // Expand memory if needed:
        if (gap_size() <= GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND)
        {
            reserve_gap(GAP_BUF_SIZE_AFTER_REALLOC);
            LOG_WARN("Gap size after reallocing: %ld", gap_size());
        }

        ASSERT(gap_size() > GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND);
#ifdef GAP_BUF_SSO
    }
#endif
}

#if 0
void gap_buffer::insert_sequence_at_point(size_t point, misc::length_buffer sequence) // LATIN2 characters only
{
    reserve_gap(size() + sequence.length);
    for(auto i = 0_u64; i < sequence.length; ++i)
        insert_at_point(point + i, sequence.data[i]);
}
#endif

void gap_buffer::replace_at_point(size_t point, u32 character)
{
    ASSERT(point < size());

#ifdef GAP_BUF_SSO
    if(sso_enabled())
    {
        auto idx = (static_cast<ssize_t>(point) < (gap_start - buffer)
                    ? point
                    : GAP_BUF_SSO_GAP_END + (point - GAP_BUF_SSO_GAP_START));

        data[idx] = character;
    }
    else
    {
#endif
        auto ptr = (static_cast<ssize_t>(point) < (gap_start - buffer)
                        ? buffer + point
                        : gap_end + (point - (gap_start - buffer)));

        *ptr = character;
#ifdef GAP_BUF_SSO
    }
#endif
}

bool gap_buffer::delete_char_backward(size_t point)
{
    ASSERT(0 < point && point <= size());

#ifdef GAP_BUF_SSO
    if(sso_enabled())
    {
        if(point != 0)
        {
            if(point != GAP_BUF_SSO_GAP_START)
                move_gap_to_point(point);
            GAP_BUF_SSO_GAP_START--;

            return true;
        }
        else
            return false;
    }
    else
    {
#endif
        auto curr_point = buffer + point;
        if (curr_point != buffer)
        {
            if (curr_point != gap_start)
                move_gap_to_point(point);

            gap_start--;

            // TODO(#3): This is the only place that the memory can be shrinked, if
            //           we want to shrink it on the deleteion which im not sure if
            //           this is good idea.
            return true;
        }
        else
            return false;
#ifdef GAP_BUF_SSO
    }
#endif
}

bool gap_buffer::delete_char_forward(size_t point)
{
    if(point == size())
        return false;

    delete_char_backward(point + 1);
    return true;
}

bool gap_buffer::delete_to_the_end_of_line(size_t point)
{
    ASSERT(point <= size());

#ifdef GAP_BUF_SSO
    if(sso_enabled())
    {
        move_gap_to_point(point);
        GAP_BUF_SSO_GAP_END = GAP_BUF_SSO_CAP;
    }
    else
    {
#endif
        move_gap_to_point(point);
        gap_end = buffer + capacity;
#ifdef GAP_BUF_SSO
    }
#endif

    return true;
}

size_t gap_buffer::size() const
{
#ifdef GAP_BUF_SSO
    if(sso_enabled())
    {
        return sso_size();
    }
    else
    {
#endif
        return capacity - gap_size();
#ifdef GAP_BUF_SSO
    }
#endif
}

size_t gap_buffer::gap_size() const
{
#ifdef GAP_BUF_SSO
    if(sso_enabled())
    {
        return sso_gap_size();
    }
    else
    {
#endif
        return (gap_end - gap_start);
#ifdef GAP_BUF_SSO
    }
#endif
}

u32 gap_buffer::get(size_t idx) const
{
#ifdef GAP_BUF_SSO
    if(sso_enabled())
    {
        if (static_cast<ssize_t>(idx) < GAP_BUF_SSO_GAP_START)
            return data[idx];
        else
            return data[GAP_BUF_SSO_GAP_END + (idx - GAP_BUF_SSO_GAP_START)];
    }
    else
    {
#endif
        if (static_cast<ssize_t>(idx) < (gap_start - buffer))
            return *(buffer + idx);
        else
            return *(gap_end + (idx - (gap_start - buffer)));
#ifdef GAP_BUF_SSO
    }
#endif
}

u32 gap_buffer::operator [](size_t idx) const
{
    return get(idx);
}

#if 0
char* gap_buffer::to_c_str() const
{
#ifdef GAP_BUF_SSO
    if(sso_enabled())
    {
        auto result = static_cast<char*>(malloc(sizeof(u32) * GAP_BUF_SSO_CAP));
        memcpy(result, data, GAP_BUF_SSO_GAP_START);
        memcpy(result + GAP_BUF_SSO_GAP_START,
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
#ifdef GAP_BUF_SSO
    if(sso_enabled())
    {
        refs[0] = strref{ c_cast<u32*>(data),
                          c_cast<u32*>(data + GAP_BUF_SSO_GAP_START) };
        refs[1] = strref{ c_cast<u32*>(&data[GAP_BUF_SSO_GAP_END]),
                          c_cast<u32*>(data + GAP_BUF_SSO_CAP) };
    }
    else
#endif
    {
        refs[0] = strref{ c_cast<u32*>(buffer),
                          c_cast<u32*>(gap_start) };
        refs[1] = strref{ c_cast<u32*>(gap_end),
                          c_cast<u32*>(buffer + capacity) };
    }
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

#ifdef GAP_BUF_SSO

bool gap_buffer::sso_enabled() const
{
    return data[GAP_BUF_SSO_ENABLED_BYTE] != 0;
}

/// When SOO is not enougth.
// TODO: This can be further optimized, because we are losing the gap on copy,
// which can be done correctly with not-so-hard math.
void gap_buffer::sso_move_from_sso_to_full()
{
    u32 data_copied[GAP_BUF_SSO_CAP];
    u32 len = GAP_BUF_SSO_GAP_START + (GAP_BUF_SSO_CAP - GAP_BUF_SSO_GAP_END);

    memcpy(data_copied, data, GAP_BUF_SSO_GAP_START);
    memcpy(data_copied + GAP_BUF_SSO_GAP_START,
           data + GAP_BUF_SSO_GAP_END,
           GAP_BUF_SSO_CAP - GAP_BUF_SSO_GAP_END);

    // LOG_WARN("Copied string is %s (len = %d)", data_copied, len);

    capacity = GAP_BUF_DEFAULT_CAPACITY;
    buffer = static_cast<u32*>(std::malloc(sizeof(u32) * capacity));

    if (!buffer)
        PANIC("Allocating memory failed.");

    memcpy(buffer, data_copied, len);
    gap_start = buffer + len;
    gap_end = buffer + capacity;
}

// TODO: Make sure nothing calls it.
size_t gap_buffer::sso_gap_size() const
{
    return GAP_BUF_SSO_GAP_END - GAP_BUF_SSO_GAP_START;
}

// TODO: Make sure nothing calls it.
size_t gap_buffer::sso_size() const
{
    return GAP_BUF_SSO_CAP - sso_gap_size();
}

#endif

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
#ifdef GAP_BUF_SSO
    if(from->sso_enabled())
    {
        memcpy(to, reinterpret_cast<u32*>(from), sizeof(gap_buffer));
    }
    else
    {
#endif
        auto gap_start_offset = from->gap_start - from->buffer;
        auto gap_end_offset = from->gap_end - from->buffer;

        to->capacity = from->capacity;
        to->buffer = from->buffer;
        to->gap_start = (to->buffer + gap_start_offset);
        to->gap_end = (to->buffer + gap_end_offset);

        from->capacity = 0;
        from->buffer = nullptr;
        from->gap_start = nullptr;
        from->gap_end = nullptr;
#ifdef GAP_BUF_SSO
    }
#endif
}
