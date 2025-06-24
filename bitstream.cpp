#include "bitstream.hpp"
#include "endianness.hpp"
#include <cstddef>
#include <stdexcept>
#include <utility>

bool Bitstream::get_bit(size_t index)
{
    size_t resolved_index = _start + index;
    std::pair<size_t, size_t> bit_positions = bit_index(resolved_index);
    uint8_t current_byte = _data[bit_positions.first];
    return ((current_byte >> bit_positions.second) & 1) == 1;
}
std::pair<size_t, size_t> Bitstream::bit_index(size_t index)
{
    std::pair<size_t, size_t> indices = {index / 8, index % 8};
    return indices;
}

bool Bitstream::pop_bit()
{
    bool bit = get_bit(0);
    _start++;
    return bit;
}

uint64_t Bitstream::pop_bits(size_t bits)
{
    if (bits > 64 || bits == 0)
    {
        throw std::out_of_range("Invalid number of bits");
    }
    uint64_t result = 0;
    for (size_t i = 0; i < bits; i++)
    {
        if (_current_endianness == Endianness::little_endian)
        {
            result |= (pop_bit() ? 1 : 0) << i;
        }
        else
        {
            result <<= 1;
            result |= (pop_bit() ? 1 : 0);
        }
    }
    return result;
}

void Bitstream::skip_to_byte_boundary()
{
    // Each byte boundary is on a multiple of 8.
    if ((_start & 0b111) == 0)
    {
        // Already on a byte boundary
        return;
    }
    _start &= ~(0b111);
    _start += 8;
}