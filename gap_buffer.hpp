// TODO:
//       * Don't touch the gap, when buffer is allocated.
//       * fancy up the api for inserting sequences, etc.

#ifndef GAP_BUFFER_HPP
#define GAP_BUFFER_HPP

#include "strref.hpp"

/// Initial size of the gap after initialization.
#define GAP_BUF_DEFAULT_CAPACITY (80)

/// Minimal size of the buffer before it gets expanded.
#define GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND (2)

/// The size of a gap after expanding the memory.
#define GAP_BUF_SIZE_AFTER_REALLOC (64)

/// Max size of the gap before shrinking.
#define GAP_BUF_MAX_SIZE_BEFORE_MEM_SHRINK (128)


struct gap_buffer
{
    /// Allocated capacity of the buffer.
    umm capacity;

    /// Pointer to the first character that is not in the gap. Character
    /// pointer by this pointer is not in the structure and is not
    /// defined.
    u32* gap_start;

    /// Pointer to the first vaild character that is outside of the
    /// gap. If the gap is at the end of a buffer, this poitner poitns
    /// outside to the allocated structure, and should not be
    /// dereferenced.
    u32* gap_end;

    /// The content of the text buffer.
    u32* buffer;

    /// Must be called before any action with this data structure is done.
    void initialize();

    /// Move tha gap so that gap_start is at the point. Called by inserting
    /// character if the gap is at the different place by the start of the
    /// operation. Assertion: gap_start == buffer + point after this operation.
    void move_gap_to_point(mm point);

    /// Place the gap and the end of tha buffer.
    void move_gap_to_buffer_end();

    /// Set the gap size to 'n' characters if is is smaller. Does nothing if the
    /// gap size is already greater or equal 'n'.
    void reserve_gap(mm n);

    /// Insert character at point. Will move the gap to the pointed location if
    /// necesarry. Can exapnd buffer memory.
    void insert(mm point, u32 character);

    /// Insert character at point. Will move the gap to the pointed location if
    /// necesarry. Can exapnd buffer memory.
    void insert_range(mm point, u32 const* begin, u32 const* end);

    /// Insert character at point. This doesn't move a gap or expand memory.
    void replace_at_point(mm point, u32 character);

    /// Delete the character currently on the given point. Will move the gap to
    /// the pointed location if necesarry. Can shrink buffer memory.
    bool delete_char_backward(mm point);

    /// Delete the character currently on the given point. Should give the same
    /// result as moving forward one character and removing character backward.
    /// Will move the gap to the pointed location if necesarry. Can shrink
    /// buffer memory.
    bool delete_char_forward(mm point);

    /// Efficiently clears the text to the end of the line by moving a gap.
    bool delete_to_the_end_of_line(mm point);

    /// Returns the number of characters stored in the buffer. Assertion: size()
    /// + gap_size() == capacity for every vaild state of gap buffer.
    mm size() const;

    /// Returns a size of the gap. Assertion: size() + gap_size() == capacity,
    /// for every vaild state of gap buffer.
    mm gap_size() const;

    /// Return the idx'th character in the buffer.
    u32 get(mm idx) const;

    /// Operator alias for the 'get' function. Returns the idx'th character in
    /// the buffer.
    u32 operator[](mm idx) const;

    /// Returns the c_str representation of the line. Allocates the memory for
    /// the string. The caller is repsonsible for freeing this memory.
    char* to_c_str() const;

    /// Returns two strrefs representing the stored string. Refs must point to
    /// the array of size at least 2.
    void to_str_refs(strref* refs) const;

    /// Pretty prints the structure state to console.
    void DEBUG_print_state() const;

    struct iterator
    {
        gap_buffer* gapb;
        u32* curr;

        bool operator==(const iterator&);
        bool operator!=(const iterator&);
        void operator++();
        u32 operator*() const;
    };

    iterator begin();
    iterator end();
};

/// The position from where the buffer has been moved is undefined.
static void move_gap_bufffer(gap_buffer* from, gap_buffer* to);

#endif // GAP_BUFFER_HPP
