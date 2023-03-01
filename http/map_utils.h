#ifndef MAP_UTILS_H
#define MAP_UTILS_H

#include <string.h>
#include <string>

// 用于unordered_map的cmp
struct cmp
{
    bool operator()(const char *s1, const char *s2) const
    {
        return strcmp(s1, s2) == 0;
    }

};
// 用于unordered_map的hash
struct hash_func
{
    size_t operator()(const char *str) const
    {
        return std::hash<std::string>()(str);
    }
};

#endif