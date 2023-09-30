
const uint RECT         = 1 << 18;
const uint ROUNDED_RECT = 2 << 18;
const uint TEXTURE_RECT = 3 << 18;
const uint BITMAP_GLYPH = 4 << 18;
const uint VECTOR_GLYPH = 5 << 18;
const uint LINE         = 6 << 18;

struct RectPrimitive {
  vec4 rect;
};

struct RoundedRectPrimitive {
  vec4 dimensions;
  uint clip_rect_idx;
  uint color;
  float corner_radius;
  uint corner_mask;
};

struct TextureRectPrimitive {
  vec4 dimensions;
  vec4 uv_bounds;
  uint texture_idx;
  uint clip_rect_idx;
};

struct BitmapGlyphPrimitive {
  vec4 dimensions;
  vec4 uv_bounds;
  uint clip_rect_idx;
  uint color;
};

struct ConicCurvePrimitive {
  vec2 p0;
  vec2 p1;
  vec2 p2;
};

struct VectorGlyphPrimitive {
  vec4 dimensions;
  uint curve_start_idx;
  uint curve_count;
  uint color;
  uint clip_rect_idx;
};

struct LinePrimitive {
  vec2 a;
  vec2 b;
  uint color;
  uint clip_rect_idx;
};

layout(std140, set = 0, binding = 1) readonly buffer Primitives {
  RectPrimitive clip_rects[1024];
  RoundedRectPrimitive rounded_rects[1024];
  BitmapGlyphPrimitive bitmap_glyphs[1024];
  TextureRectPrimitive texture_rects[1024];
  VectorGlyphPrimitive vector_glyphs[1024];
  ConicCurvePrimitive conic_curves[4096];
  LinePrimitive lines[4096];
} primitives;