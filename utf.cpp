#include <utility>
#include <functional>

// TODO: This is linuxish. On windows the header is named intrin.h
#include <emmintrin.h>
#include <immintrin.h>
#include <xmmintrin.h>

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

    return std::make_pair(str, fsize);
}

namespace detail
{

// TODO: Make sure this is correct.
static inline bool
validate_utf32(u32 cdpt)
{
    return ((cdpt <= 0xD7FF) || (cdpt >= 0xE000 && cdpt <= 0x10FFFF));
}

// There is a problem, with the incomplete/invalid sequence as this algorithm
// will look beyond alloced mem. This does not check the range and should be
// called when there are at least 3 bytes more allocated after src. If this is
// not true use decode_next_and_advance_safe.
static inline void
decode_next_and_advance(u8 const*& src, u32*& dest)
{
    u32 retval;
    if(*src <= 0x7F)
    {
        retval  = *src++;
    }
    else if ((*src & 0xE0) == 0xC0)
    {
        retval = (((*src & 0x1F) << 6)
                  | (*(src + 1) & 0x3F));
        src += 2;
    }
    else if ((*src & 0xF0) == 0xE0)
    {
        retval = (((*src & 0x0F) << 12)
                  | ((*(src + 1) & 0x3F) << 6)
                  | (*(src + 2) & 0x3F));
        src += 3;
    }
    else if ((*src & 0xF8) == 0xF0)
    {
        retval = (((*src & 0x07) << 18)
                  | ((*(src + 1) & 0x3F) << 12)
                  | ((*(src + 2) & 0x3F) << 6)
                  | (*(src + 3) & 0x3F));
        src += 4;
    }
    else
    {
        LOG_WARN("Invalid sequence starting with %02x", *src);

        // TODO: Insert different character than '?'
        *dest++ = static_cast<u32>('?');
        src++;

        return;
    }

    if(!validate_utf32(retval))
    {
        LOG_WARN("Invalid character. %08x is not utf32", retval);

        // TODO: Insert different character than '?'
        *dest++ = static_cast<u32>('?');
    }
    else
        *dest++ = retval;
}

// NOTE: This will check boundry for the src ptr (based on src_end) and it is
//       save when there is incomplete sequence at the end of the alloced mem.
static inline void
decode_next_and_advance_safe(u8 const*& src,
                             u8 const* src_end,
                             u32*& dest)
{
    u32 retval;
    if(*src <= 0x7F && src < src_end)
    {
        retval  = *src++;
    }
    else if ((*src & 0xE0) == 0xC0 && src + 1 < src_end)
    {
        retval = (((*src & 0x1F) << 6)
                  | (*(src + 1) & 0x3F));
        src += 2;
    }
    else if ((*src & 0xF0) == 0xE0 && src + 2 < src_end)
    {
        retval = (((*src & 0x0F) << 12)
                  | ((*(src + 1) & 0x3F) << 6)
                  | (*(src + 2) & 0x3F));
        src += 3;
    }
    else if ((*src & 0xF8) == 0xF0 && src + 3 < src_end)
    {
        retval = (((*src & 0x07) << 18)
                  | ((*(src + 1) & 0x3F) << 12)
                  | ((*(src + 2) & 0x3F) << 6)
                  | (*(src + 3) & 0x3F));
        src += 4;
    }
    else
    {
        LOG_WARN("Invalid sequence starting with %02x", *src);

        // TODO: Insert different character than '?'
        *dest++ = static_cast<u32>('?');
        src++;

        return;
    }

    if(!validate_utf32(retval))
    {
        LOG_WARN("Invalid character. %08x is not utf32", retval);

        // TODO: Insert different character than '?'
        *dest++ = static_cast<u32>('?');
    }
    else
        *dest++ = retval;
}

// NOTE: This assumes that src and dest are allocated at least for next 15 bytes
//       each.
static inline void
decode_next_and_advance_ascii_optimized(u8 const*& src, u32*& dest)
{
    if(*src <= 0x7F)
    {
        __m128i chunk, half, qrtr, zero;

        zero  = _mm_set1_epi8(0);
        chunk = _mm_loadu_si128(reinterpret_cast<__m128i const*>(src));

        half = _mm_unpacklo_epi8(chunk, zero);
        qrtr = _mm_unpacklo_epi16(half, zero);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest), qrtr);
        qrtr = _mm_unpackhi_epi16(half, zero);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest + 4), qrtr);

        half = _mm_unpackhi_epi8(chunk, zero);
        qrtr = _mm_unpacklo_epi16(half, zero);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest + 8), qrtr);
        qrtr = _mm_unpackhi_epi16(half, zero);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dest + 12), qrtr);

        i32 mask = _mm_movemask_epi8(chunk);
        i32 adv = (mask == 0 ? 16 : intr::ctz32(mask));

        src += adv;
        dest += adv;
    }
    else
    {
        decode_next_and_advance(src, dest);
    }
}
} // namespace detail

static inline std::pair<u8 const*, u32 const*>
utf8_to_utf32(u8 const* src_begin,
              u8 const* src_end,
              u32* dest_begin,
              u32* dest_end)
{
    while(src_begin + 16 <= src_end && dest_begin + 16 <= dest_end)
        detail::decode_next_and_advance_ascii_optimized(src_begin, dest_begin);

    while(src_begin + 4 <= src_end && dest_begin < dest_end)
        detail::decode_next_and_advance(src_begin, dest_begin);

    while(src_begin < src_end && dest_begin < dest_end)
        detail::decode_next_and_advance_safe(src_begin, src_end, dest_begin);

    return std::make_pair(src_begin, dest_begin);
}
