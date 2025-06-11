#include "scanner.hpp"
#include <ios>
#include <stdexcept>

Scanner::Scanner(std::string filename, Endianness endianness)
    : _current_file(filename), _current_endianness(endianness)
{
    if (!_current_file.good())
    {
        throw std::runtime_error("File was not good!");
    }
}

Scanner::~Scanner()
{
    _current_file.close();
}

uint8_t Scanner::read_byte()
{
    uint8_t temp;
    temp = _current_file.get();
    return temp;
}
uint16_t Scanner::read_word()
{
    uint16_t byte0 = _current_file.get();
    uint16_t byte1 = _current_file.get();
    uint16_t temp;
    if (_current_endianness == Endianness::little_endian)
    {
        temp = (byte1 << 8) | byte0;
    }
    else
    {
        temp = (byte0 << 8) | byte1;
    }
    return temp;
}
uint32_t Scanner::read_dword()
{
    uint32_t byte0 = _current_file.get();
    uint32_t byte1 = _current_file.get();
    uint32_t byte2 = _current_file.get();
    uint32_t byte3 = _current_file.get();
    uint32_t temp;
    if (_current_endianness == Endianness::little_endian)
    {
        temp = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;
    }
    else
    {
        temp = (byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3;
    }
    return temp;
}
uint64_t Scanner::read_qword()
{
    uint64_t byte0 = _current_file.get();
    uint64_t byte1 = _current_file.get();
    uint64_t byte2 = _current_file.get();
    uint64_t byte3 = _current_file.get();
    uint64_t byte4 = _current_file.get();
    uint64_t byte5 = _current_file.get();
    uint64_t byte6 = _current_file.get();
    uint64_t byte7 = _current_file.get();
    uint64_t temp;
    if (_current_endianness == Endianness::little_endian)
    {
        temp = (byte7 << 56) | (byte6 << 48) | (byte5 << 40) | (byte4 << 32) | (byte3 << 24) |
               (byte2 << 16) | (byte1 << 8) | byte0;
    }
    else
    {
        temp = (byte0 << 56) | (byte1 << 48) | (byte2 << 40) | (byte3 << 32) | (byte4 << 24) |
               (byte5 << 16) | (byte6 << 8) | byte7;
    }
    return temp;
}

void Scanner::skip_bytes(size_t n)
{
    _current_file.seekg(n, std::ios::cur);
}
void Scanner::seek(size_t n)
{
    _current_file.seekg(n, std::ios::beg);
}