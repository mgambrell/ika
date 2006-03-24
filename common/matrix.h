#pragma once

#include <cassert>
#include <algorithm>

#include "common/utility.h"

#define FOLLOWTHEWHITERABBIT

template <typename T> struct Matrix;

template <typename T>
struct MatrixIterator {
    MatrixIterator(Matrix<T>& m, uint x, uint y, uint width, uint height) 
        : subject(m)
        , x(x)
        , y(y)
        , width(width)
        , height(height)
        , curX(x)
        , curY(y)
    {}

    const T& operator *() const {
        return subject(curX, curY);
    }

    T& operator *() {
        return subject(curX, curY);
    }

    MatrixIterator& operator ++() {
        curX++;
        if (curX >= x + width) {
            curX = x;
            curY++;
        }

        return *this;
    }

    MatrixIterator& operator --() {
        if (curX <= x) {
            curY--;
            curX = x + width - 1;
        } else {
            curX--;
        }
        return *this;
    }

    bool Valid() const {
        return curY < y + height && curX < x + width;
    }

private:
    Matrix<T>& subject;
    uint x;
    uint y;
    uint width;
    uint height;
    uint curX;
    uint curY;
};

/**
 * General purpose 2D array container thing.
 */
template <typename T>
struct Matrix {
    typedef MatrixIterator<T> iterator;

    Matrix(uint w, uint h)
        : _width(w)
        , _height(h) 
    {
        if (w > 0 && h > 0) {
            _data = new T[w * h];
            std::fill(_data, _data + w * h, T());
        } else {
            _data = 0;
        }
    }

    Matrix(uint w, uint h, T* d)
        : _width(w)
        , _height(h)
    {
        if (w > 0 && h > 0) {
            _data = new T[w * h];
            std::copy(d, d + w * h, _data);
        } else {
            _width = _height = 0;
            _data = 0;
        }
    }

    // Pitch is the number of elements between two vertical lines.
    // This is exceptionally handy for getting subsections of other
    // matrices.  Somewhat dangerous because range checking cannot
    // be done.
    Matrix(uint w, uint h, T* d, uint pitch)
        : _width(w)
        , _height(h) 
    {
        if (w > 0 && h > 0) {
            _data = new T[w * h];
            int y = h;
            T* dest = _data;
            while (y--) {
                std::copy(d, d + w, dest);
                d += pitch;
                dest += w;
            }
        } else {
            _width = _height = 0;
            _data = 0;
        }
    }

    Matrix(const Matrix& rhs)
        : _width(rhs._width)
        , _height(rhs._height) 
    {
        if (rhs._data) {
            _data = new T[_width * _height];
            std::copy(rhs._data, rhs._data + _width * _height, _data);
        } else {
            _data = 0;
        }
    }

    Matrix()
        : _width(0)
        , _height(0)
        , _data(0) 
    {}

    ~Matrix() {
        delete[] _data;
    }

    T& operator ()(uint x, uint y) {
        if (x >= _width || y >= _height) {
            // x_x
            // TODO: make this raise an exception.
            static T blah;
            return blah;
        }

        return _data[y * _width + x];
    }

    const T& operator ()(uint x, uint y) const {
        return (*const_cast<Matrix<T>*>(this))(x, y);
    }

    // allows efficient sequential access
    // TODO: iterator thing for extra safety
    // and robustness
    const T* GetPointer(uint x, uint y) const {
        assert(x >= 0 && y >= 0);
        assert(x < _width && y < _height);

        return _data + (y * _width) + x;
    }

    iterator Begin(uint x, uint y, uint width, uint height) {
        return iterator(*this, x, y, width, height);
    }

    uint Width() const  { return _width; }
    uint Height() const { return _height; }
    bool Empty() const { return _data == 0; }

    void Resize(uint newx, uint newy) {
        // trivial case
        if (newx == _width && newy == _height) {
            return;
        }

        if (newx == 0 || newy == 0) {
            _width = _height = 0;
            delete[] _data;
            _data = 0;

        } else {
            int sx = newx < _width  ? newx : _width;
            int sy = newy < _height ? newy : _height;

            T* tempData = new T[newx * newy];
            std::fill(tempData, tempData + newx * newy, T());

            T* src = _data;
            T* dest = tempData;

            while (sy) {
                std::copy(src, src + sx, dest);
                src += _width;
                dest += newx;
                sy--;
            }

            delete[] _data;
            _data = tempData;
            _width  = newx;
            _height = newy;
        }
    }

    Matrix& operator = (const Matrix& rhs) {
        delete[] _data;
        _width = rhs._width;
        _height = rhs._height;

        if (rhs._data) {
            _data = new T[_width * _height];
            std::copy(rhs._data, rhs._data + _width * _height, _data);
        } else {
            _data = 0;
        }

        return *this;
    }

private:
    T* _data;
    uint _width;
    uint _height;
};
