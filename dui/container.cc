#include "dui/container.hpp"

namespace Dui
{
void Container::start_frame(DuiState *s, Input *input, i64 current_frame)
{
  if (parent->windows[parent->active_window_idx] != id) return;

  if (last_frame_started == current_frame) {
    return;
  }
  last_frame_started = current_frame;
  last_frame         = current_frame;

  last_frame_minimum_content_span    = current_frame_minimum_content_span;
  current_frame_minimum_content_span = {0, 0};

  cursor      = {0, 0};
  cursor_size = 0;

  // FYI doing this here can cause the position of the scrollbar to lag behind
  // a moving container. Its ok though because it will still be drawn in the
  // correct position, and you shouldn't be interacting with a scrollbar while
  // moving a container anyway.
  content_rect = get_content_rect();
  if (last_frame_minimum_content_span.x > content_rect.width) {
    if (input->keys[(i32)Keys::LSHIFT]) {
      scroll_offset_target.x += input->scrollwheel_count * 100.f;
    }
  }
  if (last_frame_minimum_content_span.y > content_rect.height) {
    if (!input->keys[(i32)Keys::LSHIFT]) {
      scroll_offset_target.y += input->scrollwheel_count * 100.f;
    }
  }

  scroll_offset_target =
      clamp(scroll_offset_target,
            -last_frame_minimum_content_span + content_rect.span(), {0.f, 0.f});
  scroll_offset = (scroll_offset + scroll_offset_target) / 2.f;

  s->cc = this;
  parent->root->dl->push_scissor(content_rect);
}

Container *get_current_container(DuiState *s)
{
  Container *cc = s->cc;
  if (cc->parent->windows[cc->parent->active_window_idx] != cc->id)
    return nullptr;

  return cc;
}
}  // namespace Dui