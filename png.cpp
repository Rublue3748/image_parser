#include "png.hpp"

#include "scanner.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <zlib.h>

struct PNG_Header_Info
{
    uint32_t width;
    uint32_t height;
    uint8_t bpc;
    uint8_t color_type;
    uint8_t compression_method;
    uint8_t filter_method;
    uint8_t interlace_method;
};

static PNG_Header_Info process_header(Scanner &file);
static Image process_data();
static void deflate_png();
static void unfilter_png();
static Image post_process_data();
static Image generate_palette_image();
static uint8_t paeth_predictor(uint8_t a, uint8_t b, uint8_t c);

static PNG_Header_Info png_header;
static std::vector<uint8_t> compressed_png_data;
static std::vector<uint8_t> uncompressed_png_data;
static std::vector<uint8_t> unfiltered_png_data;
static std::vector<uint8_t> palette_data;
static std::vector<uint8_t> transparency_data;

bool PNG::is_PNG_file(Scanner &file)
{
    uint64_t magic_word = file.read_qword();

    bool is_png = magic_word == static_cast<uint64_t>(0x0A1A0A0D474E5089);
    file.seek(0);
    return is_png;
}

Image PNG::PNG_Try_Parse(Scanner &file)
{
    compressed_png_data.clear();
    uncompressed_png_data.clear();
    unfiltered_png_data.clear();
    palette_data.clear();
    transparency_data.clear();
    if (!is_PNG_file(file))
    {
        throw std::runtime_error("Not a PNG file!");
    }
    // Skip over header checked in file
    file.skip_bytes(8);
    file.set_endianness(Scanner::Endianness::big_endian);

    // Loop over all chunks
    std::string chunk_type = "XXXX";
    Image result;
    do
    {
        // Chunk format:
        // | 4 byte length | 4 byte chunk id | "length" byte data | 4 byte CRC |
        uint32_t chunk_length = file.read_dword();

        chunk_type[0] = file.read_byte();
        chunk_type[1] = file.read_byte();
        chunk_type[2] = file.read_byte();
        chunk_type[3] = file.read_byte();

        if (chunk_type == "IHDR")
        {
            png_header = process_header(file);
        }
        else if (chunk_type == "PLTE")
        {
            if (chunk_length % 3 != 0)
            {
                throw std::runtime_error("Invalid palette");
            }
            file.read_bytes(palette_data, chunk_length);
        }
        else if (chunk_type == "tRNS")
        {
            file.read_bytes(transparency_data, chunk_length);
        }
        else if (chunk_type == "IDAT")
        {
            file.read_bytes(compressed_png_data, chunk_length);
        }
        else if (chunk_type == "IEND")
        {
            result = process_data();
            file.skip_bytes(chunk_length);
        }
        else
        {
            std::cerr << "Unknown PNG Chunk type: " << chunk_type << std::endl;
            file.skip_bytes(chunk_length);
        }
        // Skip CRC
        file.skip_bytes(4);
    } while (chunk_type != "IEND");
    return result;
}

static PNG_Header_Info process_header(Scanner &file)
{
    PNG_Header_Info current_info;
    current_info.width = file.read_dword();
    current_info.height = file.read_dword();
    current_info.bpc = file.read_byte();
    current_info.color_type = file.read_byte();
    current_info.compression_method = file.read_byte();
    current_info.filter_method = file.read_byte();
    current_info.interlace_method = file.read_byte();

    // Throw errors if formats are unsupported (Skip parsing the rest of the data)
    if (current_info.interlace_method == 1)
    {
        throw std::runtime_error("Unimplemented interlace method: Adam7");
    }
    if (current_info.bpc != 8)
    {
        std::stringstream error_message;
        error_message << "Unsupported bpc: " << static_cast<uint32_t>(current_info.bpc);
        throw std::runtime_error(error_message.str());
    }
    std::cout << "Image: (" << current_info.width << "x" << current_info.height << "), " << static_cast<uint32_t>(current_info.bpc)
              << " bpc Color Type " << static_cast<uint32_t>(current_info.color_type) << std::endl;
    return current_info;
}

static Image process_data()
{

    deflate_png();

    std::vector<uint8_t> unfiltered_data;
    unfilter_png();

    return post_process_data();
}

static void deflate_png()
{
    // TODO: Rewrite deflate algorithm
    std::array<uint8_t, 4096> buffer;

    z_stream_s decode_stream;

    decode_stream.next_in = const_cast<uint8_t *>(compressed_png_data.data());
    decode_stream.avail_in = compressed_png_data.size();
    decode_stream.total_in = 0;

    decode_stream.next_out = buffer.data();
    decode_stream.avail_out = buffer.size();
    decode_stream.total_out = 0;

    decode_stream.zalloc = Z_NULL;
    decode_stream.zfree = Z_NULL;
    decode_stream.opaque = Z_NULL;

    decode_stream.data_type = Z_BINARY;

    inflateInit(&decode_stream);
    int result;
    do
    {
        result = inflate(&decode_stream, Z_SYNC_FLUSH);
        std::copy_n(buffer.begin(), buffer.size() - decode_stream.avail_out, std::back_inserter(uncompressed_png_data));
        decode_stream.next_in = const_cast<uint8_t *>(compressed_png_data.data()) + decode_stream.total_in;
        decode_stream.avail_in = compressed_png_data.size() - decode_stream.total_in;
        decode_stream.next_out = buffer.data();
        decode_stream.avail_out = buffer.size();
    } while ((result == Z_OK) && (decode_stream.avail_out != 0));
}

static void unfilter_png()
{
    if (png_header.interlace_method == 1)
    {
        throw std::runtime_error("Unimplemented interlace method: Adam7");
    }

    // # channels depends on image type
    uint64_t number_channels;
    switch (png_header.color_type)
    {
    case 0: // Grayscale (1 channel)
    case 3: // Indexed (1 "channel")
        number_channels = 1;
        break;
    case 4: // Grayscale + alpha (2 channels)
        number_channels = 2;
        break;
    case 2: // True color RGB (3 channels)
        number_channels = 3;
        break;
    case 6: // True color RGBA (4 channels)
        number_channels = 4;
        break;
    default: {
        std::stringstream error_message;
        error_message << "Unsupported color mode: " << static_cast<uint32_t>(png_header.color_type);
        throw std::runtime_error(error_message.str());
    }
    }

    // Bytes per scanline = width * # channels * (bits per channel / 8)
    uint64_t bytes_per_scanline = png_header.width * number_channels * (png_header.bpc / 8);
    uint64_t filtered_bytes_per_scanline = bytes_per_scanline + 1;
    uint64_t num_scanlines = uncompressed_png_data.size() / filtered_bytes_per_scanline;

    // TODO: Fix to account for channeling (see: https://www.w3.org/TR/png/#serializing-and-filtering-scanline)
    for (size_t i = 0; i < num_scanlines; i++)
    {
        uint64_t filtered_start_offset = i * filtered_bytes_per_scanline;
        uint64_t unfiltered_start_offset = i * bytes_per_scanline;

        uint8_t filter = uncompressed_png_data[filtered_start_offset];

        for (size_t pixel_x = 0; pixel_x < png_header.width; pixel_x++)
        {
            for (size_t channel = 0; channel < number_channels; channel++)
            {
                uint16_t left = (pixel_x == 0) ? 0 : unfiltered_png_data[unfiltered_png_data.size() - number_channels];
                uint16_t up = (i == 0) ? 0 : unfiltered_png_data[unfiltered_png_data.size() - bytes_per_scanline];
                uint16_t up_left =
                    (i == 0 || pixel_x == 0) ? 0 : unfiltered_png_data[unfiltered_png_data.size() - bytes_per_scanline - number_channels];
                uint16_t current_filtered_pixel = uncompressed_png_data[filtered_start_offset + (pixel_x * number_channels + channel) + 1];
                uint16_t reconstructed_pixel_temp;
                switch (filter)
                {
                case 0:
                    // std::cout << "Filter: None" << std::endl;
                    reconstructed_pixel_temp = current_filtered_pixel;
                    break;
                case 1:
                    // std::cout << "Filter: Left -> " << current_filtered_pixel << " + " << left << std::endl;
                    reconstructed_pixel_temp = current_filtered_pixel + left;
                    break;
                case 2:
                    // std::cout << "Filter: Up -> " << current_filtered_pixel << " + " << up << std::endl;
                    reconstructed_pixel_temp = current_filtered_pixel + up;
                    break;
                case 3:
                    // std::cout << "Filter: Average" << std::endl;
                    reconstructed_pixel_temp = current_filtered_pixel + ((up + left) / 2);
                    break;
                case 4:
                    // std::cout << "Filter: Paeth" << std::endl;
                    reconstructed_pixel_temp = current_filtered_pixel + paeth_predictor(left, up, up_left);
                    break;
                default: {
                    std::stringstream error_message;
                    error_message << "Unsupported filter type: " << static_cast<uint32_t>(filter);
                    throw std::runtime_error(error_message.str());
                }
                }
                uint8_t reconstructed_pixel = static_cast<uint8_t>(reconstructed_pixel_temp & 0xff);
                unfiltered_png_data.push_back(reconstructed_pixel);
            }
        }
    }
}

uint8_t paeth_predictor(uint8_t a, uint8_t b, uint8_t c)
{
    int32_t p = a + b - c;
    int32_t pa = std::abs(p - static_cast<int32_t>(a));
    int32_t pb = std::abs(p - static_cast<int32_t>(b));
    int32_t pc = std::abs(p - static_cast<int32_t>(c));

    if (pa <= pb && pa <= pc)
    {
        return a;
    }
    else if (pb <= pc)
    {
        return b;
    }
    else
    {
        return c;
    }
}

static Image post_process_data()
{
    switch (png_header.color_type)
    {
    case 0:
        // Do grayscale things
        return Image::from_GRAY({png_header.width, png_header.height}, unfiltered_png_data);
        break;
    case 3:
        // Convert from palette
        return generate_palette_image();
        break;
    case 4:
        // Grayscale + Alpha
        return Image::from_GRAY_ALPHA({png_header.width, png_header.height}, unfiltered_png_data);
        break;
    case 2:
        // RGB
        // TODO: Sanitize 16 bit colors down to 8 bit colors
        // TODO: Properly handle transparency chunk
        return Image::from_RGB({png_header.width, png_header.height}, unfiltered_png_data);
        break;
    case 6:
        // RGBA
        // TODO: Sanitize 16 bit colors down to 8 bit colors
        // TODO: Properly handle transparency chunk
        return Image::from_RGBA({png_header.width, png_header.height}, unfiltered_png_data);
        break;
    }
    return Image();
}

static Image generate_palette_image()
{
    std::vector<uint8_t> final_image_data;
    for (size_t y = 0; y < png_header.height; y++)
    {
        for (size_t x = 0; x < png_header.width; x++)
        {
            uint8_t index = unfiltered_png_data.at(y * png_header.width + x);
            final_image_data.push_back(palette_data[index * 3]);
            final_image_data.push_back(palette_data[index * 3 + 1]);
            final_image_data.push_back(palette_data[index * 3 + 2]);
            if (transparency_data.size() != 0)
            {
                final_image_data.push_back(transparency_data[index]);
            }
            else
            {
                final_image_data.push_back(255);
            }
        }
    }
    return Image::from_RGBA({png_header.width, png_header.height}, final_image_data);
}