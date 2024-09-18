#pragma once

#include "containers/array.hpp"
#include "containers/static_pool.hpp"
#include "containers/static_stack.hpp"
#include "dui/basic.hpp"
#include "dui/container.hpp"
#include "dui/group.hpp"
#include "input.hpp"
#include "math/math.hpp"
#include "string.hpp"
#include "types.hpp"

namespace Dui
{

struct Popup {
  Container container;
  RoundedRectPrimitive *outline_rect;
  RoundedRectPrimitive *background_rect;
};

struct DuiState {
  StaticPool<Container, 1024> containers;
  StaticPool<Group, 1024> groups;

  Array<GroupId, 1024> root_groups;
  GroupId fullscreen_group = -1;
  GroupId empty_group      = -1;

  DrawList dl;

  // currently being worked on
  // pointers because they don't persist between frames
  Group *cg     = nullptr;
  Container *cw = nullptr;
  StaticStack<Container*, 256> cc;

  Array<Popup, 256> popups;
  i32 started_popups_count  = 0;
  b8 clear_popups           = false;
  DuiId previously_selected = -1;

  Input *input = nullptr;

  Vec2f window_span;
  Engine::Rect canvas;
  i64 frame = 0;

  GroupId top_root_group_at_mouse_pos = -1;
  DuiId top_container_at_mouse_pos    = -1;
  b8 top_container_is_popup           = false;

  b8 menubar_visible = true;

  INTERACTION_STATE(hot);
  INTERACTION_STATE(active);
  INTERACTION_STATE(dragging);
  INTERACTION_STATE(selected);

  Vec2f dragging_total_delta;
  Vec2f dragging_frame_delta;
  Vec2f dragging_start_local_position;

  // text input state
  i32 cursor_idx          = 0;
  i32 highlight_start_idx = 0;
  f32 text_pos            = 0.f;
  StaticString<256> clipboard;

  Platform::CursorShape cursor_shape;

  // stuff to do next frame
  struct SnapGroupArgs {
    b8 need_to = false;
    GroupId g;
    GroupId target;
    i32 axis;  // -1 for center, used only for replacing the empty group
    b8 dir;
  };
  SnapGroupArgs snap_group_to_snap;
};

DuiState s;
}  // namespace Dui