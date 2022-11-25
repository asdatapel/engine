#pragma once

#include "logging.hpp"
#include "types.hpp"

template <typename T, u64 CAPACITY>
struct StaticStack {
  const static u64 MAX_SIZE = CAPACITY; 
  T elements[CAPACITY];
  u64 count = 0;

  StaticStack() = default;

  T &top()
  {
    assert(count > 0);
    return elements[count - 1];
  };

  T pop()
  {
    assert(count > 0);
    count--;
    return elements[count];
  };

  T &operator[](u64 i)
  {
    assert(i < count);
    return elements[i];
  }

  int push_back(T val)
  {
    if (count >= MAX_SIZE) {
      fatal("static stack overfull\n");
      return -1;
    }

    elements[count] = val;
    return count++;
  }

  void clear() { count = 0; }
};