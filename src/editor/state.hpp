#pragma once

#include "dui/draw.hpp"
#include "dui/dui.hpp"
#include "dui/dui_state.hpp"
#include "dui/nodes/nodes.hpp"

// struct ShaderPinDefinition {
//   String name;
//   //DataType type;
// };
// struct ShaderNodeDefinition {
//   Array<ShaderPinDefinition, 12> inputs;
//   Array<ShaderPinDefinition, 12> outputs;
// };
// struct ShaderNode {
//   ShaderNodeDefinition *node_def;
//   Dui::Node dui_node_ref;
// };

struct MaterialEditorWindow {
  StaticString<512> filename;
  // Array<ShaderNode, 512> nodes;
  // Array<Dui::NodeLink, 2048> links;
};

namespace Editor
{
struct State {
  String resource_path = "../";

  Gpu::Pipeline pipeline;

  Array<MaterialEditorWindow, 12> material_editor_windows;
};

}  // namespace Editor