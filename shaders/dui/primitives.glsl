
const uint RECT         = (1 << 18);
const uint ROUNDED_RECT = (2 << 18);
const uint BITMAP_GLYPH = (3 << 18);

struct RectPrimitive {
  vec4 rect;
};

struct RoundedRectPrimitive {
    vec4 dimensions;
    uint clip_rect_idx;
    uint color;
    float corner_radius;
};

struct BitmapGlyphPrimitive {
    vec4 dimensions;
    vec4 uv_bounds;
    uint clip_rect_idx;
    uint color;
};

layout(std140, set = 0, binding = 1) readonly buffer Primitives {
  RectPrimitive clip_rects[1024];
  RoundedRectPrimitive rounded_rects[1024];
  BitmapGlyphPrimitive bitmap_glyphs[1024];
} primitives;