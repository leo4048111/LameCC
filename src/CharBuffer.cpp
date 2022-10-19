#include "lcc.hpp"
#include <cstring>

#define CHARBUFFER_INITSIZE 8

#ifndef ZeroMemory
#define ZeroMemory(addr, size) memset((void*)addr, 0, size)
#endif

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
        if(_size + 1 == _capacity) reserve(2 * _capacity);
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
            ZeroMemory(_buffer, newSize);
            _capacity = newSize;
        } 
        else if(_capacity < newSize)
        {
            char* newBuffer = (char*)malloc(newSize * sizeof(char));
            ZeroMemory(newBuffer, newSize);
            memcpy((void*)newBuffer, (void*)_buffer, _size);
            _capacity = newSize;
            free(_buffer);
            _buffer = newBuffer;
        }
    }

    const char* CharBuffer::c_str()
    {
        if(_size >= _capacity) reserve(_capacity + 1);
        return _buffer;
    }

    const char* CharBuffer::new_c_str() const
    {
        char* buffer = (char*)malloc(sizeof(char) * (_size + 1));
        ZeroMemory(buffer, _size + 1);
        memcpy(buffer, _buffer, _size * sizeof(char));
        return buffer;
    }

    bool CharBuffer::operator == (const char* s)
    {
        return !strcmp(this->c_str(), s);
    }
}