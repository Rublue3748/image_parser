#include "inflate.hpp"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

bool test_deflate();

int main()
{
    bool all_tests_passed = true;
    all_tests_passed = all_tests_passed && test_deflate();
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
    const std::string input_names[] = {"test_data/default_output.hex", "test_data/filtered_output.hex", "test_data/fixed_output.hex",
                                       "test_data/huffman_output.hex"};
    const std::string output_file_name = "test_data/test.txt";

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
        std::cout << (test_passed ? "Passed" : "Failed") << std::endl;
        pass = pass && test_passed;
    }
    return pass;
}