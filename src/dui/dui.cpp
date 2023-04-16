#include <unordered_map>

#include "dui/basic.hpp"
#include "dui/container.hpp"
#include "dui/draw.hpp"
#include "dui/dui.hpp"
#include "dui/dui_state.hpp"
#include "dui/group.hpp"
#include "dui/interaction.hpp"
#include "font.hpp"
#include "gpu/vulkan/framebuffer.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "math/math.hpp"
#include "plugin.hpp"
#include "string.hpp"
#include "types.hpp"

// https://www.color-hex.com/color-palette/1294
Color highlight = Color::from_int(0xF98125);

Color d_dark  = Color::from_int(0x011f4b);  //{0, 0.13725, 0.27843, 1};
Color d       = Color::from_int(0x03396c);  //{0, 0.2, 0.4, 1};
Color d_light = Color::from_int(0x005b96);  //{0, 0.24706, 0.4902, 1};
Color l_light = Color::from_int(0x19578a);  //{1, 0.55686, 0, 1};
Color l       = Color::from_int(0x6497b1);  //{0.99216, 0.46667, 0.00784, 1};
Color l_dark  = Color::from_int(0x003e71);  //{1, 0.31373, 0.01176, 1};

namespace Dui
{

void set_cursor_shape(Platform::CursorShape shape)
{
  if (s.cursor_shape == Platform::CursorShape::NORMAL) {
    s.cursor_shape = shape;
  }
}

// -1 if not root
i32 get_group_z(Group *g)
{
  for (i32 i = 0; i < s.root_groups.size; i++) {
    if (s.root_groups[i] == g->id) return i;
  }
  return -1;
}

b8 is_empty(Group *g) { return g->id == s.empty_group; }

void add_root_group(Group *g)
{
  assert(get_group_z(g) == -1);

  i32 index = s.root_groups.insert(0, g->id);
  g->parent = -1;
  g->root   = g->id;
}

void remove_root_group(Group *g)
{
  i32 z = get_group_z(g);
  s.root_groups.shift_delete(z);
}

Group *create_group(Group *parent, Rect rect)
{
  Group *g = s.groups.push_back({});
  g->id    = s.groups.index_of(g);
  g->rect  = rect;

  g->parent = parent ? parent->id : GroupId(-1);
  if (!parent) {
    add_root_group(g);
  }

  return g;
}

void free_group(Group *g)
{
  if (!g->parent.valid()) {
    i32 z = get_group_z(g);
    s.root_groups.shift_delete(z);
  }

  i64 index = s.groups.index_of(g);
  s.groups.remove(index);
}

Group *get_group(i64 id) { return &s.groups.wrapped_get(id); }

// walks through children of parent_group until a leaf is found at the given
// pos.
Group *get_leaf_group_at_pos(Vec2f pos, Group *parent_group)
{
  if (parent_group->is_leaf() || is_empty(parent_group)) {
    return in_rect(pos, parent_group->rect) ? parent_group : nullptr;
  }

  for (i32 i = 0; i < parent_group->splits.size; i++) {
    Group *g = parent_group->splits[i].child.get();

    Group *possible_result = get_leaf_group_at_pos(pos, g);
    if (possible_result) return possible_result;
  }
  return nullptr;
}

Group *get_top_leaf_group_at_pos(Vec2f pos, b8 ignore_top = false)
{
  for (i32 i = ignore_top ? 1 : 0; i < s.root_groups.size; i++) {
    Group *g = s.root_groups[i].get();
    if (in_rect(pos, g->rect)) {
      return get_leaf_group_at_pos(pos, g);
    }
  }
  return nullptr;
}

b8 contained_in(Group *g, Group *in)
{
  if (g == in) return true;
  if (!g->parent.valid()) return false;
  return contained_in(g->parent.get(), in);
}

void propagate_groups(Group *g, GroupId root = -1, Group *parent = nullptr)
{
  if (parent) {
    g->parent = parent->id;
    g->z      = parent->z;
  }

  g->root = root.valid() ? root : g->id;

  if (!g->is_leaf()) {
    f32 x = g->rect.x;
    f32 y = g->rect.y;

    for (i32 i = 0; i < g->splits.size; i++) {
      Split &split = g->splits[i];
      Group *child = split.child.get();

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
      Container *c = get_container(g->windows[i]);
      c->parent    = g->id;
      c->rect      = g->get_window_rect();
      c->z         = g->z;
    }
  }
}

// returns px
f32 get_minimum_size(Group *g, f32 g_size_px, i32 axis, b8 dir,
                     b8 commmit = false)
{
  if (g->splits.size > 0 && g->split_axis != axis) {
    f32 min_size =
        get_minimum_size(g->splits[0].child.get(), g_size_px, axis, dir);
    for (i32 i = 1; i < g->splits.size; i++) {
      min_size = fmaxf(min_size, get_minimum_size(g->splits[i].child.get(),
                                                  g_size_px, axis, dir));
    }
    return min_size;
  } else if (g->splits.size > 0) {
    f32 non_changing_divs_size_px = 0.f;
    for (i32 i = 0; i < g->splits.size - 1; i++) {
      non_changing_divs_size_px += g_size_px * g->splits[i - dir + 1].div_pct;
    }

    Split *end_split = &g->splits[dir ? g->splits.size - 1 : 0];
    return non_changing_divs_size_px +
           get_minimum_size(end_split->child.get(),
                            g_size_px * end_split->div_pct, axis, dir);
  }

  const f32 MINIMUM_GROUP_SIZE[2] = {TITLEBAR_HEIGHT + 15.f, 50.f};
  return MINIMUM_GROUP_SIZE[axis];
}

void resize_border_splits_and_propagate(Group *g, f32 pct_change, i32 axis,
                                        b8 dir)
{
  if (g->split_axis != axis) {
    for (i32 i = 0; i < g->splits.size; i++) {
      resize_border_splits_and_propagate(g->splits[i].child.get(), pct_change,
                                         axis, dir);
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
    resize_border_splits_and_propagate(g->splits[changing_split_i].child.get(),
                                       pct_change * (new_value / old_value),
                                       axis, dir);

    g->splits[changing_split_i].div_pct = new_value;
  }
};

void parent_window(Group *g, DuiId window_id)
{
  Container *w = &s.containers.wrapped_get(window_id);

  if (w->parent != -1) {
    Group *old_g = w->parent.get();
    old_g->windows.shift_delete(old_g->get_window_idx(w->id));
    if (old_g->active_window_idx >= old_g->windows.size) {
      old_g->active_window_idx--;
    }

    if (old_g->windows.size == 0 && !is_empty(old_g)) {
      free_group(old_g);
    }
  }

  w->parent = g->id;

  g->windows.push_back(window_id);
  g->active_window_idx = g->windows.size - 1;
}

Group *unparent_window(DuiId window_id)
{
  Container *w = get_container(window_id);
  Group *old_g = w->parent.get();

  if (!is_empty(old_g) && old_g->windows.size == 1) return old_g;

  Group *g = create_group(nullptr, old_g->rect);
  parent_window(g, window_id);

  g->rect.width  = fminf(g->rect.width / 2, s.canvas.width);
  g->rect.height = fminf(g->rect.height / 2, s.canvas.height);

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
    Group *child = g->splits[i].child.get();
    if (!child->is_leaf() && !is_empty(child) &&
        child->split_axis == g->split_axis) {
      Split split = g->splits[i];
      g->splits.shift_delete(i);

      for (i32 child_i = 0; child_i < child->splits.size; child_i++) {
        Split new_split = child->splits[child_i];
        new_split.div_pct *= split.div_pct;
        g->splits.insert(i + child_i, new_split);

        new_split.child.get()->parent = g->id;
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
  Vec2f old_size = g->rect.span();

  // target must either be a leaf node or be split on the same axis as the
  // sibling
  if (!target->is_leaf() && (sibling && target->split_axis != axis))
    return false;

  // try adding it to the parent first
  if (target->parent != -1 &&
      snap_group(g, target->parent.get(), axis, dir, target))
    return true;

  if (target->is_leaf()) {
    // create a new group to move the windows to
    Group *new_g             = create_group(target, target->rect);
    new_g->windows           = target->windows;
    new_g->active_window_idx = target->active_window_idx;
    if (target->id == s.empty_group) s.empty_group = new_g->id;

    target->clear_windows();
    target->split_axis = axis;

    Split orig_split;
    orig_split.child   = new_g->id;
    orig_split.div_pct = .5f;
    target->splits.push_back(orig_split);

    Split new_split;
    new_split.child   = g->id;
    new_split.div_pct = .5f;
    target->splits.insert(dir ? 1 : 0, new_split);

  } else if (target->split_axis != axis) {  // already split, but new axis
    assert(!sibling);

    // move original children to a new group
    Group *new_g      = create_group(target, target->rect);
    new_g->split_axis = target->split_axis;
    new_g->splits     = target->splits;
    for (i32 i = 0; i < new_g->splits.size; i++) {
      Group *target = new_g->splits[i].child.get();
    }
    if (target->id == s.empty_group) s.empty_group = new_g->id;

    target->splits.clear();

    Split orig_split;
    orig_split.child   = new_g->id;
    orig_split.div_pct = .5f;
    target->splits.push_back(orig_split);

    Split new_split;
    new_split.child   = g->id;
    new_split.div_pct = .5f;
    target->splits.insert(dir ? 1 : 0, new_split);

    target->split_axis = axis;
  } else if (sibling) {  // already split, same axis, in the middle
    i32 sibling_idx = -1;
    for (i32 i = 0; i < target->splits.size; i++) {
      if (target->splits[i].child == sibling->id) {
        sibling_idx = i;
        break;
      }
    }
    assert(sibling_idx > -1);

    f32 half_size = target->splits[sibling_idx].div_pct / 2;
    target->splits[sibling_idx].div_pct = half_size;

    Split new_split;
    new_split.child   = g->id;
    new_split.div_pct = half_size;
    target->splits.insert(dir ? sibling_idx + 1 : sibling_idx, new_split);
  } else {  // already split, same axis, at the ends
    f32 new_split_pct = 1.f / (target->splits.size + 1);

    for (i32 i = 0; i < target->splits.size; i++) {
      target->splits[i].div_pct *= 1 - new_split_pct;
    }

    Split new_split;
    new_split.child   = g->id;
    new_split.div_pct = new_split_pct;
    target->splits.insert(dir ? target->splits.size : 0, new_split);
  }

  g->span_before_snap = old_size;

  if (!g->parent.valid()) {
    remove_root_group(g);
  }
  g->parent = target->id;

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

    g->clear_windows();
    g->active_window_idx = -1;
    return new_g;
  }

  if (!g->parent.valid()) return g;
  Group *parent_g = g->parent.get();

  DuiId idx;
  f32 free_space;
  for (i32 i = 0; i < parent_g->splits.size; i++) {
    Split split = parent_g->splits[i];
    if (split.child == g->id) {
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
    Group *child                = parent_g->splits[0].child.get();
    parent_g->split_axis        = child->split_axis;
    parent_g->splits            = child->splits;
    parent_g->windows           = child->windows;
    parent_g->active_window_idx = child->active_window_idx;
    if (is_empty(child)) s.empty_group = parent_g->id;

    free_group(child);

    if (parent_g->parent != -1) {
      merge_splits(parent_g->parent.get());
    }
  }

  if (g->span_before_snap.x > 0.f && g->span_before_snap.y > 0) {
    g->rect.width  = g->span_before_snap.x;
    g->rect.height = g->span_before_snap.y;
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
  // some controls might overlap, so this tracks whether we've already found a
  // control we're on top of. This means controls need to be evaluated in order
  // of priority.
  b8 already_found_hot = false;

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
                            Group *g, Group *target_group, i32 axis, b8 dir,
                            b8 already_found_hot) -> b8 {
    DuiId hot = do_hot(control_id, rect);
    if (hot)
      push_rect(&s.dl, 0, rect, l_dark);
    else
      push_rect(&s.dl, 0, rect, l);

    if (hot && !already_found_hot) {
      Rect preview_rect = target_group->rect;
      if (axis == 0) {
        preview_rect.height /= 2;
        if (dir) preview_rect.y += preview_rect.height;
      } else {
        preview_rect.width /= 2;
        if (dir) preview_rect.x += preview_rect.width;
      }

      push_rect(&s.dl, 0, preview_rect, {1, 1, 1, .5});  // preview
      if (s.just_stopped_being_dragging == dragging_id) {
        s.snap_group_to_snap = {true, g->id, target_group->id, axis, dir};
      }
    }

    return hot;
  };

  if (s.fullscreen_group == g->id) return g;

  Group *target_group = get_top_leaf_group_at_pos(s.input->mouse_pos, true);

  if (target_group && !contained_in(target_group, g)) {
    Rect window_rect = target_group->get_window_rect();

    f32 dock_controls_width = DOCK_CONTROLS_WIDTH;
    dock_controls_width = fminf(dock_controls_width, window_rect.width * .75f);
    dock_controls_width = fminf(dock_controls_width, window_rect.height * .75f);

    f32 half_controls_width       = dock_controls_width / 2.f;
    f32 fifth_dock_controls_width = dock_controls_width / 5.f;
    Vec2f horizontal_button_span  = {half_controls_width,
                                     fifth_dock_controls_width};
    Vec2f vertical_button_span    = {fifth_dock_controls_width,
                                     half_controls_width};

    if (target_group->root != target_group->id) {
      Rect root_window_rect;
      if (target_group->root == s.fullscreen_group) {
        root_window_rect = target_group->root.get()->rect;
      } else {
        root_window_rect = target_group->root.get()->get_window_rect();
      }

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

      b8 left_hot = do_snap_control(
          left_root_dock_control_id, id, left_root_dock_control_rect, g,
          target_group->root.get(), 1, false, already_found_hot);
      b8 right_hot = do_snap_control(
          right_root_dock_control_id, id, right_root_dock_control_rect, g,
          target_group->root.get(), 1, true, already_found_hot);
      b8 top_hot = do_snap_control(
          top_root_dock_control_id, id, top_root_dock_control_rect, g,
          target_group->root.get(), 0, false, already_found_hot);
      b8 bottom_hot = do_snap_control(
          bottom_root_dock_control_id, id, bottom_root_dock_control_rect, g,
          target_group->root.get(), 0, true, already_found_hot);
      if (left_hot || right_hot || top_hot || bottom_hot) {
        already_found_hot = true;
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

    b8 left_hot =
        do_snap_control(left_dock_control_id, id, left_dock_control_rect, g,
                        target_group, 1, false, already_found_hot);
    b8 right_hot =
        do_snap_control(right_dock_control_id, id, right_dock_control_rect, g,
                        target_group, 1, true, already_found_hot);
    b8 top_hot = do_snap_control(top_dock_control_id, id, top_dock_control_rect,
                                 g, target_group, 0, false, already_found_hot);
    b8 bottom_hot =
        do_snap_control(bottom_dock_control_id, id, bottom_dock_control_rect, g,
                        target_group, 0, true, already_found_hot);
    if (left_hot || right_hot || top_hot || bottom_hot) {
      already_found_hot = true;
    }

    if (target_group->id == s.empty_group &&
        s.empty_group.get()->windows.size == 0) {
      SUB_ID(center_dock_control_id, id);
      Rect center_dock_control_rect =
          inset(dock_control_rect, dock_controls_width / 4.f);

      DuiId hot = do_hot(center_dock_control_id, center_dock_control_rect);
      if (hot)
        push_rect(&s.dl, 0, center_dock_control_rect, l_dark);
      else
        push_rect(&s.dl, 0, center_dock_control_rect, l);

      if (hot && !already_found_hot) {
        push_rect(&s.dl, 0, target_group->rect, {1, 1, 1, .5});  // preview
        already_found_hot = true;

        if (s.just_stopped_being_dragging == id) {
          s.snap_group_to_snap = {true, g->id, target_group->id, -1, false};
          return g;
        }
      }
    }

    if (!already_found_hot && g->is_leaf() &&
        (target_group->id != s.empty_group ||
         target_group->windows.size != 0) &&
        in_rect(s.input->mouse_pos, target_group->get_titlebar_full_rect())) {
      push_rect(&s.dl, 0, target_group->rect, {1, 1, 1, .5});  // preview
      already_found_hot = true;

      if (s.just_stopped_being_dragging == id) {
        combine_leaf_groups(target_group, g);

        return target_group;
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
DuiId popup_id(DuiId id) { return extend_hash(s.started_popups_count, id); }

Popup *get_current_popup()
{
  assert(s.started_popups_count > -1);

  if (s.started_popups_count <= s.popups.size) {
    return &s.popups[s.started_popups_count - 1];
  }
  return nullptr;
}

Popup *get_next_popup()
{
  assert(s.started_popups_count > -1);

  if (s.started_popups_count < s.popups.size) {
    return &s.popups[s.started_popups_count];
  }
  return nullptr;
}

b8 is_popup_open(DuiId id)
{
  id                   = popup_id(id);
  Popup *current_popup = get_next_popup();
  return (current_popup && id == current_popup->container.id);
}
b8 is_popup_open(String name) { return is_popup_open(hash(name)); }

void clear_popups()
{
  s.clear_popups = true;

  s.selected = s.previously_selected;
}

void close_child_popups() { s.popups.resize(s.started_popups_count); }

void open_popup(DuiId id, Vec2f pos, f32 width)
{
  // clear all popups beyond what is already started
  close_child_popups();

  if (s.popups.size == 0) {
    s.previously_selected = s.selected;
  }

  Popup *p        = &s.popups[s.popups.push_back({})];
  p->container.id = popup_id(id);
  p->container.z  = 0;

  p->container.rect = {pos.x, pos.y, width, 20};
}
void open_popup(String name, Vec2f pos, f32 width)
{
  open_popup(hash(name), pos, width);
}

b8 start_popup(DuiId id)
{
  id                   = popup_id(id);
  Popup *current_popup = get_next_popup();
  if (!current_popup || id != current_popup->container.id) return false;

  current_popup->container.start_frame(&s, true);

  push_base_scissor(&s.dl);
  current_popup->outline_rect    = push_rounded_rect(&s.dl, 0, {}, 2, d_light);
  current_popup->background_rect = push_rounded_rect(&s.dl, 0, {}, 2, d);

  s.started_popups_count++;

  return true;
}
b8 start_popup(String name) { return start_popup(hash(name)); }

void end_popup()
{
  s.cc->expand_rect_to_content();
  s.cc->end_frame(&s, true);

  pop_scissor(&s.dl);

  Popup *current_popup = get_current_popup();
  if (current_popup->outline_rect)
    current_popup->outline_rect->dimensions = current_popup->container.rect;
  if (current_popup->background_rect)
    current_popup->background_rect->dimensions =
        inset(current_popup->container.rect, 1);

  s.started_popups_count--;

  // kinda hacky, need to resume the container from the previous popup layer
  if (s.started_popups_count > 0) {
    s.cc = &s.popups[s.started_popups_count - 1].container;
  } else {
    s.cc = s.cw;
  }
}

}  // namespace Dui

namespace Dui
{

f32 MENU_ITEM_HEIGHT = 24;

b8 menu_item_button(DuiId id, String text)
{
  Container *c = get_current_container(&s);
  if (!c) return false;

  id = extend_hash((DuiId)c, id);

  Rect rect = c->place({0, MENU_ITEM_HEIGHT}, true, true, 0);

  b8 hot     = do_hot(id, rect, c);
  b8 active  = do_active(id);
  b8 clicked = hot && s.just_stopped_being_active == id;

  if (hot) {
    push_rounded_rect(&s.dl, c->z, rect, 4.f, {.5, .5, .5, .5});
    set_cursor_shape(Platform::CursorShape::POINTING_HAND);
  }

  if (clicked) {
    clear_popups();
  }

  Vec2f text_pos  = {rect.x + WINDOW_MARGIN_SIZE, rect.y + WINDOW_MARGIN_SIZE};
  f32 text_height = MENU_ITEM_HEIGHT - (WINDOW_MARGIN_SIZE * 2);
  push_vector_text(&s.dl, c->z, text, text_pos, {1, 1, 1, 1}, text_height);

  return clicked;
}
b8 menu_item_button(String text) { return menu_item_button(hash(text), text); }

b8 menu_item_submenu(String text, String submenu_name)
{
  Container *c = get_current_container(&s);
  if (!c) return false;

  DuiId id = extend_hash((DuiId)c, text);

  Rect rect           = c->place({0, MENU_ITEM_HEIGHT}, true, true, 0);
  Rect highlight_rect = inset(rect, 1.f);

  b8 hot     = do_hot(id, rect, c);
  b8 active  = do_active(id);
  b8 clicked = hot && s.just_stopped_being_active == id;

  if (hot) {
    set_cursor_shape(Platform::CursorShape::POINTING_HAND);
  }

  if (hot || clicked || is_popup_open(submenu_name)) {
    push_rounded_rect(&s.dl, c->z, rect, 4.f, {.5, .5, .5, .5});
  }

  Vec2f text_pos  = {rect.x + WINDOW_MARGIN_SIZE, rect.y + WINDOW_MARGIN_SIZE};
  f32 text_height = MENU_ITEM_HEIGHT - (WINDOW_MARGIN_SIZE * 2);
  push_vector_text(&s.dl, c->z, text, text_pos, {1, 1, 1, 1}, text_height);

  Vec2f arrow_pos = {rect.right() - WINDOW_MARGIN_SIZE,
                     rect.y + WINDOW_MARGIN_SIZE};
  push_vector_text_justified(&s.dl, c->z, ">", arrow_pos, {1, 1, 1, 1},
                             text_height, {true, false});

  if (clicked) {
    open_popup(submenu_name, Vec2f(rect.right(), rect.top()), 200);
  }

  return start_popup(submenu_name);
}

void menu_divider()
{
  Container *c = get_current_container(&s);
  if (!c) return;

  c->next_line(0);  // make sure we start below other controls
  Rect rect = c->place({0, 2.5}, true, true, 0);

  rect.x -= WINDOW_MARGIN_SIZE;
  rect.width += WINDOW_MARGIN_SIZE * 2;

  push_rounded_rect(&s.dl, c->z, rect, 2, d_light);
}

// test menubar stuff
void submenu2(Vec2f pos);

void submenu(Vec2f pos)
{
  if (menu_item_submenu("Go to Definition", "submenu2")) {
    info("submenu2");
    submenu2({pos.x + 200, pos.y});
    end_popup();
  }

  if (menu_item_button("Peek")) {
    info("submenu2");
  }
}

void submenu2(Vec2f pos)
{
  if (menu_item_submenu("Cut", "submenu1")) {
    info("submenu1");
    submenu({pos.x + 200, pos.y});
    end_popup();
  }

  if (menu_item_submenu("Copy", "submenu2")) {
    info("submenu2");
    submenu2({pos.x + 200, pos.y});
    end_popup();
  }

  menu_divider();

  if (menu_item_button("Paste")) {
    info("submenu2");
  }
}

}  // namespace Dui

namespace Dui
{
void draw_group_and_children(Group *g)
{
  if (!g->is_leaf()) {
    for (i32 i = 0; i < g->splits.size; i++) {
      draw_group_and_children(g->splits[i].child.get());
    }
    return;
  }

  Rect titlebar_rect               = g->get_titlebar_full_rect();
  Rect titlebar_bottom_border_rect = g->get_titlebar_bottom_border_rect();
  Rect group_border_rect           = g->get_border_rect();

  push_rect(&s.dl, g->z, titlebar_rect, d);
  push_rect(&s.dl, g->z, titlebar_bottom_border_rect, d_light);
  push_rect(&s.dl, g->z, group_border_rect, d);

  f32 combined_extra_desired_tab_space =
      g->get_combined_extra_desired_tab_space();
  f32 available_extra_tab_space = g->get_available_extra_tab_space();
  Vec2f tab_pos                 = g->get_tabs_rect().xy();
  for (i32 w_i = 0; w_i < g->windows.size; w_i++) {
    DuiId window_id = g->windows[w_i];
    Container *w    = get_container(window_id);

    Vec2f tab_span = g->get_tab_margin_span(
        w_i, combined_extra_desired_tab_space, available_extra_tab_space);

    Rect tab_rect = {tab_pos.x, tab_pos.y, tab_span.x, tab_span.y + 1.f};
    tab_pos.x += tab_rect.width + TAB_GAP;

    Color tab_color = d_dark;
    if (w_i == g->active_window_idx) {
      tab_color = d_light;
    }

    push_rounded_rect(&s.dl, g->z, tab_rect, 3.f, tab_color, CornerMask::TOP);

    push_scissor(&s.dl, tab_rect);
    push_vector_text(&s.dl, g->z, w->title,
                     tab_rect.xy() + Vec2f{TAB_MARGIN, TAB_MARGIN},
                     {1, 1, 1, 1}, tab_rect.height - (TAB_MARGIN * 2));
    pop_scissor(&s.dl);
  }
}

void start_frame_for_leaf(Group *g)
{
  if (g->root == g->id) {
    g->span_before_snap = g->rect.span();
  }

  assert(g->active_window_idx < g->windows.size);
  DuiId active_window_id = g->windows[g->active_window_idx];

  for (i32 w_i = 0; w_i < g->windows.size; w_i++) {
    DuiId window_id = g->windows[w_i];
    Container *w    = get_container(window_id);

    Rect tab_rect = g->get_tab_margin_rect(w_i);

    SUB_ID(tab_handle_id, window_id);
    DuiId tab_handle_hot      = do_hot(tab_handle_id, tab_rect, g->root.get());
    DuiId tab_handle_active   = do_active(tab_handle_id);
    DuiId tab_handle_dragging = do_dragging(tab_handle_id);
    if (tab_handle_active) {
      g->active_window_idx = w_i;
    }
    if (tab_handle_dragging || s.just_stopped_being_dragging == tab_handle_id) {
      if (g->windows.size == 1 ||
          !in_rect(s.input->mouse_pos, g->get_titlebar_full_rect())) {
        g = unparent_window(window_id);
        g = unsnap_group(g);
        g = handle_dragging_group(g, tab_handle_id);
      } else {
        i32 target_tab_idx = g->get_tab_at_pos(s.input->mouse_pos);
        g->windows.shift_delete(w_i);
        g->windows.insert(target_tab_idx, w->id);
        g->active_window_idx = target_tab_idx;
      }
    }
  }

  Rect titlebar_rect = g->get_titlebar_margin_rect();
  SUB_ID(root_handle_id, active_window_id);
  DuiId root_handle_hot = do_hot(root_handle_id, titlebar_rect, g->root.get());
  DuiId root_handle_active   = do_active(root_handle_id);
  DuiId root_handle_dragging = do_dragging(root_handle_id);
  if (root_handle_dragging || s.just_stopped_being_dragging == root_handle_id) {
    if (s.input->keys[(i32)Keys::LCTRL]) {
      g = unsnap_group(g);
      g = handle_dragging_group(g, root_handle_id);
    } else {
      Group *root_g = g;
      while (root_g->parent != -1) {
        root_g = root_g->parent.get();
      }
      root_g = handle_dragging_group(root_g, root_handle_id);
    }
  }
}

void start_frame_for_group(Group *g)
{
  s.cg = g;

  if (s.fullscreen_group == g->id) {
    g->rect = s.canvas;
  }

  if (!g->parent.valid()) {
    // only do movement/resizing logic if window is not fullscreen
    if (s.fullscreen_group != g->id) {
      // easy keyboard-based window manipulation
      if (s.input->keys[(i32)Keys::LALT]) {
        SUB_ID(root_controller_control_id, g->id.id);
        DuiId root_controller_control_hot =
            do_hot(root_controller_control_id, g->rect, g->root.get());
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

        Vec2f clamped_mouse_pos = clamp_to_rect(s.input->mouse_pos, s.canvas);

        Rect left_top_handle_rect;
        left_top_handle_rect.x = g->rect.x - RESIZE_HANDLES_OVERSIZE;
        left_top_handle_rect.y = g->rect.y - RESIZE_HANDLES_OVERSIZE;
        left_top_handle_rect.width =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        left_top_handle_rect.height =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;

        SUB_ID(left_top_handle_id, g->id.id);
        DuiId left_top_handle_hot =
            do_hot(left_top_handle_id, left_top_handle_rect, g->root.get());
        DuiId left_top_handle_active   = do_active(left_top_handle_id);
        DuiId left_top_handle_dragging = do_dragging(left_top_handle_id);

        if (left_top_handle_hot || left_top_handle_dragging) {
          set_cursor_shape(Platform::CursorShape::NWSE_RESIZE);
        }

        if (left_top_handle_dragging) {
          Vec2f delta = clamped_mouse_pos - g->rect.xy();

          g->rect.x += delta.x;
          g->rect.y += delta.y;
          g->rect.width -= delta.x;
          g->rect.height -= delta.y;

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
        SUB_ID(right_top_handle_id, g->id.id);

        DuiId right_top_handle_hot =
            do_hot(right_top_handle_id, right_top_handle_rect, g->root.get());
        DuiId right_top_handle_active   = do_active(right_top_handle_id);
        DuiId right_top_handle_dragging = do_dragging(right_top_handle_id);

        if (right_top_handle_hot || right_top_handle_dragging) {
          set_cursor_shape(Platform::CursorShape::SWNE_RESIZE);
        }

        if (right_top_handle_dragging) {
          Vec2f delta =
              clamped_mouse_pos - Vec2f(g->rect.x + g->rect.width, g->rect.y);

          g->rect.width += delta.x;
          g->rect.y += delta.y;
          g->rect.height -= delta.y;

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

        SUB_ID(left_bottom_handle_id, g->id.id);
        DuiId left_bottom_handle_hot = do_hot(
            left_bottom_handle_id, left_bottom_handle_rect, g->root.get());
        DuiId left_bottom_handle_active   = do_active(left_bottom_handle_id);
        DuiId left_bottom_handle_dragging = do_dragging(left_bottom_handle_id);

        if (left_bottom_handle_hot || left_bottom_handle_dragging) {
          set_cursor_shape(Platform::CursorShape::SWNE_RESIZE);
        }

        if (left_bottom_handle_dragging) {
          Vec2f delta =
              clamped_mouse_pos - Vec2f(g->rect.x, g->rect.y + g->rect.height);

          g->rect.x += delta.x;
          g->rect.width -= delta.x;
          g->rect.height += delta.y;

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

        SUB_ID(right_bottom_handle_id, g->id.id);
        DuiId right_bottom_handle_hot = do_hot(
            right_bottom_handle_id, right_bottom_handle_rect, g->root.get());
        DuiId right_bottom_handle_active = do_active(right_bottom_handle_id);
        DuiId right_bottom_handle_dragging =
            do_dragging(right_bottom_handle_id);

        if (right_bottom_handle_hot || right_bottom_handle_dragging) {
          set_cursor_shape(Platform::CursorShape::NWSE_RESIZE);
        }

        if (right_bottom_handle_dragging) {
          Vec2f delta = clamped_mouse_pos - Vec2f(g->rect.x + g->rect.width,
                                                  g->rect.y + g->rect.height);

          g->rect.width += delta.x;
          g->rect.height += delta.y;

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

        SUB_ID(left_handle_id, g->id.id);
        DuiId left_handle_hot =
            do_hot(left_handle_id, left_handle_rect, g->root.get());
        DuiId left_handle_active   = do_active(left_handle_id);
        DuiId left_handle_dragging = do_dragging(left_handle_id);

        if (left_handle_hot || left_handle_dragging) {
          set_cursor_shape(Platform::CursorShape::HORIZ_RESIZE);
        }

        if (left_handle_dragging) {
          f32 delta = clamped_mouse_pos.x - g->rect.left();

          g->rect.x += delta;
          g->rect.width -= delta;

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

        SUB_ID(right_handle_id, g->id.id);
        DuiId right_handle_hot =
            do_hot(right_handle_id, right_handle_rect, g->root.get());
        DuiId right_handle_active   = do_active(right_handle_id);
        DuiId right_handle_dragging = do_dragging(right_handle_id);

        if (right_handle_hot || right_handle_dragging) {
          set_cursor_shape(Platform::CursorShape::HORIZ_RESIZE);
        }

        if (right_handle_dragging) {
          f32 delta = clamped_mouse_pos.x - g->rect.right();

          g->rect.width += delta;

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

        SUB_ID(top_handle_id, g->id.id);
        DuiId top_handle_hot =
            do_hot(top_handle_id, top_handle_rect, g->root.get());
        DuiId top_handle_active   = do_active(top_handle_id);
        DuiId top_handle_dragging = do_dragging(top_handle_id);

        if (top_handle_hot || top_handle_dragging) {
          set_cursor_shape(Platform::CursorShape::VERT_RESIZE);
        }

        if (top_handle_dragging) {
          f32 delta = clamped_mouse_pos.y - g->rect.top();

          g->rect.y += delta;
          g->rect.height -= delta;

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

        SUB_ID(bottom_handle_id, g->id.id);
        DuiId bottom_handle_hot =
            do_hot(bottom_handle_id, bottom_handle_rect, g->root.get());
        DuiId bottom_handle_active   = do_active(bottom_handle_id);
        DuiId bottom_handle_dragging = do_dragging(bottom_handle_id);

        if (bottom_handle_hot || bottom_handle_dragging) {
          set_cursor_shape(Platform::CursorShape::VERT_RESIZE);
        }

        if (bottom_handle_dragging) {
          f32 delta = clamped_mouse_pos.y - g->rect.bottom();

          g->rect.height += delta;

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
            s.canvas.right()) {
          g->rect.x -= (left_window_control.x + left_window_control.width) -
                       s.canvas.right();
        }
        if (left_window_control.y + left_window_control.height >
            s.canvas.bottom()) {
          g->rect.y -= (left_window_control.y + left_window_control.height) -
                       s.canvas.bottom();
        }
        if (right_window_control.x < s.canvas.left()) {
          g->rect.x -= right_window_control.x - s.canvas.left();
        }
        if (right_window_control.y < s.canvas.top()) {
          g->rect.y -= right_window_control.y - s.canvas.top();
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

      SUB_ID(split_move_handle_id, g->id.id + i);
      DuiId split_move_handle_hot =
          do_hot(split_move_handle_id, split_move_handle, g->root.get());
      DuiId split_move_handle_active   = do_active(split_move_handle_id);
      DuiId split_move_handle_dragging = do_dragging(split_move_handle_id);

      if (split_move_handle_hot || split_move_handle_dragging) {
        set_cursor_shape(Platform::CursorShape::VERT_RESIZE);
      }

      if (split_move_handle_dragging) {
        f32 delta_px = s.input->mouse_pos.y - split_move_handle.y;

        f32 min_px_up =
            get_minimum_size(g->splits[i - 1].child.get(),
                             g->splits[i - 1].div_pct * g->rect.height, 0, 1);
        f32 min_px_down =
            get_minimum_size(g->splits[i].child.get(),
                             g->splits[i].div_pct * g->rect.height, 0, 0);

        b8 too_far_up =
            min_px_up > g->splits[i - 1].child.get()->rect.height + delta_px;
        b8 too_far_down =
            min_px_down > g->splits[i].child.get()->rect.height - delta_px;
        if (too_far_up && too_far_down) {
          break;
        } else if (too_far_up) {
          delta_px = min_px_up - g->splits[i - 1].child.get()->rect.height;
        } else if (too_far_down) {
          delta_px = -(min_px_down - g->splits[i].child.get()->rect.height);
        }

        f32 move_pct = delta_px / g->rect.height;

        f32 up_pct_change =
            (g->splits[i - 1].div_pct + move_pct) / g->splits[i - 1].div_pct;
        f32 down_pct_change =
            (g->splits[i].div_pct - move_pct) / g->splits[i].div_pct;

        resize_border_splits_and_propagate(g->splits[i - 1].child.get(),
                                           up_pct_change, 0, 1);
        resize_border_splits_and_propagate(g->splits[i].child.get(),
                                           down_pct_change, 0, 0);
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

      SUB_ID(split_move_handle_id, g->id.id + i);
      DuiId split_move_handle_hot =
          do_hot(split_move_handle_id, split_move_handle, g->root.get());
      DuiId split_move_handle_active   = do_active(split_move_handle_id);
      DuiId split_move_handle_dragging = do_dragging(split_move_handle_id);

      if (split_move_handle_hot || split_move_handle_dragging) {
        set_cursor_shape(Platform::CursorShape::HORIZ_RESIZE);
      }

      if (split_move_handle_dragging) {
        f32 delta_px = s.input->mouse_pos.x - split_move_handle.x;

        f32 min_px_left =
            get_minimum_size(g->splits[i - 1].child.get(),
                             g->splits[i - 1].div_pct * g->rect.width, 1, 1);
        f32 min_px_right =
            get_minimum_size(g->splits[i].child.get(),
                             g->splits[i].div_pct * g->rect.width, 1, 0);

        b8 too_far_left =
            min_px_left > g->splits[i - 1].child.get()->rect.width + delta_px;
        b8 too_far_right =
            min_px_right > g->splits[i].child.get()->rect.width - delta_px;
        if (too_far_left && too_far_right) {
          break;
        } else if (too_far_left) {
          delta_px = min_px_left - g->splits[i - 1].child.get()->rect.width;
        } else if (too_far_right) {
          delta_px = -(min_px_right - g->splits[i].child.get()->rect.width);
        }

        f32 move_pct = delta_px / g->rect.width;

        f32 left_pct_change =
            (g->splits[i - 1].div_pct + move_pct) / g->splits[i - 1].div_pct;
        f32 right_pct_change =
            (g->splits[i].div_pct - move_pct) / g->splits[i].div_pct;

        resize_border_splits_and_propagate(g->splits[i - 1].child.get(),
                                           left_pct_change, 1, 1);
        resize_border_splits_and_propagate(g->splits[i].child.get(),
                                           right_pct_change, 1, 0);
        g->splits[i - 1].div_pct *= left_pct_change;
        g->splits[i].div_pct *= right_pct_change;
      }
    }
  }

  s.cg = nullptr;

  for (i32 i = 0; i < g->splits.size; i++) {
    start_frame_for_group(g->splits[i].child.get());
  }
}

void start_frame(Input *input, Platform::GlfwWindow *window)
{
  s.input = input;

  s.window_span = window->get_size();
  s.canvas      = {0, 0, s.window_span.x, s.window_span.y};

  s.frame++;

  s.just_started_being_hot      = -1;
  s.just_stopped_being_hot      = -1;
  s.just_started_being_active   = -1;
  s.just_stopped_being_active   = -1;
  s.just_started_being_dragging = -1;
  s.just_stopped_being_dragging = -1;
  s.just_started_being_selected = -1;
  s.just_stopped_being_selected = -1;

  s.cursor_shape = Platform::CursorShape::NORMAL;

  draw_system_start_frame(&s.dl);

  if (s.menubar_visible) {
    s.canvas.y += MENUBAR_HEIGHT;
    s.canvas.height -= MENUBAR_HEIGHT;
  }

  // handle pending changes
  if (s.snap_group_to_snap.need_to) {
    if (s.snap_group_to_snap.axis != -1) {
      snap_group(s.snap_group_to_snap.g.get(),
                 s.snap_group_to_snap.target.get(), s.snap_group_to_snap.axis,
                 s.snap_group_to_snap.dir);
    } else {
      Group *g           = s.snap_group_to_snap.g.get();
      Group *empty_group = s.empty_group.get();

      empty_group->splits            = g->splits;
      empty_group->split_axis        = g->split_axis;
      empty_group->windows           = g->windows;
      empty_group->active_window_idx = g->active_window_idx;

      Group *first_leaf_node = empty_group;
      while (!first_leaf_node->is_leaf()) {
        first_leaf_node = first_leaf_node->splits[0].child.get();
      }
      s.empty_group = first_leaf_node->id;

      free_group(g);
    }
  }
  s.snap_group_to_snap.need_to = false;

  // figure out the top groups and containers first
  s.top_container_at_mouse_pos = -1;
  s.top_container_is_popup     = false;
  for (i32 i = 0; i < s.popups.size; i++) {
    Container *popup_c = &s.popups[i].container;
    if (in_rect(s.input->mouse_pos, popup_c->rect)) {
      s.top_container_at_mouse_pos = popup_c->id;
      s.top_container_is_popup     = true;
    }
  }
  if (s.top_container_at_mouse_pos == -1 &&
      s.top_root_group_at_mouse_pos != -1) {
    s.top_container_at_mouse_pos =
        s.top_root_group_at_mouse_pos.get()->get_child_at_pos(
            s.input->mouse_pos);
  }

  s.top_root_group_at_mouse_pos     = -1;
  i32 top_root_group_at_mouse_pos_z = -1;
  if (!s.top_container_is_popup) {
    for (i32 i = 0; i < s.root_groups.size; i++) {
      Rect rect          = s.root_groups[i].get()->rect;
      Rect oversize_rect = {rect.x - RESIZE_HANDLES_OVERSIZE,
                            rect.y - RESIZE_HANDLES_OVERSIZE,
                            rect.width + RESIZE_HANDLES_OVERSIZE * 2,
                            rect.height + RESIZE_HANDLES_OVERSIZE * 2};
      if (in_rect(s.input->mouse_pos, oversize_rect)) {
        s.top_root_group_at_mouse_pos = s.root_groups[i];
        top_root_group_at_mouse_pos_z = i;
        break;
      }
    }
    if (s.input->mouse_button_down_events[(i32)MouseButton::LEFT]) {
      if (top_root_group_at_mouse_pos_z != -1) {
        Group *g = s.root_groups[top_root_group_at_mouse_pos_z].get();
        if (g->id != s.fullscreen_group) {
          s.root_groups.shift_delete(top_root_group_at_mouse_pos_z);
          s.root_groups.insert(0, g->id);
        }
      }
    }
  }

  // one pass for input handling
  for (i32 i = 0; i < s.root_groups.size; i++) {
    Group *g = s.root_groups[i].get();
    start_frame_for_group(g);
  }

  // one pass to propagate changes
  for (i32 i = 0; i < s.root_groups.size; i++) {
    Group *root_group = s.root_groups[i].get();
    root_group->z     = i + 1;
    propagate_groups(root_group);
  }

  // one pass for drawing, in reverse z order
  for (i32 i = s.root_groups.size - 1; i >= 0; i--) {
    Group *g = s.root_groups[i].get();
    draw_group_and_children(g);
  }

  if ((!s.top_container_is_popup &&
       input->mouse_button_down_events[(i32)MouseButton::LEFT]) ||
      s.clear_popups) {
    s.popups.clear();
    s.clear_popups = false;
  }
}

void end_frame(Platform::GlfwWindow *window, Gpu::Device *device,
               Gpu::Pipeline pipeline)
{
  if (s.menubar_visible) {
    Rect menubar_rect = {0, 0, s.window_span.x, MENUBAR_HEIGHT};
    push_rect(&s.dl, 0, menubar_rect, d);

    String menuitems[]           = {"File", "Edit", "View", "Window"};
    f32 next_menubar_item_offset = MENUBAR_MARGIN;
    for (i32 i = 0; i < 4; i++) {
      DuiId menubar_item_id = hash("MenubarItem", menuitems[i]);

      Rect menubar_item_rect = {
          next_menubar_item_offset - MENUBAR_MARGIN, MENUBAR_MARGIN,
          s.dl.vfont.get_text_width(menuitems[i], MENUBAR_FONT_SIZE) +
              MENUBAR_MARGIN * 2,
          MENUBAR_FONT_SIZE};

      b8 hot     = do_hot(menubar_item_id, menubar_item_rect);
      b8 active  = do_active(menubar_item_id);
      b8 clicked = hot && s.just_stopped_being_active == menubar_item_id;

      if (clicked) {
        open_popup(menuitems[i], {menubar_item_rect.x, MENUBAR_HEIGHT}, 200);
      }

      if (start_popup(menuitems[i])) {
        submenu({menubar_item_rect.x + 200, MENUBAR_HEIGHT});
        end_popup();
      }

      if (hot) {
        push_rounded_rect(&s.dl, 0, menubar_item_rect, 4, {.7, .7, .7, .5});
      }
      push_vector_text(&s.dl, 0, menuitems[i],
                       {next_menubar_item_offset, MENUBAR_MARGIN}, {1, 1, 1, 1},
                       MENUBAR_FONT_SIZE);

      next_menubar_item_offset +=
          s.dl.vfont.get_text_width(menuitems[i], MENUBAR_FONT_SIZE) +
          MENUBAR_MARGIN * 2;
    }
  }

  draw_system_end_frame(&s.dl, device, pipeline, s.window_span, s.frame);

  window->set_cursor_shape(s.cursor_shape);
}

DuiId start_window(String name, Rect initial_rect)
{
  DuiId id = hash(name);

  if (s.cw) {
    warning("starting a window without ending another!");
    return id;
  }

  Container *c = s.containers.wrapped_exists(id)
                     ? &s.containers.wrapped_get(id)
                     : create_new_window(id, name, initial_rect);
  s.cw         = c;

  Group *parent                  = c->parent.get();
  DuiId parents_active_window_id = parent->windows[parent->active_window_idx];
  if (parents_active_window_id != id) {
    return id;
  }

  c->start_frame(&s);

  return id;
}

void end_window()
{
  if (s.cc) {
    s.cc->end_frame(&s);
  }

  if (s.cw) {
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
  if (!c) return;

  c->next_line();
}

b8 button(String text, Vec2f size, Color color, b8 fill)
{
  Container *c = get_current_container(&s);
  if (!c) return false;

  DuiId id = extend_hash((DuiId)c, text);

  Rect rect = c->place(size, true, fill);

  b8 hot     = do_hot(id, rect, c->rect);
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

  if (hot) {
    set_cursor_shape(Platform::CursorShape::POINTING_HAND);
    color = darken(color, .1f);
  }

  push_rounded_rect(&s.dl, c->z, rect, 5.f, color);
  push_rounded_rect(&s.dl, c->z, inset(rect, 1.5f), 5.f, darken(color, .2f));
  push_vector_text_centered(&s.dl, c->z, text, rect.center(), {1, 1, 1, 1},
                            CONTENT_FONT_HEIGHT, {true, true});

  return clicked;
}

void texture(Vec2f size, u32 texture_id)
{
  Container *c = get_current_container(&s);
  if (!c) return;

  Rect rect = c->place(size, true);

  push_texture_rect(&s.dl, c->z, rect, {0, 1, 1, 0}, texture_id);
}

template <u64 N>
void text_input(StaticString<N> *str)
{
  Container *c = get_current_container(&s);
  if (!c) return;

  DuiId id = extend_hash((DuiId)c, (DuiId)str);

  f32 height       = CONTENT_FONT_HEIGHT + 4 + 4;
  Rect border_rect = c->place({0, height}, true, true);
  Rect rect        = inset(border_rect, 1);

  b8 hot      = do_hot(id, rect, c->rect);
  b8 active   = do_active(id);
  b8 dragging = do_dragging(id);
  b8 selected = do_selected(id, true);
  b8 clicked  = hot && s.just_started_being_active == id;

  if (hot) {
    set_cursor_shape(Platform::CursorShape::IBEAM);
  }

  i32 cursor_idx          = 0;
  i32 highlight_start_idx = 0;
  f32 text_pos            = rect.x + 3;

  if (selected && s.just_started_being_selected != id) {
    cursor_idx          = s.cursor_idx;
    highlight_start_idx = s.highlight_start_idx;
    text_pos            = s.text_pos;
  }

  static f32 cursor_blink_time = 0.f;
  cursor_blink_time += 1 / 60.f;
  if (s.just_started_being_selected == id) {
    cursor_blink_time = 0.f;
  }

  f32 font_size = CONTENT_FONT_HEIGHT;

  f32 cursor_pos = text_pos + s.dl.vfont.get_text_width(
                                  str->to_str().sub(0, cursor_idx), font_size);
  if (cursor_pos < rect.x) {
    // add one just to make sure cursor is fully in the rect.
    text_pos += rect.x - cursor_pos + 1;
  }
  if (cursor_pos > rect.x + rect.width) {
    // add one just to make sure cursor is fully in the rect.
    text_pos -= cursor_pos - (rect.x + rect.width) + 1;
  }

  auto reset_cursor = [&]() {
    highlight_start_idx = cursor_idx;
    cursor_blink_time   = 0.f;
  };
  auto clear_highlight = [&]() {
    if (highlight_start_idx != cursor_idx) {
      u32 from = std::min(highlight_start_idx, cursor_idx);
      u32 to   = std::max(highlight_start_idx, cursor_idx);
      str->shift_delete_range(from, to);

      cursor_idx          = from;
      highlight_start_idx = from;
    }
  };
  auto get_highlighted_text = [&]() {
    u32 from = std::min(highlight_start_idx, cursor_idx);
    u32 to   = std::max(highlight_start_idx, cursor_idx);

    return String(str->data + from, to - from);
  };

  auto copy = [&]() { s.clipboard = get_highlighted_text(); };
  auto cut  = [&]() {
    s.clipboard = get_highlighted_text();
    clear_highlight();
  };
  auto paste = [&]() {
    clear_highlight();
    for (int i = 0; i < s.clipboard.size; i++) {
      if (str->size < str->MAX_SIZE) {
        str->push_middle(s.clipboard[i], cursor_idx);
        cursor_idx++;
        reset_cursor();
      }
    }
  };

  if (clicked || s.just_started_being_selected == id) {
    cursor_idx = s.dl.vfont.char_index_at_pos(
        str->to_str(), {text_pos, rect.y + (rect.height / 2)},
        s.input->mouse_pos, font_size);
    reset_cursor();
  }
  if (dragging) {
    if (s.just_started_being_dragging == id) {
      highlight_start_idx = cursor_idx;
    }
    cursor_idx = s.dl.vfont.char_index_at_pos(
        str->to_str(), {text_pos, rect.y + (rect.height / 2)},
        s.input->mouse_pos, font_size);
  }

  if (selected) {
    if (s.input->key_down_events[(i32)Keys::LEFT]) {
      cursor_idx--;
      if (s.input->keys[(i32)Keys::LCTRL]) {
        while (cursor_idx > 0 && !std::isspace((*str)[cursor_idx - 1])) {
          cursor_idx--;
        }
      }
      if (!s.input->keys[(i32)Keys::LSHIFT]) {
        reset_cursor();
      }
    }
    if (s.input->key_down_events[(i32)Keys::RIGHT]) {
      cursor_idx++;
      if (s.input->keys[(i32)Keys::LCTRL]) {
        while (cursor_idx < str->size && !std::isspace((*str)[cursor_idx])) {
          cursor_idx++;
        }
      }
      if (!s.input->keys[(i32)Keys::LSHIFT]) {
        reset_cursor();
      }
    }
    if (s.input->key_down_events[(i32)Keys::HOME]) {
      cursor_idx = 0;
      if (!s.input->keys[(i32)Keys::LSHIFT]) {
        reset_cursor();
      }
    }
    if (s.input->key_down_events[(i32)Keys::END]) {
      cursor_idx = str->size;
      if (!s.input->keys[(i32)Keys::LSHIFT]) {
        reset_cursor();
      }
    }

    if (s.input->key_down_events[(i32)Keys::BACKSPACE]) {
      if (highlight_start_idx == cursor_idx) {
        if (cursor_idx > 0) {
          str->shift_delete(cursor_idx - 1);
        }
        cursor_idx--;
      } else {
        clear_highlight();
      }
      reset_cursor();
    }
    if (s.input->key_down_events[(i32)Keys::DEL]) {
      if (highlight_start_idx == cursor_idx) {
        if (cursor_idx < str->size) {
          str->shift_delete(cursor_idx);
        }
      } else {
        clear_highlight();
      }
      reset_cursor();
    }

    if (s.input->keys[(i32)Keys::LCTRL]) {
      if (s.input->key_down_events[(i32)Keys::C]) {
        copy();
      }
      if (s.input->key_down_events[(i32)Keys::X]) {
        cut();
      }
      if (s.input->key_down_events[(i32)Keys::V]) {
        paste();
      }
    }

    if (s.input->text_input.size > 0) {
      clear_highlight();
    }
    for (int i = 0; i < s.input->text_input.size; i++) {
      if (str->size < str->MAX_SIZE) {
        str->push_middle(s.input->text_input[i], cursor_idx);
        cursor_idx++;
        reset_cursor();
      }
    }
  }

  cursor_idx          = clamp(cursor_idx, 0, str->size);
  highlight_start_idx = clamp(highlight_start_idx, 0, str->size);

  if (hot && s.input->mouse_button_down_events[(i32)MouseButton::RIGHT]) {
    open_popup(id, s.input->mouse_pos, 200);
  }
  if (start_popup(id)) {
    if (menu_item_submenu("Edit", "edit")) {
      if (menu_item_button("Copy")) {
        copy();
      }
      if (menu_item_button("Cut")) {
        cut();
      }
      if (menu_item_button("Paste")) {
        paste();
      }
      end_popup();
    }

    menu_divider();

    if (menu_item_button("Undo")) {
    }
    if (menu_item_button("Redo")) {
    }

    end_popup();
  }

  f32 highlight_start_pos =
      text_pos + s.dl.vfont.get_text_width(
                     str->to_str().sub(0, highlight_start_idx), font_size);

  push_rect(&s.dl, c->z, border_rect, selected ? l : d_light);
  push_rect(&s.dl, c->z, rect, l_dark);

  push_scissor(&s.dl, border_rect);

  if (selected && highlight_start_idx != cursor_idx) {
    Rect highlight_rect = {fminf(highlight_start_pos, cursor_pos), rect.y + 3,
                           fabsf(cursor_pos - highlight_start_pos),
                           rect.height - 6};
    push_rect(&s.dl, c->z, highlight_rect, highlight);
  }

  push_vector_text_centered(&s.dl, c->z, str->to_str(),
                            {text_pos, rect.y + (rect.height / 2)},
                            {1, 1, 1, 1}, font_size, {false, true});

  if (selected) {
    Rect cursor_rect = {floorf(cursor_pos), rect.y + 3, 1.25f, rect.height - 6};
    Color cursor_color = highlight;
    cursor_color.a     = 1.f - (f32)((i32)(cursor_blink_time * 1.5) % 2);
    push_rect(&s.dl, c->z, cursor_rect, cursor_color);
  }
  pop_scissor(&s.dl);

  if (selected) {
    s.cursor_idx          = cursor_idx;
    s.highlight_start_idx = highlight_start_idx;
    s.text_pos            = text_pos;
  }
}

void start_list() {}

void end_list() {}

b8 directory_item(DuiId id, String text, b8 expandable, b8 selected = false)
{
  static StaticPool<b8, 2048> open_list_items(false);

  Container *c = get_current_container(&s);
  if (!c) return false;

  id = extend_hash((DuiId)c, id);

  f32 height = CONTENT_FONT_HEIGHT + 4 + 4;
  Rect rect  = c->place({0, height}, true, true);

  b8 hot      = do_hot(id, rect, c->rect);
  b8 active   = do_active(id);
  b8 dragging = do_dragging(id);
  selected    = do_selected(id, true) || selected;
  b8 clicked  = hot && s.just_started_being_active == id;

  b8 open = false;
  if (expandable) {
    open = open_list_items.wrapped_get(id);
    if (clicked) {
      open = !open;
      open_list_items.emplace_wrapped(id, open);
    }
  }

  Vec2f cursor = {rect.x + WINDOW_MARGIN_SIZE, rect.y + WINDOW_MARGIN_SIZE};

  if (hot && !selected) push_rect(&s.dl, c->z, rect, {.6, .6, .6, .5});
  if (selected) push_rect(&s.dl, c->z, rect, {.4, .4, .4, .5});

  if (expandable) {
    String arrow_icon = open ? "D" : "G";
    f32 arrow_x       = cursor.x;
    f32 arrow_width =
        fmaxf(s.dl.icon_font.get_text_width("D", CONTENT_FONT_HEIGHT),
              s.dl.icon_font.get_text_width("G", CONTENT_FONT_HEIGHT));
    cursor.x += arrow_width + WINDOW_MARGIN_SIZE;

    push_vector_text(&s.dl, &s.dl.icon_font, c->z, arrow_icon,
                     {arrow_x, cursor.y}, {1, 1, 1, 1}, CONTENT_FONT_HEIGHT);
  }

  String folder_icon = selected && expandable ? "A" : "B";
  f32 folder_x       = cursor.x;
  f32 folder_width =
      fmaxf(s.dl.icon_font.get_text_width("A", CONTENT_FONT_HEIGHT),
            s.dl.icon_font.get_text_width("B", CONTENT_FONT_HEIGHT));
  cursor.x += folder_width + WINDOW_MARGIN_SIZE;

  push_vector_text(&s.dl, &s.dl.icon_font, c->z, folder_icon,
                   {folder_x, cursor.y}, {1, 1, 1, 1}, CONTENT_FONT_HEIGHT);

  push_vector_text(&s.dl, &s.dl.vfont, c->z, text, {cursor.x, cursor.y},
                   {1, 1, 1, 1}, CONTENT_FONT_HEIGHT);

  return open;
}

void init_dui(Gpu::Device *device, Gpu::Pipeline pipeline)
{
  init_draw_system(&s.dl, device, pipeline);

  s.empty_group      = create_group(nullptr, {})->id;
  s.fullscreen_group = s.empty_group;
}

void asset_browser()
{
  start_window("Asset Browser", {100, 100, 900, 500});

  start_list();

  // {
  //   DuiId sub_container_id = extend_hash(s.cc->id, "subcontainer");

  //   Container *sub_container =
  //       s.containers.wrapped_exists(sub_container_id)
  //           ? &s.containers.wrapped_get(sub_container_id)
  //           : s.containers.emplace_wrapped(sub_container_id, {});

  //   sub_container->id   = sub_container_id;
  //   sub_container->z    = s.cc->z;
  //   sub_container->rect = s.cc->rect;
  //   sub_container->rect.width *= .25;

  //   sub_container->start_frame(&s, false);
  //   push_scissor(&s.dl, sub_container->rect);

  //   Array<String, 32> directories = {
  //       "Documents",   "Downloads", "Pictures",       "Garbage",
  //       "Other Stuff", "textures",  "project_config", ".vscode",
  //       "helpers",     "util",      "even more crap", "is this overflowing"};
  //   for (i32 i = 0; i < directories.size; i++) {
  //     DuiId id = hash(directories[i]);
  //     if (directory_item(id, directories[i], i % 2, false)) {
  //       button("inside tree view", {100, 40}, {1, 0, 1, 1});
  //       next_line();
  //     }
  //   }

  //   sub_container->end_frame(&s, false);
  //   pop_scissor(&s.dl);

  //   s.cc = s.cw;
  // }

  // {
  //   DuiId sub_container_id = extend_hash(s.cc->id, "subcontainer3");

  //   Container *sub_container =
  //       s.containers.wrapped_exists(sub_container_id)
  //           ? &s.containers.wrapped_get(sub_container_id)
  //           : s.containers.emplace_wrapped(sub_container_id, {});

  //   sub_container->id   = sub_container_id;
  //   sub_container->z    = s.cc->z;
  //   sub_container->rect = s.cc->rect;
  //   sub_container->rect.x += sub_container->rect.width * .25;
  //   sub_container->rect.width *= .75;

  //   sub_container->start_frame(&s, false);
  //   push_scissor(&s.dl, sub_container->rect);

  //   Array<String, 32> directories = {
  //       "Documents",   "Downloads", "Pictures",       "Garbage",
  //       "Other Stuff", "textures",  "project_config", ".vscode",
  //       "helpers",     "util",      "even more crap", "is this overflowing"};
  //   for (i32 i = 0; i < directories.size; i++) {
  //     DuiId id = hash(directories[i]);
  //     if (directory_item(id, directories[i], true, false)) {
  //       button("inside tree view", {100, 40}, {1, 0, 1, 1});
  //       next_line();
  //     }
  //   }

  //   sub_container->end_frame(&s, false);
  //   pop_scissor(&s.dl);

  //   s.cc = s.cw;
  // }

  end_list();

  end_window();
}

}  // namespace Dui

namespace Dui
{
b8 clicked(DuiId id)
{
  Container *c = get_current_container(&s);
  if (!c) return false;
  id = extend_hash((DuiId)c, id);
  return s.hot == id && s.just_started_being_active == id;
}
}  // namespace Dui

/*
  if (Dui::PopupOpenerMenuItem("File")) {
    Dui::OpenPopup("File")
  }
  Dui::StartPopup("File", pos, width);
  if (Dui::MenuButton("New")) {
    ... // new project / file dialog
  }

  if(Dui::PopupOpenerMenuItem("Preferences")) {
    Dui::OpenPopup("Preferences");
  }
  Dui::StartPopup("Preferences");

  Dui::EndPopup(); // "Preferences"


  Dui::EndPopup(); // "File"
*/

/*
  if (Dui::PopupOpenerMenuItem("File")) {
    Dui::open_popup("File")
  }

  DuiId popup_id = Dui::start_popup("File", pos, width);
  if (Dui::MenuButton("New")) {
    ... // new project / file dialog
  }

  if(Dui::PopupOpenerMenuItem("Preferences")) {
    Dui::OpenPopup("Preferences");
  }
  Dui::StartPopup("Preferences");

  Dui::EndPopup(); // "Preferences"


  Dui::EndPopup(); // "File"
*/

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
#include "group.cc"