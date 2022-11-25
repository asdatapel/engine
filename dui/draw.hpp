#pragma once

#include "containers/static_stack.hpp"
#include "math/math.hpp"
#include "types.hpp"
#include "vulkan.hpp"

namespace Dui
{

struct Vertex {
  Vec2f pos;
  Vec3f color;
};

struct DrawCall {
  i32 vert_offset;
  i32 tri_count;

  Rect scissor;
  // Texture texture;
};

struct DrawList {
  Vertex *verts        = nullptr;
  DrawCall *draw_calls = nullptr;
  i32 vert_count       = 0;
  i32 draw_call_count  = 0;

  Vulkan::Buffer vb;

  StaticStack<Rect, 1024> scissors;
  void push_scissor(Rect rect) { scissors.push_back(rect); }
  void pop_scissor() { scissors.pop(); }
};

DrawList main_dl;
DrawList forground_dl;

void init_draw_list(Vulkan *gpu, DrawList *dl)
{
  dl->verts      = (Vertex *)malloc(sizeof(Vertex) * 1024 * 1024);
  dl->draw_calls = (DrawCall *)malloc(sizeof(DrawCall) * 1024 * 1024);

  dl->vb = gpu->create_vertex_buffer(MB);
}

void clear_draw_list(DrawList *dl)
{
  dl->vert_count      = 0;
  dl->draw_call_count = 0;
}

void draw_draw_list(Vulkan *vulkan, DrawList *dl, Vec2f canvas_size)
{
  vulkan->upload_buffer(dl->vb, dl->verts, dl->vert_count * sizeof(Vertex));
  vulkan->push_constant(&canvas_size, sizeof(Vec2f));
  for (i32 i = 0; i < dl->draw_call_count; i++) {
    DrawCall call = dl->draw_calls[i];

    vulkan->set_scissor(call.scissor);
    vulkan->draw_vertex_buffer(dl->vb, call.vert_offset, call.tri_count * 3);
  }
}

void push_vert(DrawList *dl, Vec2f pos, Color color)
{
  dl->verts[dl->vert_count++] = {pos, {color.r, color.g, color.b}};
}

void push_draw_call(DrawList *dl, i32 tri_count)
{
  Rect current_scissor_rect =
      (dl->scissors.count == 0) ? Rect{0, 0, 10000000, 10000000} : dl->scissors.top();

  if (dl->draw_call_count > 0 &&
      dl->draw_calls[dl->draw_call_count - 1].scissor == current_scissor_rect) {
    dl->draw_calls[dl->draw_call_count - 1].tri_count += tri_count;
    return;
  }

  DrawCall dc = {dl->vert_count - (tri_count * 3), tri_count};
  dc.scissor  = current_scissor_rect;

  dl->draw_calls[dl->draw_call_count] = dc;
  dl->draw_call_count++;
}

void push_rect(DrawList *dl, Rect rect, Color color)
{
  push_vert(dl, {rect.x, rect.y}, color);
  push_vert(dl, {rect.x + rect.width, rect.y}, color);
  push_vert(dl, {rect.x + rect.width, rect.y + rect.height}, color);

  push_vert(dl, {rect.x, rect.y}, color);
  push_vert(dl, {rect.x + rect.width, rect.y + rect.height}, color);
  push_vert(dl, {rect.x, rect.y + rect.height}, color);

  push_draw_call(dl, 2);
}

// void push_text(DrawList *dl, Font *font, String text, Vec2f pos, Color color, f32 height)
// {
//   float baseline = font->baseline;
//   for (int i = 0; i < text.len; i++) {
//     f32 resize_ratio = height / font->font_size_px;

//     Character c     = font->characters[text.data[i]];
//     Rect shape_rect = {pos.x + c.shape.x * resize_ratio, pos.y + (baseline + c.shape.y) * resize_ratio, c.shape.width * resize_ratio,
//                        c.shape.height * resize_ratio};
//     pos.x += c.advance * resize_ratio;

//     push_vert(dl, {shape_rect.x, shape_rect.y}, {c.uv.x, c.uv.y + c.uv.height}, color, color.a);
//     push_vert(dl, {shape_rect.x + shape_rect.width, shape_rect.y},
//               {c.uv.x + c.uv.width, c.uv.y + c.uv.height}, color, color.a);
//     push_vert(dl, {shape_rect.x + shape_rect.width, shape_rect.y + shape_rect.height},
//               {c.uv.x + c.uv.width, c.uv.y}, color, color.a);

//     push_vert(dl, {shape_rect.x, shape_rect.y}, {c.uv.x, c.uv.y + c.uv.height}, color, color.a);
//     push_vert(dl, {shape_rect.x + shape_rect.width, shape_rect.y + shape_rect.height},
//               {c.uv.x + c.uv.width, c.uv.y}, color, color.a);
//     push_vert(dl, {shape_rect.x, shape_rect.y + shape_rect.height}, {c.uv.x, c.uv.y}, color,
//               color.a);
//     push_draw_call(dl, font->atlas, 2);
//   }
// };

// TODO allow pushing a temporary drawcall then can be updated later.
// Useful for grouping controls with a background
DrawCall *push_temp();

}  // namespace Dui