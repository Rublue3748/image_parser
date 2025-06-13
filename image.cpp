#include "image.hpp"
#include <stdexcept>

static size_t x_y_to_index(size_t x, size_t y, size_t width, size_t data_size = 1)
{
    return data_size * ((y * width) + x);
}

Image Image::from_RGB(Image_Size dimensions, std::vector<uint8_t> data)
{
    // TODO: Fix
    Image ret_img;
    ret_img._dims = dimensions;

    ret_img._image_data.resize(dimensions.width * dimensions.height * 4);

    for (size_t y = 0; y < dimensions.height; y++)
    {
        for (size_t x = 0; x < dimensions.width; x++)
        {
            ret_img._image_data[x_y_to_index(x, y, dimensions.width, 4) + 0] = data[x_y_to_index(x, y, dimensions.width, 3) + 0];
            ret_img._image_data[x_y_to_index(x, y, dimensions.width, 4) + 1] = data[x_y_to_index(x, y, dimensions.width, 3) + 1];
            ret_img._image_data[x_y_to_index(x, y, dimensions.width, 4) + 2] = data[x_y_to_index(x, y, dimensions.width, 3) + 2];
            ret_img._image_data[x_y_to_index(x, y, dimensions.width, 4) + 3] = 255;
        }
    }
    return ret_img;
}
Image Image::from_RGBA(Image_Size dimensions, std::vector<uint8_t> data)
{
    // TODO: Fix
    Image ret_img;
    ret_img._dims = dimensions;

    ret_img._image_data.resize(dimensions.width * dimensions.height * 4);

    for (size_t y = 0; y < dimensions.height; y++)
    {
        for (size_t x = 0; x < dimensions.width; x++)
        {
            ret_img._image_data[x_y_to_index(x, y, dimensions.width, 4) + 0] = data[x_y_to_index(x, y, dimensions.width, 4) + 0];
            ret_img._image_data[x_y_to_index(x, y, dimensions.width, 4) + 1] = data[x_y_to_index(x, y, dimensions.width, 4) + 1];
            ret_img._image_data[x_y_to_index(x, y, dimensions.width, 4) + 2] = data[x_y_to_index(x, y, dimensions.width, 4) + 2];
            ret_img._image_data[x_y_to_index(x, y, dimensions.width, 4) + 3] = data[x_y_to_index(x, y, dimensions.width, 4) + 3];
        }
    }
    return ret_img;
}
Image Image::from_GRAY(Image_Size dimensions, std::vector<uint8_t> data)
{
    throw std::runtime_error("Grayscale image not supported");
}
Image Image::from_GRAY_ALPHA(Image_Size dimensions, std::vector<uint8_t> data)
{
    throw std::runtime_error("Grayscale+Alpha image not supported");
}

std::ostream &operator<<(std::ostream &out, const Image &img)
{
    const uint8_t *width_ptr = reinterpret_cast<const uint8_t *>(&(img._dims.width));
    const uint8_t *height_ptr = reinterpret_cast<const uint8_t *>(&(img._dims.height));
    for (size_t i = 0; i < 4; i++)
    {
        out.put(width_ptr[i]);
    }
    for (size_t i = 0; i < 4; i++)
    {
        out.put(height_ptr[i]);
    }
    for (const uint8_t &data : img._image_data)
    {
        out.put(data);
    }
    return out;
}
std::istream &operator>>(std::istream &in, Image &img)
{
    uint8_t *width_ptr = reinterpret_cast<uint8_t *>(&(img._dims.width));
    uint8_t *height_ptr = reinterpret_cast<uint8_t *>(&(img._dims.height));
    for (size_t i = 0; i < 4; i++)
    {
        width_ptr[i] = in.get();
    }
    for (size_t i = 0; i < 4; i++)
    {
        height_ptr[i] = in.get();
    }
    img._image_data.clear();
    for (size_t i = 0; i < img._dims.width * img._dims.height * 4; i++)
    {
        img._image_data.push_back(in.get());
    }
    return in;
}