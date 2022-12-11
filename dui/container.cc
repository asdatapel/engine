#include "dui/container.hpp"

namespace Dui
{

// axis 0 is horizontal
void draw_scrollbar(DrawList *dl, Rect rect)
{
  auto push_line = [&](Vec2f p0, Vec2f p1, f32 thickness) {
    Vec2f direction = p1 - p0;
    Vec2f normal    = thickness * normalize(Vec2f(direction.y, -direction.x));

    Vec2f c0 = p0 + normal;
    Vec2f c1 = p0 - normal;
    Vec2f c2 = p1 - normal;
    Vec2f c3 = p1 + normal;

    push_quad(dl, c0, c1, c2, c3, highlight);
  };

  auto push_circle = [&](Vec2f c, f32 radius) {
    const f32 PI      = 3.1415;
    const f32 PI2     = 2 * PI;
    f32 circumference = PI2 * radius;
    i32 samples       = circumference;
    for (i32 i = 0; i < samples; i++) {
      f32 pct_a = (f32)i / samples;
      f32 pct_b = (f32)(i + 1) / samples;

      Vec2f radius_vec = {radius, radius};

      Vec2f p0 = c + Vec2f{cosf(pct_a * PI2), sinf(pct_a * PI2)} * radius_vec;
      Vec2f p1 = c + Vec2f{cosf(pct_b * PI2), sinf(pct_b * PI2)} * radius_vec;

      push_tri(dl, rect.center(), p0, p1, highlight);
    }
  };

  f32 end_radius = rect.width / 2.f;
  push_rect(dl, {rect.x, rect.y + end_radius, rect.width,
                 rect.height - end_radius * 2}, highlight);
  // push_circle(rect.xy() + Vec2f(end_radius, end_radius), end_radius);
  // push_circle(rect.xy() + rect.span() - Vec2f(end_radius, end_radius),
  //             end_radius);
}

void Container::start_frame(DuiState *s, Input *input, i64 current_frame)
{
  if (parent.get()->windows[parent.get()->active_window_idx] != id) return;

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
    f32 scrollbar_height =
        content_rect.height / last_frame_minimum_content_span.y * content_rect.height;
    f32 scrollbar_range = content_rect.height - scrollbar_height;

    f32 scrollbar_y = -scroll_offset_target.y /
                      last_frame_minimum_content_span.y * content_rect.height;

    Rect scrollbar_rect =
        Rect{content_rect.x + content_rect.width + SCROLLBAR_BORDER, content_rect.y + scrollbar_y,
             SCROLLBAR_SIZE, scrollbar_height};
    draw_scrollbar(parent.get()->root.get()->dl, scrollbar_rect);

    // push_rect(parent.get()->root.get()->dl,
    //           {content_rect.x + content_rect.width, rect.y + scrollbar_y, 10,
    //            scrollbar_height},
    //           {1, 0, 0, 1});
    if (!input->keys[(i32)Keys::LSHIFT]) {
      scroll_offset_target.y += input->scrollwheel_count * 100.f;
    }
  }

  scroll_offset_target =
      clamp(scroll_offset_target,
            -last_frame_minimum_content_span + content_rect.span(), {0.f, 0.f});
  scroll_offset = (scroll_offset + scroll_offset_target) / 2.f;

  s->cc = id;
  parent.get()->root.get()->dl->push_scissor(content_rect);
}

Container *get_container(DuiId id) { return &s.containers.wrapped_get(id); }

Container *get_current_container(DuiState *s)
{
  Container *cc = s->cc != -1 ? get_container(s->cc) : nullptr;
  if (!cc ||
      cc->parent.get()->windows[cc->parent.get()->active_window_idx] != cc->id)
    return nullptr;

  return cc;
}
}  // namespace Dui