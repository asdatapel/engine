#pragma once

#include <cassert>
#include <initializer_list>


#include "types.hpp"

template <typename T, u32 N>
struct Array {
  const static u32 MAX_SIZE = N;
  T data[N];
  u32 size = 0;

  Array() = default;
  Array(std::initializer_list<T> il)
  {
    assert(il.size() <= N);
    size = il.size();

    u32 i = 0;
    for (auto t : il) {
      data[i] = t;
      i++;
    }
  }

  T &operator[](u32 i)
  {
    assert(i < size);
    return data[i];
  }

  T &operator[](i32 i)
  {
    assert(i < size);
    return data[i];
  }

  u32 push_back(T val)
  {
    if (size >= MAX_SIZE) {
      fatal("Array overfull");
    }

    data[size] = val;
    return size++;
  }

  i32 insert(u32 i, T val)
  {
    if (size >= MAX_SIZE) {
      fatal("static array past capacity\n");
      return -1;
    }

    memcpy(&data[i + 1], &data[i], sizeof(T) * (size - i));
    data[i] = val;
    size++;
    return i;
  }

  void swap_delete(u32 i)
  {
    data[i] = data[size - 1];
    size--;
  }

  void shift_delete(u32 i)
  {
    while (i + 1 < size) {
      data[i] = data[i + 1];
      i++;
    }
    size--;
  }

  void resize(u32 new_size)
  {
    assert(new_size <= MAX_SIZE);
    size = new_size;
  }

  void clear() { size = 0; }
};