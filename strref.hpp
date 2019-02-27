// This is basically std::string_view now. Will have to refactor this later.

#ifndef STRREF_HPP
#define STRREF_HPP

#include <iterator>

#ifndef STRREF_ASSERT
#  ifndef ASSERT
#    include <cassert>
#    define STRREF_ASSERT assert
#  else
#    define STRREF_ASSERT ASSERT
#  endif
#endif

// TODO: Move some stuff to .cpp file.

struct strref
{
    using iterator = u32*;
    using const_iterator = u32 const*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    u32* first;
    u32* last;

    strref() = default;
    strref(strref const& other) = default;
    strref& operator=(strref const&) = default;

    strref(u32* first_, u32* last_)
    {
        first = first_;
        last = last_;
    }

    strref(u32* first_, mm len)
    {
        first = first_;
        last = first_ + len;
    }

    strref(u32* first_)
    {
        first = first_;
        auto p = first;
        while(*p) { ++p; }
        last = p;
    }

    mm size() const { return last - first; }
    bool empty() const { return first == last; }

    u32& operator[](mm pos) { return *(first + pos); }

    void shrink_front(mm npos)
    {
        first += npos;
        STRREF_ASSERT(first <= last);
    }

    void shrink_front(u32* newpos)
    {
        first = newpos;
        STRREF_ASSERT(first <= last);
    }

    void shrink_back(mm npos)
    {
        last -= npos;
        STRREF_ASSERT(first <= last);
    }

    void shrink_back(u32* newpos)
    {
        last = newpos;
        STRREF_ASSERT(first <= last);
    }

    iterator first_of(u32 const* characters)
    {
        for(auto i = begin(); i != end(); ++i)
            for(auto c = characters; *c; ++c)
            {
                if(*i == *c)
                    return i;
            }
        return end();
    }

    iterator first_of(strref const& characters)
    {
        for(auto i = begin(); i != end(); ++i)
            for(auto const c : characters)
            {
                if(*i == c)
                    return i;
            }
        return end();
    }

    iterator last_of(u32* characters)
    {
        for(auto i = last - 1; i >= first; --i)
            for(auto c = characters; *c; ++c)
            {
                if(*i == *c)
                    return i;
            }
        return end();
    }

    iterator last_of(strref const& characters)
    {
        for(auto i = last - 1; i >= first; --i)
            for(auto const c : characters)
            {
                if(*i == c)
                    return i;
            }
        return end();
    }

    // TODO: Nodiscard, if cpp version supports it.
    strref substr(mm pos, mm count) const
    {
        STRREF_ASSERT(first + pos <= first + pos + count);
        return strref{ first + pos, first + pos + count };
    }

    int compare(strref const& other) const
    {
        for(auto s = first, o = other.first;
            s != last && o != other.last;
            ++s, ++o)
        {
            if (*s != *o)
                return *s < *o ? -1 : 1;
        }

        return size() < other.size() ? -1 : (size() == other.size() ? 0 : 1);
    }

    iterator begin() const { return first; }
    iterator end() const { return last; }
    const_iterator cbegin() const { return first; }
    const_iterator cend() const { return last; }
    reverse_iterator rbegin() const { return reverse_iterator(end()); }
    reverse_iterator rend() const { return reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const{ return const_reverse_iterator(end()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(begin());}
};

inline bool operator<(strref const& x, strref const& y) { return x.compare(y) < 0; }
inline bool operator==(strref const& x, strref const& y) { return x.compare(y) == 0; }
inline bool operator!=(strref const& x, strref const& y) { return !(x == y); }
inline bool operator>(strref const& x, strref const& y) { return y < x; }
inline bool operator<=(strref const& x, strref const& y) { return !(y < x); }
inline bool operator>=(strref const& x, strref const& y) { return !(x < y); }

#endif //STRREF_HPP
