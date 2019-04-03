#include <utility>
#include <functional>

// TODO: This is linuxish. On windows the header is named intrin.h
#include <emmintrin.h>
#include <immintrin.h>
#include <xmmintrin.h>

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
// not true use utf8_to_utf32_dec_advc_safe.
static inline void
utf8_to_utf32_dec_advc(u8 const*& src, u32*& dest)
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
//       safe when there is incomplete sequence at the end of the alloced mem.
static inline bool
utf8_to_utf32_dec_advc_safe(u8 const*& src,
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
        return false;
    }

    if(!validate_utf32(retval))
    {
        LOG_WARN("Invalid character. %08x is not utf32", retval);

        // TODO: Insert different character than '?'
        *dest++ = static_cast<u32>('?');
    }
    else
        *dest++ = retval;

    return true;
}

// NOTE: This assumes that src and dest are allocated at least for next 15 bytes
//       each.
static inline void
utf8_to_utf32_dec_advc_ascii_optimized(u8 const*& src, u32*& dest)
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
        utf8_to_utf32_dec_advc(src, dest);
    }
}

// This will not validate the utf32 codepoint and assumes that the sequence is
// valid. This varion will also not check the dest boundries and assume that at
// least 3 bytes after dest are still in the same allocation. If this is not
// true, use _safe version.
static inline void
utf32_to_utf8_dec_advc(u32 const*& src, u8*& dest)
{
    u32 cdpt = *src++;
    if (cdpt <= 0x7F)
    {
        *dest++ = static_cast<u8>(cdpt);
    }
    else if (cdpt <= 0x7FF)
    {
        *dest++ = static_cast<u8>(0xC0 | ((cdpt >> 6) & 0x1F));
        *dest++ = static_cast<u8>(0x80 | (cdpt & 0x3F));
    }
    else if (cdpt <= 0xFFFF)
    {
        *dest++ = static_cast<u8>(0xE0 | ((cdpt >> 12) & 0x0F));
        *dest++ = static_cast<u8>(0x80 | ((cdpt >> 6) & 0x3F));
        *dest++ = static_cast<u8>(0x80 | (cdpt & 0x3F));
    }
    else if (cdpt <= 0x10FFFF)
    {
        *dest++ = static_cast<u8>(0xF0 | ((cdpt >> 18) & 0x07));
        *dest++ = static_cast<u8>(0x80 | ((cdpt >> 12) & 0x3F));
        *dest++ = static_cast<u8>(0x80 | ((cdpt >> 6) & 0x3F));
        *dest++ = static_cast<u8>(0x80 | (cdpt  & 0x3F));
    }
    else
    {
        PANIC("The UTF32 sequence appears to be invalid, but this fucntion assumes it is not!");
        UNREACHABLE();
    }
}

// bool must be returned, becuase it is possible that we still have a space in
// dest buffer, but we caanot put the next character. e.g. we have 2 bytes, but
// we need to write 3 byte sequence.
static inline bool
utf32_to_utf8_dec_advc_safe(u32 const*& src, u8*& dest, u8 const* const& dest_end)
{
    u32 cdpt = *src;
    if (cdpt <= 0x7F && dest < dest_end)
    {
        *dest++ = static_cast<u8>(cdpt);
        src++;
        return true;
    }
    else if (cdpt <= 0x7FF && dest + 1 < dest_end)
    {
        *dest++ = static_cast<u8>(0xC0 | ((cdpt >> 6) & 0x1F));
        *dest++ = static_cast<u8>(0x80 | (cdpt & 0x3F));
        src++;
        return true;
    }
    else if (cdpt <= 0xFFFF && dest + 2 < dest_end)
    {
        *dest++ = static_cast<u8>(0xE0 | ((cdpt >> 12) & 0x0F));
        *dest++ = static_cast<u8>(0x80 | ((cdpt >> 6) & 0x3F));
        *dest++ = static_cast<u8>(0x80 | (cdpt & 0x3F));
        src++;
        return true;
    }
    else if (cdpt <= 0x10FFFF && dest + 3 < dest_end)
    {
        *dest++ = static_cast<u8>(0xF0 | ((cdpt >> 18) & 0x07));
        *dest++ = static_cast<u8>(0x80 | ((cdpt >> 12) & 0x3F));
        *dest++ = static_cast<u8>(0x80 | ((cdpt >> 6) & 0x3F));
        *dest++ = static_cast<u8>(0x80 | (cdpt  & 0x3F));
        src++;
        return true;
    }

    return false;
}

} // namespace detail

static inline std::pair<u8 const*, u32*>
utf8_to_utf32(u8 const* src_begin, u8 const* src_end,
              u32* dest_begin, u32* dest_end)
{
    while(src_begin + 16 <= src_end && dest_begin + 16 <= dest_end)
        detail::utf8_to_utf32_dec_advc_ascii_optimized(src_begin, dest_begin);

    while(src_begin + 4 <= src_end && dest_begin < dest_end)
        detail::utf8_to_utf32_dec_advc(src_begin, dest_begin);

    while(src_begin < src_end && dest_begin < dest_end)
        if (!detail::utf8_to_utf32_dec_advc_safe(src_begin, src_end, dest_begin))
            break;

    return { src_begin, dest_begin };
}

static inline std::pair<u32*, mm>
utf8_to_utf32_allocate(u8 const* src_begin, u8 const* src_end)
{
    mm retval_space = 256;
    mm curr_dest_end = 0;
    u32* retval = static_cast<u32*>(malloc(sizeof(u32) * retval_space));
    u8 const* src_curr = src_begin;

    while (src_curr != src_end)
    {
        auto[src_ret, dest_ret] = utf8_to_utf32(src_curr,
                                                src_end,
                                                retval + curr_dest_end,
                                                retval + retval_space);

        curr_dest_end = dest_ret - retval; // How many chars were written.
        src_curr = src_ret;

        if (src_curr != src_end)
        {
            LOG_ERROR("WE NEED TO REALLOCATE!");

            retval_space *= 2;
            retval = static_cast<u32*>(realloc(retval, sizeof(u32) * retval_space));
        }

    }

    return { retval, curr_dest_end };
}

static inline std::pair<u32 const*, u8*>
utf32_to_utf8(u32 const* src_begin,
              u32 const* src_end,
              u8* dest_begin,
              u8* dest_end)
{
    while(src_begin < src_end && dest_begin + 4 <= dest_end)
        detail::utf32_to_utf8_dec_advc(src_begin, dest_begin);

    while(src_begin < src_end && dest_begin < dest_end)
        if (!detail::utf32_to_utf8_dec_advc_safe(src_begin, dest_begin, dest_end))
            break;

    return { src_begin, dest_begin };
}
