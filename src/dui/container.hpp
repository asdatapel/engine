#pragma once

#include "dui/basic.hpp"
#include "dui/group.hpp"
#include "input.hpp"
#include "types.hpp"

namespace Dui
{

const f32 SCROLLBAR_BORDER = 2.f;
const f32 SCROLLBAR_SIZE   = 8.f;

struct Container {
  DuiId id;
  StaticString<128> title;

  GroupId parent = -1;
  i32 z          = -1;

  Engine::Rect rect;
  Color color;
  b8 hot_mask  = true;
  f32 line_gap = DEFAULT_LINE_GAP;

  Vec2f scroll_offset_target = {0, 0};
  Vec2f scroll_offset        = {0, 0};

  Engine::Rect content_rect;
  Vec2f cursor = {0, 0};
  f32 cursor_size;

  // used to prevent start_frame from being called multiple times for the same
  // container.
  i64 last_frame_started = -1;
  i64 last_frame         = -1;

  // total space requested by content last frame. this is whats used to
  // determine free space and stretch controls to bounds.
  Vec2f last_frame_minimum_content_span    = {0, 0};
  Vec2f current_frame_minimum_content_span = {0, 0};

  u32 draw_scissor_idx = 0;

  Engine::Rect get_content_rect()
  {
    Engine::Rect rect = inset(this->rect, WINDOW_MARGIN_SIZE);

    b8 shrunk_height = false;
    if (last_frame_minimum_content_span.x > rect.width) {
      rect.height -= SCROLLBAR_SIZE;
      shrunk_height = true;
    }
    if (last_frame_minimum_content_span.y > rect.height) {
      rect.width -= SCROLLBAR_SIZE;

      // do this again because it might be true now
      if (!shrunk_height && last_frame_minimum_content_span.x > rect.width) {
        rect.height -= SCROLLBAR_SIZE;
      }
    }

    return rect;
  }

  void start_frame(DuiState *s, b8 is_popup = false);
  void end_frame(DuiState *s, b8 is_popup = false);

  void fit_rect_to_content();
  void expand_rect_to_content();

  Engine::Rect place(Vec2f size, b8 commit = true, b8 fill = false)
  {
    Vec2f scrolled_cursor = cursor + scroll_offset;

    Vec2f min_size    = size;
    Vec2f actual_size = size;
    Vec2f pos         = content_rect.xy() + scrolled_cursor;

    if (fill && pos.x + min_size.x < content_rect.x + content_rect.width) {
      actual_size.x = content_rect.x + content_rect.width - pos.x;
    }

    if (commit) {
      f32 min_cursor_size = fmaxf(cursor_size, min_size.y);
      current_frame_minimum_content_span.x =
          fmaxf(current_frame_minimum_content_span.x, cursor.x + min_size.x);
      current_frame_minimum_content_span.y = fmaxf(
          current_frame_minimum_content_span.y, cursor.y + min_cursor_size);

      cursor_size = fmaxf(cursor_size, actual_size.y);
      cursor.x += actual_size.x;
    }

    if (fill && commit) next_line();

    return {
        content_rect.x + scrolled_cursor.x,
        content_rect.y + scrolled_cursor.y,
        actual_size.x,
        actual_size.y,
    };
  }

  Engine::Rect get_remaining_rect()
  {
    f32 next_line_y = cursor.y + scroll_offset.y + cursor_size;
    return {content_rect.x, content_rect.y + next_line_y, content_rect.width,
            content_rect.height - next_line_y};
  }

  void next_line()
  {
    cursor.y += cursor_size + line_gap;
    cursor.x    = 0;
    cursor_size = 0;
  }

  b8 do_hot(DuiState *s, DuiId id, Engine::Rect control_rect);

  // stretch + minimum size (causing scroll)
  // shrink + minimum size (causing scroll) + maximum size
  // forced size (causing scroll)
  // anything that expands scroll affects last_frame_minimum_content_span
  // actiual remaining space is calculated by max(window_rect,
  // last_frame_minimum_content_span)
};

Container *get_container(DuiId id);
Container *get_current_container(DuiState *s);

}  // namespace Dui