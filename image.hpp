#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <vector>

/*
    Represents a 32-bit RGBA image
 */
struct Image_Size
{
    uint32_t width;
    uint32_t height;
};

struct Image
{
    Image_Size _dims;
    std::vector<uint8_t> _image_data;

    static Image from_RGB(Image_Size dimensions, const std::vector<uint8_t> &data, bool alpha = false);
    static Image from_RGBA(Image_Size dimensions, const std::vector<uint8_t> &data)
    {
        return from_RGB(dimensions, data, true);
    }
    static Image from_GRAY(Image_Size dimensions, const std::vector<uint8_t> &data, bool alpha = false);
    static Image from_GRAY_ALPHA(Image_Size dimensions, const std::vector<uint8_t> &data)
    {
        return from_GRAY(dimensions, data, true);
    }

    friend std::ostream &operator<<(std::ostream &out, const Image &img);
    friend std::istream &operator>>(std::istream &in, Image &img);
};

std::ostream &operator<<(std::ostream &out, const Image &img);
std::istream &operator>>(std::istream &in, Image &img);