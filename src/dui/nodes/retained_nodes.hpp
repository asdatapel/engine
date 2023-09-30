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
const f32 PIN_TEXT_SIZE = PIN_HEIGHT + 8;

struct Control {
  b8 select_on_mouse_down = false;

  b8 hot      = false;
  b8 active   = false;
  b8 dragging = false;
  b8 selected = false;

  Rect rect;

  void do_control(Input *input, b8 node_on_top)
  {
    hot = in_rect(s.input->mouse_pos, rect);

    b8 mouse_down_event =
        s.input->mouse_button_down_events[(i32)MouseButton::LEFT] &&
        node_on_top;
    b8 mouse_up_event = s.input->mouse_button_up_events[(i32)MouseButton::LEFT];

    if (mouse_down_event) {
      if (hot) {
        active = true;
      }

      if (select_on_mouse_down) selected = hot;
    }

    if (active) {
      if (abs(s.start_position_active - s.input->mouse_pos).len() > 2.5f) {
        dragging = true;
      }
    }

    if (mouse_up_event) {
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

  Rect calc_rect(Vec2f center, f32 scale, Vec2f origin)
  {
    rect.width  = PIN_WIDTH * scale;
    rect.height = PIN_HEIGHT * scale;
    rect.x      = center.x - (rect.width / 2);
    rect.y      = center.y - (rect.height / 2);
    return rect;
  }
};

struct NodesData;
void node_output_pin(NodesData *, Pin *);
struct Node : Control {
  String name;
  Array<Pin, 64> inputs;
  Array<Pin, 64> outputs;
  Color color;
  f32 size;
  Array<Pin, 64> pins;

  Vec2f position;

  f32 previous_frame_content_height = 0;

  void (*content_callback)(Dui::NodesData *data,
                           Node *self) = default_content_callback;

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
    rect.x     = parent_pos.x + origin.x + position.x * scale;
    rect.y     = parent_pos.y + origin.y + position.y * scale;
    rect.width = NODE_WIDTH * scale;
    rect.height =
        previous_frame_content_height + (NODE_TITLEBAR_HEIGHT * scale);
    return rect;
  }

  Rect get_titlebar_rect(f32 scale)
  {
    return {rect.x, rect.y, rect.width, NODE_TITLEBAR_HEIGHT * scale};
  }

  Rect get_body_rect(f32 scale)
  {
    f32 extra_margin = 5.f * scale;
    return {rect.x + extra_margin, rect.y + NODE_TITLEBAR_HEIGHT * scale,
            rect.width - (extra_margin * 2),
            rect.height - NODE_TITLEBAR_HEIGHT * scale};
  }

  static void default_content_callback(Dui::NodesData *data, Node *self)
  {
    Dui::Pin output;
    Dui::node_output_pin(data, &output);
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
  f32 target_scale    = .75f;
  Vec2f target_origin = {0, 0};

  b8 selection_in_progress = false;
  Vec2f selection_start_pos;
  Rect selection_rect;

  b8 link_in_progress             = false;
  b8 link_in_progress_free_output = false;
  Link in_progress_link;

  Node *top_node_this_frame = nullptr;
  Node *top_node_last_frame = nullptr;
  b8 mouse_is_on_control    = false;

  // per node data
  Node *current_node             = nullptr;
  u32 container_draw_scissor_idx = 0;
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
  for (i32 i = 0; i < node->pins.size; i++) {
    Pin *pin = &node->pins[i];
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
  data->top_node_last_frame = data->top_node_this_frame;
  data->top_node_this_frame = nullptr;
  data->mouse_is_on_control = false;

  Container *c = get_current_container(&s);
  if (!c) return;
  data->container_draw_scissor_idx = c->draw_scissor_idx;

  Rect node_editor_rect = c->place(c->content_rect.span(), true, false);
  {
    if (s.input->mouse_buttons[(i32)MouseButton::MIDDLE]) {
      data->target_origin += s.input->mouse_pos_delta;
    }

    Vec2f old_middle = ((node_editor_rect.span() / 2) - data->target_origin) /
                       data->target_scale;
    data->target_scale += s.input->scrollwheel_count * .1f;
    data->target_scale = clamp(data->target_scale, .1f, 1.5f);
    Vec2f new_middle   = ((node_editor_rect.span() / 2) - data->target_origin) /
                       data->target_scale;
    data->target_origin -= (old_middle - new_middle) * data->target_scale;
    data->target_origin =
        clamp(data->target_origin, {-2000, -2000}, {2000, 2000});

    if (data->top_node_last_frame == nullptr) {
      if (s.top_container_at_mouse_pos == c->id &&
          s.input->mouse_button_down_events[(i32)MouseButton::LEFT]) {
        data->selection_in_progress = true;
        data->selection_start_pos   = s.input->mouse_pos;
      }
    }
    if (data->selection_in_progress) {
      data->selection_rect.x =
          fminf(data->selection_start_pos.x, s.input->mouse_pos.x);
      data->selection_rect.y =
          fminf(data->selection_start_pos.y, s.input->mouse_pos.y);
      data->selection_rect.width =
          fabsf(s.input->mouse_pos.x - data->selection_start_pos.x);
      data->selection_rect.height =
          fabsf(s.input->mouse_pos.y - data->selection_start_pos.y);
    }
  }

  data->scale  = (data->scale * .5 + data->target_scale * .5);
  data->origin = (data->origin + data->target_origin) / 2;

  auto draw_link = [&](Link link) {
    Vec2f input_position  = s.input->mouse_pos;
    Vec2f output_position = s.input->mouse_pos;

    if (link.input_pin) {
      input_position = link.input_pin->get_anchor();
    }
    if (link.output_pin) {
      output_position = link.output_pin->get_anchor();
    }

    f32 spline_control_point_distance =
        fminf(abs(input_position.x - output_position.x) / (2.f * data->scale),
              200.f * data->scale);

    Vec2f spline_points[4];
    spline_points[0] = output_position;
    spline_points[1] =
        output_position + Vec2f{spline_control_point_distance, 0};
    spline_points[2] = input_position - Vec2f{spline_control_point_distance, 0};
    spline_points[3] = input_position;

    i32 n_samples = (input_position - output_position).len() / 10 + 4;
    push_cubic_spline(&s.dl, c->z, spline_points, {.9, .9, .9, 1}, n_samples);
  };
  for (i32 i = 0; i < data->links.size; i++) {
    draw_link(data->links[i]);
  }

  b8 mouse_on_selected = false;

  i32 node_to_move_up_z = -1;
  for (i32 i = data->node_order.size - 1; i >= 0; i--) {
    i32 node_idx = data->node_order[i];
    Node *node   = &data->nodes[node_idx];

    data->current_node = node;

    Rect node_rect =
        node->calc_rect(node_editor_rect.xy(), data->scale, data->origin);
    Rect content_rect  = node->get_body_rect(data->scale);
    Rect border_rect   = outset(node_rect, 1.f);
    Rect titlebar_rect = node->get_titlebar_rect(data->scale);

    b8 is_in_selection_box = data->selection_in_progress &&
                             overlaps(node_rect, data->selection_rect);
    b8 highlighted = node->selected || is_in_selection_box;

    push_rounded_rect(
        &s.dl, c->z, border_rect, 5.f * data->scale,
        highlighted ? Color{.9, .9, .9, 1} : Color{.1, .1, .1, 1});
    push_rounded_rect(&s.dl, c->z, node_rect, 5.f * data->scale,
                      {.25, .25, .25, 1});
    push_rounded_rect(&s.dl, c->z, titlebar_rect, 5.f * data->scale,
                      node->color, CornerMask::TOP);
    push_vector_text_centered(
        &s.dl, c->z, node->name, titlebar_rect.center(), {1, 1, 1, 1},
        (NODE_TITLEBAR_HEIGHT - 3) * data->scale, {true, true});

    Container container;
    container.rect     = content_rect;
    container.z        = c->z;
    container.hot_mask = (data->top_node_last_frame == node);
    container.line_gap = DEFAULT_LINE_GAP * data->scale;

    container.start_frame(&s, false);
    node->content_callback(data, node);
    container.end_frame(&s, false);

    container.fit_rect_to_content();
    node->previous_frame_content_height = container.rect.height;

    if (in_rect(s.input->mouse_pos, node_rect)) {
      data->top_node_this_frame = node;
    }

    b8 mouse_on_empty_space =
        !data->mouse_is_on_control && s.just_started_being_hot == -1;

    node->do_control(s.input,
                     mouse_on_empty_space && data->top_node_last_frame == node);
    if (node->active) {
      node_to_move_up_z = i;
    }
    if (node->dragging) {
      node->position += s.input->mouse_pos_delta / data->scale;
    }

    if (node->selected && s.input->keys[(i32)Keys::DEL]) {
      data->node_order.shift_delete(i);
      data->nodes.remove(node_idx);
    }
  }
  if (node_to_move_up_z > -1) {
    i32 copy = data->node_order[node_to_move_up_z];
    data->node_order.shift_delete(node_to_move_up_z);
    data->node_order.insert(0, copy);
  }

  {
    if (s.input->mouse_button_up_events[(i32)MouseButton::LEFT]) {
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

  {
    if (data->link_in_progress) {
      draw_link(data->in_progress_link);
    }
  }

  if (data->selection_in_progress) {
    push_rounded_rect(&s.dl, c->z, data->selection_rect, 0, {.9, .9, .9, .6});
    push_rounded_rect(&s.dl, c->z, inset(data->selection_rect, 1), 0,
                      {.2, .2, .2, .2});

    if (s.input->mouse_button_up_events[(i32)MouseButton::LEFT]) {
      data->selection_in_progress = false;

      for (i32 i = 0; i < data->node_order.size; i++) {
        i32 node_idx = data->node_order[i];
        Node *node   = &data->nodes[node_idx];
        if (overlaps(node->rect, data->selection_rect)) {
          node->selected = true;
        }
      }
    }
  }
}

void node_output_pin(NodesData *data, Pin *pin)
{
  Container *c = get_current_container(&s);

  Vec2f pin_center =
      Vec2f{data->current_node->rect.x + data->current_node->rect.width,
            c->content_rect.y + c->cursor.y + (c->cursor_size / 2)};
  Rect pin_rect = pin->calc_rect(pin_center, data->scale, data->origin);

  pin->do_control(s.input, data->top_node_last_frame == data->current_node);
  if (pin->hot) {
    data->top_node_this_frame = data->current_node;
    data->mouse_is_on_control = true;
  }
  if (pin->dragging) {
    data->link_in_progress             = true;
    data->link_in_progress_free_output = false;
    data->in_progress_link.output_node = data->current_node;
    data->in_progress_link.output_pin  = pin;
    pin->reset_interaction();
  }

  if (data->link_in_progress && data->link_in_progress_free_output) {
    if (in_rect(s.input->mouse_pos, pin->get_anchor_interaction_rect())) {
      data->in_progress_link.output_node = data->current_node;
      data->in_progress_link.output_pin  = pin;
    } else if (data->in_progress_link.output_pin == pin) {
      data->in_progress_link.output_node = nullptr;
      data->in_progress_link.output_pin  = nullptr;
    }
  }

  Color color = {1, 0, 1, 1};
  if (pin->hot) color = darken(color, .1f);

  push_draw_settings(&s.dl, {true, data->container_draw_scissor_idx});

  Rect pin_border_rect = outset(pin_rect, 1);
  push_rounded_rect(&s.dl, c->z, pin_border_rect, pin_border_rect.width / 2,
                    {0, 0, 0, 1});
  push_rounded_rect(&s.dl, c->z, pin_rect, pin_rect.width / 2, color);

  pop_draw_settings(&s.dl);
}

void node_input_pin(NodesData *data, Pin *pin)
{
  Container *c = get_current_container(&s);

  Vec2f pin_center =
      Vec2f{data->current_node->rect.x,
            c->content_rect.y + c->cursor.y + (c->cursor_size / 2)};
  Rect pin_rect = pin->calc_rect(pin_center, data->scale, data->origin);

  pin->do_control(s.input, data->top_node_last_frame == data->current_node);
  if (pin->hot) {
    data->top_node_this_frame = data->current_node;
    data->mouse_is_on_control = true;
  }
  if (pin->dragging) {
    data->link_in_progress             = true;
    data->link_in_progress_free_output = true;
    data->in_progress_link.input_node  = data->current_node;
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
      data->in_progress_link.input_node = data->current_node;
      data->in_progress_link.input_pin  = pin;
    } else if (data->in_progress_link.input_pin == pin) {
      data->in_progress_link.input_node = nullptr;
      data->in_progress_link.input_pin  = nullptr;
    }
  }

  Color color = {1, 0, 1, 1};
  if (pin->hot) color = darken(color, .1f);

  push_draw_settings(&s.dl, {true, data->container_draw_scissor_idx});

  Rect pin_border_rect = outset(pin_rect, 1);
  push_rounded_rect(&s.dl, c->z, pin_border_rect, pin_border_rect.width / 2,
                    {0, 0, 0, 1});
  push_rounded_rect(&s.dl, c->z, pin_rect, pin_rect.width / 2, color);

  pop_draw_settings(&s.dl);
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
void add_node(NodesData *data, Node node)
{
  node.position = {10, 10};

  Node *new_ptr = data->nodes.push_back(node);
  data->node_order.insert(0, data->nodes.index_of(new_ptr));
}

}  // namespace Dui