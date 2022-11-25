#pragma once

#include <cassert>
#include <cstring>

#include "types.hpp"

struct String {
  u8 *data = nullptr;
  u32 size = 0;

  String() {}

  String(u8 *data, u32 size)
  {
    this->data = data;
    this->size = size;
  }

  template <u32 N>
  String(const char (&str)[N])
  {
    data = (u8 *)&str;
    size = N - 1;  // remove '\0'
  }

  u8 &operator[](u32 i)
  {
    assert(i < size);
    return data[i];
  }

  u64 to_uint64()
  {
    u64 val = 0;
    for (int i = 0; i < size; i++) {
      if (data[i] < '0' && data[i] > '9') return val;
      val = 10 * val + (data[i] - '0');
    }
    return val;
  }
};

template <u64 CAPACITY>
struct StaticString {
  static const u64 MAX_SIZE = CAPACITY;
  u8 data[CAPACITY + 1];
  u64 size = 0;

  StaticString() { data[0] = '\0'; }

  template <u64 CAPACITY_2>
  void operator=(const StaticString<CAPACITY_2> &str2)
  {
    size = fmin(str2.size, MAX_SIZE);
    memcpy(data, str2.data, size);
    data[size] = '\0';
  }
  
  void operator=(const String &str2)
  {
    size = fmin(str2.size, MAX_SIZE);
    memcpy(data, str2.data, size);
    data[size] = '\0';
  }

  template <u64 CAPACITY_2>
  StaticString(StaticString<CAPACITY_2> &str2)
  {
    size = fmin(str2.size, MAX_SIZE);
    memcpy(data, str2.data, size);
    data[size] = '\0';
  }
};