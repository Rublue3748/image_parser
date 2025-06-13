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

    static Image from_RGB(Image_Size dimensions, std::vector<uint8_t> data);
    static Image from_RGBA(Image_Size dimensions, std::vector<uint8_t> data);
    static Image from_GRAY(Image_Size dimensions, std::vector<uint8_t> data);
    static Image from_GRAY_ALPHA(Image_Size dimensions, std::vector<uint8_t> data);

    friend std::ostream &operator<<(std::ostream &out, const Image &img);
    friend std::istream &operator>>(std::istream &in, Image &img);
};

std::ostream &operator<<(std::ostream &out, const Image &img);
std::istream &operator>>(std::istream &in, Image &img);