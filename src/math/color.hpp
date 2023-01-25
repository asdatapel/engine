#pragma once

#include <math.h>

#include "math/math.hpp"

struct Color {
  f32 r, g, b, a;

  static Color from_int(i32 i)
  {
    return {(u8)(i >> 16) / 255.f, (u8)(i >> 8) / 255.f, (u8)i / 255.f, 1};
  }
};

u32 color_to_int(Color c)
{
  u32 r = (u32)(c.r * 255) & 0xFF;
  u32 g = (u32)(c.g * 255) & 0xFF;
  u32 b = (u32)(c.b * 255) & 0xFF;
  u32 a = (u32)(c.a * 255) & 0xFF;
  return (r << 24) | (g << 16) | (b << 8) | (a);
}

// http://marcocorvi.altervista.org/games/imgpr/rgb-hsl.htm
Color rgb_to_hsl(Color in)
{
  f32 h, s, l;

  // Find min and max values of R, B, G, say Xmin and Xmax
  f32 max = fmaxf(in.r, fmaxf(in.g, in.b));
  f32 min = fminf(in.r, fminf(in.g, in.b));
  l       = (max + min) / 2;

  // If Xmax and Xmin are equal, S is defined to be 0, and H is undefined but in
  // programs usually written as 0
  if (max == min) {
    s = 0;
    h = 0;
  }

  if (max == min) {
    s = 0.f;
  }
  // If L < 1/2, S=(Xmax - Xmin)/(Xmax + Xmin)
  else if (l < 0.5f) {
    s = (max - min) / (max + min);
  }
  // Else, S=(Xmax - Xmin)/(2 - Xmax - Xmin)
  else {
    s = (max - min) / (2 - max - min);
  }

  if (max == min) {
    h = 0.f;
  }
  // If R=Xmax, H = (G-B)/(Xmax - Xmin)
  else if (in.r == max) {
    h = (in.g - in.b) / (max - min);
  }
  // If G=Xmax, H = 2 + (B-R)/(Xmax - Xmin)
  else if (in.g == max) {
    h = 2 + (in.b - in.r) / (max - min);
  }
  // If B=Xmax, H = 4 + (R-G)/(Xmax - Xmin)
  else if (in.b == max) {
    h = 4 + (in.r - in.g) / (max - min);
  }

  // If H < 0 set H = H + 6.
  if (h < 0) {
    h = h + 6;
  }
  // Notice that H ranges from 0 to 6. RGB space is a cube, and HSL space is a
  // double hexacone, where L is the principal diagonal of the RGB cube. Thus
  // corners of the RGB cube; red, yellow, green, cyan, blue, and magenta,
  // become the vertices of the HSL hexagon. Then the value 0-6 for H tells you
  // which section of the hexgon you are in. H is most commonly given as in
  // degrees, so to convert H = H*60.0 (If H is negative, add 360 to complete
  // the conversion.)
  h = h * 60.f;

  return {h, s, l, in.a};
}

Color hsl_to_rgb(Color in)
{
  // https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion
  f32 h = in.r / 360.f;
  f32 s = in.g;
  f32 l = in.b;

  Color out = {l, l, l, in.a};
  if (s != 0) {
    auto hue2rgb = [](f32 p, f32 q, f32 t) {
      if (t < 0) t += 1;
      if (t > 1) t -= 1;
      if (t < 1 / 6.f) return p + (q - p) * 6 * t;
      if (t < 1 / 2.f) return q;
      if (t < 2 / 3.f) return p + (q - p) * (2 / 3.f - t) * 6;
      return p;
    };

    f32 q = l < 0.5f ? l * (1 + s) : l + s - l * s;
    f32 p = 2 * l - q;
    out.r = hue2rgb(p, q, h + 1 / 3.f);
    out.g = hue2rgb(p, q, h);
    out.b = hue2rgb(p, q, h - 1 / 3.f);
  }

  return out;
}

Color lighten(Color in, f32 val)
{
  auto hsl = rgb_to_hsl(in);
  return hsl_to_rgb({hsl.r, hsl.g, hsl.b + val, hsl.a});
}

Color darken(Color in, f32 val)
{
  auto hsl = rgb_to_hsl(in);
  return hsl_to_rgb({hsl.r, hsl.g, hsl.b - val, hsl.a});
}