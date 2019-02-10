#ifndef UTF_H
#define UTF_H

// TODO: Describe endianees
// Converts until one buffer ends. Returns the pair of iterators at which the
// conversion has ended.
static inline std::pair<u8 const*, u32 const*>
utf8_to_utf32(u8 const* src_begin,
              u8 const* src_end,
              u32* dest_begin,
              u32* dest_end);

static inline std::pair<u32 const*, u8 const*>
utf32_to_utf8(u32 const* src_begin,
              u32 const* src_end,
              u8* dest_begin,
              u8* dest_end);

#endif // UTF_H
