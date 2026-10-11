#pragma once

#include <string>
#include <cstddef>

namespace RmlLoadingTips
{
struct Tip
{
    std::string faction;
    std::string title;
    std::string advice;
    std::string author;
    std::string description;
    std::string image;
    std::string cost;
    std::string modelObject;
    bool valid = false;
};
std::size_t select(const std::string &side);
Tip resolve(std::size_t index, const std::string &side);
}
