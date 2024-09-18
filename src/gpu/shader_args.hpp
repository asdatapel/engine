#pragma once

#include "gpu/device.hpp"
#include "types.hpp"

namespace Gpu
{

enum Access {
  READ  = 1,
  WRITE = 2,
};

struct ShaderArgumentDefinition {
  enum struct Type {
    DATA,
    SAMPLER,
  };

  Type type;
  Access access;
  i32 count = 0;
};

struct ShaderArgBuffer;
ShaderArgBuffer create_shader_arg_buffer(Device *device);

}  // namespace Gpu