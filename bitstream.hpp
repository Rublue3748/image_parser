#pragma once

#include "endianness.hpp"
#include <cstddef>
#include <cstdint>
#include <endian.h>
#include <utility>
#include <vector>

class Bitstream
{
  public:
    Bitstream(const std::vector<uint8_t> &data) : _data(data), _start(0), _current_endianness(Endianness::little_endian) {};

    void set_endianness(Endianness endianness)
    {
        _current_endianness = endianness;
    }

    bool pop_bit();
    uint64_t pop_bits(size_t bits);

  private:
    std::vector<uint8_t> _data;
    size_t _start;
    Endianness _current_endianness;

    bool get_bit(size_t index);
    static std::pair<size_t, size_t> bit_index(size_t index);
};