#include "dui/group.hpp"

namespace Dui
{

Rect Group::get_titlebar_full_rect()
{
  Rect titlebar_rect;
  titlebar_rect.x      = rect.x;
  titlebar_rect.y      = rect.y;
  titlebar_rect.width  = rect.width;
  titlebar_rect.height = TITLEBAR_HEIGHT;

  return titlebar_rect;
}

Rect Group::get_titlebar_margin_rect()
{
  Rect titlebar_full_rect = get_titlebar_full_rect();
  titlebar_full_rect.height -= TITLEBAR_BOTTOM_BORDER_HEIGHT;

  return titlebar_full_rect;
}

Rect Group::get_titlebar_bottom_border_rect()
{
  Rect titlebar_full_rect = get_titlebar_full_rect();

  Rect rect;
  rect.x = titlebar_full_rect.x;
  rect.y = titlebar_full_rect.y + titlebar_full_rect.height -
           TITLEBAR_BOTTOM_BORDER_HEIGHT;
  rect.width  = titlebar_full_rect.width;
  rect.height = TITLEBAR_BOTTOM_BORDER_HEIGHT;
  return rect;
}

Rect Group::get_titlebar_content_rect()
{
  return inset(get_titlebar_margin_rect(), WINDOW_MARGIN_SIZE);
}

Rect Group::get_tabs_rect()
{
  Rect titlebar_content_rect = get_titlebar_content_rect();

  // shrink left and right sides for window controls
  Rect tabs_rect;
  tabs_rect.x      = titlebar_content_rect.x + WINDOW_CONTROL_WIDTH;
  tabs_rect.y      = titlebar_content_rect.y;
  tabs_rect.width  = titlebar_content_rect.width - (WINDOW_CONTROL_WIDTH * 2);
  tabs_rect.height = titlebar_content_rect.height + WINDOW_MARGIN_SIZE;

  return tabs_rect;
}
f32 Group::get_combined_extra_desired_tab_space()
{
  f32 base_tab_width = get_base_tab_width();
  f32 total          = 0.f;
  for (i32 i = 0; i < windows.size; i++) {
    total += fmaxf(get_desired_tab_width(i) - base_tab_width, 0);
  }
  return total;
}
f32 Group::get_available_extra_tab_space()
{
  Rect tabs_rect          = get_tabs_rect();
  f32 combined_tab_gap    = (windows.size - 1) * TAB_GAP;
  f32 available_tab_space = tabs_rect.width - combined_tab_gap;

  f32 base_tab_width = get_base_tab_width();

  return available_tab_space - (base_tab_width * windows.size);
}
f32 Group::get_base_tab_width()
{
  Rect tabs_rect       = get_tabs_rect();
  f32 combined_tab_gap = (windows.size - 1) * TAB_GAP;
  return fminf((tabs_rect.width - combined_tab_gap) / windows.size,
               DEFAULT_TAB_WIDTH);
}

f32 Group::get_desired_tab_width(i32 window_idx)
{
  Container *window = get_container(windows[window_idx]);
  return get_text_width(s.font, window->title) + TAB_MARGIN * 2;
}

Vec2f Group::get_tab_margin_span(i32 window_idx,
                                 f32 combined_extra_desired_tab_space,
                                 f32 available_extra_tab_space)
{
  Rect tabs_rect = get_tabs_rect();

  f32 base_width    = get_base_tab_width();
  f32 desired_width = get_desired_tab_width(window_idx);

  f32 extra_desired_tab_space = fmax(desired_width - base_width, 0);
  f32 extra_tab_space_share_pct =
      extra_desired_tab_space / combined_extra_desired_tab_space;
  f32 extra_width = fminf(extra_tab_space_share_pct * available_extra_tab_space,
                          extra_desired_tab_space);

  f32 width = base_width + extra_width;

  return {width, tabs_rect.height};
}

Rect Group::get_tab_margin_rect(i32 window_idx)
{
  f32 combined_extra_desired_tab_space = get_combined_extra_desired_tab_space();
  f32 available_extra_tab_space        = get_available_extra_tab_space();

  Rect rect;
  Vec2f tab_pos = get_tabs_rect().xy();
  for (i32 w_i = 0; w_i <= window_idx; w_i++) {
    Vec2f tab_span = get_tab_margin_span(w_i, combined_extra_desired_tab_space,
                                         available_extra_tab_space);

    rect = {tab_pos.x, tab_pos.y, tab_span.x, tab_span.y};
    tab_pos.x += rect.width + TAB_GAP;
  }

  return rect;
}

i32 Group::get_tab_at_pos(Vec2f pos)
{
  for (i32 i = 0; i < windows.size; i++) {
    Rect tab_rect = get_tab_margin_rect(i);
    if (pos.x >= tab_rect.x && pos.x < tab_rect.x + tab_rect.width) return i;
  }
  return 0;
}

Rect Group::get_border_rect()
{
  Rect titlebar_rect = get_titlebar_full_rect();

  Rect window_border_rect;
  window_border_rect.x      = rect.x;
  window_border_rect.y      = rect.y + titlebar_rect.height;
  window_border_rect.width  = rect.width;
  window_border_rect.height = rect.height - TITLEBAR_HEIGHT;

  return window_border_rect;
}
Rect Group::get_window_rect()
{
  Rect window_border_rect = get_border_rect();
  return inset(window_border_rect, WINDOW_BORDER_SIZE);
}

i32 Group::get_window_idx(DuiId window_id)
{
  for (i32 i = 0; i < windows.size; i++) {
    if (windows[i] == window_id) return i;
  }
  return -1;
}
}  // namespace Dui