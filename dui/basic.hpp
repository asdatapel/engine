#pragma once

#include "string.hpp"
#include "types.hpp"

typedef i64 DuiId;
namespace Dui
{
// http://www.cse.yorku.ca/~oz/hash.html
DuiId hash(String str)
{
  u64 hash = 5381;
  for (int i = 0; i < str.size; i++) {
    int c = str.data[i];
    hash  = ((hash << 5) + hash) + c;
  }

  return hash;
}
DuiId extend_hash(u64 hash, String str)
{
  for (int i = 0; i < str.size; i++) {
    int c = str.data[i];
    hash  = ((hash << 5) + hash) + c;
  }
  return hash;
}

#define INTERACTION_STATE(var)                     \
  DuiId var                      = -1;             \
  DuiId just_started_being_##var = -1;             \
  DuiId just_stopped_being_##var = -1;             \
  b8 was_last_control_##var      = -1;             \
  Vec2f start_position_##var     = {};             \
                                                   \
  b8 is_##var(DuiId id) { return var == id; }      \
                                                   \
  void set_##var(DuiId id)                         \
  {                                                \
    if (var != id) {                               \
      just_started_being_##var = id;               \
      start_position_##var     = input->mouse_pos; \
    }                                              \
                                                   \
    var = id;                                      \
  }                                                \
  void clear_##var(DuiId id)                       \
  {                                                \
    if (var == id) {                               \
      var                      = 0;                \
      just_stopped_being_##var = id;               \
    }                                              \
  }

#define SUB_ID(name, id) DuiId name = extend_hash(id, #name)

struct DuiState;
}  // namespace Dui