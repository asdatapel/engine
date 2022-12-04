#include <iostream>

#include "dui/draw.hpp"
#include "dui/dui.hpp"
#include "dui/dui_state.hpp"
#include "gpu/vulkan/vulkan.hpp"
#include "platform.hpp"
#include "types.hpp"

namespace DuiCrap
{
VkDescriptorSet desc_set;

void init_dui_crap(Dui::DuiState *s, Pipeline pipeline)
{
  ImageBuffer image = create_image(s->font.atlas.width, s->font.atlas.height);
  VkImageView image_view = create_image_view(image);
  VkSampler sampler      = create_sampler();
  upload_image(image, s->font.atlas);

  desc_set = create_descriptor_set(pipeline);
  write_sampler(desc_set, image_view, sampler);

  s->main_dl.vb      = create_vertex_buffer(MB);
  s->forground_dl.vb = create_vertex_buffer(MB);
  for (i32 i = 0; i < s->draw_lists.size; i++) {
    s->draw_lists[i].vb = create_vertex_buffer(MB);
  }

  vkDeviceWaitIdle(vk.device);
}

void draw_draw_list(Dui::DuiState *s, Pipeline pipeline, Dui::DrawList *dl,
                    Vec2f canvas_size)
{
  upload_buffer_staged(dl->vb, dl->verts, dl->vert_count * sizeof(Dui::Vertex));
  bind_descriptor_set(pipeline, desc_set);

  push_constant(pipeline, &canvas_size, sizeof(Vec2f));
  for (i32 i = 0; i < dl->draw_call_count; i++) {
    Dui::DrawCall call = dl->draw_calls[i];

    set_scissor(call.scissor);
    draw_vertex_buffer(dl->vb, call.vert_offset, call.tri_count * 3);
  }
}

void draw_all(Dui::DuiState *s, Pipeline pipeline, Vec2f canvas_size)
{
  for (i32 i = s->root_groups.size - 1; i >= 0; i--) {
    draw_draw_list(s, pipeline, s->root_groups[i]->dl, canvas_size);
  }
  draw_draw_list(s, pipeline, &s->main_dl, canvas_size);
  draw_draw_list(s, pipeline, &s->forground_dl, canvas_size);
}
}  // namespace DuiCrap

int main()
{
  Platform::init();

  Platform::GlfwWindow window;
  window.init();

  init_vulkan(&window);

  Pipeline pipeline = create_pipeline(vk.render_pass);

  Input input;
  Platform::setup_input_callbacks(&window, &input);

  init_dui_dll();
  init_dui();
  Dui::DuiState *s = get_dui_state();
  DuiCrap::init_dui_crap(s, pipeline);

  while (!window.should_close()) {
    Platform::fill_input(&window, &input);

    start_frame(pipeline);

    debug_ui_test(&input, window.get_size());
    DuiCrap::draw_all(s, pipeline, window.get_size());

    end_frame();

    if (input.key_down_events[(i32)Keys::ENTER]) {
      init_dui_dll();
      init_dui();
      s = get_dui_state();
      DuiCrap::init_dui_crap(s, pipeline);
    }
  }

  destroy_pipeline(pipeline);

  destroy_vulkan();
  window.destroy();

  return 0;
}