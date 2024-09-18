#pragma once

#include <ctype.h>

#include "containers/array.hpp"
#include "dui/nodes/retained_nodes.hpp"
// #include "gpu/vulkan/shader_models/pbr_lit.hpp"
#include "logging.hpp"
#include "math/math.hpp"
#include "memory.hpp"
#include "string.hpp"

struct ShaderModel {
  String frag_header_filepath;
  u32 push_constants_size;
};
ShaderModel pbr_lit_shader_model = {"resources/shader_models/pbr_lit/frag.glsl",
                                    sizeof(Mat4f)};

struct GeneratedShader {
  String src;
  i32 num_textures;
};

struct Parser {
  String src;
  u32 cursor = 0;

  String next_line_name;
  String next_line_type;
  Array<String, 32> next_line_args;

  u8 next() { return cursor < src.size ? src.data[cursor] : '\0'; }
};

String parse_token(Parser *parser)
{
  String token = {parser->src.data + parser->cursor, 0};
  if (parser->next() == '\"') {
    token.size++;
    parser->cursor++;
    while (parser->cursor < parser->src.size && parser->next() != '\"') {
      token.size++;
      parser->cursor++;
    }
    token.size++;
    parser->cursor++;

    return token;
  }

  while (parser->cursor < parser->src.size &&
         (std::isalnum(parser->next()) || parser->next() == '_' ||
          parser->next() == '.')) {
    token.size++;
    parser->cursor++;
  }

  return token;
}
void eat_whitespace(Parser *parser)
{
  while (parser->cursor < parser->src.size && std::isspace(parser->next())) {
    parser->cursor++;
  }
}
void eat_char(Parser *parser, u8 character)
{
  assert(parser->next() == character);
  parser->cursor++;
}

void parse_next_line(Parser *parser)
{
  parser->next_line_name = parse_token(parser);
  eat_whitespace(parser);
  eat_char(parser, '=');
  eat_whitespace(parser);

  parser->next_line_type = parse_token(parser);
  eat_whitespace(parser);
  eat_char(parser, '(');
  eat_whitespace(parser);

  parser->next_line_args.clear();
  while (parser->next() != ')') {
    parser->next_line_args.push_back(parse_token(parser));
    eat_whitespace(parser);

    if (parser->next() == ',') {
      eat_char(parser, ',');
      eat_whitespace(parser);
    } else {
      break;
    }
  }

  eat_char(parser, ')');
  eat_whitespace(parser);
}

f32 to_float(String s)
{
  char value[128];
  assert(s.size < 128);
  memcpy(value, s.data, s.size);
  value[s.size] = '\0';
  return strtof(value, nullptr);
}

struct Vec4fValue {
  Vec4f value;
};

struct DataType {
  i32 num_channels;
};
struct Node {
  String name;

  enum struct Type {
    INPUT,
    CONSTANT,
    ADD,
    TEXTURE,
    OUTPUT,
  };
  Type type;

  DataType data_type;
};

struct InputNode : Node {
  String input_name;
};

struct ConstantNode : Node {
  Vec4f value;
};

struct AddNode : Node {
  Node *a;
  Node *b;
};

struct TextureNode : Node {
  u32 texture_slot;
  Node *uv;
};

struct OutputNode : Node {
  Array<Node *, 32> args;
};

struct TextureSlot {
  String asset_path;
};
Array<TextureSlot, 12> texture_slots;
u32 push_texture(String asset_path)
{
  return texture_slots.push_back({asset_path});
}

StackAllocator node_buffer(&system_allocator, MB);
Array<Node *, 512> nodes;
template <typename T>
T *push_node(T node)
{
  Mem mem = node_buffer.alloc(sizeof(T));
  T *data = (T *)mem.data;
  *data   = node;

  nodes.push_back(data);
  return data;
}
Node *lookup_node(String name)
{
  for (i32 i = 0; i < nodes.size; i++) {
    if (nodes[i]->name == name) return nodes[i];
  }
  assert(false);
  return nullptr;
}

Node *create_next_node(Parser *parser)
{
  if (parser->next_line_type == "input") {
    InputNode n;
    n.name                   = parser->next_line_name;
    n.type                   = Node::Type::INPUT;
    n.input_name             = parser->next_line_args[0];
    n.data_type.num_channels = to_float(parser->next_line_args[1]);

    return push_node(n);
  } else if (parser->next_line_type == "constant") {
    ConstantNode n;
    n.name                   = parser->next_line_name;
    n.type                   = Node::Type::CONSTANT;
    n.data_type.num_channels = parser->next_line_args.size;
    for (i32 i = 0; i < n.data_type.num_channels; i++) {
      n.value.values[i] = to_float(parser->next_line_args[i]);
    }

    return push_node(n);
  } else if (parser->next_line_type == "add") {
    AddNode n;
    n.name = parser->next_line_name;
    n.type = Node::Type::ADD;
    n.a    = lookup_node(parser->next_line_args[0]);
    n.b    = lookup_node(parser->next_line_args[1]);
    n.data_type.num_channels =
        std::max(n.a->data_type.num_channels, n.b->data_type.num_channels);

    return push_node(n);
  } else if (parser->next_line_type == "texture") {
    TextureNode n;
    n.name                   = parser->next_line_name;
    n.type                   = Node::Type::TEXTURE;
    n.data_type.num_channels = to_float(parser->next_line_args[1]);
    n.texture_slot           = push_texture(parser->next_line_args[0]);
    n.uv                     = lookup_node(parser->next_line_args[2]);

    return push_node(n);
  } else if (parser->next_line_type == "output_node") {
    OutputNode n;
    n.name                   = parser->next_line_name;
    n.type                   = Node::Type::OUTPUT;
    n.data_type.num_channels = 0;

    for (i32 i = 0; i < parser->next_line_args.size; i++) {
      n.args.push_back(lookup_node(parser->next_line_args[i]));
    }

    return push_node(n);
  }

  return nullptr;
}

String gen_glsl(Allocator *allocator)
{
  Temp temp;
  Mem builder_mem = temp.alloc(MB);

  String builder = {builder_mem.data, 0};

  auto push = [&](String s) {
    memcpy(builder.data + builder.size, s.data, s.size);
    builder.size += s.size;
  };
  auto push_i32 = [&](i32 i) {
    builder.size += snprintf((char *)builder.data + builder.size, builder_mem.size, "%d", i);
  };
  auto push_f32 = [&](f32 i) {
    builder.size += snprintf((char *)builder.data + builder.size, builder_mem.size, "%f", i);
  };
  auto push_massaged = [&](String s, i32 in_components_n,
                           i32 out_components_n) {
    if (in_components_n > out_components_n) {
      String components = "xyzw";
      components.size   = out_components_n;

      push(s);
      push(".");
      push(components);
    } else if (in_components_n < out_components_n) {
      push("vec");
      push_i32(out_components_n);
      push("(");
      push(s);
      for (i32 i = 0; i < (out_components_n - in_components_n); i++) {
        push(", 0");
      }
      push(")");
    } else {
      push(s);
    }
  };

  push("\n\n");
  for (i32 i = 0; i < texture_slots.size; i++) {
    push("layout(binding = ");
    push_i32(i);
    push(") uniform sampler2D sampler");
    push_i32(i);
    push(";");
  }
  push("\n\n");

  push("void main() {\n");
  for (i32 i = 0; i < nodes.size; i++) {
    push("  ");

    Node *node = nodes[i];

    String var_name = nodes[i]->name;

    u8 var_type_buf[] = {'v', 'e', 'c', 'x'};
    var_type_buf[3]   = '0' + node->data_type.num_channels;
    String var_type   = {var_type_buf, 4};
    if (node->data_type.num_channels == 1) {
      var_type = "float";
    }

    if (node->data_type.num_channels != 0) {
      push(var_type);
      push(" ");
      push(var_name);
      push(" = ");
    }

    if (node->type == Node::Type::INPUT) {
      push(((InputNode *)node)->input_name);
    } else if (node->type == Node::Type::CONSTANT) {
      push(var_type);
      push("(");

      ConstantNode *cn = (ConstantNode *)node;
      for (i32 i = 0; i < cn->data_type.num_channels - 1; i++) {
        push_f32(cn->value.values[i]);
        push(", ");
      }
      push_f32(cn->value.values[cn->data_type.num_channels - 1]);

      push(")");
    } else if (node->type == Node::Type::ADD) {
      AddNode *n = (AddNode *)node;

      push_massaged(n->a->name, n->a->data_type.num_channels,
                    n->data_type.num_channels);
      push(" + ");
      push_massaged(n->b->name, n->b->data_type.num_channels,
                    n->data_type.num_channels);
    } else if (node->type == Node::Type::TEXTURE) {
      TextureNode *n = (TextureNode *)node;
      push("texture(");
      push("sampler");
      push_i32(n->texture_slot);
      push(", ");
      push(n->uv->name);
      push(")");
    } else if (node->type == Node::Type::OUTPUT) {
      Array<i32, 2> arg_sizes = {3, 1};

      OutputNode *n = (OutputNode *)node;
      push("output_node(");
      for (i32 i = 0; i < n->args.size - 1; i++) {
        push_massaged(n->args[i]->name, n->args[i]->data_type.num_channels,
                      arg_sizes[i]);
        push(", ");
      }
      push_massaged(n->args[n->args.size - 1]->name,
                    n->args[n->args.size - 1]->data_type.num_channels,
                    arg_sizes[n->args.size - 1]);

      push(")");
    }
    push(";\n");
  }
  push("}");

  String output;
  output.data = allocator->alloc(builder.size).data;
  output.size = builder.size;
  memcpy(output.data, builder.data, builder.size);

  return output;
}

GeneratedShader material_nodes_test(String graph, ShaderModel shader_model,
                                    Allocator *allocator)
{
  Parser parser;
  parser.src = graph;

  while (parser.next() != '\0') {
    parse_next_line(&parser);
    create_next_node(&parser);
  }

  Temp temp;
  String generated_code = gen_glsl(&temp);
  File frag_model_file  = read_file(shader_model.frag_header_filepath, &temp);

  u32 output_size = frag_model_file.data.size + generated_code.size;
  Mem output_mem  = allocator->alloc(output_size);
  String output   = {output_mem.data, output_size};

  memcpy(output.data, frag_model_file.data.data, frag_model_file.data.size);
  memcpy(output.data + frag_model_file.data.size, generated_code.data,
         generated_code.size);
  info(output);

  GeneratedShader shader;
  shader.src          = output;
  shader.num_textures = 1;
  return shader;
}

// void material_usage(Gpu::Device *gpu)
// {
//   String graph =
//       "time = input(per_frame_data.time, 1)"
//       "uv = input(in_uv, 2)"
//       "uv2 = add(time, uv)"0
//       "diffuse = "
//       "texture(\"../fracas/set/models/pedestal/Pedestal_Albedo.png\", "
//       "4, uv2)"
//       "output_node = output_node(uv, diffuse)";

//   GeneratedShader vert_shader =
//       material_nodes_test(graph, pbr_lit_shader_model, &tmp_allocator);
//   GeneratedShader frag_shader =
//       material_nodes_test(graph, pbr_lit_shader_model, &tmp_allocator);

//   auto descriptor_set_layouts =
//       Gpu::Vulkan::create_descriptor_set_layouts(gpu,
//       frag_shader.num_textures);

//   Gpu::Pipeline pipeline = Gpu::create_pipeline(
//       gpu, vert_shader.src, frag_shader.src, descriptor_set_layouts,
//       pbr_lit_shader_model.push_constants_size, standard_vertex_description,
//       standard_render_pass);

//   String graph2 =
//       "time = input(per_frame_data.time, 1)"
//       "uv = input(in_uv, 2)"
//       "uv2 = add(time, uv)"
//       "diffuse = "
//       "texture(\"../fracas/set/models/pedestal/Pedestal_Albedo.png\", "
//       "4, uv2)"
//       "output_node = output_node(uv, diffuse)";

//   Gpu::destroy_pipeline(gpu, pipeline);

//   descriptor_set_layouts =
//       Gpu::Vulkan::create_descriptor_set_layouts(gpu,
//       frag_shader.num_textures);
//   pipeline = Gpu::create_pipeline(
//       gpu, vert_shader.src, frag_shader.src, descriptor_set_layouts,
//       pbr_lit_shader_model.push_constants_size, standard_vertex_description,
//       standard_render_pass);
// }

// ShaderNodeDefinition dumb = {{{"namme"}, {"other name"}}};

// struct InputNode : Node {
//   String input_name;
// };

// struct ConstantNode : Node {
//   Vec4f value;
// };

// struct AddNode : Node {
//   Node *a;
//   Node *b;
// };

// struct TextureNode : Node {
//   u32 texture_slot;
//   Node *uv;
// };

// struct OutputNode : Node {
//   Array<Node *, 32> args;
// };

Dui::NodeDefinition add_node_def = Dui::NodeDefinition{
    "Add", {{{"A", 1}, {"B", 1}}}, {{{"Out", 1}}}, {.9, .3, .2, 1}, 100};
Dui::NodeDefinition output_node_def = Dui::NodeDefinition{
    "Add",
    {{{"Color", 1}, {"Roughness", 1}, {"Metal", 1}, {"AO", 1}}},
    {},
    {.9, .3, .7, 1},
    100};

struct AddNodeA : Dui::Node {
  AddNodeA()
  {
    this->content_callback = do_node_contents;

    this->name  = "Add";
    this->color = {.5, .2, .3, 1};
    this->size  = 300;

    this->pins.push_back({"A", 1});
    this->pins.push_back({"B", 1});
    this->pins.push_back({"Out", 1});
  }

  static void do_node_contents(Dui::NodesData *data, Node *self)
  {
    Dui::Pin *a      = &self->pins[0];
    Dui::Pin *b      = &self->pins[1];
    Dui::Pin *output = &self->pins[2];

    Dui::label(a->label, Dui::PIN_TEXT_SIZE * data->scale, {1, 1, 1, 1}, false);
    Dui::node_input_pin(data, a);
    Dui::next_line();
    Dui::label(b->label, Dui::PIN_TEXT_SIZE * data->scale, {1, 1, 1, 1}, false);
    Dui::node_input_pin(data, b);
    Dui::next_line();
    Dui::button("Test", {0, 20 * data->scale}, {.5, .2, .3, 1}, true);
    Dui::label(output->label, Dui::PIN_TEXT_SIZE * data->scale, {1, 1, 1, 1}, true);
    Dui::node_output_pin(data, output);
  }
};

struct LerpNode : Dui::Node {
  LerpNode()
  {
    this->content_callback = do_node_contents;

    this->name  = "Lerp";
    this->color = {.3, .5, .2, 1};
    this->size  = 0;

    this->pins.push_back({"A", 1});
    this->pins.push_back({"B", 1});
    this->pins.push_back({"T", 1});
    this->pins.push_back({"Out", 1});
  }

  static void do_node_contents(Dui::NodesData *data, Node *self)
  {
    Dui::Pin *a      = &self->pins[0];
    Dui::Pin *b      = &self->pins[1];
    Dui::Pin *t      = &self->pins[2];
    Dui::Pin *output = &self->pins[3];

    Dui::label(a->label, Dui::PIN_TEXT_SIZE * data->scale, {1, 1, 1, 1}, false);
    Dui::node_input_pin(data, a);
    Dui::next_line();

    static Dui::DropdownData ddd;
    Dui::start_dropdown(&ddd, {0, 20 * data->scale}, true);

    Dui::dropdown_item(&ddd, "test1");
    Dui::dropdown_item(&ddd, "test2");

    Dui::end_dropdown(&ddd);

    Dui::label(b->label, Dui::PIN_TEXT_SIZE * data->scale, {1, 1, 1, 1}, false);
    Dui::node_input_pin(data, b);
    Dui::next_line();
    Dui::label(t->label, Dui::PIN_TEXT_SIZE * data->scale, {1, 1, 1, 1}, false);
    Dui::node_input_pin(data, t);
    Dui::next_line();
    Dui::label(output->label, Dui::PIN_TEXT_SIZE * data->scale, {1, 1, 1, 1}, true);
    Dui::node_output_pin(data, output);
  }
};

void do_material_editor_window(Editor::State *state, MaterialEditorWindow *mew)
{
  Temp tmp;

  Dui::start_window(mew->filename, {200, 200, 300, 300});

  // Dui::start_node_editor_interactions("material_id");

  // static b8 initted = false;
  // if (!initted) {
  //   initted    = true;
  //   mew->nodes = {
  //       {&dumb, Dui::create_node({100, 100}, 100, {.7, .2, .3, 1})},
  //       {&dumb, Dui::create_node({100, 200}, 100, {.2, .7, .3, 1})},
  //   };
  // }

  // for (i32 i = 0; i < mew->nodes.size; i++) {
  //   ShaderNodeDefinition *node_def = mew->nodes[i].node_def;

  //   Dui::start_node(&mew->nodes[i].dui_node_ref, i, "title");

  //   for (i32 pin = 0; pin < node_def->inputs.size; pin++) {
  //     Dui::node_input_pin(node_def->inputs[pin].name);
  //   }
  //   for (i32 pin = 0; pin < node_def->inputs.size; pin++) {
  //     Dui::node_output_pin(node_def->inputs[pin].name);
  //   }

  //   Dui::end_node();
  // }
  // Dui::end_node_editor_interactions();

  // // for (i32 i = 0; i < mew->links.size; i++) {
  // //   Dui::node_link(&mew->links[i]);
  // // }

  // i32 node_to_move_up = Dui::which_node_moved_to_top();
  // if (node_to_move_up > -1) {
  //   ShaderNode this_node = mew->nodes[node_to_move_up];
  //   for (i32 i = node_to_move_up; i < mew->nodes.size - 1; i++) {
  //     mew->nodes[i] = mew->nodes[i + 1];
  //   }

  //   mew->nodes[mew->nodes.size - 1] = this_node;
  // }

  // if (Dui::button("Compile", {200, 100}, {.8, .2, .3, 1})) {
  //   Node *output_node = find_output_node();

  //   push("output(");
  //   // specifically the pbr output type:
  //   Pin *albedo_input = get_input(output_node, 0);
  //   if (albedo_input) {
  //     NodeOutput result = evaluate_pin(albedo_input);
  //     validate(result.type, expected_pbr_type);
  //     push(result.name);
  //     push(", ");
  //   }
  //   else {
  //     // use the default value
  //   }
  //   Pin roughness_input = get_input(output_node, 1);

  // }

  // if (Dui::button("Do it!", {200, 100}, {.8, .2, .3, 1})) {
  //   GeneratedShader shader = material_nodes_test(
  //       "time = input(per_frame_data.time, 1)"
  //       "uv = input(in_uv, 2)"
  //       "uv2 = add(time, uv)"
  //       "diffuse = "
  //       "texture(\"../fracas/set/models/pedestal/Pedestal_Albedo.png\", "
  //       "4, uv2)"
  //       "output_node = output_node(uv, diffuse)",
  //       pbr_lit_shader_model, &tmp);

  //   write_file(NullTerminatedString::concatenate(state->resource_path,
  //                                                mew->filename, &tmp),
  //              shader.src, true);
  // }

  static Dui::NodeDefinition node_def_1;
  static Dui::NodeDefinition node_def_2;
  node_def_1.name    = "node_name_1";
  node_def_2.name    = "node_name_2";
  node_def_1.color   = {0.17, .4, .17, 1};
  node_def_2.color   = Color::from_int(0x83314a);
  node_def_1.size    = 200;
  node_def_2.size    = 100;
  node_def_1.outputs = {{{"Position", 1}}};
  node_def_2.outputs = {{{"AO", 1}, {"Color", 1}}};
  node_def_1.inputs  = {{{"pin0", 1}}};
  node_def_2.inputs  = {{{"pin1", 1}, {"pin2", 1}}};

  static b8 init = false;
  static Dui::NodesData nodes_data;
  if (!init) {
    init = true;

    Dui::add_node(&nodes_data, &add_node_def);
    Dui::add_node(&nodes_data, &node_def_1);
    Dui::add_node(&nodes_data, &node_def_2);
    Dui::add_node(&nodes_data, &node_def_2);
    Dui::add_node(&nodes_data, &node_def_2);
    Dui::add_node(&nodes_data, &node_def_2);
    Dui::add_node(&nodes_data, &node_def_2);
    Dui::add_node(&nodes_data, &output_node_def);
    Dui::add_node(&nodes_data, LerpNode());
    Dui::add_node(&nodes_data, AddNodeA());
    Dui::add_node(&nodes_data, AddNodeA());
  }

  Dui::do_nodes(&nodes_data);

  Dui::end_window();
}