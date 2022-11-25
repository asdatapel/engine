#pragma once

#include "types.hpp"

template <typename T, i64 CAPACITY>
struct StaticPool {
  struct Element {
    bool assigned;
    union {
      T value = {};
      Element *next;
    };

    ~Element()
    {
      if (assigned) value.~T();
    }
  };

  const static i64 SIZE = CAPACITY;
  Element elements[SIZE];
  Element *next = nullptr, *last = nullptr;

  StaticPool()
  {
    next = elements;
    last = next;

    for (i64 i = 0; i < SIZE; i++) {
      remove(i);
    }
  }

  bool exists(i64 i) { return elements[i].assigned; }

  bool wrapped_exists(i64 i) { return elements[i % SIZE].assigned; }

  T &operator[](i64 i)
  {
    assert(i < SIZE);
    return elements[i].value;
  }

  T &wrapped_get(i64 i) { return elements[i % SIZE].value; }

  T *push_back(T value)
  {
    Element *current = next;
    if (!current) return nullptr;

    next              = current->next;
    current->value    = value;
    current->assigned = true;
    return &current->value;
  }

  T *emplace(T value, i64 index)
  {
    if (next == &elements[index]) {
      next = elements[index].next;
    }
    elements[index].value    = value;
    elements[index].assigned = true;

    return &elements[index].value;
  }

  T *emplace_wrapped(i64 index, T value) { return emplace(value, index % SIZE); }

  void remove(Element *elem)
  {
    elem->assigned = false;
    elem->next     = nullptr;
    last->next     = elem;
    last           = elem;
  }

  void remove(i64 i)
  {
    Element *elem  = &elements[i];
    elem->assigned = false;
    elem->next     = nullptr;
    last->next     = elem;
    last           = elem;
  }

  i64 index_of(T *ptr) { return ((Element *)ptr - elements); }
};
