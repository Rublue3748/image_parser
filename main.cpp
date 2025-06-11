#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <zlib.h>

#include "scanner.hpp"

struct PNG_Info
{
    uint32_t width;
    uint32_t height;
    uint8_t bpc;
    uint8_t color_type;
    uint8_t compression_method;
    uint8_t filter_method;
    uint8_t interlace_method;
};

PNG_Info process_header(Scanner &file);
void deflate_png(const std::vector<uint8_t> &compressed_data, std::vector<uint8_t> &uncompressed_data);
void unfilter_png(const PNG_Info &image_info, const std::vector<uint8_t> &filtered_data, std::vector<uint8_t> &unfiltered_data);
void unfilter_scanline(uint8_t const *source, size_t num_bytes, uint8_t *dest);
uint8_t paeth_predictor(uint8_t a, uint8_t b, uint8_t c);
void process_data(std::vector<uint8_t> image_data, const PNG_Info &image_info);

int main()
{
    Scanner current_file("images.png");

    uint64_t magic_word = current_file.read_qword();

    bool is_png = magic_word == static_cast<uint64_t>(0x0A1A0A0D474E5089);
    current_file.set_endianness(Scanner::Endianness::big_endian);

    if (!is_png)
    {
        std::cout << "Is not a png!" << std::endl;
        return 1;
    }

    std::cout << "Is a png! Reading headers." << std::endl;

    std::cout << "Chunks: " << std::endl;

    bool run = true;

    PNG_Info file_info;
    std::vector<uint8_t> image_data;

    while (run)
    {
        // 4B Length, 4B Type, Length-B Data, 4B CRC

        uint32_t length = current_file.read_dword();
        std::stringstream chunk_name;
        chunk_name << current_file.read_byte() << current_file.read_byte() << current_file.read_byte() << current_file.read_byte();
        std::cout << "L: " << length << "\tH: " << chunk_name.str() << std::endl;

        if (chunk_name.str() == "IHDR")
        {
            file_info = process_header(current_file);
        }
        else if (chunk_name.str() == "IDAT")
        {
            current_file.read_bytes(image_data, length);
            current_file.skip_bytes(4);
        }
        else if (chunk_name.str() == "IEND")
        {
            run = false;
            process_data(image_data, file_info);
        }
        else
        {
            current_file.skip_bytes(length);
            current_file.skip_bytes(4);
        }
    }

    std::cout << "Image width: " << file_info.width << " Image height: " << file_info.height << " bpc: " << static_cast<uint32_t>(file_info.bpc)
              << " Color mode: " << static_cast<uint32_t>(file_info.color_type) << std::endl;

    return 0;
}

PNG_Info process_header(Scanner &file)
{
    PNG_Info current_info;
    current_info.width = file.read_dword();
    current_info.height = file.read_dword();
    current_info.bpc = file.read_byte();
    current_info.color_type = file.read_byte();
    current_info.compression_method = file.read_byte();
    current_info.filter_method = file.read_byte();
    current_info.interlace_method = file.read_byte();
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
    file.skip_bytes(4);
    return current_info;
}

void process_data(std::vector<uint8_t> image_data, const PNG_Info &image_info)
{

    std::vector<uint8_t> uncompressed_data;
    deflate_png(image_data, uncompressed_data);

    std::ofstream uncompressed_out("uncompressed.out");
    std::for_each(uncompressed_data.begin(), uncompressed_data.end(), [&uncompressed_out](uint8_t data) { uncompressed_out.put(data); });
    uncompressed_out.close();

    std::vector<uint8_t> unfiltered_data;
    unfilter_png(image_info, uncompressed_data, unfiltered_data);

    std::ofstream output("unfiltered.out");
    std::for_each(unfiltered_data.begin(), unfiltered_data.end(), [&output](uint8_t data) { output.put(data); });
    output.close();
}

void deflate_png(const std::vector<uint8_t> &compressed_data, std::vector<uint8_t> &uncompressed_data)
{
    // TODO: Rewrite deflate algorithm
    std::array<uint8_t, 4096> buffer;

    z_stream_s decode_stream;

    decode_stream.next_in = const_cast<uint8_t *>(compressed_data.data());
    decode_stream.avail_in = compressed_data.size();
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
        std::copy_n(buffer.begin(), buffer.size() - decode_stream.avail_out, std::back_inserter(uncompressed_data));
        decode_stream.next_in = const_cast<uint8_t *>(compressed_data.data()) + decode_stream.total_in;
        decode_stream.avail_in = compressed_data.size() - decode_stream.total_in;
        decode_stream.next_out = buffer.data();
        decode_stream.avail_out = buffer.size();
    } while ((result == Z_OK) && (decode_stream.avail_out != 0));
}

void unfilter_png(const PNG_Info &image_info, const std::vector<uint8_t> &filtered_data, std::vector<uint8_t> &unfiltered_data)
{
    if (image_info.interlace_method == 1)
    {
        throw std::runtime_error("Unimplemented interlace method: Adam7");
    }

    // # channels depends on image type
    uint64_t number_channels;
    switch (image_info.color_type)
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
        error_message << "Unsupported color mode: " << static_cast<uint32_t>(image_info.color_type);
        throw std::runtime_error(error_message.str());
    }
    }

    // Bytes per scanline = width * # channels * (bits per channel / 8)
    uint64_t bytes_per_scanline = image_info.width * number_channels * (image_info.bpc / 8);
    uint64_t filtered_bytes_per_scanline = bytes_per_scanline + 1;
    uint64_t num_scanlines = filtered_data.size() / filtered_bytes_per_scanline;

    // TODO: Fix to account for channeling (see: https://www.w3.org/TR/png/#serializing-and-filtering-scanline)
    for (size_t i = 0; i < num_scanlines; i++)
    {
        uint64_t filtered_start_offset = i * filtered_bytes_per_scanline;
        uint64_t unfiltered_start_offset = i * bytes_per_scanline;

        uint8_t filter = filtered_data[filtered_start_offset];

        for (size_t pixel_x = 0; pixel_x < image_info.width; pixel_x++)
        {
            for (size_t channel = 0; channel < number_channels; channel++)
            {
                uint16_t left = (pixel_x == 0) ? 0 : unfiltered_data[unfiltered_data.size() - number_channels];
                uint16_t up = (i == 0) ? 0 : unfiltered_data[unfiltered_data.size() - bytes_per_scanline];
                uint16_t up_left = (i == 0 || pixel_x == 0) ? 0 : unfiltered_data[unfiltered_data.size() - bytes_per_scanline - number_channels];
                uint16_t current_filtered_pixel = filtered_data[filtered_start_offset + (pixel_x * number_channels + channel) + 1];
                uint16_t reconstructed_pixel_temp;
                switch (filter)
                {
                case 0:
                    std::cout << "Filter: None" << std::endl;
                    reconstructed_pixel_temp = current_filtered_pixel;
                    break;
                case 1:
                    std::cout << "Filter: Left -> " << current_filtered_pixel << " + " << left << std::endl;
                    reconstructed_pixel_temp = current_filtered_pixel + left;
                    break;
                case 2:
                    std::cout << "Filter: Up -> " << current_filtered_pixel << " + " << up << std::endl;
                    reconstructed_pixel_temp = current_filtered_pixel + up;
                    break;
                case 3:
                    std::cout << "Filter: Average" << std::endl;
                    reconstructed_pixel_temp = current_filtered_pixel + ((up + left) / 2);
                    break;
                case 4:
                    std::cout << "Filter: Paeth" << std::endl;
                    reconstructed_pixel_temp = current_filtered_pixel + paeth_predictor(left, up, up_left);
                    break;
                default: {
                    std::stringstream error_message;
                    error_message << "Unsupported filter type: " << static_cast<uint32_t>(filter);
                    throw std::runtime_error(error_message.str());
                }
                }
                uint8_t reconstructed_pixel = static_cast<uint8_t>(reconstructed_pixel_temp & 0xff);
                unfiltered_data.push_back(reconstructed_pixel);
            }
        }

        //     for (size_t j = 1; j < filtered_bytes_per_scanline; j++)
        //     {
        //         uint16_t left = (j == 1) ? 0 : unfiltered_data[unfiltered_data.size() - 1];
        //         uint16_t up = (i == 0) ? 0 : unfiltered_data[unfiltered_data.size() - bytes_per_scanline - 1];
        //         uint16_t up_left = (i == 0 || j == 1) ? 0 : unfiltered_data[unfiltered_data.size() - bytes_per_scanline - 2];
        //         uint16_t current_filtered_pixel = filtered_data[filtered_start_offset + j];
        //         uint16_t reconstructed_pixel_temp;
        //         switch (filter)
        //         {
        //         case 0:
        //             std::cout << "Filter: None" << std::endl;
        //             reconstructed_pixel_temp = current_filtered_pixel;
        //             break;
        //         case 1:
        //             std::cout << "Filter: Left -> " << current_filtered_pixel << " + " << left << std::endl;
        //             reconstructed_pixel_temp = current_filtered_pixel + left;
        //             break;
        //         case 2:
        //             std::cout << "Filter: Up" << std::endl;
        //             reconstructed_pixel_temp = current_filtered_pixel + up;
        //             break;
        //         case 3:
        //             std::cout << "Filter: Average" << std::endl;
        //             reconstructed_pixel_temp = current_filtered_pixel + ((up + left) / 2);
        //             break;
        //         case 4:
        //             std::cout << "Filter: Paeth" << std::endl;
        //             reconstructed_pixel_temp = current_filtered_pixel + paeth_predictor(left, up, up_left);
        //             break;
        //         default: {
        //             std::stringstream error_message;
        //             error_message << "Unsupported filter type: " << static_cast<uint32_t>(filter);
        //             throw std::runtime_error(error_message.str());
        //         }
        //         }
        //         uint8_t reconstructed_pixel = static_cast<uint8_t>(reconstructed_pixel_temp & 0xff);
        //         unfiltered_data.push_back(reconstructed_pixel);
        //     }
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