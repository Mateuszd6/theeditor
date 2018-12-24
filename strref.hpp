#ifndef STRREF_HPP
#define STRREF_HPP

// TODO: Move some stuff to .cpp file.

struct strref
{
    char* first;
    char* last;

    size_t size()
    {
        return last - first;
    }

    char* data()
    {
        return first;
    }
};

#endif //STRREF_HPP
