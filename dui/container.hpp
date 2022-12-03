#pragma once

#include "dui/basic.hpp"
#include "dui/group.hpp"
#include "input.hpp"
#include "types.hpp"

namespace Dui
{
struct Container {
  DuiId id;
  StaticString<128> title;

  Group *parent = nullptr;

  Vec2f scroll_offset_target = {0, 0};
  Vec2f scroll_offset        = {0 ,0};

  Rect rect;
  Color color;

  Rect content_rect;
  Vec2f cursor = {0, 0};
  f32 cursor_size;

  // used to prevent start_frame from being called multiple times for the same
  // container.
  i64 last_frame_started = -1;
  i64 last_frame         = -1;
  // total space requested by content last frame. this is whats used to
  // determine free space and stretch controls to bounds.
  Vec2f last_frame_minimum_content_span     = {0, 0};
  Vec2f current_frame_minimum_content_span = {0, 0};
  f32 debug_last_frame_height              = 0;
  f32 debug_last_frame_width               = 0;

  Rect get_content_rect()
  {
    Rect rect = inset(this->rect, WINDOW_MARGIN_SIZE);

    if (last_frame_minimum_content_span.x > rect.width) {
      rect.height -= 10;
    }
    if (last_frame_minimum_content_span.y > rect.height) {
      rect.width -= 10;
    }

    return rect;
  }

  void start_frame(DuiState *s, Input *input, i64 current_frame);

  Rect place(Vec2f size, b8 commit = true, b8 fill = false)
  {
    Vec2f scrolled_cursor = cursor + scroll_offset;

    Rect rect;
    // if (vertical_layout) {
    //   rect.x      = 0;
    //   rect.y      = next_line_y;
    //   rect.width  = size.x;
    //   rect.height = size.y;

    //   if (stretch && rect.x + rect.width < content_rect.width) {
    //     rect.width += content_rect.width - (rect.x + rect.width);
    //   }
    // } else
    {
      rect.x      = content_rect.x + scrolled_cursor.x;
      rect.y      = content_rect.y + scrolled_cursor.y;
      rect.width  = size.x;
      rect.height = size.y;

      if (fill && rect.x + rect.width < content_rect.x + content_rect.width) {
        rect.width = content_rect.x + content_rect.width - rect.x;
      }
    }

    if (commit) {
      cursor_size = fmaxf(cursor_size, size.y + LINE_GAP);
      cursor.x += size.x;

      current_frame_minimum_content_span.x =
          fmaxf(current_frame_minimum_content_span.x, cursor.x);
      current_frame_minimum_content_span.y =
          fmaxf(current_frame_minimum_content_span.y, cursor.y + cursor_size);
    }

    if (fill) next_line();

    return rect;
  }

  Rect get_remaining_rect()
  {
    f32 next_line_y = cursor.y + scroll_offset.y + cursor_size;
    return {content_rect.x, content_rect.y + next_line_y, content_rect.width,
            content_rect.height - next_line_y};
  }

  void next_line()
  {
    cursor.y += cursor_size;
    cursor.x    = 0;
    cursor_size = 0;
  }

  // stretch + minimum size (causing scroll)
  // shrink + minimum size (causing scroll) + maximum size
  // forced size (causing scroll)
  // anything that expands scroll affects last_frame_minimum_content_span
  // actiual remaining space is calculated by max(window_rect,
  // last_frame_minimum_content_span)
};

Container *get_current_container(DuiState *s);

}  // namespace Dui