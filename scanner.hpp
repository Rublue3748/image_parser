#pragma once
#include "endianness.hpp"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>

class Scanner
{
  public:
    Scanner(Endianness endianness = Endianness::little_endian);
    Scanner(std::string filename, Endianness endianness = Endianness::little_endian);
    ~Scanner();

    uint8_t read_byte();
    uint16_t read_word();
    uint32_t read_dword();
    uint64_t read_qword();

    /**
     * @param container container to fill
     * @param n number of bytes to read
     */
    template <typename T>
    void read_bytes(T &container, size_t n)
    {
        for (size_t i = 0; i < n; i++)
        {
            container.push_back(_current_file.get());
        }
    }

    void skip_bytes(size_t n);
    void seek(size_t n);

    bool eof() const
    {
        return _current_file.eof();
    }

    void set_endianness(Endianness new_endianness)
    {
        _current_endianness = new_endianness;
    }

  private:
    std::ifstream _current_file;
    Endianness _current_endianness;
};