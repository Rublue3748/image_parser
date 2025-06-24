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
    //  = {0x78, 0x00, 0x1d, 0xc6, 0x49, 0x01, 0x00, 0x00, 0x10, 0x40, 0xc0, 0xac, 0xa3,
    // 0x7f, 0x88, 0x3d, 0x3c, 0x20, 0x2a, 0x97, 0x9d, 0x37, 0x5e, 0x1d, 0x0c};
    std::vector<uint8_t> uncompressed_data;
    //  = {'a', 'b', 'a', 'a', 'b', 'b', 'b', 'a', 'b', 'a', 'a', 'b', 'a', 'b', 'b', 'a', 'a', 'b',
    //   'a', 'b', 'a', 'a', 'a', 'a', 'b', 'a', 'a', 'a', 'b', 'b', 'b', 'b', 'b', 'a', 'a'};
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
    // std::cout << compressed_data.size() << std::endl;
}