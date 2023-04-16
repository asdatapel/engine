#pragma once

#include "containers/array.hpp"
#include "containers/static_pool.hpp"
#include "dui/dui_state.hpp"
#include "math/math.hpp"
#include "string.hpp"

namespace Dui
{

const f32 NODE_WIDTH           = 150.f;
const f32 NODE_TITLEBAR_HEIGHT = 24.f;

const f32 PIN_WIDTH     = 10;
const f32 PIN_HEIGHT    = 10;
const f32 PIN_TEXT_SIZE = PIN_HEIGHT + 6;

struct Control {
  b8 select_on_mouse_down = false;

  b8 hot      = false;
  b8 active   = false;
  b8 dragging = false;
  b8 selected = false;

  Rect rect;

  void do_control(b8 *mouse_down_event, b8 *mouse_up_event)
  {
    hot = in_rect(s.input->mouse_pos, rect);

    if (*mouse_down_event) {
      if (hot) {
        active            = true;
        *mouse_down_event = false;  // eat the event
      }

      if (select_on_mouse_down) selected = hot;
    }

    if (active) {
      if (abs(s.start_position_active - s.input->mouse_pos).len() > 2.5f) {
        dragging = true;
      }
    }

    if (*mouse_up_event) {
      if (hot) {
        selected = active;
      } else {
        selected = false;
      }

      active   = false;
      dragging = false;
    }
  }

  Vec2f get_anchor() { return rect.center(); }
  Rect get_anchor_interaction_rect()
  {
    const f32 INTERACTION_RECT_RADIUS = 3.f;

    Vec2f anchor = get_anchor();
    return {anchor.x - INTERACTION_RECT_RADIUS,
            anchor.y - INTERACTION_RECT_RADIUS, INTERACTION_RECT_RADIUS * 2,
            INTERACTION_RECT_RADIUS * 2};
  }

  void reset_interaction()
  {
    hot      = false;
    active   = false;
    dragging = false;
    selected = false;
  }
};

struct Pin : Control {
  String label;
  i64 type;

  Pin() {}
  Pin(String label, i64 type)
  {
    this->label = label;
    this->type  = type;
  }

  Rect calc_rect(Rect parent_rect, i32 index, f32 scale, Vec2f origin,
                 b8 is_output)
  {
    f32 y_offset = index * (PIN_TEXT_SIZE + WINDOW_MARGIN_SIZE);

    rect.width  = PIN_WIDTH * scale;
    rect.height = PIN_HEIGHT * scale;
    rect.x      = parent_rect.x - (rect.width / 2);
    rect.y      = parent_rect.y +
             (NODE_TITLEBAR_HEIGHT + WINDOW_MARGIN_SIZE + y_offset) * scale;

    if (is_output) rect.x += parent_rect.width;

    return rect;
  }
};

struct Node : Control {
  String name;
  Array<Pin, 64> inputs;
  Array<Pin, 64> outputs;
  Color color;
  f32 size;

  Vec2f position;

  Node() { select_on_mouse_down = true; }

  Node(String name, Array<Pin, 64> inputs, Array<Pin, 64> outputs, Color color,
       f32 size)
  {
    this->name    = name;
    this->inputs  = inputs;
    this->outputs = outputs;
    this->color   = color;
    this->size    = size;

    select_on_mouse_down = true;
  }

  Rect calc_rect(Vec2f parent_pos, f32 scale, Vec2f origin)
  {
    rect.x      = parent_pos.x + origin.x + position.x * scale;
    rect.y      = parent_pos.y + origin.y + position.y * scale;
    rect.width  = NODE_WIDTH * scale;
    rect.height = size * scale;
    return rect;
  }

  Rect get_titlebar_rect(f32 scale)
  {
    return {rect.x, rect.y, rect.width, NODE_TITLEBAR_HEIGHT * scale};
  }

  Rect get_body_rect(f32 scale)
  {
    return {rect.x, rect.y + NODE_TITLEBAR_HEIGHT * scale, rect.width,
            rect.height - NODE_TITLEBAR_HEIGHT * scale};
  }
};

struct Link {
  Node *input_node  = nullptr;
  Pin *input_pin    = nullptr;
  Node *output_node = nullptr;
  Pin *output_pin   = nullptr;
};

struct NodeDefinition {
  String name;
  Array<Pin, 64> inputs;
  Array<Pin, 64> outputs;
  Color color;
  f32 size;
};

struct NodesData {
  StaticPool<Node, 2048> nodes;
  Array<Link, 4096> links;

  Array<i32, 2048> node_order;

  f32 scale           = .5f;
  Vec2f origin        = {0, 0};
  f32 target_scale    = .5f;
  Vec2f target_origin = {0, 0};

  b8 link_in_progress             = false;
  b8 link_in_progress_free_output = false;
  Link in_progress_link;
};

void delete_link(NodesData *data, Link *link)
{
  i32 link_idx = data->links.index_of(link);
  data->links.swap_delete(link_idx);
}

Link *get_link_to_input(NodesData *data, Pin *input)
{
  for (i32 i = 0; i < data->links.size; i++) {
    Link *link = &data->links[i];
    // TODO: make faster lookups
    if (link->input_pin == input) {
      return link;
    }
  }

  return nullptr;
}

b8 is_ancestor(NodesData *data, Node *node, Node *ancestor)
{
  if (node == ancestor) return true;
  for (i32 i = 0; i < node->inputs.size; i++) {
    Pin *pin = &node->inputs[i];
    for (i32 i = 0; i < data->links.size; i++) {
      Link *link = &data->links[i];
      // TODO: make faster lookups
      if (link->input_node == node) {
        if (is_ancestor(data, link->output_node, ancestor)) return true;
      }
    }
  }

  return false;
}

void do_nodes(NodesData *data)
{
  Container *c = get_current_container(&s);
  if (!c) return;

  Rect node_editor_rect = c->place(c->content_rect.span(), true, false, 0);

  b8 mouse_down_event =
      s.input->mouse_button_down_events[(i32)MouseButton::LEFT];
  b8 mouse_up_event = s.input->mouse_button_up_events[(i32)MouseButton::LEFT];

  {
    if (s.input->mouse_buttons[(i32)MouseButton::MIDDLE]) {
      data->target_origin += s.input->mouse_pos_delta;
    }

    Vec2f old_middle = ((node_editor_rect.span() / 2) - data->target_origin) /
                       data->target_scale;
    data->target_scale += s.input->scrollwheel_count * .1f;
    data->target_scale = clamp(data->target_scale, .1f, 1.f);
    Vec2f new_middle   = ((node_editor_rect.span() / 2) - data->target_origin) /
                       data->target_scale;
    data->target_origin -= (old_middle - new_middle) * data->target_scale;

    data->target_origin =
        clamp(data->target_origin, {-2000, -2000}, {2000, 2000});
  }

  data->scale  = (data->scale * .5 + data->target_scale * .5);
  data->origin = (data->origin + data->target_origin) / 2;

  i32 node_to_move_up_z = -1;
  for (i32 i = 0; i < data->node_order.size; i++) {
    i32 node_idx = data->node_order[i];
    Node *node   = &data->nodes[node_idx];

    Rect node_rect =
        node->calc_rect(node_editor_rect.xy(), data->scale, data->origin);

    {
      for (i32 output_i = 0; output_i < node->outputs.size; output_i++) {
        Pin *pin = &node->outputs[output_i];

        pin->calc_rect(node_rect, output_i, data->scale, data->origin, true);
        pin->do_control(&mouse_down_event, &mouse_up_event);
        if (pin->dragging) {
          data->link_in_progress             = true;
          data->link_in_progress_free_output = false;
          data->in_progress_link.output_node = node;
          data->in_progress_link.output_pin  = pin;
          pin->reset_interaction();
        }

        if (data->link_in_progress && data->link_in_progress_free_output) {
          if (in_rect(s.input->mouse_pos, pin->get_anchor_interaction_rect())) {
            data->in_progress_link.output_node = node;
            data->in_progress_link.output_pin  = pin;
          } else if (data->in_progress_link.output_pin == pin) {
            data->in_progress_link.output_node = nullptr;
            data->in_progress_link.output_pin  = nullptr;
          }
        }
      }
      for (i32 input_i = 0; input_i < node->inputs.size; input_i++) {
        Pin *pin = &node->inputs[input_i];

        pin->calc_rect(node_rect, input_i, data->scale, data->origin, false);
        pin->do_control(&mouse_down_event, &mouse_up_event);
        if (pin->dragging) {
          data->link_in_progress             = true;
          data->link_in_progress_free_output = true;
          data->in_progress_link.input_node  = node;
          data->in_progress_link.input_pin   = pin;
          pin->reset_interaction();

          Link *existing_link = get_link_to_input(data, pin);
          if (existing_link) {
            data->link_in_progress_free_output = false;
            data->in_progress_link.output_node = existing_link->output_node;
            data->in_progress_link.output_pin  = existing_link->output_pin;
            data->in_progress_link.input_node  = nullptr;
            data->in_progress_link.input_pin   = nullptr;

            delete_link(data, existing_link);
          }
        }

        if (data->link_in_progress && !data->link_in_progress_free_output) {
          if (in_rect(s.input->mouse_pos, pin->get_anchor_interaction_rect())) {
            data->in_progress_link.input_node = node;
            data->in_progress_link.input_pin  = pin;
          } else if (data->in_progress_link.input_pin == pin) {
            data->in_progress_link.input_node = nullptr;
            data->in_progress_link.input_pin  = nullptr;
          }
        }
      }
    }

    node->do_control(&mouse_down_event, &mouse_up_event);
    if (node->active) {
      node_to_move_up_z = i;
    }
    if (node->dragging) {
      node->position += s.input->mouse_pos_delta / data->scale;
    }
  }
  if (node_to_move_up_z > -1) {
    i32 copy = data->node_order[node_to_move_up_z];
    data->node_order.shift_delete(node_to_move_up_z);
    data->node_order.insert(0, copy);
  }

  {
    if (mouse_up_event) {
      if (data->link_in_progress && data->in_progress_link.input_pin &&
          data->in_progress_link.output_pin) {
        if (!is_ancestor(data, data->in_progress_link.output_node,
                         data->in_progress_link.input_node)) {
          data->links.push_back(data->in_progress_link);
        }
      }

      data->link_in_progress = false;
      data->in_progress_link = {};
    }
  }

  // TODO:
  // for (links) {
  //   check intersection with mouse handle focus handle delete
  // }

  auto draw_link = [&](Link link) {
    Vec2f input_position  = s.input->mouse_pos;
    Vec2f output_position = s.input->mouse_pos;

    if (link.input_pin) {
      input_position = link.input_pin->get_anchor();
    }
    if (link.output_pin) {
      output_position = link.output_pin->get_anchor();
    }

    Vec2f line_rect_pos  = min(input_position, output_position);
    Vec2f line_rect_span = abs(input_position - output_position);
    Rect line_rect       = {line_rect_pos.x, line_rect_pos.y, line_rect_span.x,
                            line_rect_span.y};

    push_rounded_rect(&s.dl, c->z, line_rect, 2.f, {1, 1, 1, .5});
  };

  {
    for (i32 i = 0; i < data->links.size; i++) {
      draw_link(data->links[i]);
    }
  }

  for (i32 i = data->node_order.size - 1; i >= 0; i--) {
    i32 node_idx = data->node_order[i];
    Node *node   = &data->nodes[node_idx];

    Rect node_rect =
        node->calc_rect(node_editor_rect.xy(), data->scale, data->origin);

    Rect border_rect = outset(node_rect, 1.f);
    push_rounded_rect(
        &s.dl, c->z, border_rect, 5.f * data->scale,
        node->selected ? Color{.9, .9, .9, 1} : Color{.1, .1, .1, 1});
    push_rounded_rect(&s.dl, c->z, node_rect, 5.f * data->scale,
                      {.4, .4, .4, 1});

    Rect titlebar_rect = node->get_titlebar_rect(data->scale);
    push_rounded_rect(&s.dl, c->z, titlebar_rect, 5.f * data->scale,
                      node->color, CornerMask::TOP);
    push_vector_text_centered(
        &s.dl, c->z, node->name, titlebar_rect.center(), {1, 1, 1, 1},
        (NODE_TITLEBAR_HEIGHT - 3) * data->scale, {true, true});

    for (i32 output_i = 0; output_i < node->outputs.size; output_i++) {
      Pin *pin = &node->outputs[output_i];

      Color color = {1, 1, 0, 1};
      if (pin->hot) color = darken(color, .1f);

      Rect pin_rect =
          pin->calc_rect(node_rect, output_i, data->scale, data->origin, true);
      Rect pin_border_rect = outset(pin_rect, 1);
      push_rounded_rect(&s.dl, c->z, pin_border_rect, pin_border_rect.width / 2,
                        {0, 0, 0, 1});
      push_rounded_rect(&s.dl, c->z, pin_rect, pin_rect.width / 2, color);

      Vec2f text_pos = {pin_rect.x, pin_rect.y + pin_rect.height / 2};
      push_vector_text_justified(&s.dl, c->z, pin->label, text_pos,
                                 {1, 1, 1, 1}, PIN_TEXT_SIZE * data->scale,
                                 {2, 1});
    }
    for (i32 input_i = 0; input_i < node->inputs.size; input_i++) {
      Pin *pin = &node->inputs[input_i];

      Color color = {1, 0, 1, 1};
      if (pin->hot) color = darken(color, .1f);

      Rect pin_rect =
          pin->calc_rect(node_rect, input_i, data->scale, data->origin, false);
      Rect pin_border_rect = outset(pin_rect, 1);
      push_rounded_rect(&s.dl, c->z, pin_border_rect, pin_border_rect.width / 2,
                        {0, 0, 0, 1});
      push_rounded_rect(&s.dl, c->z, pin_rect, pin_rect.width / 2, color);
    }
  }

  {
    if (data->link_in_progress) {
      draw_link(data->in_progress_link);
    }
  }
}

void add_node(NodesData *data, NodeDefinition *node_def)
{
  Node new_node;
  new_node.name     = node_def->name;
  new_node.inputs   = node_def->inputs;
  new_node.outputs  = node_def->outputs;
  new_node.color    = node_def->color;
  new_node.size     = node_def->size;
  new_node.position = {10, 10};

  Node *new_ptr = data->nodes.push_back(new_node);
  data->node_order.insert(0, data->nodes.index_of(new_ptr));
}

}  // namespace Dui