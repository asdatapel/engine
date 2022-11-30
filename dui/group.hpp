#pragma once

#include "containers/array.hpp"
#include "dui/basic.hpp"
#include "dui/draw.hpp"
#include "math/math.hpp"

const f32 TITLEBAR_HEIGHT               = 32;
const f32 TITLEBAR_BOTTOM_BORDER_HEIGHT = 3.f;
const f32 TAB_MARGIN                    = 6.f;
const f32 WINDOW_BORDER_SIZE            = 2.f;
const f32 WINDOW_CONTROL_WIDTH          = 20.f;
const f32 WINDOW_MARGIN_SIZE            = 4.f;
const f32 SCROLLBAR_WIDTH               = 8.f;
const f32 DOCK_CONTROLS_WIDTH           = 100.f;
const f32 LINE_GAP                      = 5.f;
const f32 CONTENT_FONT_HEIGHT           = 21.f;

namespace Dui
{
struct Group;
struct Split {
  Group *child;
  f32 div_pct = .5;

  f32 div_position;  // cache
};

struct Group {
  DuiId id      = -1;
  Group *parent = nullptr;
  Rect rect;

  // can change frame to frame
  DrawList *dl;

  // cache to prevent walking up the tree constantly
  Group *root;

  // Either split into children groups...
  i32 split_axis = -1;
  Array<Split, 32> splits;

  // or is a child node with windows
  Array<DuiId, 32> windows;
  i32 active_window_idx = -1;

  b8 is_leaf() { return windows.size > 0; }

  Rect get_titlebar_full_rect();
  Rect get_titlebar_margin_rect();
  Rect get_titlebar_bottom_border_rect();
  Rect get_titlebar_content_rect();
  Rect get_tab_margin_rect(i32 window_idx);
  i32 get_tab_at_pos(Vec2f pos);
  Rect get_border_rect();
  Rect get_window_rect();
  i32 get_window_idx(DuiId window_id);
};
}  // namespace Dui