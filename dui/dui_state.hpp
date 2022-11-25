#pragma once

#include "containers/array.hpp"
#include "containers/static_pool.hpp"
#include "dui/basic.hpp"
#include "dui/group.hpp"
#include "dui/container.hpp"
#include "input.hpp"
#include "math/math.hpp"
#include "string.hpp"
#include "types.hpp"

namespace Dui
{
struct DuiState {
  StaticPool<Container, 1024> containers;
  StaticPool<Group, 1024> groups;

  Array<Group *, 1024> root_groups;
  
  Array<DrawList, 20> draw_lists;

  Group *cg     = nullptr;
  Container *cw = nullptr;
  Container *cc = nullptr;

  Input *input;
  Vec2f canvas_span;

  Group *fullscreen_group = nullptr;
  Group *empty_group      = nullptr;

  INTERACTION_STATE(hot);
  INTERACTION_STATE(active);
  INTERACTION_STATE(dragging);

  Vec2f dragging_total_delta;
  Vec2f dragging_frame_delta;

  i64 frame                          = 0;
  Group *top_root_group_at_mouse_pos = nullptr;
};
}  // namespace Dui