#ifndef FS_HPP
#define FS_HPP

namespace fs
{

// Checks the name does not contain any / (as it is a name of the single
// file/folder/etc).
static bool
valid_name(u8* file_or_dir_name);

struct record
{
    u8* name;
    u8 type;

    // This COPIES name.
    record(u8* name, u8 type);
    record(record&& other);
    ~record();

    record& operator= (record&&);
};

struct path
{
    // All filenames/dirnames are encoded in utf8.
    std::vector<u8> data;

    bool exists();

    bool is_file();
    bool is_directory();
    bool is_symlink();

    bool push_dir(u8* dirname);
    bool pop_dir();

    u8 const* get_name() const;
    std::vector<record> get_contents() const;

    FILE* open_file();

    // Empty ctor will create a path from the current directory calling getcwd
    path();

    path(u8 const* cstr_pathname);
};

}

#endif // FS_HPP
