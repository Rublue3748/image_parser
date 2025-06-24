#include "inflate.hpp"
#include "bitstream.hpp"
#include "endianness.hpp"
#include "huffman_tree.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

static bool inflate_block(Bitstream &data, std::vector<uint8_t> &return_data);
static void copy_uncompressed(Bitstream &data, std::vector<uint8_t> &return_data);
static std::pair<Huffman_Tree, Huffman_Tree> get_static_trees();
static std::pair<Huffman_Tree, Huffman_Tree> get_dynamic_trees(Bitstream &stream);
static void get_data_from_trees(Bitstream &stream, std::vector<uint8_t> &return_data, std::pair<Huffman_Tree, Huffman_Tree> trees);
static uint32_t get_length_from_literal(uint32_t literal, Bitstream &stream);
static uint32_t get_offset_from_literal(uint32_t literal, Bitstream &stream);

std::vector<uint8_t> inflate_data(const std::vector<uint8_t> &compressed_data)
{
    Bitstream stream(compressed_data);
    stream.set_endianness(Endianness::little_endian);
    std::vector<uint8_t> return_data;

    // Decode zlib header
    uint64_t CM = stream.pop_bits(4);
    uint64_t CINFO = stream.pop_bits(4);
    uint64_t FCHECK = stream.pop_bits(5);
    uint64_t FDICT = stream.pop_bits(1);
    uint64_t FLEVEL = stream.pop_bits(2);
    (void)CINFO;
    (void)FCHECK;
    (void)FLEVEL;

    if (CM != 8)
    {
        throw std::runtime_error("Unknown compression method!");
    }

    // Skip dictonary CRC bit
    if (FDICT)
    {
        stream.pop_bits(8);
    }
    // Decode inflate header

    while (inflate_block(stream, return_data))
        ;

    return return_data;
};

// Returns if there are more blocks to decode
static bool inflate_block(Bitstream &stream, std::vector<uint8_t> &return_data)
{
    bool is_final = stream.pop_bit();
    uint8_t compression_type = stream.pop_bits(2);

    std::pair<Huffman_Tree, Huffman_Tree> trees;
    switch (compression_type)
    {
    case 0b00:
        copy_uncompressed(stream, return_data);
        break;
    case 0b01:
        trees = get_static_trees();
        get_data_from_trees(stream, return_data, trees);
        break;
    case 0b10:
        trees = get_dynamic_trees(stream);
        get_data_from_trees(stream, return_data, trees);
        break;
    case 0b11:
        throw std::runtime_error("Invalid compression type!");
    }

    return !is_final;
}

static void copy_uncompressed(Bitstream &stream, std::vector<uint8_t> &return_data)
{
    stream.set_endianness(Endianness::little_endian);
    stream.skip_to_byte_boundary();

    uint32_t data_length = stream.pop_bits(16);
    uint32_t n_data_length = stream.pop_bits(16);
    if ((~data_length) != n_data_length)
    {
        throw std::runtime_error("LEN and NLEN do not match!");
    }

    // Copy LEN bytes to return and exit
    for (size_t i = 0; i < data_length; i++)
    {
        return_data.push_back(stream.pop_bits(8));
    }
}

static std::pair<Huffman_Tree, Huffman_Tree> get_static_trees()
{
    std::map<uint32_t, uint32_t> literal_map;
    for (size_t i = 0; i < 144; i++)
    {
        literal_map.insert({i, 8});
    }
    for (size_t i = 144; i < 256; i++)
    {
        literal_map.insert({i, 9});
    }
    for (size_t i = 256; i < 280; i++)
    {
        literal_map.insert({i, 7});
    }
    for (size_t i = 280; i < 288; i++)
    {
        literal_map.insert({i, 8});
    }
    Huffman_Tree literal_tree = Huffman_Tree::generate_tree(literal_map);
    Huffman_Tree distance_tree;
    for (size_t i = 0; i < 32; i++)
    {
        distance_tree.add_code(i, 5, i);
    }
    return {literal_tree, distance_tree};
}

static std::pair<Huffman_Tree, Huffman_Tree> get_dynamic_trees(Bitstream &stream)
{

    uint16_t HLIT = stream.pop_bits(5) + 257;
    uint16_t HDIST = stream.pop_bits(5) + 1;
    uint16_t HCLEN = stream.pop_bits(4) + 4;

    const std::vector<uint32_t> codes = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    std::map<uint32_t, uint32_t> code_lengths;

    for (size_t i = 0; i < HCLEN; i++)
    {
        uint32_t code_length = stream.pop_bits(3);
        if (code_length != 0)
        {
            code_lengths.insert_or_assign(codes[i], code_length);
        }
    }

    Huffman_Tree initial_tree = Huffman_Tree::generate_tree(code_lengths);
    std::vector<uint32_t> literals;
    while (literals.size() < HLIT + HDIST)
    {
        uint32_t val = initial_tree.get_next_value(stream);
        if (val == 16)
        {
            uint32_t repeat_amount = 3 + stream.pop_bits(2);
            for (size_t i = 0; i < repeat_amount; i++)
            {
                literals.push_back(literals.back());
            }
        }
        else if (val == 17)
        {
            uint32_t repeat_amount = 3 + stream.pop_bits(3);
            for (size_t i = 0; i < repeat_amount; i++)
            {
                literals.push_back(0);
            }
        }
        else if (val == 18)
        {
            uint32_t repeat_amount = 11 + stream.pop_bits(7);
            for (size_t i = 0; i < repeat_amount; i++)
            {
                literals.push_back(0);
            }
        }
        else
        {
            literals.push_back(val);
        }
    }
    if (literals.size() > HLIT + HDIST)
    {
        throw std::runtime_error("Literals size somehow larger than HLIT + HDIST!");
    }

    std::map<uint32_t, uint32_t> lit_map;

    for (uint32_t i = 0; i < HLIT; i++)
    {
        if (literals[i] != 0)
        {
            lit_map.insert(std::pair(i, literals[i]));
        }
    }
    Huffman_Tree literal_tree = Huffman_Tree::generate_tree(lit_map);

    lit_map.clear();
    for (uint32_t i = 0; i < HDIST; i++)
    {
        if (literals[HLIT + i] != 0)
        {
            lit_map.insert(std::pair(i, literals[HLIT + i]));
        }
    }
    Huffman_Tree distance_tree = Huffman_Tree::generate_tree(lit_map);

    return {literal_tree, distance_tree};
}

static void get_data_from_trees(Bitstream &stream, std::vector<uint8_t> &return_data, std::pair<Huffman_Tree, Huffman_Tree> trees)
{
    Huffman_Tree literal_tree = trees.first;
    Huffman_Tree distance_tree = trees.second;

    // Decode data using the given trees
    bool run = true;
    while (run)
    {

        uint32_t literal_value = literal_tree.get_next_value(stream);
        if (literal_value == 256) // Special end of block value
        {
            run = false;
        }
        else if (literal_value < 256)
        {
            return_data.push_back(literal_value);
        }
        else // >256, represents a <length,offset> pair
        {
            uint32_t length = get_length_from_literal(literal_value, stream);
            uint32_t offset_value = distance_tree.get_next_value(stream);
            uint32_t offset = get_offset_from_literal(offset_value, stream);
            for (size_t i = 0; i < length; i++)
            {
                return_data.push_back(return_data.at(return_data.size() - offset));
            }
        }
    }
}

static uint32_t get_length_from_literal(uint32_t literal, Bitstream &stream)
{
    uint32_t length;
    if (literal >= 257 && literal <= 264)
    {
        length = (literal - 257 + 3); // 257-264 -> 3-10
    }
    else if (literal >= 265 && literal <= 268)
    {
        length = ((literal - 265) * 2 + 11) + stream.pop_bits(1);
    }
    else if (literal >= 269 && literal <= 272)
    {
        length = ((literal - 269) * 4 + 19) + stream.pop_bits(2);
    }
    else if (literal >= 273 && literal <= 276)
    {
        length = ((literal - 273) * 8 + 35) + stream.pop_bits(3);
    }
    else if (literal >= 277 && literal <= 280)
    {
        length = ((literal - 277) * 16 + 67) + stream.pop_bits(4);
    }
    else if (literal >= 281 && literal <= 284)
    {
        length = ((literal - 281) * 32 + 131) + stream.pop_bits(5);
    }
    else
    { // literal value == 285, return a length of 258 (not a typo)
        length = 258;
    }
    return length;
}

static uint32_t get_offset_from_literal(uint32_t literal, Bitstream &stream)
{
    uint32_t offset;
    if (literal <= 3)
    {
        offset = literal + 1;
    }
    else
    {
        uint32_t num_extra_bits = (literal / 2) - 1;
        offset = 1 << (num_extra_bits + 1);
        offset |= (literal & 1) << num_extra_bits;
        offset |= stream.pop_bits(num_extra_bits);
        offset += 1;
    }

    return offset;
}
