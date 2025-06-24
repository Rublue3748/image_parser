#include "inflate.hpp"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>

int main()
{
    std::ifstream compressed_file("huffman_output.hex", std::ifstream::binary);
    std::ifstream uncompressed_file("test.txt", std::ifstream::binary);

    std::vector<uint8_t> compressed_data;
    std::vector<uint8_t> uncompressed_data;
    std::copy(std::istreambuf_iterator<char>(compressed_file), std::istreambuf_iterator<char>(), std::back_inserter(compressed_data));
    std::copy(std::istreambuf_iterator<char>(uncompressed_file), std::istreambuf_iterator<char>(), std::back_inserter(uncompressed_data));

    std::vector<uint8_t> result = inflate_data(compressed_data);

    if (!std::equal(uncompressed_data.begin(), uncompressed_data.end(), result.begin()))
    {
        std::cout << "Unequal data!" << std::endl;
    }
    else
    {
        std::cout << "Equal data!" << std::endl;
    }
}