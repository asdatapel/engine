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
Rect Group::get_tab_margin_rect(i32 window_idx)
{
  Rect titlebar_content_rect = get_titlebar_content_rect();

  // shrink left and right sides for window controls
  Rect tabs_rect;
  tabs_rect.x      = titlebar_content_rect.x + WINDOW_CONTROL_WIDTH;
  tabs_rect.y      = titlebar_content_rect.y;
  tabs_rect.width  = titlebar_content_rect.width - (WINDOW_CONTROL_WIDTH * 2);
  tabs_rect.height = titlebar_content_rect.height + WINDOW_MARGIN_SIZE;

  const f32 UNSELECTED_TAB_WIDTH = 128.f;
  const f32 TAB_GAP              = 2.f;

  Container *window = get_container(windows[window_idx]);
  f32 tab_width     = fmaxf(UNSELECTED_TAB_WIDTH,
                            get_text_width(font, window->title) + TAB_MARGIN * 2);
  if (UNSELECTED_TAB_WIDTH * windows.size > tabs_rect.width) {
    tab_width -= ((UNSELECTED_TAB_WIDTH * windows.size) - tabs_rect.width) /
                 windows.size;
  }
  tab_width = fmaxf(tab_width, 0.f);

  Rect rect;
  rect.x      = tabs_rect.x + (tab_width + TAB_GAP) * window_idx;
  rect.y      = tabs_rect.y;
  rect.width  = tab_width;
  rect.height = tabs_rect.height;

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