#version 450

#include "primitives.glsl"

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;
layout(location = 2) out vec2 out_rect_positive_extents;
layout(location = 3) out vec2 out_position;
layout(location = 4) out vec2 out_rect_center;
layout(location = 5) out float out_rect_corner_radius;
layout(location = 6) out vec4 out_clip_rect_bounds;
layout(location = 7) out flat uint out_primitive_type;
layout(location = 8) out flat uint out_primitive_idx;
layout(location = 9) out flat uint out_texture_idx;

layout( push_constant ) uniform constants
{
    vec2 canvas_size;
} push_constants;

vec4 uint_to_vec_color(uint color) {
    float r = ((color >> 24) & 0xFF) / 255.0;
    float g = ((color >> 16) & 0xFF) / 255.0;
    float b = ((color >> 8) & 0xFF) / 255.0;
    float a = ((color >> 0) & 0xFF) / 255.0;

    return vec4(r, g, b, a);
}

void main() {
    uint primitive_idx = gl_VertexIndex & 0xFFFF;

    out_primitive_type = gl_VertexIndex & 0xFFFC0000;
    out_primitive_idx = primitive_idx;

    if (out_primitive_type == ROUNDED_RECT) {
        RoundedRectPrimitive r = primitives.rounded_rects[primitive_idx];
        RectPrimitive clip = primitives.clip_rects[r.clip_rect_idx];

        uint corner = (gl_VertexIndex >> 16) & 0x3;
        vec2 verts[] = {
            {r.dimensions.x, r.dimensions.y},
            {r.dimensions.x + r.dimensions.z, r.dimensions.y},
            {r.dimensions.x,  r.dimensions.y + r.dimensions.w},
            {r.dimensions.x + r.dimensions.z,  r.dimensions.y + r.dimensions.w},
        };

        gl_Position = vec4(verts[corner] / push_constants.canvas_size * vec2(2, 2) - vec2(1, 1), 0, 1);

        vec2 rect_positive_extents = r.dimensions.zw / 2;
        vec2 rect_center = r.dimensions.xy + r.dimensions.zw / 2;

        out_color = uint_to_vec_color(r.color);
        out_rect_positive_extents = rect_positive_extents;
        out_position = verts[corner];
        out_rect_center = rect_center;
        out_rect_corner_radius = r.corner_radius * smoothstep(0, 1, r.corner_mask & (1 << corner));
        out_clip_rect_bounds = vec4(clip.rect.x, clip.rect.y, clip.rect.x + clip.rect.z, clip.rect.y + clip.rect.w);
    } else if (out_primitive_type == TEXTURE_RECT) {
        TextureRectPrimitive p = primitives.texture_rects[primitive_idx];
        RectPrimitive clip = primitives.clip_rects[p.clip_rect_idx];

        uint corner = (gl_VertexIndex >> 16) & 0x3;
        vec2 verts[] = {
            {p.dimensions.x, p.dimensions.y},
            {p.dimensions.x + p.dimensions.z, p.dimensions.y},
            {p.dimensions.x,  p.dimensions.y + p.dimensions.w},
            {p.dimensions.x + p.dimensions.z,  p.dimensions.y + p.dimensions.w},
        };
        vec2 uvs[] = {
            {p.uv_bounds.x, p.uv_bounds.y},
            {p.uv_bounds.z, p.uv_bounds.y},
            {p.uv_bounds.x,  p.uv_bounds.w},
            {p.uv_bounds.z,  p.uv_bounds.w},
        };

        gl_Position = vec4(verts[corner] / push_constants.canvas_size * vec2(2, 2) - vec2(1, 1), 0, 1);

        out_uv = uvs[corner];
        out_position = verts[corner];
        out_texture_idx = p.texture_idx;
        out_clip_rect_bounds = vec4(clip.rect.x, clip.rect.y, clip.rect.x + clip.rect.z, clip.rect.y + clip.rect.w);
    } else if (out_primitive_type == BITMAP_GLYPH) {
        BitmapGlyphPrimitive p = primitives.bitmap_glyphs[primitive_idx];
        RectPrimitive clip = primitives.clip_rects[p.clip_rect_idx];

        uint corner = (gl_VertexIndex >> 16) & 0x3;
        vec2 verts[] = {
            {p.dimensions.x, p.dimensions.y},
            {p.dimensions.x + p.dimensions.z, p.dimensions.y},
            {p.dimensions.x,  p.dimensions.y + p.dimensions.w},
            {p.dimensions.x + p.dimensions.z,  p.dimensions.y + p.dimensions.w},
        };
        vec2 uvs[] = {
            {p.uv_bounds.x, p.uv_bounds.y},
            {p.uv_bounds.z, p.uv_bounds.y},
            {p.uv_bounds.x,  p.uv_bounds.w},
            {p.uv_bounds.z,  p.uv_bounds.w},
        };

        gl_Position = vec4(verts[corner] / push_constants.canvas_size * vec2(2, 2) - vec2(1, 1), 0, 1);

        out_uv = uvs[corner];
        out_color = uint_to_vec_color(p.color);
        out_position = verts[corner];
        out_clip_rect_bounds = vec4(clip.rect.x, clip.rect.y, clip.rect.x + clip.rect.z, clip.rect.y + clip.rect.w);
    } else if (out_primitive_type == VECTOR_GLYPH) {
        VectorGlyphPrimitive p = primitives.vector_glyphs[primitive_idx];
        RectPrimitive clip = primitives.clip_rects[p.clip_rect_idx];

        uint corner = (gl_VertexIndex >> 16) & 0x3;
        vec2 verts[] = {
            {p.dimensions.x - 1 , p.dimensions.y - 1},
            {p.dimensions.x + p.dimensions.z + 1, p.dimensions.y - 1},
            {p.dimensions.x - 1,  p.dimensions.y + p.dimensions.w + 1},
            {p.dimensions.x + p.dimensions.z + 1,  p.dimensions.y + p.dimensions.w + 1},
        };
        vec2 uvs[] = {
            {-(1 / p.dimensions.z),       1 + (1 / p.dimensions.w)},
            {1 + (1 / p.dimensions.z),   1 + (1 / p.dimensions.w)},
            {-(1 / p.dimensions.z),     -(1 / p.dimensions.w)},
            {1 + (1 / p.dimensions.z), -(1 / p.dimensions.w)},
        };

        gl_Position = vec4(verts[corner] / push_constants.canvas_size * vec2(2, 2) - vec2(1, 1), 0, 1);

        out_uv = uvs[corner];
        out_color = uint_to_vec_color(p.color);
        out_position = verts[corner];
        out_clip_rect_bounds = vec4(clip.rect.x, clip.rect.y, clip.rect.x + clip.rect.z, clip.rect.y + clip.rect.w);
    } else if (out_primitive_type == LINE) {
        LinePrimitive p = primitives.lines[primitive_idx];
        RectPrimitive clip = primitives.clip_rects[p.clip_rect_idx];

        vec2 d = normalize(p.b - p.a);
        vec2 tangent = vec2(-d.y, d.x) * 10;

        uint corner = (gl_VertexIndex >> 16) & 0x3;
        vec2 verts[] = {
            p.a + tangent,
            p.a - tangent,
            p.b + tangent,
            p.b - tangent,
        };
        gl_Position = vec4(verts[corner] / push_constants.canvas_size * vec2(2, 2) - vec2(1, 1), 0, 1);

        out_color = uint_to_vec_color(p.color);
        out_position = verts[corner];
        out_clip_rect_bounds = vec4(clip.rect.x, clip.rect.y, clip.rect.x + clip.rect.z, clip.rect.y + clip.rect.w);
    }
}

