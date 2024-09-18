#pragma once

#include "types.hpp"

template <typename T, u64 CAPACITY>
struct StaticPool {
  struct Element {
    bool assigned;
    union {
      T value = {};
      u64 next;
    };

    ~Element()
    {
      if (assigned) value.~T();
    }
  };

  const static u64 SIZE = CAPACITY;
  Element elements[SIZE];
  u64 next = 0, last = 0;
  u64 size = 0;

  StaticPool() { init(); }

  StaticPool(T init_value)
  {
    init();

    for (i32 i = 0; i < SIZE; i++) {
      elements[i].value = init_value;
    }
  }

  void init()
  {
    next = 0;
    last = next;

    for (u64 i = 0; i < SIZE; i++) {
      remove(i);
    }
  }

  void operator=(StaticPool<T, CAPACITY> &other)
  {
    memcpy(elements, other.elements, SIZE * sizeof(Element));
    next = other.next;
    last = other.last;
  }

  bool exists(u64 i) { return elements[i].assigned; }

  bool wrapped_exists(u64 i) { return elements[i % SIZE].assigned; }

  T &operator[](u64 i)
  {
    assert(i < SIZE);
    return elements[i].value;
  }

  T &wrapped_get(u64 i) { return elements[i % SIZE].value; }

  T *push_back(T value)
  {
    Element *current = &elements[next];
    if (!current) return nullptr;

    next              = current->next;
    current->value    = value;
    current->assigned = true;

    size++;

    return &current->value;
  }

  T *emplace(T value, u64 index)
  {
    if (!elements[index].assigned) {
      size++;
    }

    if (next == index) {
      next = elements[index].next;
    }
    elements[index].value    = value;
    elements[index].assigned = true;

    return &elements[index].value;
  }

  T *emplace_wrapped(u64 index, T value)
  {
    return emplace(value, index % SIZE);
  }

  void remove(Element *elem) { remove(index_of(&elem->value)); }

  void remove(u64 i)
  {
    Element *elem  = &elements[i];
    elem->assigned = false;
    elem->next     = 0;

    elements[last].next = i;
    last                = i;

    size--;
  }

  u64 index_of(T *ptr) { return ((Element *)ptr - elements); }

  u64 get_size() { return size; }
};
