#pragma once

#include "containers/array.hpp"
#include "containers/static_pool.hpp"
#include "dui/basic.hpp"
#include "dui/container.hpp"
#include "dui/group.hpp"
#include "input.hpp"
#include "math/math.hpp"
#include "string.hpp"
#include "types.hpp"

namespace Dui
{
struct DuiState {
  StaticPool<Container, 1024> containers;
  StaticPool<Group, 1024> groups;

  Array<GroupId, 1024> root_groups;
  GroupId fullscreen_group = -1;
  GroupId empty_group      = -1;

  // currently being worked on
  GroupId cg = -1;
  DuiId cw   = -1;
  DuiId cc   = -1;

  Input *input = nullptr;
  Vec2f canvas_span;

  INTERACTION_STATE(hot);
  INTERACTION_STATE(active);
  INTERACTION_STATE(dragging);
  INTERACTION_STATE(selected);

  Vec2f dragging_total_delta;
  Vec2f dragging_frame_delta;
  Vec2f dragging_start_local_position;

  i64 frame                           = 0;
  GroupId top_root_group_at_mouse_pos = -1;

  // text input state
  i32 cursor_idx          = 0;
  i32 highlight_start_idx = 0;
  f32 text_pos            = 0.f;

  GlobalDrawListData gdld;
  Array<DrawList, 20> draw_lists;  // TODO: make this dynamic
  DrawList main_dl;
  DrawList forground_dl;

  // stuff to do next frame
  struct SnapGroupArgs {
    b8 need_to = false;
    GroupId g;
    GroupId target;
    i32 axis;
    b8 dir;
  };
  SnapGroupArgs snap_group_to_snap;
  Array<GroupId, 256> groups_to_delete;
};
}  // namespace Dui