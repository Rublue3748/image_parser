#include "huffman_tree.hpp"
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>

Huffman_Tree::Huffman_Tree(const Huffman_Tree &other) noexcept : _type(other._type), _value(other._value)
{

    if (other._zero == nullptr)
    {
        _zero = nullptr;
    }
    else
    {
        _zero = new Huffman_Tree(*(other._zero));
    }
    if (other._one == nullptr)
    {
        _one = nullptr;
    }
    else
    {
        _one = new Huffman_Tree(*(other._one));
    }
}

Huffman_Tree::Huffman_Tree(Huffman_Tree &&other) noexcept : _type(other._type), _value(other._value), _zero(other._zero), _one(other._one)
{
    other._zero = nullptr;
    other._one = nullptr;
}

Huffman_Tree &Huffman_Tree::operator=(const Huffman_Tree &other) noexcept
{
    if (&other == this)
    {
        return *this;
    }
    _type = other._type;
    _value = other._value;
    delete _zero;
    delete _one;
    if (other._zero == nullptr)
    {
        _zero = nullptr;
    }
    else
    {
        _zero = new Huffman_Tree(*(other._zero));
    }
    if (other._one == nullptr)
    {
        _one = nullptr;
    }
    else
    {
        _one = new Huffman_Tree(*(other._one));
    }
    return *this;
}

Huffman_Tree &Huffman_Tree::operator=(Huffman_Tree &&other) noexcept
{
    this->_type = other._type;
    this->_value = other._value;
    std::swap(this->_zero, other._zero);
    std::swap(this->_one, other._one);
    return *this;
}

void Huffman_Tree::add_code(uint32_t code, size_t code_length, size_t pos, uint32_t value)
{
    // Base case
    if (pos == code_length)
    {
        _type = Leaf_Type::Leaf;
        delete _zero;
        delete _one;
        _value = value;
        return;
    }
    _type = Leaf_Type::Internal_Node;

    if (get_code_bit(code, code_length, pos) == 0)
    {
        if (_zero == nullptr)
        {
            _zero = new Huffman_Tree();
        }
        _zero->add_code(code, code_length, pos + 1, value);
    }
    else
    {
        if (_one == nullptr)
        {
            _one = new Huffman_Tree();
        }
        _one->add_code(code, code_length, pos + 1, value);
    }
}

bool Huffman_Tree::is_valid_code(uint32_t code, size_t code_length, size_t pos) const
{
    if (pos == code_length)
    {
        return _type == Leaf_Type::Leaf;
    }
    if (get_code_bit(code, code_length, pos) == 0)
    {
        if (_zero == nullptr)
        {
            throw std::runtime_error("Invalid code");
        }
        return _zero->is_valid_code(code, code_length, pos + 1);
    }
    else
    {
        if (_one == nullptr)
        {
            throw std::runtime_error("Invalid code");
        }
        return _one->is_valid_code(code, code_length, pos + 1);
    }
}
uint32_t Huffman_Tree::get_value(uint32_t code, size_t code_length, size_t pos) const
{
    if (pos == code_length)
    {
        return _value;
    }
    if (get_code_bit(code, code_length, pos) == 0)
    {
        return _zero->get_value(code, code_length, pos + 1);
    }
    else
    {
        return _one->get_value(code, code_length, pos + 1);
    }
}

uint32_t Huffman_Tree::get_code_bit(uint32_t code, size_t code_length, size_t pos)
{
    return (code >> (code_length - pos - 1)) & 1;
}

Huffman_Tree Huffman_Tree::generate_tree(const std::map<uint32_t, uint32_t> &value_length_pairs)
{

    std::map<uint32_t, uint32_t> converted_lengths;
    for (const auto &vl_pair : value_length_pairs)
    {
        converted_lengths.try_emplace(vl_pair.second, 0);
        converted_lengths.at(vl_pair.second)++;
    }

    // Convert to start addresses
    uint32_t max_value = (*converted_lengths.rbegin()).first;

    std::map<uint32_t, uint32_t> start_addresses;

    uint32_t start_address = 0;
    for (size_t i = 1; i <= max_value; i++)
    {
        if (converted_lengths.count(i - 1) != 0)
        {
            start_address += converted_lengths.at(i - 1);
        }
        start_address <<= 1;
        start_addresses.insert_or_assign(i, start_address);
    }

    Huffman_Tree final_tree;

    for (const auto &vl_pair : value_length_pairs)
    {
        uint32_t code_length = vl_pair.second;
        uint32_t code = start_addresses.at(code_length);
        uint32_t code_value = vl_pair.first;
        start_addresses.at(code_length)++;
        final_tree.add_code(code, code_length, code_value);
    }

    return final_tree;
}

std::ostream &operator<<(std::ostream &out, const Huffman_Tree &tree)
{
    return tree.output(out, 0, 0);
}

std::ostream &Huffman_Tree::output(std::ostream &out, size_t current_code, size_t code_length) const
{
    // Base case
    if (_type == Leaf_Type::Leaf)
    {
        for (size_t i = 0; i < code_length; i++)
        {
            out << ((((current_code >> (code_length - i - 1)) & 1) == 1) ? 1 : 0);
        }
        out << " -> " << _value << std::endl;
    }
    else
    {
        if (_zero != nullptr)
        {
            _zero->output(out, current_code << 1, code_length + 1);
        }
        if (_one != nullptr)
        {
            _one->output(out, (current_code << 1) | 1, code_length + 1);
        }
    }
    return out;
}

uint32_t Huffman_Tree::get_next_value(Bitstream &stream) const
{
    uint32_t current_code = 0;
    uint32_t code_length = 0;
    while (1)
    {
        uint32_t val = stream.pop_bit() ? 1 : 0;
        current_code <<= 1;
        current_code |= val;
        code_length++;
        if (is_valid_code(current_code, code_length))
        {
            break;
        }
    }
    return get_value(current_code, code_length);
}