#include "inflate.hpp"
#include "bitstream.hpp"
#include "endianness.hpp"
#include "huffman_tree.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

/**
 * @brief Inflates a deflated block of data from the bitstream. Returns whether there is potentially more data to decode (if the block is not marked
 as final)
 *
 * @param data The bitstream to read from. Bitstream is modified regardless of if the data was valid
 * @param return_data Buffer to return the data in. Appends to the end of the buffer
 * @return Returns whether there is more data available (block is not marked as final)
 */
static bool inflate_block(Bitstream &data, std::vector<uint8_t> &return_data);

/**
 * @brief Copies the uncompressed block of data from Bitstream to return_data
 *
 * @param data The bitstream to copy from
 * @param return_data The buffer to return data in
 */
static void copy_uncompressed(Bitstream &data, std::vector<uint8_t> &return_data);

/**
 * @brief Generates the static Huffman Trees for the literal and distance codes. The codes are described in
 * https://datatracker.ietf.org/doc/html/rfc1951#page-12
 *
 * @return Pair of <literal tree, distance tree>
 */
static std::pair<Huffman_Tree, Huffman_Tree> get_static_trees();

/**
 * @brief Generates the dynamic Huffman Trees for the literal and distance codes, given by the stream.
 *
 * @param stream Datastream that points to the start of the dynamic trees block
 * @return Pair of <literal tree, distance tree>
 */
static std::pair<Huffman_Tree, Huffman_Tree> get_dynamic_trees(Bitstream &stream);

/**
 * @brief Reads in a block of data given the provided Huffman Trees
 *
 * @param stream Datastream to read from
 * @param return_data Buffer to append data to
 * @param trees Pair of <literal tree, distance tree> to use to decode the data
 */
static void get_data_from_trees(Bitstream &stream, std::vector<uint8_t> &return_data, std::pair<Huffman_Tree, Huffman_Tree> trees);

/**
 * @brief Takes a literal value >256 and converts it into a length, according to https://datatracker.ietf.org/doc/html/rfc1951#page-12
 *
 * @param literal The literal value to translate
 * @param stream The datastream to read from for extra bits
 * @return uint32_t The value of the length
 */
static uint32_t get_length_from_literal(uint32_t literal, Bitstream &stream);

/**
 * @brief Takes an offset value and converts it into the actual offset, according to https://datatracker.ietf.org/doc/html/rfc1951#page-12
 *
 * @param offset The offset value to translate
 * @param stream The datastream to read from for extra bits
 * @return uint32_t The (positive) value of the offset
 */
static uint32_t get_offset_from_literal(uint32_t offset, Bitstream &stream);

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

    // We aren't doing anything with these values as of now
    (void)CINFO;
    (void)FCHECK;
    (void)FLEVEL;

    if (CM != 8)
    {
        throw std::runtime_error("Unknown compression method!");
    }

    // Skip dictonary CRC byte
    if (FDICT)
    {
        stream.pop_bits(8);
    }

    // Decode inflate blocks until we reach the final block
    while (inflate_block(stream, return_data))
        ;

    return return_data;
};

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
    // See https://datatracker.ietf.org/doc/html/rfc1951#page-7 (section 3.2.2)
    // and https://datatracker.ietf.org/doc/html/rfc1951#page-12 (section 3.2.6)

    // Generates a mapping of code lengths to values
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

    // Generates the actual tree using the map
    Huffman_Tree literal_tree = Huffman_Tree::generate_tree(literal_map);

    // Generates a 1:1 mapping for the distance tree so we can just reuse the distance code for the dynamic case
    Huffman_Tree distance_tree;
    for (size_t i = 0; i < 32; i++)
    {
        distance_tree.add_code(i, 5, i);
    }
    return {literal_tree, distance_tree};
}

static std::pair<Huffman_Tree, Huffman_Tree> get_dynamic_trees(Bitstream &stream)
{
    // See https://datatracker.ietf.org/doc/html/rfc1951#page-13 (section 3.2.7)
    uint16_t HLIT = stream.pop_bits(5) + 257;
    uint16_t HDIST = stream.pop_bits(5) + 1;
    uint16_t HCLEN = stream.pop_bits(4) + 4;

    const std::vector<uint32_t> codes = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    std::map<uint32_t, uint32_t> code_lengths;

    // Map given code lengths to codes
    for (size_t i = 0; i < HCLEN; i++)
    {
        uint32_t code_length = stream.pop_bits(3);
        if (code_length != 0)
        {
            code_lengths.insert_or_assign(codes[i], code_length);
        }
    }

    // Build the code length tree using this first set of code lengths
    Huffman_Tree initial_tree = Huffman_Tree::generate_tree(code_lengths);

    // Decode code lengths for the literals and the distances
    // (Distance codes can backreference to literal codes, so decode as one big block and then split)
    std::vector<uint32_t> literals;
    while (literals.size() < HLIT + HDIST)
    {
        uint32_t val = initial_tree.get_next_value(stream);
        if (val == 16) // Repeat previous code 3-6 times (read in 2 extra bits)
        {
            uint32_t repeat_amount = 3 + stream.pop_bits(2);
            for (size_t i = 0; i < repeat_amount; i++)
            {
                literals.push_back(literals.back());
            }
        }
        else if (val == 17) // Repeat a code of zero 3-10 times (read in 3 extra bits)
        {
            uint32_t repeat_amount = 3 + stream.pop_bits(3);
            for (size_t i = 0; i < repeat_amount; i++)
            {
                literals.push_back(0);
            }
        }
        else if (val == 18) // Repeat a code of zero 11-138 times (read in 7 extra bits)
        {
            uint32_t repeat_amount = 11 + stream.pop_bits(7);
            for (size_t i = 0; i < repeat_amount; i++)
            {
                literals.push_back(0);
            }
        }
        else // Push literal into buffer
        {
            literals.push_back(val);
        }
    }

    if (literals.size() > HLIT + HDIST)
    {
        throw std::runtime_error("Literals size somehow larger than HLIT + HDIST!");
    }

    // Generate Literal Tree using first part of the buffer
    std::map<uint32_t, uint32_t> lit_map;
    for (uint32_t i = 0; i < HLIT; i++)
    {
        if (literals[i] != 0)
        {
            lit_map.insert(std::pair(i, literals[i]));
        }
    }
    Huffman_Tree literal_tree = Huffman_Tree::generate_tree(lit_map);

    // Generate Distance Tree using second part of the buffer
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
    // https://datatracker.ietf.org/doc/html/rfc1951#page-12 (Section 3.2.5 tables)

    uint32_t length;
    if (literal >= 257 && literal <= 264)
    {
        length = (literal - 257 + 3);
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
    // https://datatracker.ietf.org/doc/html/rfc1951#page-12 (Section 3.2.5 tables)

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
