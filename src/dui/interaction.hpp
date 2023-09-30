#pragma once

#include "dui/dui_state.hpp"

namespace Dui
{
// when control is not tied to any group or container
b8 do_hot(DuiId id, Rect rect)
{
  b8 is_hot = in_rect(s.input->mouse_pos, rect);
  if (is_hot)
    s.set_hot(id);
  else
    s.clear_hot(id);

  return is_hot;
}

// for group controls
b8 do_hot(DuiId id, Rect rect, Group *root_group)
{
  b8 not_blocked_by_container = s.top_container_at_mouse_pos == -1;
  b8 is_in_top_group          = !s.top_root_group_at_mouse_pos.valid() ||
                       root_group->id == s.top_root_group_at_mouse_pos;

  b8 is_hot = not_blocked_by_container && is_in_top_group &&
              in_rect(s.input->mouse_pos, rect);
  if (is_hot)
    s.set_hot(id);
  else
    s.clear_hot(id);

  return is_hot;
}

// // for widgets inside windows
// b8 do_hot(DuiId id, Rect control_rect, Rect container_rect)
// {
//   b8 is_top_container     = !s.cw || s.cw->id == s.top_container_at_mouse_pos;
//   b8 is_in_container_rect = in_rect(s.input->mouse_pos, container_rect);

//   b8 is_hot = is_top_container && is_in_container_rect &&
//               in_rect(s.input->mouse_pos, control_rect);
//   if (is_hot)
//     s.set_hot(id);
//   else
//     s.clear_hot(id);

//   return is_hot;
// }

// for widgets inside popups
b8 do_hot(DuiId id, Rect control_rect, Container *popup)
{
  b8 is_top_container     = popup->id == s.top_container_at_mouse_pos;
  b8 is_in_container_rect = in_rect(s.input->mouse_pos, popup->rect);

  b8 is_hot = is_top_container && is_in_container_rect &&
              in_rect(s.input->mouse_pos, control_rect);
  if (is_hot)
    s.set_hot(id);
  else
    s.clear_hot(id);

  return is_hot;
}

b8 do_active(DuiId id)
{
  // TODO handle left and right mouse buttons

  if (s.input->mouse_button_up_events[(i32)MouseButton::LEFT]) {
    s.clear_active(id);
  }
  if (s.input->mouse_button_down_events[(i32)MouseButton::LEFT]) {
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

b8 do_selected(DuiId id, b8 on_down = false)
{
  b8 clicked = s.just_stopped_being_active == id;
  if (on_down) {
    clicked = s.just_started_being_active == id;
  }
  if (s.is_hot(id) && clicked && s.just_stopped_being_dragging != id) {
    s.set_selected(id);
  } else if (!s.top_container_is_popup &&
             s.input->mouse_button_down_events[(i32)MouseButton::LEFT] &&
             s.just_started_being_active != id) {
    s.clear_selected(id);
  }

  s.was_last_control_selected = s.is_selected(id);
  return s.was_last_control_selected;
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
    Vec2f clamped_mouse_pos    = clamp_to_rect(s.input->mouse_pos, s.canvas);
    Vec2f dragging_total_delta = clamped_mouse_pos - s.start_position_active;
    s.dragging_frame_delta     = dragging_total_delta - s.dragging_total_delta;
    s.dragging_total_delta     = dragging_total_delta;

  } else {
    s.clear_dragging(id);
  }

  s.was_last_control_dragging = s.is_dragging(id);
  return s.is_dragging(id);
}

}  // namespace Dui
