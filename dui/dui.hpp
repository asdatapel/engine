#pragma once

#include <unordered_map>

#include "dui/basic.hpp"
#include "dui/container.hpp"
#include "dui/draw.hpp"
#include "dui/dui_state.hpp"
#include "dui/group.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "math/math.hpp"
#include "string.hpp"
#include "types.hpp"

Color d_dark  = {0, 0.13725, 0.27843, 1};
Color d       = {0, 0.2, 0.4, 1};
Color d_light = {0, 0.24706, 0.4902, 1};
Color l_light = {1, 0.55686, 0, 1};
Color l       = {0.99216, 0.46667, 0.00784, 1};
Color l_dark  = {1, 0.31373, 0.01176, 1};

#define push_text(...)  //

namespace Dui
{
DuiState s;

b8 do_hot(DuiId id, Rect rect, Group *root_group = nullptr, Container *c = nullptr, Rect clip_rect = {})
{
  b8 is_in_top_group = !s.top_root_group_at_mouse_pos || !root_group ||
                       (root_group == s.top_root_group_at_mouse_pos &&
                        in_rect(s.input->mouse_pos, root_group->rect));

  b8 is_in_clip_rect =
      clip_rect.x == 0 || in_rect(s.input->mouse_pos, clip_rect);
  b8 is_in_current_window = !c || in_rect(s.input->mouse_pos, c->rect);
  b8 is_hot = is_in_clip_rect && is_in_top_group && is_in_current_window &&
              in_rect(s.input->mouse_pos, rect);
  if (is_hot)
    s.set_hot(id);
  else
    s.clear_hot(id);

  return is_hot;
}

b8 do_active(DuiId id)
{
  // TODO handle left and right mouse buttons

  if (s.input->mouse_button_up_events[(int)MouseButton::LEFT]) {
    s.clear_active(id);
  }
  if (s.input->mouse_button_down_events[(int)MouseButton::LEFT]) {
    if (s.just_started_being_active ==
        -1) {  // only one control should be activated in a frame
      if (s.is_hot(id)) {
        s.set_active(id);
      } else {
        s.clear_active(id);
      }
    }
  }

  s.was_last_control_active = s.is_active(id);
  return s.was_last_control_active;
}

b8 do_dragging(DuiId id)
{
  // TODO handle left and right mouse buttons

  const f32 drag_start_distance = 2.f;

  const b8 active = s.is_active(id);
  if (active) {
    if ((s.input->mouse_pos - s.start_position_active).len() >
        drag_start_distance) {
      s.set_dragging(id);
    }

    // prevent dragging off screen by clamping mouse_pos
    Vec2f clamped_mouse_pos =
        clamp_to_rect(s.input->mouse_pos, {{0, 0}, s.canvas_span});
    Vec2f dragging_total_delta = clamped_mouse_pos - s.start_position_active;
    s.dragging_frame_delta     = dragging_total_delta - s.dragging_total_delta;
    s.dragging_total_delta     = dragging_total_delta;

  } else {
    s.clear_dragging(id);
  }

  s.was_last_control_dragging = s.is_dragging(id);
  return s.is_dragging(id);
}

b8 do_dragging_but_only_start_if(DuiId id, b8 condition)
{
  // TODO handle left and right mouse buttons

  const f32 drag_start_distance = 2.f;

  const b8 active = s.is_active(id);
  if (active) {
    if (condition && (s.input->mouse_pos - s.start_position_active).len() >
                         drag_start_distance) {
      s.set_dragging(id);
    }
    s.dragging_total_delta = s.input->mouse_pos - s.start_position_active;
    s.dragging_frame_delta = s.input->mouse_pos_delta;
  } else {
    s.clear_dragging(id);
  }

  s.was_last_control_dragging = s.is_dragging(id);
  return s.is_dragging(id);
}

}  // namespace Dui

namespace Dui
{

// -1 if not root
i32 get_group_z(Group *g)
{
  for (i32 i = 0; i < s.root_groups.size; i++) {
    if (s.root_groups[i] == g) return i;
  }
  return -1;
}

b8 is_empty(Group *g) { return g == s.empty_group; }

void add_root_group(Group *g)
{
  // sanity check
  assert(get_group_z(g) == -1);

  i32 index = s.root_groups.insert(0, g);
  g->parent = nullptr;
  g->root   = g;
  g->dl     = &s.draw_lists[index];
}

void remove_root_group(Group *g)
{
  i32 z = get_group_z(g);
  s.root_groups.shift_delete(z);
}

Group *create_group(Group *parent, Rect rect)
{
  Group *g = s.groups.push_back({});
  g->id    = rand();
  g->rect  = rect;

  g->parent = parent;
  if (!parent) {
    add_root_group(g);
  }

  return g;
}

void free_group(Group *g)
{
  if (!g->parent) {
    i32 z = get_group_z(g);
    s.root_groups.shift_delete(z);
  }

  i64 index = s.groups.index_of(g);
  s.groups.remove(index);
}

Container *get_container(DuiId id) { return &s.containers.wrapped_get(id); }

// walks through children of parent_group until a leaf is found at the given
// pos.
Group *get_leaf_group_at_pos(Vec2f pos, Group *parent_group)
{
  if (parent_group->is_leaf() || is_empty(parent_group)) {
    return in_rect(pos, parent_group->rect) ? parent_group : nullptr;
  }

  for (i32 i = 0; i < parent_group->splits.size; i++) {
    Group *g = parent_group->splits[i].child;

    Group *possible_result = get_leaf_group_at_pos(pos, g);
    if (possible_result) return possible_result;
  }
  return nullptr;
}

Group *get_top_leaf_group_at_pos(Vec2f pos, b8 ignore_top = false)
{
  for (i32 i = ignore_top ? 1 : 0; i < s.root_groups.size; i++) {
    if (in_rect(pos, s.root_groups[i]->rect)) {
      return get_leaf_group_at_pos(pos, s.root_groups[i]);
    }
  }
  return nullptr;
}

b8 contained_in(Group *g, Group *in)
{
  if (g == in) return true;
  if (g->parent == nullptr) return false;
  return contained_in(g->parent, in);
}

void propagate_groups(Group *g, Group *root = nullptr, Group *parent = nullptr)
{
  if (parent) g->parent = parent;
  g->root = root ? root : g;

  if (!g->is_leaf()) {
    f32 x = g->rect.x;
    f32 y = g->rect.y;

    for (i32 i = 0; i < g->splits.size; i++) {
      Split &split = g->splits[i];
      Group *child = split.child;

      if (g->split_axis == 0) {
        split.div_position = y;

        f32 height  = g->rect.height * split.div_pct;
        child->rect = {x, y, g->rect.width, height};
        y += height;
      } else {
        split.div_position = x;

        f32 width   = g->rect.width * split.div_pct;
        child->rect = {x, y, width, g->rect.height};
        x += width;
      }

      propagate_groups(child, g->root, g);
    }
  } else {
    for (i32 i = 0; i < g->windows.size; i++) {
      Container *w = get_container(g->windows[i]);
      w->parent    = g;
      w->rect      = g->get_window_rect();
    }
  }
}

// returns px
f32 get_minimum_size(Group *g, f32 g_size_px, i32 axis, b8 dir,
                     b8 commmit = false)
{
  if (g->splits.size > 0 && g->split_axis != axis) {
    f32 min_size = get_minimum_size(g->splits[0].child, g_size_px, axis, dir);
    for (i32 i = 1; i < g->splits.size; i++) {
      min_size = fmaxf(
          min_size, get_minimum_size(g->splits[i].child, g_size_px, axis, dir));
    }
    return min_size;
  } else if (g->splits.size > 0) {
    f32 non_changing_divs_size_px = 0.f;
    for (i32 i = 0; i < g->splits.size - 1; i++) {
      non_changing_divs_size_px += g_size_px * g->splits[i - dir + 1].div_pct;
    }

    Split *end_split = &g->splits[dir ? g->splits.size - 1 : 0];
    return non_changing_divs_size_px +
           get_minimum_size(end_split->child, g_size_px * end_split->div_pct,
                            axis, dir);
  }

  const f32 MINIMUM_GROUP_SIZE[2] = {TITLEBAR_HEIGHT + 15.f, 50.f};
  return MINIMUM_GROUP_SIZE[axis];
}

void resize_border_splits_and_propagate(Group *g, f32 pct_change, i32 axis,
                                        b8 dir)
{
  if (g->split_axis != axis) {
    for (i32 i = 0; i < g->splits.size; i++) {
      resize_border_splits_and_propagate(g->splits[i].child, pct_change, axis,
                                         dir);
    }
    return;
  }
  if (g->splits.size > 0) {
    for (i32 i = 0; i < g->splits.size - 1; i++) {
      g->splits[i - dir + 1].div_pct /= pct_change;
    }

    i32 changing_split_i = dir ? g->splits.size - 1 : 0;
    f32 old_value        = g->splits[changing_split_i].div_pct;
    f32 new_value        = 1 - ((1 - old_value) / pct_change);
    // really don't know why this worked
    resize_border_splits_and_propagate(g->splits[changing_split_i].child,
                                       pct_change * (new_value / old_value),
                                       axis, dir);

    g->splits[changing_split_i].div_pct = new_value;
  }
};

// FIXME parent_window and unparent_window do too much. parent should only
// parent and unparent should only unparent.
void parent_window(Group *g, DuiId window_id)
{
  Container *w = &s.containers.wrapped_get(window_id);

  if (w->parent) {
    Group *old_g = w->parent;
    old_g->windows.shift_delete(old_g->get_window_idx(w->id));
    if (old_g->active_window_idx >= old_g->windows.size) {
      old_g->active_window_idx--;
    }

    if (old_g->windows.size == 0 && !is_empty(old_g)) {
      free_group(old_g);
    }
  }

  w->parent = g;

  g->windows.push_back(window_id);
  g->active_window_idx = g->windows.size - 1;
}

Group *unparent_window(DuiId window_id)
{
  Container *w = get_container(window_id);
  Group *old_g = w->parent;

  if (!is_empty(old_g) && old_g->windows.size == 1) return old_g;

  Group *g = create_group(nullptr, old_g->rect);
  parent_window(g, window_id);

  return g;
}

Container *create_new_window(DuiId id, String name, Rect rect)
{
  Container *window = s.containers.emplace_wrapped(id, {});
  window->id        = id;
  window->title     = name;

  Group *g = create_group(nullptr, rect);
  parent_window(g, window->id);

  return window;
}

void merge_splits(Group *g)
{
  for (i32 i = 0; i < g->splits.size; i++) {
    Group *child = g->splits[i].child;
    if (!child->is_leaf() && !is_empty(child) &&
        child->split_axis == g->split_axis) {
      Split split = g->splits[i];
      g->splits.shift_delete(i);

      for (i32 child_i = 0; child_i < child->splits.size; child_i++) {
        Split new_split = child->splits[child_i];
        new_split.div_pct *= split.div_pct;
        g->splits.insert(i + child_i, new_split);

        new_split.child->parent = g;
      }

      free_group(child);
    }
  }
}

// returns true if successful.
// if any new groups are created, they will be descendents of target (i.e.
// target will never move down the hierarchy)
b8 snap_group(Group *g, Group *target, i32 axis, b8 dir,
              Group *sibling = nullptr)
{
  // target must either be a leaf node or be split on the same axis as the
  // sibling
  if (!target->is_leaf() && (sibling && target->split_axis != axis))
    return false;

  // try adding it to the parent first
  if (target->parent && snap_group(g, target->parent, axis, dir, target))
    return true;

  if (target->is_leaf()) {
    // create a new group to move the windows to
    Group *new_g             = create_group(target, target->rect);
    new_g->windows           = target->windows;
    new_g->active_window_idx = target->active_window_idx;
    if (s.empty_group == target) s.empty_group = new_g;

    target->windows.clear();
    target->split_axis = axis;

    Split orig_split;
    orig_split.child   = new_g;
    orig_split.div_pct = .5f;
    target->splits.push_back(orig_split);

    Split new_split;
    new_split.child   = g;
    new_split.div_pct = .5f;
    target->splits.insert(dir ? 1 : 0, new_split);

  } else if (target->split_axis != axis) {  // already split, but new axis
    assert(!sibling);

    // move original children to a new group
    Group *new_g      = create_group(target, target->rect);
    new_g->split_axis = target->split_axis;
    new_g->splits     = target->splits;
    for (i32 i = 0; i < new_g->splits.size; i++) {
      Group *target = new_g->splits[i].child;
    }
    if (s.empty_group == target) s.empty_group = new_g;

    target->splits.clear();

    Split orig_split;
    orig_split.child   = new_g;
    orig_split.div_pct = .5f;
    target->splits.push_back(orig_split);

    Split new_split;
    new_split.child   = g;
    new_split.div_pct = .5f;
    target->splits.insert(dir ? 1 : 0, new_split);

    target->split_axis = axis;
  } else if (sibling) {  // already split, same axis, in the middle
    i32 sibling_idx = -1;
    for (i32 i = 0; i < target->splits.size; i++) {
      if (target->splits[i].child == sibling) {
        sibling_idx = i;
        break;
      }
    }
    assert(sibling_idx > -1);

    f32 half_size = target->splits[sibling_idx].div_pct / 2;
    target->splits[sibling_idx].div_pct = half_size;

    Split new_split;
    new_split.child   = g;
    new_split.div_pct = half_size;
    target->splits.insert(dir ? sibling_idx + 1 : sibling_idx, new_split);
  } else {  // already split, same axis, at the ends
    f32 new_split_pct = 1.f / (target->splits.size + 1);

    for (i32 i = 0; i < target->splits.size; i++) {
      target->splits[i].div_pct *= 1 - new_split_pct;
    }

    Split new_split;
    new_split.child   = g;
    new_split.div_pct = new_split_pct;
    target->splits.insert(dir ? target->splits.size : 0, new_split);
  }

  if (!g->parent) {
    remove_root_group(g);
  }
  g->parent = target;

  merge_splits(target);

  return true;
}

Group *unsnap_group(Group *g)
{
  assert(g->is_leaf());

  if (is_empty(g)) {
    // move all windows to a new group so the empty stays where it is.
    Group *new_g             = create_group(nullptr, g->rect);
    new_g->windows           = g->windows;
    new_g->active_window_idx = g->active_window_idx;

    g->windows.clear();
    g->active_window_idx = -1;
    return new_g;
  }

  if (!g->parent) return g;
  Group *parent_g = g->parent;

  DuiId idx;
  f32 free_space;
  for (i32 i = 0; i < parent_g->splits.size; i++) {
    Split split = parent_g->splits[i];
    if (split.child == g) {
      idx        = i;
      free_space = split.div_pct;
      break;
    }
  }
  parent_g->splits.shift_delete(idx);

  for (i32 i = 0; i < parent_g->splits.size; i++) {
    parent_g->splits[i].div_pct *= 1 / (1 - free_space);
  }

  if (parent_g->splits.size == 1) {
    Group *child                = parent_g->splits[0].child;
    parent_g->split_axis        = child->split_axis;
    parent_g->splits            = child->splits;
    parent_g->windows           = child->windows;
    parent_g->active_window_idx = child->active_window_idx;
    if (is_empty(child)) s.empty_group = parent_g;

    free_group(child);

    if (parent_g->parent) {
      merge_splits(parent_g->parent);
    }
  }

  add_root_group(g);

  return g;
}

void combine_leaf_groups(Group *target, Group *src)
{
  assert(target->is_leaf() || is_empty(target));
  assert(src->is_leaf());

  while (src->windows.size > 0) {
    parent_window(target, src->windows[0]);
  }
}

// input group might get destroyed, so returns the new one
Group *handle_dragging_group(Group *g, DuiId id)
{
  // _pos = 0 for left/top, 1 for middle, 2 for right/bottom
  auto snap_in_rect = [](Vec2f span, Rect container, i32 x_pos,
                         i32 y_pos) -> Rect {
    Rect rect;
    rect.width  = span.x;
    rect.height = span.y;

    if (x_pos == 0) {
      rect.x = container.x;
    } else if (x_pos == 1) {
      rect.x = container.x + (container.width / 2.f) - (rect.width / 2.f);
    } else {
      rect.x = container.x + container.width - rect.width;
    }

    if (y_pos == 0) {
      rect.y = container.y;
    } else if (y_pos == 1) {
      rect.y = container.y + (container.height / 2.f) - (rect.height / 2.f);
    } else {
      rect.y = container.y + container.height - rect.height;
    }

    return rect;
  };

  auto do_snap_control = [](DuiId control_id, DuiId dragging_id, Rect rect,
                            Group *g, Group *target_group, i32 axis,
                            b8 dir) -> b8 {
    DuiId hot = do_hot(control_id, rect);
    if (hot)
      push_rect(&forground_dl, rect, l_dark);
    else
      push_rect(&forground_dl, rect, l);

    if (hot) {
      Rect preview_rect = target_group->rect;
      if (axis == 0) {
        preview_rect.height /= 2;
        if (dir) preview_rect.y += preview_rect.height;
      } else {
        preview_rect.width /= 2;
        if (dir) preview_rect.x += preview_rect.width;
      }

      push_rect(&forground_dl, preview_rect, {1, 1, 1, .5});  // preview
      if (s.just_stopped_being_dragging == dragging_id) {
        snap_group(g, target_group, axis, dir);
        return true;
      }
    }

    return false;
  };

  if (s.fullscreen_group == g) return g;

  Group *target_group = get_top_leaf_group_at_pos(s.input->mouse_pos, true);

  if (target_group && !contained_in(target_group, g)) {
    Rect window_rect = target_group->get_window_rect();

    f32 dock_controls_width = DOCK_CONTROLS_WIDTH;
    dock_controls_width = fminf(dock_controls_width, window_rect.width * .75f);
    dock_controls_width = fminf(dock_controls_width, window_rect.height * .75f);

    f32 three_fifth_dock_controls_width = dock_controls_width / 2.f;
    f32 fifth_dock_controls_width       = dock_controls_width / 5.f;
    Vec2f horizontal_button_span        = {three_fifth_dock_controls_width,
                                           fifth_dock_controls_width};
    Vec2f vertical_button_span          = {fifth_dock_controls_width,
                                           three_fifth_dock_controls_width};

    if (target_group->root != target_group) {
      Rect root_window_rect = target_group->root->get_window_rect();

      SUB_ID(left_root_dock_control_id, id);
      SUB_ID(right_root_dock_control_id, id);
      SUB_ID(top_root_dock_control_id, id);
      SUB_ID(bottom_root_dock_control_id, id);
      Rect left_root_dock_control_rect =
          snap_in_rect(vertical_button_span, root_window_rect, 0, 1);
      Rect right_root_dock_control_rect =
          snap_in_rect(vertical_button_span, root_window_rect, 2, 1);
      Rect top_root_dock_control_rect =
          snap_in_rect(horizontal_button_span, root_window_rect, 1, 0);
      Rect bottom_root_dock_control_rect =
          snap_in_rect(horizontal_button_span, root_window_rect, 1, 2);
      if (do_snap_control(left_root_dock_control_id, id,
                          left_root_dock_control_rect, g, target_group->root, 1,
                          false) ||
          do_snap_control(right_root_dock_control_id, id,
                          right_root_dock_control_rect, g, target_group->root,
                          1, true) ||
          do_snap_control(top_root_dock_control_id, id,
                          top_root_dock_control_rect, g, target_group->root, 0,
                          false) ||
          do_snap_control(bottom_root_dock_control_id, id,
                          bottom_root_dock_control_rect, g, target_group->root,
                          0, true)) {
        return g;
      }
    }

    Rect dock_control_rect = snap_in_rect(
        {dock_controls_width, dock_controls_width}, window_rect, 1, 1);

    SUB_ID(left_dock_control_id, id);
    SUB_ID(right_dock_control_id, id);
    SUB_ID(top_dock_control_id, id);
    SUB_ID(bottom_dock_control_id, id);
    Rect left_dock_control_rect =
        snap_in_rect(vertical_button_span, dock_control_rect, 0, 1);
    Rect right_dock_control_rect =
        snap_in_rect(vertical_button_span, dock_control_rect, 2, 1);
    Rect top_dock_control_rect =
        snap_in_rect(horizontal_button_span, dock_control_rect, 1, 0);
    Rect bottom_dock_control_rect =
        snap_in_rect(horizontal_button_span, dock_control_rect, 1, 2);
    if (do_snap_control(left_dock_control_id, id, left_dock_control_rect, g,
                        target_group, 1, false) ||
        do_snap_control(right_dock_control_id, id, right_dock_control_rect, g,
                        target_group, 1, true) ||
        do_snap_control(top_dock_control_id, id, top_dock_control_rect, g,
                        target_group, 0, false) ||
        do_snap_control(bottom_dock_control_id, id, bottom_dock_control_rect, g,
                        target_group, 0, true)) {
      return g;
    }

    if (g->is_leaf() &&
        in_rect(s.input->mouse_pos, target_group->get_titlebar_full_rect())) {
      push_rect(&forground_dl, window_rect, {1, 1, 1, .5});  // preview
      if (s.just_stopped_being_dragging == id) {
        combine_leaf_groups(target_group, g);
        return target_group;
      }
    }

    if (target_group == s.fullscreen_group &&
        s.fullscreen_group == s.empty_group &&
        in_rect(s.input->mouse_pos, target_group->get_titlebar_full_rect())) {
      push_rect(&forground_dl, window_rect, {1, 1, 1, .5});  // preview
      if (s.just_stopped_being_dragging == id) {
        Group *first_leaf_node = g;
        while (!first_leaf_node->is_leaf()) {
          first_leaf_node = first_leaf_node->splits[0].child;
        }
        s.empty_group = first_leaf_node;

        free_group(s.fullscreen_group);
        s.fullscreen_group = g;

        // move new fullscreen group to the back
        i32 fullscreen_group_z = get_group_z(g);
        s.root_groups.shift_delete(fullscreen_group_z);
        s.root_groups.push_back(g);

        return g;
      }
    }
  }

  g->rect.x += s.dragging_frame_delta.x;
  g->rect.y += s.dragging_frame_delta.y;

  return g;
}

}  // namespace Dui

namespace Dui
{

void draw_group_and_children(Group *g)
{
  if (!g->is_leaf()) {
    for (i32 i = 0; i < g->splits.size; i++) {
      draw_group_and_children(g->splits[i].child);
    }
    return;
  }

  Rect titlebar_rect               = g->get_titlebar_full_rect();
  Rect titlebar_bottom_border_rect = g->get_titlebar_bottom_border_rect();
  Rect group_border_rect           = g->get_border_rect();
  Rect window_rect                 = g->get_window_rect();

  push_rect(g->root->dl, titlebar_rect, d);
  push_rect(g->root->dl, titlebar_bottom_border_rect, d_light);
  push_rect(g->root->dl, group_border_rect, d);
  push_rect(g->root->dl, window_rect, d_dark);

  if (is_empty(g)) {
    push_rect(g->root->dl, titlebar_rect, {1, 0, 0, 1});
  }

  Rect titlebar_content_rect = g->get_titlebar_content_rect();

  for (i32 w_i = 0; w_i < g->windows.size; w_i++) {
    DuiId window_id = g->windows[w_i];
    Container *w    = get_container(window_id);

    Rect tab_rect = g->get_tab_margin_rect(g->get_window_idx(window_id));

    Color tab_color = d_dark;
    if (w_i == g->active_window_idx) {
      tab_color = d_light;
    }
    push_rect(g->root->dl, tab_rect, tab_color);
    push_text(g->root->dl, &font, w->title, tab_rect.xy(), {1, 1, 1, 1},
              tab_rect.height);
  }
}

void start_frame_for_leaf(Group *g)
{
  assert(g->active_window_idx < g->windows.size);
  DuiId active_window_id = g->windows[g->active_window_idx];

  for (i32 w_i = 0; w_i < g->windows.size; w_i++) {
    DuiId window_id = g->windows[w_i];
    Container *w    = get_container(window_id);

    i32 window_idx = g->get_window_idx(window_id);
    Rect tab_rect  = g->get_tab_margin_rect(window_idx);

    SUB_ID(tab_handle_id, window_id);
    DuiId tab_handle_hot      = do_hot(tab_handle_id, tab_rect, g);
    DuiId tab_handle_active   = do_active(tab_handle_id);
    DuiId tab_handle_dragging = do_dragging(tab_handle_id);
    if (tab_handle_active) {
      g->active_window_idx = w_i;
    }
    if (tab_handle_dragging) {
      if (g->windows.size == 1 ||
          !in_rect(s.input->mouse_pos, g->get_titlebar_full_rect())) {
        g = unparent_window(window_id);
        g = unsnap_group(g);
        g = handle_dragging_group(g, tab_handle_id);
      } else {
        i32 target_tab_idx = g->get_tab_at_pos(s.input->mouse_pos);
        DuiId tmp          = g->windows[target_tab_idx];
        g->windows.shift_delete(window_idx);
        g->windows.insert(target_tab_idx, w->id);
        g->active_window_idx = target_tab_idx;
      }
    }
  }

  Rect titlebar_rect = g->get_titlebar_margin_rect();
  SUB_ID(root_handle_id, active_window_id);
  DuiId root_handle_hot      = do_hot(root_handle_id, titlebar_rect, g);
  DuiId root_handle_active   = do_active(root_handle_id);
  DuiId root_handle_dragging = do_dragging(root_handle_id);
  if (root_handle_dragging) {
    if (s.input->keys[(i32)Keys::LCTRL]) {
      g = unsnap_group(g);
      g = handle_dragging_group(g, root_handle_id);
    } else {
      Group *root_g = g;
      while (root_g->parent) {
        root_g = root_g->parent;
      }
      root_g = handle_dragging_group(root_g, root_handle_id);
    }
  }
}

void start_frame_for_group(Group *g)
{
  const f32 RESIZE_HANDLES_OVERSIZE = 2.f;

  s.cg = g;

  if (!g->parent) {
    // only do movement/resizing logic if window is not fullscreen
    if (s.fullscreen_group != g) {
      // easy keyboard-based window manipulation
      if (s.input->keys[(i32)Keys::LALT]) {
        SUB_ID(root_controller_control_id, g->id);
        DuiId root_controller_control_hot =
            do_hot(root_controller_control_id, g->rect, g);
        DuiId root_controller_control_active =
            do_active(root_controller_control_id);
        DuiId root_controller_control_dragging =
            do_dragging(root_controller_control_id);
        if (root_controller_control_dragging) {
          g->rect.x += s.dragging_frame_delta.x;
          g->rect.y += s.dragging_frame_delta.y;
        }
      }

      // resize handles that extend out of group
      {
        const f32 MINIMUM_ROOT_WIDTH  = 50.f;
        const f32 MINIMUM_ROOT_HEIGHT = 75.f;

        Rect left_top_handle_rect;
        left_top_handle_rect.x = g->rect.x - RESIZE_HANDLES_OVERSIZE;
        left_top_handle_rect.y = g->rect.y - RESIZE_HANDLES_OVERSIZE;
        left_top_handle_rect.width =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        left_top_handle_rect.height =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        SUB_ID(left_top_handle_id, g->id);
        DuiId left_top_handle_hot =
            do_hot(left_top_handle_id, left_top_handle_rect, g);
        DuiId left_top_handle_active   = do_active(left_top_handle_id);
        DuiId left_top_handle_dragging = do_dragging(left_top_handle_id);
        if (left_top_handle_hot || left_top_handle_dragging) {
          push_rect(&forground_dl, left_top_handle_rect, {1, 1, 1, 1});
        }
        if (left_top_handle_dragging) {
          g->rect.x += s.dragging_frame_delta.x;
          g->rect.y += s.dragging_frame_delta.y;
          g->rect.width -= s.dragging_frame_delta.x;
          g->rect.height -= s.dragging_frame_delta.y;

          if (g->rect.width < MINIMUM_ROOT_WIDTH) {
            g->rect.x -= MINIMUM_ROOT_WIDTH - g->rect.width;
            g->rect.width = MINIMUM_ROOT_WIDTH;
          }
          if (g->rect.height < MINIMUM_ROOT_HEIGHT) {
            g->rect.y -= MINIMUM_ROOT_HEIGHT - g->rect.height;
            g->rect.height = MINIMUM_ROOT_HEIGHT;
          }
        }

        Rect right_top_handle_rect;
        right_top_handle_rect.x =
            g->rect.x + g->rect.width - WINDOW_BORDER_SIZE - WINDOW_MARGIN_SIZE;

        right_top_handle_rect.y = g->rect.y - RESIZE_HANDLES_OVERSIZE;
        right_top_handle_rect.width =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        right_top_handle_rect.height =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        SUB_ID(right_top_handle_id, g->id);
        DuiId right_top_handle_hot =
            do_hot(right_top_handle_id, right_top_handle_rect, g);
        DuiId right_top_handle_active   = do_active(right_top_handle_id);
        DuiId right_top_handle_dragging = do_dragging(right_top_handle_id);
        if (right_top_handle_hot || right_top_handle_dragging) {
          push_rect(&forground_dl, right_top_handle_rect, {1, 1, 1, 1});
        }
        if (right_top_handle_dragging) {
          g->rect.width += s.dragging_frame_delta.x;
          g->rect.y += s.dragging_frame_delta.y;
          g->rect.height -= s.dragging_frame_delta.y;

          if (g->rect.width < MINIMUM_ROOT_WIDTH) {
            g->rect.width = MINIMUM_ROOT_WIDTH;
          }
          if (g->rect.height < MINIMUM_ROOT_HEIGHT) {
            g->rect.y -= MINIMUM_ROOT_HEIGHT - g->rect.height;
            g->rect.height = MINIMUM_ROOT_HEIGHT;
          }
        }

        Rect left_bottom_handle_rect;
        left_bottom_handle_rect.x = g->rect.x - RESIZE_HANDLES_OVERSIZE;
        left_bottom_handle_rect.y = g->rect.y + g->rect.height -
                                    WINDOW_BORDER_SIZE - WINDOW_MARGIN_SIZE;
        left_bottom_handle_rect.width =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        left_bottom_handle_rect.height =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        SUB_ID(left_bottom_handle_id, g->id);
        DuiId left_bottom_handle_hot =
            do_hot(left_bottom_handle_id, left_bottom_handle_rect, g);
        DuiId left_bottom_handle_active   = do_active(left_bottom_handle_id);
        DuiId left_bottom_handle_dragging = do_dragging(left_bottom_handle_id);
        if (left_bottom_handle_hot || left_bottom_handle_dragging) {
          push_rect(&forground_dl, left_bottom_handle_rect, {1, 1, 1, 1});
        }
        if (left_bottom_handle_dragging) {
          g->rect.x += s.dragging_frame_delta.x;
          g->rect.width -= s.dragging_frame_delta.x;
          g->rect.height += s.dragging_frame_delta.y;

          if (g->rect.width < MINIMUM_ROOT_WIDTH) {
            g->rect.x -= MINIMUM_ROOT_WIDTH - g->rect.width;
            g->rect.width = MINIMUM_ROOT_WIDTH;
          }
          if (g->rect.height < MINIMUM_ROOT_HEIGHT) {
            g->rect.height = MINIMUM_ROOT_HEIGHT;
          }
        }

        Rect right_bottom_handle_rect;
        right_bottom_handle_rect.x =
            g->rect.x + g->rect.width - WINDOW_BORDER_SIZE - WINDOW_MARGIN_SIZE;
        right_bottom_handle_rect.y = g->rect.y + g->rect.height -
                                     WINDOW_BORDER_SIZE - WINDOW_MARGIN_SIZE;
        right_bottom_handle_rect.width =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        right_bottom_handle_rect.height =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        SUB_ID(right_bottom_handle_id, g->id);
        DuiId right_bottom_handle_hot =
            do_hot(right_bottom_handle_id, right_bottom_handle_rect, g);
        DuiId right_bottom_handle_active = do_active(right_bottom_handle_id);
        DuiId right_bottom_handle_dragging =
            do_dragging(right_bottom_handle_id);
        if (right_bottom_handle_hot || right_bottom_handle_dragging) {
          push_rect(&forground_dl, right_bottom_handle_rect, {1, 1, 1, 1});
        }
        if (right_bottom_handle_dragging) {
          g->rect.width += s.dragging_frame_delta.x;
          g->rect.height += s.dragging_frame_delta.y;
          if (g->rect.width < MINIMUM_ROOT_WIDTH) {
            g->rect.width = MINIMUM_ROOT_WIDTH;
          }
          if (g->rect.height < MINIMUM_ROOT_HEIGHT) {
            g->rect.height = MINIMUM_ROOT_HEIGHT;
          }
        }

        Rect left_handle_rect;
        left_handle_rect.x = g->rect.x - RESIZE_HANDLES_OVERSIZE;
        left_handle_rect.y = g->rect.y;
        left_handle_rect.width =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        left_handle_rect.height = g->rect.height;
        SUB_ID(left_handle_id, g->id);
        DuiId left_handle_hot      = do_hot(left_handle_id, left_handle_rect, g);
        DuiId left_handle_active   = do_active(left_handle_id);
        DuiId left_handle_dragging = do_dragging(left_handle_id);
        if (left_handle_hot || left_handle_dragging) {
          push_rect(&forground_dl, left_handle_rect, {1, 1, 1, 1});
        }
        if (left_handle_dragging) {
          g->rect.x += s.dragging_frame_delta.x;
          g->rect.width -= s.dragging_frame_delta.x;
          if (g->rect.width < MINIMUM_ROOT_WIDTH) {
            g->rect.x -= MINIMUM_ROOT_WIDTH - g->rect.width;
            g->rect.width = MINIMUM_ROOT_WIDTH;
          }
        }

        Rect right_handle_rect;
        right_handle_rect.x =
            g->rect.x + g->rect.width - WINDOW_BORDER_SIZE - WINDOW_MARGIN_SIZE;
        right_handle_rect.y = g->rect.y;
        right_handle_rect.width =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        right_handle_rect.height = g->rect.height;
        SUB_ID(right_handle_id, g->id);
        DuiId right_handle_hot    = do_hot(right_handle_id, right_handle_rect, g);
        DuiId right_handle_active = do_active(right_handle_id);
        DuiId right_handle_dragging = do_dragging(right_handle_id);
        if (right_handle_hot || right_handle_dragging) {
          push_rect(&forground_dl, right_handle_rect, {1, 1, 1, 1});
        }
        if (right_handle_dragging) {
          g->rect.width += s.dragging_frame_delta.x;
          if (g->rect.width < MINIMUM_ROOT_WIDTH) {
            g->rect.width = MINIMUM_ROOT_WIDTH;
          }
        }

        Rect top_handle_rect;
        top_handle_rect.x     = g->rect.x;
        top_handle_rect.y     = g->rect.y - RESIZE_HANDLES_OVERSIZE;
        top_handle_rect.width = g->rect.width;
        top_handle_rect.height =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        SUB_ID(top_handle_id, g->id);
        DuiId top_handle_hot      = do_hot(top_handle_id, top_handle_rect, g);
        DuiId top_handle_active   = do_active(top_handle_id);
        DuiId top_handle_dragging = do_dragging(top_handle_id);
        if (top_handle_hot || top_handle_dragging) {
          push_rect(&forground_dl, top_handle_rect, {1, 1, 1, 1});
        }
        if (top_handle_dragging) {
          g->rect.y += s.dragging_frame_delta.y;
          g->rect.height -= s.dragging_frame_delta.y;
          if (g->rect.height < MINIMUM_ROOT_HEIGHT) {
            g->rect.y -= MINIMUM_ROOT_HEIGHT - g->rect.height;
            g->rect.height = MINIMUM_ROOT_HEIGHT;
          }
        }

        Rect bottom_handle_rect;
        bottom_handle_rect.x = g->rect.x;
        bottom_handle_rect.y = g->rect.y + g->rect.height - WINDOW_BORDER_SIZE -
                               WINDOW_MARGIN_SIZE;
        bottom_handle_rect.width = g->rect.width;
        bottom_handle_rect.height =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        SUB_ID(bottom_handle_id, g->id);
        DuiId bottom_handle_hot = do_hot(bottom_handle_id, bottom_handle_rect, g);
        DuiId bottom_handle_active   = do_active(bottom_handle_id);
        DuiId bottom_handle_dragging = do_dragging(bottom_handle_id);
        if (bottom_handle_hot || bottom_handle_dragging) {
          push_rect(&forground_dl, bottom_handle_rect, {1, 1, 1, 1});
        }
        if (bottom_handle_dragging) {
          g->rect.height += s.dragging_frame_delta.y;
          if (g->rect.height < MINIMUM_ROOT_HEIGHT) {
            g->rect.height = MINIMUM_ROOT_HEIGHT;
          }
        }
      }

      // either the right or left window control should be fully in the canvas
      {
        Rect titlebar_content_rect = g->get_titlebar_content_rect();

        Rect left_window_control  = titlebar_content_rect;
        left_window_control.width = WINDOW_CONTROL_WIDTH;

        Rect right_window_control = titlebar_content_rect;
        right_window_control.x    = titlebar_content_rect.x +
                                 titlebar_content_rect.width -
                                 WINDOW_CONTROL_WIDTH;
        right_window_control.width = WINDOW_CONTROL_WIDTH;

        if (left_window_control.x + left_window_control.width >
            s.canvas_span.x) {
          g->rect.x -= (left_window_control.x + left_window_control.width) -
                       s.canvas_span.x;
        }
        if (left_window_control.y + left_window_control.height >
            s.canvas_span.y) {
          g->rect.y -= (left_window_control.y + left_window_control.height) -
                       s.canvas_span.y;
        }
        if (right_window_control.x < 0) {
          g->rect.x -= right_window_control.x;
        }
        if (right_window_control.y < 0) {
          g->rect.y -= right_window_control.y;
        }
      }
    }
  }

  if (g->is_leaf()) {
    start_frame_for_leaf(g);
    return;
  }

  for (i32 i = 1; i < g->splits.size; i++) {
    if (g->split_axis == 0) {
      Rect split_move_handle;
      split_move_handle.x = g->rect.x;
      split_move_handle.y =
          g->splits[i].div_position - WINDOW_BORDER_SIZE - WINDOW_MARGIN_SIZE;
      split_move_handle.width  = g->rect.width;
      split_move_handle.height = 2 * (WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE);
      SUB_ID(split_move_handle_id, g->id + i);
      DuiId split_move_handle_hot =
          do_hot(split_move_handle_id, split_move_handle, g);
      DuiId split_move_handle_active   = do_active(split_move_handle_id);
      DuiId split_move_handle_dragging = do_dragging(split_move_handle_id);
      if (split_move_handle_dragging) {
        f32 delta_px = s.input->mouse_pos.y - split_move_handle.y;

        f32 min_px_up =
            get_minimum_size(g->splits[i - 1].child,
                             g->splits[i - 1].div_pct * g->rect.height, 0, 1);
        f32 min_px_down = get_minimum_size(
            g->splits[i].child, g->splits[i].div_pct * g->rect.height, 0, 0);

        b8 too_far_up =
            min_px_up > g->splits[i - 1].child->rect.height + delta_px;
        b8 too_far_down =
            min_px_down > g->splits[i].child->rect.height - delta_px;
        if (too_far_up && too_far_down) {
          break;
        } else if (too_far_up) {
          delta_px = min_px_up - g->splits[i - 1].child->rect.height;
        } else if (too_far_down) {
          delta_px = -(min_px_down - g->splits[i].child->rect.height);
        }

        f32 move_pct = delta_px / g->rect.height;

        f32 up_pct_change =
            (g->splits[i - 1].div_pct + move_pct) / g->splits[i - 1].div_pct;
        f32 down_pct_change =
            (g->splits[i].div_pct - move_pct) / g->splits[i].div_pct;

        resize_border_splits_and_propagate(g->splits[i - 1].child,
                                           up_pct_change, 0, 1);
        resize_border_splits_and_propagate(g->splits[i].child, down_pct_change,
                                           0, 0);
        g->splits[i - 1].div_pct *= up_pct_change;
        g->splits[i].div_pct *= down_pct_change;
      }
    } else if (g->split_axis == 1) {
      Rect split_move_handle;
      split_move_handle.x =
          g->splits[i].div_position - WINDOW_BORDER_SIZE - WINDOW_MARGIN_SIZE;
      split_move_handle.y      = g->rect.y;
      split_move_handle.width  = 2 * (WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE);
      split_move_handle.height = g->rect.height;
      SUB_ID(split_move_handle_id, g->id + i);
      DuiId split_move_handle_hot =
          do_hot(split_move_handle_id, split_move_handle, g);
      DuiId split_move_handle_active   = do_active(split_move_handle_id);
      DuiId split_move_handle_dragging = do_dragging(split_move_handle_id);
      if (split_move_handle_dragging) {
        f32 delta_px = s.input->mouse_pos.x - split_move_handle.x;

        f32 min_px_left =
            get_minimum_size(g->splits[i - 1].child,
                             g->splits[i - 1].div_pct * g->rect.width, 1, 1);
        f32 min_px_right = get_minimum_size(
            g->splits[i].child, g->splits[i].div_pct * g->rect.width, 1, 0);

        b8 too_far_left =
            min_px_left > g->splits[i - 1].child->rect.width + delta_px;
        b8 too_far_right =
            min_px_right > g->splits[i].child->rect.width - delta_px;
        if (too_far_left && too_far_right) {
          break;
        } else if (too_far_left) {
          delta_px = min_px_left - g->splits[i - 1].child->rect.width;
        } else if (too_far_right) {
          delta_px = -(min_px_right - g->splits[i].child->rect.width);
        }

        f32 move_pct = delta_px / g->rect.width;

        f32 left_pct_change =
            (g->splits[i - 1].div_pct + move_pct) / g->splits[i - 1].div_pct;
        f32 right_pct_change =
            (g->splits[i].div_pct - move_pct) / g->splits[i].div_pct;

        resize_border_splits_and_propagate(g->splits[i - 1].child,
                                           left_pct_change, 1, 1);
        resize_border_splits_and_propagate(g->splits[i].child, right_pct_change,
                                           1, 0);
        g->splits[i - 1].div_pct *= left_pct_change;
        g->splits[i].div_pct *= right_pct_change;
      }
    }
  }

  s.cg = nullptr;

  for (i32 i = 0; i < g->splits.size; i++) {
    start_frame_for_group(g->splits[i].child);
  }

  // after handling children, honor any contraints here
  if (s.fullscreen_group == g) {
    g->rect.x      = 0;
    g->rect.y      = 0;
    g->rect.width  = s.canvas_span.x;
    g->rect.height = s.canvas_span.y;
  }
}

void start_frame(Input *input, Vec2f canvas_size)
{
  s.input       = input;
  s.canvas_span = canvas_size;

  s.frame++;

  s.just_started_being_hot      = -1;
  s.just_stopped_being_hot      = -1;
  s.just_started_being_active   = -1;
  s.just_stopped_being_active   = -1;
  s.just_started_being_dragging = -1;
  s.just_stopped_being_dragging = -1;

  clear_draw_list(&forground_dl);
  clear_draw_list(&main_dl);
  for (i32 i = 0; i < s.draw_lists.size; i++) {
    clear_draw_list(&s.draw_lists[i]);
  }

  // figure out the top groups first
  s.top_root_group_at_mouse_pos     = nullptr;
  i32 top_root_group_at_mouse_pos_z = -1;
  for (i32 i = 0; i < s.root_groups.size; i++) {
    if (in_rect(s.input->mouse_pos, s.root_groups[i]->rect)) {
      s.top_root_group_at_mouse_pos = s.root_groups[i];
      top_root_group_at_mouse_pos_z = i;
      break;
    }
  }
  if (s.input->mouse_button_down_events[(i32)MouseButton::LEFT]) {
    if (top_root_group_at_mouse_pos_z > -1) {
      Group *g = s.root_groups[top_root_group_at_mouse_pos_z];
      if (s.fullscreen_group != g) {
        s.root_groups.shift_delete(top_root_group_at_mouse_pos_z);
        s.root_groups.insert(0, g);
      }
    }
  }

  // one pass for input handling
  for (i32 i = 0; i < s.root_groups.size; i++) {
    Group *g = s.root_groups[i];
    start_frame_for_group(g);
  }

  // one pass to propagate changes
  for (i32 i = 0; i < s.root_groups.size; i++) {
    Group *g = s.root_groups[i];
    propagate_groups(g);
  }

  // one pass for drawing, in reverse z order
  for (i32 i = s.root_groups.size - 1; i >= 0; i--) {
    Group *g = s.root_groups[i];
    g->dl    = &s.draw_lists[i];
    draw_group_and_children(g);
  }
}

DuiId start_window(String name, Rect initial_rect)
{
  DuiId id = hash(name);

  if (s.cw) {
    warning("starting a window without ending another!");
    return id;
  }

  Container *w = s.containers.wrapped_exists(id)
                     ? &s.containers.wrapped_get(id)
                     : create_new_window(id, name, initial_rect);
  w->start_frame(&s, s.input, s.frame);

  s.cw = w;

  return id;
}

void end_window()
{
  if (s.cw) {
    s.cc->parent->root->dl->pop_scissor();

    s.cc = nullptr;
    s.cw = nullptr;
  } else {
    warning("end_window without start_window");
  }
}

}  // namespace Dui

namespace Dui
{

void set_window_color(Color color)
{
  Container *c = get_current_container(&s);
  if (c) {
    c->color = color;
  }
}

void next_line()
{
  Container *c = get_current_container(&s);
  c->next_line();
}

b8 button(DuiId me, String text)
{
  Container *c = get_current_container(&s);
  if (!c) return false;

  // f32 border     = 5;
  // f32 text_width = get_text_width(text, state.style.content_font_size);
  // Rect rect = c->place({text_width + border * 2,
  // state.style.content_font_size + border * 2});

  // b8 hot       = do_hot(me, rect, c->rect);
  // b8 active    = do_active(me);
  // b8 triggered = (state.just_deactivated == me && hot);

  // Color color   = {.9, .3, .2, 1};
  // Color light   = lighten(color, .1);
  // Color lighter = lighten(color, .15);
  // Color dark    = darken(color, .1);
  // Color darker  = darken(color, .15);

  // Color top   = lighter;
  // Color down  = darker;
  // Color left  = light;
  // Color right = dark;

  // f32 text_y_offset = 0;
  // if (active) {
  //   color = darken(color, 0.05f);

  //   top   = darker;
  //   down  = lighter;
  //   left  = dark;
  //   right = light;

  //   text_y_offset = 3.f;
  // } else if (hot) {
  //   color = lighten(color, .05f);
  // }

  // f32 x1 = rect.x, x2 = rect.x + border, x3 = rect.x + rect.width - border,
  //     x4 = rect.x + rect.width;

  // f32 y1 = rect.y, y2 = rect.y + border, y3 = rect.y + rect.height - border,
  //     y4 = rect.y + rect.height;

  // c->draw_list->push_quad({x4, y1}, {x3, y2}, {x2, y2}, {x1, y1}, top);
  // c->draw_list->push_quad({x4, y4}, {x1, y4}, {x2, y3}, {x3, y3}, down);
  // c->draw_list->push_quad({x1, y4}, {x2, y3}, {x2, y2}, {x1, y1}, left);
  // c->draw_list->push_quad({x4, y4}, {x3, y3}, {x3, y2}, {x4, y1}, right);
  // c->draw_list->push_quad({x2, y2},  // center
  //                         {x2, y3}, {x3, y3}, {x3, y2}, color);
  // c->draw_list->push_text(text, {rect.x + border, rect.y + border +
  // text_y_offset},
  //                         state.style.content_font_size,
  //                         state.style.content_highlighted_text_color);

  // return triggered;
  return false;
}
b8 button(String text)
{
  DuiId me = hash(text);
  return button(me, text);
}

b8 basic_test_control(String text, Vec2f size, Color color, b8 fill = false)
{
  Container *c = get_current_container(&s);
  if (!c) return false;

  DuiId id = hash(text);

  Rect rect = c->place(size, true, fill);

  b8 hot     = do_hot(id, rect, c->parent->root, c, c->content_rect);
  b8 active  = do_active(id);
  b8 clicked = hot && s.just_stopped_being_active == id;

  static f32 darkenf     = 0.f;
  static DuiId darken_id = 0;

  if (clicked) {
    darken_id = id;
    darkenf   = 1.f;
  }

  if (darken_id == id) {
    color = darken(color, darkenf);
    darkenf -= 0.01f;
    if (darkenf < 0.f) darken_id = 0;
  }

  if (hot) color = darken(color, .1f);

  push_rect(c->parent->root->dl, rect, color);
  push_rect(c->parent->root->dl, inset(rect, 3.f), darken(color, .15f));

  return clicked;
}

void debug_ui_test(Input *input, Vulkan *gpu, Vec2f canvas_size)
{
  static b8 init = false;
  if (!init) {
    init = true;
    init_draw_list(gpu, &main_dl);
    init_draw_list(gpu, &forground_dl);
    for (i32 i = 0; i < s.draw_lists.MAX_SIZE; i++) {
      s.draw_lists.push_back({});
      init_draw_list(gpu, &s.draw_lists[i]);
    }

    s.empty_group      = create_group(nullptr, {});
    s.fullscreen_group = s.empty_group;

    // FileData font_file =
    // read_entire_file("resources/fonts/RobotoCondensed_Bold.ttf"); font =
    // load_font(font_file, 64, memory.temp);

    // EMPTY_TEX            = Texture2D(1, 1, TextureFormat::RGBA8, false);
    // u8 empty_tex_data[4] = {255, 255, 255, 255};
    // EMPTY_TEX.upload(empty_tex_data, false);
  }

  static b8 paused = false;
  if (input->key_down_events[(i32)Keys::Q]) {
    paused = !paused;
  }

  if (paused) {
    draw_draw_list(gpu, &main_dl, canvas_size);
    draw_draw_list(gpu, &forground_dl, canvas_size);
    for (i32 i = 0; i < s.draw_lists.size; i++) {
      draw_draw_list(gpu, &s.draw_lists[i], canvas_size);
    }

    return;
  }

  start_frame(input, canvas_size);

  DuiId w1 = start_window("first", {100, 200, 300, 300});
  set_window_color({0, 0.13725, 0.27843, 1});
  end_window();

  DuiId w2 = start_window("second", {200, 300, 100, 300});
  set_window_color({0, 0.2, 0.4, .5});
  end_window();

  DuiId w3 = start_window("third", {300, 400, 200, 300});
  set_window_color({0, 0.24706, 0.4902, .5});
  end_window();

  DuiId w4 = start_window("fourth", {400, 500, 200, 300});
  set_window_color({1, 0.55686, 0, .5});
  end_window();

  DuiId w5 = start_window("fifth", {500, 600, 200, 300});
  set_window_color({0.99216, 0.46667, 0.00784, .5});
  if (basic_test_control("test1", {200, 300}, {1, 1, 1, 0})) info("test1");
  if (basic_test_control("test2", {150, 100}, {1, 0, 0, 1})) info("test2");
  ;
  if (basic_test_control("test3", {100, 400}, {1, 1, 0, 1}, true))
    info("test3");
  next_line();
  if (basic_test_control("test4", {200, 600}, {0, 1, 1, 1})) info("test4");
  next_line();
  if (basic_test_control("test5", {200, 700}, {1, 0, 1, 1})) info("test5");
  if (basic_test_control("test6", {200, 300}, {1, 1, 0, 1})) info("test6");
  next_line();
  if (basic_test_control("test7", {200, 100}, {0, 0, 1, 1})) info("test7");
  end_window();

  DuiId w6 = start_window("sizth", {600, 700, 200, 300});
  if (basic_test_control("test8", {200, 100}, {0, 0, 1, 1}, true)) info("test8");
  set_window_color({1, 0.31373, 0.01176, .5});
  end_window();

  DuiId w7 = start_window("seventh", {700, 800, 200, 300});
  set_window_color({.3, .6, .4, .5});
  end_window();

  DuiId w8 = start_window("eigth", {700, 100, 200, 300});
  set_window_color({.3, .6, .4, .5});
  end_window();

  auto p = [&](DuiId window_id) {
    Container *w = &s.containers.wrapped_get(window_id);
    return w->parent;
  };

  static Group *root   = p(w1);
  static Group *root_2 = p(w1);

  static i32 count = 0;
  if (input->key_down_events[(i32)Keys::SPACE]) {
    // if (count == 0) {
    //   snap_group(p(w2), p(w1), 1, true);
    // }

    // if (count == 1) {
    //   snap_group(p(w3), p(w1), 0, false);
    // }

    // if (count == 2) {
    //   root_2 = p(w2);
    //   snap_group(p(w4), p(w2), 0, false);
    // }

    // if (count == 3) {
    //   snap_group(p(w5), root, 0, true);
    // }

    // if (count == 4) {
    //   snap_group(p(w6), p(w4), 1, true);
    // }

    // if (count == 5) {
    //   parent_window(p(w1), w7);
    //   // snap_group(p(w7), root_2, 0, true);
    // }

    count++;
  }

  std::vector<Group *> groups;
  for (i32 i = 0; i < s.groups.SIZE; i++) {
    if (!s.groups.exists(i)) continue;

    groups.push_back(&s.groups[i]);
  }

  for (i32 i = s.root_groups.size - 1; i >= 0; i--) {
    draw_draw_list(gpu, s.root_groups[i]->dl, canvas_size);
  }
  draw_draw_list(gpu, &main_dl, canvas_size);
  draw_draw_list(gpu, &forground_dl, canvas_size);
}
}  // namespace Dui

/*
{
  b8 x = Dui::button(...);
  DuiId button_id = Dui::Id();
  if (x) {
    ... // can add more controls here
  }

  if (Dui::Clicked(button_id, RIGHT)) {
    ... // can add more controls here
  }

  if (Dui::StartedDrag(button_id, RIGHT)) {
    ... // can add more controls here
  }

  if (Dui::FinishedDrag(button_id, RIGHT))
    ... // can add more controls here
  }

}


*/

#include "container.cc"