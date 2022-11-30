#pragma once

#include "math/vector.hpp"

struct Rect {
  f32 x = 0, y = 0;
  f32 width = 0, height = 0;

  Rect() {}
  Rect(f32 x, f32 y, f32 width, f32 height)
  {
    this->x      = x;
    this->y      = y;
    this->width  = width;
    this->height = height;
  }
  Rect(Vec2f pos, Vec2f size)
  {
    this->x      = pos.x;
    this->y      = pos.y;
    this->width  = size.x;
    this->height = size.y;
  }

  void set_bottom(f32 down) { height = down - y; }
  void set_right(f32 right) { width = right - x; }

  Vec2f xy() { return {x, y}; }
  Vec2f span() { return {width, height}; }
  Vec2f center() { return {x + (width / 2), y + (height / 2)}; }

  static Rect from_ends(Vec2f a, Vec2f b)
  {
    return Rect(Vec2f{fminf(a.x, b.x), fminf(a.y, b.y)},
                Vec2f{fabs(b.x - a.x), fabs(b.y - a.y)});
  }
};

inline bool operator==(const Rect &lhs, const Rect &rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width &&
         lhs.height == rhs.height;
}

inline bool operator!=(const Rect &lhs, const Rect &rhs)
{
  return !(lhs == rhs);
}

bool in_rect(Vec2f point, Rect rect, Rect mask = {})
{
  if (mask.width == 0 || mask.height == 0) {
    mask = rect;
  }

  return point.x > rect.x && point.x < rect.x + rect.width &&
         point.y > rect.y && point.y < rect.y + rect.height &&
         point.x > mask.x && point.x < mask.x + mask.width &&
         point.y > mask.y && point.y < mask.y + mask.height;
}

Rect inset(Rect rect, f32 inset)
{
  rect.x += inset;
  rect.y += inset;
  rect.width -= inset * 2;
  rect.height -= inset * 2;
  return rect;
}
Vec2f clamp_to_rect(Vec2f val, Rect rect)
{
  return max(min(val, rect.xy() + rect.span()), Vec2f{0, 0});
}