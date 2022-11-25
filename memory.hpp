#pragma once

#include <stdlib.h>
#include <cassert>

#include "types.hpp"

struct Allocator;
struct Mem {
  u8 *data;
  i64 size;
  Allocator *allocator;
};

struct Allocator {
  virtual Mem alloc(u64 size) = 0;
  virtual void free(Mem mem)  = 0;
};

const auto sys_alloc = malloc;
const auto sys_free  = free;
struct SystemAllocator : Allocator {
  Mem alloc(u64 size) override
  {
    Mem mem;
    mem.data      = (u8 *)sys_alloc(size);
    mem.size      = size;
    mem.allocator = this;
    return mem;
  }

  void free(Mem mem) override
  {
    assert(mem.allocator == this);
    sys_free(mem.data);
  }
};

struct StackAllocator : Allocator {
  Mem base_memory;
  u8 *beg, *end, *next;

  u8 *last_allocation = nullptr;

  StackAllocator(Allocator *from, u64 size)
  {
    base_memory = from->alloc(size);
    beg         = base_memory.data;
    next        = beg;
    end         = beg + size;
  }

  ~StackAllocator() { base_memory.allocator->free(base_memory); }

  Mem alloc(u64 size) override
  {
    // assume that base_memory is already aligned, so this adds padding to the end of each
    // allocation.
    u32 max_alignment = alignof(std::max_align_t);
    u64 padded_size   = size + (max_alignment - (size % max_alignment));

    assert(next + padded_size < end);

    Mem mem;
    mem.data      = next;
    mem.size      = size;
    mem.allocator = this;

    last_allocation = next;

    next += padded_size;

    return mem;
  }

  void free(Mem mem) override { try_free(mem); }

  void try_free(Mem mem)
  {
    assert(mem.data >= beg && mem.data < end);

    if (mem.data == last_allocation) {
      next            = (u8 *)mem.data;
      last_allocation = nullptr;
    }
  }

  void force_free(Mem mem)
  {
    assert(mem.data >= beg && mem.data < end);

    next            = (u8 *)mem.data;
    last_allocation = nullptr;
  }

  void reset()
  {
    next            = beg;
    last_allocation = nullptr;
  }

  Mem get_top()
  {
    Mem mem;
    mem.data      = next;
    mem.size      = 0;
    mem.allocator = this;
    return mem;
  };
};

SystemAllocator system_allocator;
StackAllocator tmp_allocator(&system_allocator, 8 * MB);

struct Temp : Allocator {
  Mem stack_marker;

  Temp() { stack_marker = tmp_allocator.get_top(); }
  Temp(StackAllocator *stack) { stack_marker = stack->get_top(); }
  ~Temp() { stack_marker.allocator->free(stack_marker); }

  Mem alloc(u64 size) override { return stack_marker.allocator->alloc(size); }
  void free(Mem mem) override { stack_marker.allocator->free(mem); }
};
