#pragma once

#include "types.hpp"

f32 clamp(f32 d, f32 min, f32 max)
{
  const f32 t = d < min ? min : d;
  return t > max ? max : t;
}

i32 clamp(i32 d, i32 min, i32 max)
{
  const i32 t = d < min ? min : d;
  return t > max ? max : t;
}