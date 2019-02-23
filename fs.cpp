#include <unistd.h>
#include <dirent.h>
#include <cstring>

#include <vector>

#include "fs.hpp"

namespace fs
{

static bool
valid_name(u8* file_or_dir_name)
{
    (void(file_or_dir_name));
    return true;
}

record::record(u8* name_, u8 type_)
{
    name = reinterpret_cast<u8*>(strdup(reinterpret_cast<char const*>(name_)));
    type = type_;
}

record::record(record&& other_)
{
    *this = std::move(other_);
}

record& record::operator=(record&& other_)
{
    name = other_.name;
    type = other_.type;

    other_.name = nullptr;

    return *this;
}

record::~record()
{
    if (name)
        free(name);
}

bool
path::exists()
{
    DIR* dir = opendir(reinterpret_cast<char const*>(get_name()));
    if (dir)
    {
        closedir(dir);
        return true;
    }
    else
    {
        return false;
    }
}

bool
path::push_dir(u8* dirname)
{
    ASSERT(valid_name(dirname));
    ASSERT(data.back() == 0);

    mm len = strlen(reinterpret_cast<char const*>(dirname));

    data.reserve(data.size() + len + 1);
    data.pop_back();
    while(*dirname)
        data.push_back(*dirname++);
    if(data.back() != '/')
        data.push_back('/');
    data.push_back(0);

    if (exists())
    {
        return true;
    }
    else
    {
        LOG_WARN("Path %s does not exists! Push dir failed", data.data());

        // Rollback the path to what it was before.
        for(int i = 0; i < len + 1; ++i)
            data.pop_back();
        data.push_back(0);
        return false;
    }
}

bool
path::pop_dir()
{
    ASSERT(data.size() >= 1);
    ASSERT(data.back() == 0);

    if(* (data.end() - 2) != '/' || data.size() == 2)
        return false;

    data.pop_back();
    data.pop_back();

    auto p = data.rbegin();
    for(; p != data.rend(); ++p)
    {
        if(*p == '/')
            break;
    }

    mm slash_len = p - data.rbegin();
    for(int i = 0; i < slash_len; ++i)
        data.pop_back();
    data.push_back(0);

    return true;
}

u8 const*
path::get_name() const
{
    return data.data();
}

std::vector<record>
path::get_contents() const
{
    std::vector<record> retval;
    struct dirent *entry;
    DIR *dp;

    dp = opendir(reinterpret_cast<char const*>(data.data()));
    if (dp == nullptr)
    {
        PANIC("Opendir failed!");
        return retval;
    }

    while((entry = readdir(dp)))
        retval.emplace_back(reinterpret_cast<u8*>(entry->d_name), entry->d_type);

    std::sort(retval.begin(), retval.end(),
              [](record const& x, record const& y)
              {
                  return strcmp(reinterpret_cast<char const*>(x.name),
                                reinterpret_cast<char const*>(y.name)) < 0;
              });

    closedir(dp);
    return retval;
}

path::path()
{
    char cwd[PATH_MAX + 1];
    char* result = getcwd(cwd, PATH_MAX + 1);
    ASSERT(result);

    *this = path(reinterpret_cast<u8*>(result));
}

path::path(u8 const* cstr_pathname)
{
    data.reserve(strlen(reinterpret_cast<char const*>(cstr_pathname)) + 1);
    while(*cstr_pathname)
        data.push_back(*cstr_pathname++);

    if(data.back() != '/')
        data.push_back('/');
    data.push_back(0);
}

} // namespace fs
