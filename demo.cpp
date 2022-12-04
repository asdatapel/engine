#include <iostream>

#include "dui/draw.hpp"
#include "dui/dui.hpp"
#include "dui/dui_state.hpp"
#include "gpu/vulkan/vulkan.hpp"
#include "platform.hpp"
#include "types.hpp"

// namespace DuiCrap
// {
// VkDescriptorSet desc_set;

// void init_dui_crap(Device *device, Dui::DuiState *s, Pipeline pipeline)
// {
//   ImageBuffer image = create_image(device, s->font.atlas.width, s->font.atlas.height);
//   VkImageView image_view = create_image_view(device, image);
//   VkSampler sampler      = create_sampler(device);
//   upload_image(device, image, s->font.atlas);

//   desc_set = create_descriptor_set(device, pipeline);
//   write_sampler(device, desc_set, image_view, sampler);

//   s->main_dl.vb      = create_vertex_buffer(device, MB);
//   s->forground_dl.vb = create_vertex_buffer(device, MB);
//   for (i32 i = 0; i < s->draw_lists.size; i++) {
//     s->draw_lists[i].vb = create_vertex_buffer(device, MB);
//   }

//   vkDeviceWaitIdle(device->device);
// }

// void draw_draw_list(Device *device, Dui::DuiState *s, Pipeline pipeline, Dui::DrawList *dl,
//                     Vec2f canvas_size)
// {
//   upload_buffer_staged(device, dl->vb, dl->verts, dl->vert_count * sizeof(Dui::Vertex));
//   bind_descriptor_set(device, pipeline, desc_set);

//   push_constant(device, pipeline, &canvas_size, sizeof(Vec2f));
//   for (i32 i = 0; i < dl->draw_call_count; i++) {
//     Dui::DrawCall call = dl->draw_calls[i];

//     set_scissor(device, call.scissor);
//     draw_vertex_buffer(device, dl->vb, call.vert_offset, call.tri_count * 3);
//   }
// }

// void draw_all(Device *device, Dui::DuiState *s, Pipeline pipeline, Vec2f canvas_size)
// {
//   for (i32 i = s->root_groups.size - 1; i >= 0; i--) {
//     draw_draw_list(device, s, pipeline, s->root_groups[i]->dl, canvas_size);
//   }
//   draw_draw_list(device, s, pipeline, &s->main_dl, canvas_size);
//   draw_draw_list(device, s, pipeline, &s->forground_dl, canvas_size);
// }
// }  // namespace DuiCrap

int main()
{
  Platform::init();

  Platform::GlfwWindow window;
  window.init();

  Device device = init_vulkan(&window);

  Pipeline pipeline = create_pipeline(&device, device.render_pass);

  Input input;
  Platform::setup_input_callbacks(&window, &input);

  init_dui_dll();
  Dui::init_dui(&device, pipeline);
  Dui::DuiState *s = Dui::get_dui_state();

  while (!window.should_close()) {
    Platform::fill_input(&window, &input);

    start_frame(&device, pipeline);

    Dui::debug_ui_test(&device, pipeline, &input, window.get_size());

    end_frame(&device);

    if (input.key_down_events[(i32)Keys::ENTER]) {
      init_dui_dll();
      Dui::init_dui(&device, pipeline);
      s = Dui::get_dui_state();
    }
  }

  destroy_pipeline(&device, pipeline);

  destroy_vulkan(&device);
  window.destroy();

  return 0;
}