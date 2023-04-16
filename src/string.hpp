#pragma once

#include <cassert>
#include <cstring>

#include "memory.hpp"
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

  String sub(u32 from, u32 to)
  {
    assert(from <= size);
    assert(to <= size);

    return String(data + from, to - from);
  }

  b8 operator==(const String &other)
  {
    if (size != other.size) return false;
    for (i32 i = 0; i < size; i++) {
      if (data[i] != other.data[i]) return false;
    }
    return true;
  }

  u64 to_u64()
  {
    u64 val = 0;
    for (int i = 0; i < size; i++) {
      if (data[i] < '0' && data[i] > '9') return val;
      val = 10 * val + (data[i] - '0');
    }
    return val;
  }
};

struct NullTerminatedString : String {
  static NullTerminatedString concatenate(String str1, String str2, Allocator *allocator)
  {
    NullTerminatedString new_string;
    new_string.size = str1.size + str2.size + 1;
    new_string.data = (u8 *)allocator->alloc(new_string.size).data;

    memcpy(new_string.data, str1.data, str1.size);
    memcpy(new_string.data + str1.size, str2.data, str2.size);
    new_string.data[new_string.size - 1] = '\0';

    return new_string;
  }
};

template <u64 CAPACITY>
struct StaticString {
  static const u64 MAX_SIZE = CAPACITY;
  u8 data[CAPACITY + 1];
  u64 size = 0;

  StaticString() { data[0] = '\0'; }

  template <u64 CAPACITY_2>
  StaticString(StaticString<CAPACITY_2> &str2)
  {
    size = fmin(str2.size, MAX_SIZE);
    memcpy(data, str2.data, size);
    data[size] = '\0';
  }

  template <u32 N>
  StaticString(const char (&str)[N])
  {
    assert(N <= CAPACITY + 1);
    memcpy(data, str, N);
    size = N - 1;
  }

  StaticString(const String &str2)
  {
    assert(str2.size <= CAPACITY + 1);
    memcpy(data, str2.data, CAPACITY);
    size       = str2.size;
    data[size] = '\0';
  }

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

  String to_str() { return String(data, size); }
  operator String() { return to_str(); }

  u8 &operator[](u32 i)
  {
    assert(i < size);
    return data[i];
  }

  void shift_delete_range(u32 from, u32 to)
  {
    assert(from >= 0);
    assert(from < size);
    assert(to >= 0);
    assert(to <= size);

    size -= to - from;

    while (from < size) {
      data[from] = data[to];
      from++;
      to++;
    }
  }

  void shift_delete(u32 i)
  {
    assert(i >= 0);
    assert(i < size);

    shift_delete_range(i, i + 1);
  }

  void push_middle(u8 value, u32 i)
  {
    assert(i >= 0);
    assert(i <= size);

    u32 j = size;
    while (j > i) {
      data[j] = data[j - 1];
      j--;
    }
    data[i] = value;
    size++;
  }
};