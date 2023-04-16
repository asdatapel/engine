// #pragma once

// #include "dui/container.hpp"
// #include "dui/dui_state.hpp"

// namespace Dui
// {

// const f32 MIN_NODE_WIDTH  = 100;
// const f32 MIN_NODE_HEIGHT = 30;

// const f32 NODE_MARGIN = 3.f;
// const f32 PIN_HEIGHT  = 15.f;

// struct Node {
//   DuiId id;

//   Vec2f position;
//   f32 width;
//   Color color;
// };

// struct NodeLink {
//   Node *output_node;
//   i32 output_pin_idx;
//   Node *input_node;
//   i32 input_pin_idx;
// };

// struct NodeEditorState {
//   DuiId editor_id;
//   i32 node_count = 0;

//   f32 scale    = 1.f;
//   Vec2f origin = {100, 0};

//   i32 top_on_last_frame_idx = -1;
//   i32 top_on_this_frame_idx = -1;

//   i32 current_node_idx     = -1;
//   Node *current_node       = nullptr;
//   b8 interacted_this_frame = false;

//   Rect current_node_rect;
//   RoundedRectPrimitive *draw_rect_ref;
//   f32 next_input_y;
//   f32 next_output_y;

//   i32 input_pin_count;
//   i32 output_pin_count;

//   b8 link_in_progress = false;
//   NodeLink in_progress_link;
//   Vec2f in_progress_link_start;
//   Vec2f in_progress_link_end;
// };
// NodeEditorState nodes_state;

// Rect resize_for_zoom(Rect rect)
// {
//   return {
//       (rect.x * nodes_state.scale) + nodes_state.origin.x,
//       (rect.y * nodes_state.scale) + nodes_state.origin.y,
//       rect.width * nodes_state.scale,
//       rect.height * nodes_state.scale,
//   };
// }

// void start_node_editor_interactions(String editor_name)
// {
//   nodes_state.editor_id = hash(editor_name);

//   nodes_state.top_on_last_frame_idx = nodes_state.top_on_this_frame_idx;
//   nodes_state.top_on_this_frame_idx = -1;
//   nodes_state.interacted_this_frame = false;

//   nodes_state.scale += s.input->scrollwheel_count * .1f;
//   if (s.input->mouse_buttons[(i32)MouseButton::MIDDLE]) {
//     nodes_state.origin += s.input->mouse_pos_delta;
//   }

//   nodes_state.in_progress_link_start = s.input->mouse_pos;
//   nodes_state.in_progress_link_end   = s.input->mouse_pos;
//   nodes_state.in_progress_link.input_node = nullptr;
//   nodes_state.in_progress_link.output_node = nullptr;
// }
// void end_node_editor_interactions()
// {
//   Container *c = get_current_container(&s);

//   if (nodes_state.link_in_progress) {
//     Vec2f line_rect_pos  = min(nodes_state.in_progress_link_end,
//                                nodes_state.in_progress_link_start);
//     Vec2f line_rect_span = abs(nodes_state.in_progress_link_end -
//                                nodes_state.in_progress_link_start);
//     Rect line_rect       = {line_rect_pos.x, line_rect_pos.y, line_rect_span.x,
//                             line_rect_span.y};

//     if (c) {
//       push_rounded_rect(&s.dl, c->z, line_rect, 2.f, {1, 1, 1, .5});
//     }
//   }
//   nodes_state.link_in_progress = false;
// }

// Node create_node(Vec2f position, f32 width, Color color)
// {
//   Node node;
//   node.id       = extend_hash(nodes_state.editor_id, nodes_state.node_count++);
//   node.position = position;
//   node.width    = fminf(width, MIN_NODE_WIDTH);
//   node.color    = color;

//   return node;
// }

// void start_node(Node *node, i32 idx, String title)
// {
//   DuiId id = node->id;

//   Container *c = get_current_container(&s);
//   if (!c) return;

//   static f32 t = 0.f;
//   t += 0.001f;
//   f32 scale = 1 + sinf(t);

//   Rect titlebar_rect = {node->position.x, node->position.y, node->width,
//                         MIN_NODE_HEIGHT};
//   titlebar_rect      = resize_for_zoom(titlebar_rect);
//   titlebar_rect.x += c->content_rect.x;
//   titlebar_rect.y += c->content_rect.y;

//   b8 hot      = do_hot(id, titlebar_rect, c->rect,
//                        nodes_state.top_on_last_frame_idx == idx);
//   b8 active   = do_active(id);
//   b8 dragging = do_dragging(id);
//   b8 selected = do_selected(id, true);
//   b8 clicked  = hot && s.just_started_being_active == id;

//   nodes_state.interacted_this_frame |= active || dragging || clicked;
//   if (dragging) {
//     node->position += s.dragging_frame_delta / nodes_state.scale;
//   }

//   titlebar_rect = {node->position.x, node->position.y, node->width,
//                    MIN_NODE_HEIGHT};
//   titlebar_rect = resize_for_zoom(titlebar_rect);
//   titlebar_rect.x += c->content_rect.x;
//   titlebar_rect.y += c->content_rect.y;
//   nodes_state.draw_rect_ref =
//       push_rounded_rect(&s.dl, c->z, titlebar_rect, 5.f, node->color);

//   nodes_state.current_node      = node;
//   nodes_state.current_node_idx  = idx;
//   nodes_state.current_node_rect = titlebar_rect;
//   nodes_state.next_input_y  = node->position.y + MIN_NODE_HEIGHT + NODE_MARGIN;
//   nodes_state.next_output_y = nodes_state.next_input_y;

//   nodes_state.input_pin_count  = 0;
//   nodes_state.output_pin_count = 0;
// }

// void end_node()
// {
//   Container *c = get_current_container(&s);
//   if (!c) return;

//   Rect new_rect = resize_for_zoom(
//       {nodes_state.current_node->position.x,
//        nodes_state.current_node->position.y, nodes_state.current_node->width,
//        nodes_state.next_input_y - nodes_state.current_node->position.y});
//   new_rect.x += c->content_rect.x;
//   new_rect.y += c->content_rect.y;

//   if (in_rect(s.input->mouse_pos, new_rect) ||
//       s.dragging == nodes_state.current_node->id) {
//     nodes_state.top_on_this_frame_idx = nodes_state.current_node_idx;
//   }

//   if (nodes_state.draw_rect_ref) {
//     nodes_state.draw_rect_ref->dimensions = new_rect;
//   }
// }

// void node_input_pin(String label)
// {
//   Node *node  = nodes_state.current_node;
//   i32 pin_idx = nodes_state.input_pin_count++;
//   DuiId id    = extend_hash(node->id, pin_idx);

//   Container *c = get_current_container(&s);
//   if (!c) return;

//   Rect rect = {
//       node->position.x + NODE_MARGIN,
//       nodes_state.next_input_y,
//       (node->width / 2.f) - (2 * NODE_MARGIN),
//       PIN_HEIGHT,
//   };
//   nodes_state.next_input_y += rect.height + NODE_MARGIN;

//   rect = resize_for_zoom(rect);
//   rect.x += c->content_rect.x;
//   rect.y += c->content_rect.y;

//   b8 hot =
//       do_hot(id, rect, c->rect,
//              nodes_state.top_on_last_frame_idx == nodes_state.current_node_idx);
//   b8 active   = do_active(id);
//   b8 dragging = do_dragging(id);
//   b8 selected = do_selected(id, true);
//   b8 clicked  = hot && s.just_started_being_active == id;

//   Color color = {1, 1, 0, 1};
//   if (hot) color = darken(color, .1f);
//   push_rounded_rect(&s.dl, c->z, rect, 2.f, color);

//   Vec2f pin_anchor_pos = {rect.left(), rect.y + rect.height / 2};
//   if (nodes_state.link_in_progress &&
//       !nodes_state.in_progress_link.input_node) {
//     if (hot) {
//       nodes_state.link_in_progress               = true;
//       nodes_state.in_progress_link.input_node    = node;
//       nodes_state.in_progress_link.input_pin_idx = pin_idx;
//       nodes_state.in_progress_link_end           = pin_anchor_pos;
//     }
//   }
// }

// void node_output_pin(String label)
// {
//   Node *node  = nodes_state.current_node;
//   i32 pin_idx = nodes_state.output_pin_count++;
//   DuiId id    = extend_hash(node->id, pin_idx);

//   Container *c = get_current_container(&s);
//   if (!c) return;

//   Rect rect = {
//       node->position.x + node->width -
//           (NODE_MARGIN + (node->width / 2.f) - (2 * NODE_MARGIN)),
//       nodes_state.next_output_y,
//       (node->width / 2.f) - (2 * NODE_MARGIN),
//       PIN_HEIGHT,
//   };
//   nodes_state.next_output_y += rect.height + NODE_MARGIN;

//   rect = resize_for_zoom(rect);
//   rect.x += c->content_rect.x;
//   rect.y += c->content_rect.y;

//   b8 hot =
//       do_hot(id, rect, c->rect,
//              nodes_state.top_on_last_frame_idx == nodes_state.current_node_idx);
//   b8 active   = do_active(id);
//   b8 dragging = do_dragging(id);
//   b8 selected = do_selected(id, true);
//   b8 clicked  = hot && s.just_started_being_active == id;

//   Color color = {0, 1, 1, 1};
//   if (hot) color = darken(color, .1f);
//   push_rounded_rect(&s.dl, c->z, rect, 2.f, color);

//   Vec2f pin_anchor_pos = {rect.right(), rect.y + rect.height / 2};
//   if (dragging) {
//     nodes_state.link_in_progress                = true;
//     nodes_state.in_progress_link.output_node    = node;
//     nodes_state.in_progress_link.output_pin_idx = pin_idx;
//     nodes_state.in_progress_link_start          = pin_anchor_pos;
//   }
// }

// i32 which_node_moved_to_top()
// {
//   return nodes_state.interacted_this_frame ? nodes_state.top_on_this_frame_idx
//                                            : -1;
// }
// }  // namespace Dui