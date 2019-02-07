// TODO:
//       * Don't touch the gap, when buffer is allocated.
//       * Unicode
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


// NOTE: There is a 8-bit padding to the gap_buffer so that more strigs get
//       SSO'ed.

#define GAP_BUF_SSO
#define GAP_BUF_SSO_ADDITIONAL_SPACE (16)
#define GAP_BUF_SSO_CAP (30 + GAP_BUF_SSO_ADDITIONAL_SPACE)
#define GAP_BUF_SSO_ENABLED_BYTE (31 + GAP_BUF_SSO_ADDITIONAL_SPACE)
#define GAP_BUF_SSO_GAP_START (data[30 + GAP_BUF_SSO_ADDITIONAL_SPACE])
#define GAP_BUF_SSO_GAP_END (data[31 + GAP_BUF_SSO_ADDITIONAL_SPACE])

struct gap_buffer
{
#ifdef GAP_BUF_SSO
    union
    {
        u8 data[32 + GAP_BUF_SSO_ADDITIONAL_SPACE];
        struct
        {
#endif
            u8 padding[GAP_BUF_SSO_ADDITIONAL_SPACE];

            /// Allocated capacity of the buffer.
            size_t capacity;

            /// Pointer to the first character that is not in the gap. Character
            /// pointer by this pointer is not in the structure and is not
            /// defined.
            u8* gap_start;

            /// Pointer to the first vaild character that is outside of the
            /// gap. If the gap is at the end of a buffer, this poitner poitns
            /// outside to the allocated structure, and should not be
            /// dereferenced.
            u8* gap_end;

            /// The content of the text buffer.
            u8* buffer;
#ifdef GAP_BUF_SSO
        };
    };
#endif

#ifdef GAP_BUF_SSO

    bool sso_enabled() const;

    void sso_move_from_sso_to_full();

    size_t sso_gap_size() const;

    size_t sso_size() const;

#endif

    /// Must be called before any action with this data structure is done.
    void initialize();

    /// Move tha gap so that gap_start is at the point. Called by inserting
    /// character if the gap is at the different place by the start of the
    /// operation. Assertion: gap_start == buffer + point after this operation.
    void move_gap_to_point(size_t point);

    /// Place the gap and the end of tha buffer.
    /// NOTE: For now it assumes that buffer is not sso'ed.
    void move_gap_to_buffer_end();

    /// Set the gap size to 'n' characters if is is smaller. Does nothing if the
    /// gap size is already greater or equal 'n'. It assumes that buffer is not
    /// sso'ed.
    void reserve_gap(size_t n);

    /// Insert character at point. Will move the gap to the pointed location if
    /// necesarry. Can exapnd buffer memory.
    void insert_at_point(size_t point, u8 character); // LATIN2 characters only.

#if 0 // TODO(PROFILING): Make this fucntion and use it when it comes to profiling.
    /// Insert character at point. Will move the gap to the pointed location if
    /// necesarry. Can exapnd buffer memory.
    void insert_sequence_at_point(size_t point, misc::length_buffer sequence); // LATIN2 characters only.
#endif

    /// Insert character at point. This doesn't move a gap or expand memory.
    void replace_at_point(size_t point, u8 character);

    /// Delete the character currently on the given point. Will move the gap to
    /// the pointed location if necesarry. Can shrink buffer memory.
    bool delete_char_backward(size_t point);

    /// Delete the character currently on the given point. Should give the same
    /// result as moving forward one character and removing character backward.
    /// Will move the gap to the pointed location if necesarry. Can shrink
    /// buffer memory.
    bool delete_char_forward(size_t point);

    /// Efficiently clears the text to the end of the line by moving a gap.
    bool delete_to_the_end_of_line(size_t point);

    /// Returns the number of characters stored in the buffer. Assertion: size()
    /// + gap_size() == capacity for every vaild state of gap buffer.
    size_t size() const;

    /// Returns a size of the gap. Assertion: size() + gap_size() == capacity,
    /// for every vaild state of gap buffer.
    size_t gap_size() const;

    /// Return the idx'th character in the buffer.
    u8 get(size_t idx) const;

    /// Operator alias for the 'get' function. Returns the idx'th character in
    /// the buffer.
    u8 operator[](size_t idx) const;

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
        u8* curr;

        bool operator==(const iterator&);
        bool operator!=(const iterator&);
        void operator++();
        u8 operator*() const;
    };

    iterator begin();
    iterator end();
};

/// The position from where the buffer has been moved is undefined.
static void move_gap_bufffer(gap_buffer* from, gap_buffer* to);

#endif // GAP_BUFFER_HPP
