#include "dui/container.hpp"

namespace Dui
{

void Container::start_frame(DuiState *s, b8 is_popup)
{
  if (last_frame_started == s->frame) {
    return;
  }
  last_frame_started = s->frame;
  last_frame         = s->frame;

  last_frame_minimum_content_span    = current_frame_minimum_content_span;
  current_frame_minimum_content_span = {0, 0};

  cursor      = {0, 0};
  cursor_size = 0;

  content_rect = get_content_rect();

  s->cc.push_back(this);

  {
    b8 has_scrollwheel_focus =
        (!s->cw || s->top_container_at_mouse_pos == s->cw->id) &&
        in_rect(s->input->mouse_pos, content_rect);

    // FYI doing this here can cause the position of the scrollbar to lag behind
    // a moving container. Its ok though because it will still be drawn in the
    // correct position, and you shouldn't be interacting with a scrollbar while
    // moving a container anyway.
    if (last_frame_minimum_content_span.y > content_rect.height) {
      Color color = highlight;

      f32 scrollbar_height = content_rect.height /
                             last_frame_minimum_content_span.y *
                             content_rect.height;
      f32 scrollbar_range = content_rect.height - scrollbar_height;

      f32 scrollbar_y = -scroll_offset_target.y /
                        last_frame_minimum_content_span.y * content_rect.height;

      Engine::Rect scrollbar_area_rect =
          Engine::Rect{content_rect.x + content_rect.width + SCROLLBAR_BORDER,
               content_rect.y, SCROLLBAR_SIZE, content_rect.height};

      Engine::Rect scrollbar_rect =
          Engine::Rect{content_rect.x + content_rect.width + SCROLLBAR_BORDER,
               content_rect.y + scrollbar_y, SCROLLBAR_SIZE, scrollbar_height};

      DuiId vertical_scrollbar_id = extend_hash(id, "vertical_scrollbar");
      b8 hot      = do_hot(s, vertical_scrollbar_id, scrollbar_area_rect);
      b8 active   = do_active(vertical_scrollbar_id);
      b8 dragging = do_dragging(vertical_scrollbar_id);
      if (hot) {
        color = lighten(color, .2f);
      }
      if (s->just_started_being_dragging == vertical_scrollbar_id) {
        s->dragging_start_local_position.y =
            s->input->mouse_pos.y - scrollbar_rect.y;
        if (s->dragging_start_local_position.y < 0 ||
            s->dragging_start_local_position.y > scrollbar_rect.height) {
          s->dragging_start_local_position.y = scrollbar_rect.height / 2;
        }
      }
      if (dragging) {
        scrollbar_y += (s->input->mouse_pos.y - scrollbar_rect.y) -
                       s->dragging_start_local_position.y;
        scrollbar_y    = clamp(scrollbar_y, 0.f, scrollbar_range);
        scrollbar_rect = Engine::Rect{
            content_rect.x + content_rect.width + SCROLLBAR_BORDER,
            content_rect.y + scrollbar_y, SCROLLBAR_SIZE, scrollbar_height};

        scroll_offset_target.y =
            ((scrollbar_y / scrollbar_range)) *
            (-last_frame_minimum_content_span.y + content_rect.height);
      }

      push_rounded_rect(&s->dl, z, scrollbar_area_rect,
                        scrollbar_rect.width / 2.f,
                        {color.r, color.g, color.b, .5});
      push_rounded_rect(&s->dl, z, scrollbar_rect, scrollbar_rect.width / 2.f,
                        color);

      if (has_scrollwheel_focus && !s->input->keys[(i32)Keys::LSHIFT]) {
        scroll_offset_target.y += s->input->scrollwheel_count * 100.f;
      }
    }
    if (last_frame_minimum_content_span.x > content_rect.width) {
      Color color = highlight;

      f32 scrollbar_width = content_rect.width /
                            last_frame_minimum_content_span.x *
                            content_rect.width;
      f32 scrollbar_range = content_rect.width - scrollbar_width;

      f32 scrollbar_x = -scroll_offset_target.x /
                        last_frame_minimum_content_span.x * content_rect.width;

      Engine::Rect scrollbar_area_rect =
          Engine::Rect{content_rect.x,
               content_rect.y + content_rect.height + SCROLLBAR_BORDER,
               content_rect.width, SCROLLBAR_SIZE};
      Engine::Rect scrollbar_rect =
          Engine::Rect{content_rect.x + scrollbar_x,
               content_rect.y + content_rect.height + SCROLLBAR_BORDER,
               scrollbar_width, SCROLLBAR_SIZE};

      DuiId horizontal_scrollbar_id = extend_hash(id, "horizontal_scrollbar");
      b8 hot      = do_hot(s, horizontal_scrollbar_id, scrollbar_area_rect);
      b8 active   = do_active(horizontal_scrollbar_id);
      b8 dragging = do_dragging(horizontal_scrollbar_id);
      if (hot) {
        color = lighten(color, .2f);
      }
      if (s->just_started_being_dragging == horizontal_scrollbar_id) {
        s->dragging_start_local_position.x =
            s->input->mouse_pos.x - scrollbar_rect.x;
        if (s->dragging_start_local_position.x < 0 ||
            s->dragging_start_local_position.x > scrollbar_rect.width) {
          s->dragging_start_local_position.x = scrollbar_rect.width / 2;
        }
      }
      if (dragging) {
        scrollbar_x += (s->input->mouse_pos.x - scrollbar_rect.x) -
                       s->dragging_start_local_position.x;
        scrollbar_x = clamp(scrollbar_x, 0.f, scrollbar_range);
        scrollbar_rect =
            Engine::Rect{content_rect.x + scrollbar_x,
                 content_rect.y + content_rect.height + SCROLLBAR_BORDER,
                 scrollbar_width, SCROLLBAR_SIZE};

        scroll_offset_target.x =
            ((scrollbar_x / scrollbar_range)) *
            (-last_frame_minimum_content_span.x + content_rect.width);
      }

      push_rounded_rect(&s->dl, z, scrollbar_area_rect,
                        scrollbar_rect.height / 2.f,
                        {color.r, color.g, color.b, .5});
      push_rounded_rect(&s->dl, z, scrollbar_rect, scrollbar_rect.height / 2.f,
                        color);

      if (has_scrollwheel_focus && s->input->keys[(i32)Keys::LSHIFT]) {
        scroll_offset_target.x += s->input->scrollwheel_count * 100.f;
      }
    }

    scroll_offset_target = clamp(
        scroll_offset_target,
        -last_frame_minimum_content_span + content_rect.span(), {0.f, 0.f});
    scroll_offset = (scroll_offset + scroll_offset_target) / 2.f;
  }

  if (!is_popup) {
    draw_scissor_idx = push_scissor(&s->dl, content_rect);
  }
}

void Container::end_frame(DuiState *s, b8 is_popup)
{
  if (!is_popup) {
    pop_scissor(&s->dl);
  }

  s->cc.pop();
}

void Container::fit_rect_to_content()
{
  rect.width  = current_frame_minimum_content_span.x + (WINDOW_MARGIN_SIZE * 2);
  rect.height = current_frame_minimum_content_span.y + (WINDOW_MARGIN_SIZE * 2);
}

void Container::expand_rect_to_content()
{
  rect.width  = fmaxf(rect.width, current_frame_minimum_content_span.x +
                                      (WINDOW_MARGIN_SIZE * 2));
  rect.height = fmaxf(rect.height, current_frame_minimum_content_span.y +
                                       (WINDOW_MARGIN_SIZE * 2));
}

Container *get_container(DuiId id) { return &s.containers.wrapped_get(id); }

Container *get_current_container(DuiState *s)
{
  if (s->cc.size == 0) return nullptr;
  Container *cc = s->cc.top();
  if (cc->parent.valid() &&
      cc->parent.get()->windows[cc->parent.get()->active_window_idx] != cc->id)
    return nullptr;

  return cc;
}

b8 Container::do_hot(DuiState *s, DuiId id, Engine::Rect control_rect)
{
  b8 is_top_window = !s->cw || s->cw->id == s->top_container_at_mouse_pos;
  b8 is_in_container_rect = in_rect(s->input->mouse_pos, content_rect);

  b8 is_hot = is_top_window && is_in_container_rect &&
              in_rect(s->input->mouse_pos, control_rect) && hot_mask;
  if (is_hot)
    s->set_hot(id);
  else
    s->clear_hot(id);

  return is_hot;
}
}  // namespace Dui