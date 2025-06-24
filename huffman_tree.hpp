#pragma once
#include "bitstream.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <ostream>

class Huffman_Tree
{
  private:
    enum class Leaf_Type
    {
        Leaf,
        Internal_Node
    };

  public:
    static Huffman_Tree generate_tree(const std::map<uint32_t, uint32_t> &value_length_pairs);

    Huffman_Tree() noexcept : _type(Leaf_Type::Leaf), _value(0), _zero(nullptr), _one(nullptr) {};

    Huffman_Tree(const Huffman_Tree &other) noexcept;
    Huffman_Tree(Huffman_Tree &&other) noexcept;

    ~Huffman_Tree() noexcept
    {
        delete _zero;
        delete _one;
    }

    Huffman_Tree &operator=(const Huffman_Tree &other) noexcept;
    Huffman_Tree &operator=(Huffman_Tree &&other) noexcept;

    friend std::ostream &operator<<(std::ostream &, const Huffman_Tree &);

    void add_code(uint32_t code, size_t code_length, uint32_t value)
    {
        add_code(code, code_length, 0, value);
    }

    bool is_valid_code(uint32_t code, size_t code_length) const
    {
        return is_valid_code(code, code_length, 0);
    }
    uint32_t get_value(uint32_t code, size_t code_length) const
    {
        return get_value(code, code_length, 0);
    }

    uint32_t get_next_value(Bitstream &stream) const;

  private:
    static uint32_t get_code_bit(uint32_t code, size_t code_length, size_t pos);

    void add_code(uint32_t code, size_t code_length, size_t pos, uint32_t value);
    bool is_valid_code(uint32_t code, size_t code_length, size_t pos) const;
    uint32_t get_value(uint32_t code, size_t code_length, size_t pos) const;

    std::ostream &output(std::ostream &out, size_t current_code, size_t code_length) const;

    Leaf_Type _type;
    uint32_t _value;
    Huffman_Tree *_zero;
    Huffman_Tree *_one;
};
std::ostream &operator<<(std::ostream &, const Huffman_Tree &);
