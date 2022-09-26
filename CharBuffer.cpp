#include "lcc.hpp"
#include <cstring>

#define CHARBUFFER_INITSIZE 8

namespace cc
{
    CharBuffer::CharBuffer() :
    _size(0), _buffer(nullptr), _capacity(0)
    {
        reserve(CHARBUFFER_INITSIZE);
    }

    CharBuffer::~CharBuffer()
    {
        if(_buffer != nullptr) 
        {
            free(_buffer);
            _buffer = nullptr;
        }
    }

    const char CharBuffer::charAt(unsigned int idx)
    {
        if(idx < _size) return _buffer[idx];

        return 0;
    }

    void CharBuffer::append(const char c)
    {
        if(_size == _capacity) reserve(2 * _capacity);
        _buffer[_size] = c;
        _size++;
    }

    void CharBuffer::pop()
    {
        if(_size) _size--;
    }

    void CharBuffer::reserve(size_t newSize)
    {
        if(_buffer == nullptr)
        {
            _buffer = (char*)malloc(newSize * sizeof(char));
            _capacity = newSize;
        } 
        else if(_capacity < newSize)
        {
            _buffer = (char*)realloc(_buffer, newSize * sizeof(char));
            _capacity = newSize;
        }
    }

    char* CharBuffer::asCBuffer() const
    {
        char* buffer = (char*)malloc(sizeof(char) * _size);
        memcpy(buffer, _buffer, _size * sizeof(char));
        return buffer;
    }
}