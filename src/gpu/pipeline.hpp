#pragma once

#include "containers/array.hpp"
#include "file.hpp"
#include "gpu/buffer.hpp"
#include "gpu/shader_args.hpp"
#include "logging.hpp"
#include "math/math.hpp"
#include "platform.hpp"
#include "types.hpp"

namespace Gpu
{

struct VertexDescription {
  Array<PixelFormat, 10> attributes;
};

struct ShadersDefinition {
  String vert_shader;
  String frag_shader;

  Array<ShaderArgumentDefinition, 16> arg_defs;
};

struct Pipeline;

Pipeline create_pipeline(Device *device, ShadersDefinition shaders_def);
void destroy_pipeline(Pipeline pipeline);

void bind_pipeline(Device *device, Pipeline pipeline);
}  // namespace Gpu