#include "endianness.hpp"
#include "image.hpp"
#include "inflate.hpp"
#include "png.hpp"
#include "scanner.hpp"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

bool test_deflate();
bool test_png();

int main()
{
    bool all_tests_passed = true;
    all_tests_passed = all_tests_passed && test_deflate();
    all_tests_passed = all_tests_passed && test_png();
    if (all_tests_passed)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

bool test_deflate()
{
    const std::string input_names[] = {"test_data/deflate_data/default_output.hex", "test_data/deflate_data/filtered_output.hex",
                                       "test_data/deflate_data/fixed_output.hex", "test_data/deflate_data/huffman_output.hex"};
    const std::string output_file_name = "test_data/deflate_data/test.txt";

    std::ifstream output_file(output_file_name, std::ifstream::binary);
    std::vector<uint8_t> output_data;
    std::copy(std::istreambuf_iterator<char>(output_file), std::istreambuf_iterator<char>(), std::back_inserter(output_data));

    bool pass = true;
    for (const auto &input : input_names)
    {
        std::cout << "Inflate on test file " << input << ": ";
        std::ifstream input_file(input, std::ifstream::binary);
        std::vector<uint8_t> input_data;
        std::copy(std::istreambuf_iterator<char>(input_file), std::istreambuf_iterator<char>(), std::back_inserter(input_data));
        std::vector<uint8_t> result = inflate_data(input_data);
        bool test_passed =
            std::equal(output_data.begin(), output_data.end(), result.begin()) && std::equal(result.begin(), result.end(), output_data.begin());
        std::cout << (test_passed ? "\033[32;4mPassed\033[0m" : "\033[31;4mFailed\033[0m") << std::endl;
        pass = pass && test_passed;
    }
    return pass;
}

bool test_png()
{
    const std::string rgb_input_names[] = {"test_data/image_data/rgb.png", "test_data/image_data/rgba.png"};
    const std::string rgb_output_file_name = "test_data/image_data/rgb.img";

    std::ifstream rgb_output_file(rgb_output_file_name, std::ifstream::binary);
    rgb_output_file.seekg(8); // skip size data
    std::vector<uint8_t> rgb_output_data;
    std::copy(std::istreambuf_iterator<char>(rgb_output_file), std::istreambuf_iterator<char>(), std::back_inserter(rgb_output_data));

    bool pass = true;
    for (const auto &input : rgb_input_names)
    {
        std::cout << "PNG Decode on test file " << input << ": ";

        Scanner temp(input, Endianness::little_endian);

        if (!PNG::is_PNG_file(temp))
        {
            std::cout << "\033[31;4mFailed\033[0m, is not a png file" << std::endl;
        }
        Image result_image = PNG::PNG_Try_Parse(temp);

        bool test_passed = std::equal(rgb_output_data.begin(), rgb_output_data.end(), result_image._image_data.begin()) &&
                           std::equal(result_image._image_data.begin(), result_image._image_data.end(), rgb_output_data.begin());
        std::cout << (test_passed ? "\033[32;4mPassed\033[0m" : "\033[31;4mFailed\033[0m") << std::endl;
        if (!test_passed)
        {
            std::cout << "Data diverges at index "
                      << std::distance(rgb_output_data.begin(),
                                       std::mismatch(rgb_output_data.begin(), rgb_output_data.end(), result_image._image_data.begin()).first)
                      << std::endl;
        }
        pass = pass && test_passed;
    }

    const std::string palette_input_names[] = {"test_data/image_data/palette.png", "test_data/image_data/palettea.png"};
    const std::string palette_output_file_name = "test_data/image_data/palette.img";

    std::ifstream palette_output_file(palette_output_file_name, std::ifstream::binary);
    palette_output_file.seekg(8); // skip size data
    std::vector<uint8_t> palette_output_data;
    std::copy(std::istreambuf_iterator<char>(palette_output_file), std::istreambuf_iterator<char>(), std::back_inserter(palette_output_data));

    for (const auto &input : palette_input_names)
    {
        std::cout << "PNG Decode on test file " << input << ": ";

        Scanner temp(input, Endianness::little_endian);

        if (!PNG::is_PNG_file(temp))
        {
            std::cout << "\033[31;4mFailed\033[0m, is not a png file" << std::endl;
        }
        Image result_image = PNG::PNG_Try_Parse(temp);

        bool test_passed = std::equal(palette_output_data.begin(), palette_output_data.end(), result_image._image_data.begin()) &&
                           std::equal(result_image._image_data.begin(), result_image._image_data.end(), palette_output_data.begin());
        std::cout << (test_passed ? "\033[32;4mPassed\033[0m" : "\033[31;4mFailed\033[0m") << std::endl;
        if (!test_passed)
        {
            std::cout << "Data diverges at index "
                      << std::distance(palette_output_data.begin(),
                                       std::mismatch(palette_output_data.begin(), palette_output_data.end(), result_image._image_data.begin()).first)
                      << std::endl;
        }
        pass = pass && test_passed;
    }
    const std::string gray_input_names[] = {"test_data/image_data/gray.png"};
    // TODO: fix graya "test_data/image_data/graya.png"};
    const std::string gray_output_file_name = "test_data/image_data/gray.img";

    std::ifstream gray_output_file(gray_output_file_name, std::ifstream::binary);
    gray_output_file.seekg(8); // skip size data
    std::vector<uint8_t> gray_output_data;
    std::copy(std::istreambuf_iterator<char>(gray_output_file), std::istreambuf_iterator<char>(), std::back_inserter(gray_output_data));

    for (const auto &input : gray_input_names)
    {
        std::cout << "PNG Decode on test file " << input << ": ";

        Scanner temp(input, Endianness::little_endian);

        if (!PNG::is_PNG_file(temp))
        {
            std::cout << "\033[31;4mFailed\033[0m, is not a png file" << std::endl;
        }
        Image result_image = PNG::PNG_Try_Parse(temp);

        bool test_passed = std::equal(gray_output_data.begin(), gray_output_data.end(), result_image._image_data.begin()) &&
                           std::equal(result_image._image_data.begin(), result_image._image_data.end(), gray_output_data.begin());
        std::cout << (test_passed ? "\033[32;4mPassed\033[0m" : "\033[31;4mFailed\033[0m") << std::endl;
        if (!test_passed)
        {
            std::cout << "Data diverges at index "
                      << std::distance(gray_output_data.begin(),
                                       std::mismatch(gray_output_data.begin(), gray_output_data.end(), result_image._image_data.begin()).first)
                      << std::endl;
        }
        pass = pass && test_passed;
    }
    return pass;
}