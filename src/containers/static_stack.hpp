#pragma once

#include "logging.hpp"
#include "types.hpp"

template <typename T, u64 CAPACITY>
struct StaticStack {
  const static u64 MAX_SIZE = CAPACITY; 
  T elements[CAPACITY];
  u64 size = 0;

  StaticStack() = default;

  T &top()
  {
    assert(size > 0);
    return elements[size - 1];
  };

  T pop()
  {
    assert(size > 0);
    size--;
    return elements[size];
  };

  T &operator[](u64 i)
  {
    assert(i < size);
    return elements[i];
  }

  int push_back(T val)
  {
    if (size >= MAX_SIZE) {
      fatal("static stack overfull\n");
      return -1;
    }

    elements[size] = val;
    return size++;
  }

  void clear() { size = 0; }
};