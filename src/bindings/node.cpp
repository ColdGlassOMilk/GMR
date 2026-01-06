#include "gmr/bindings/node.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/node.hpp"
#include "gmr/types.hpp"
#include "gmr/scripting/helpers.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/array.h>
#include <cmath>

namespace gmr {
namespace bindings {

/// @class Node
/// @description A scene graph node with hierarchical transforms.
///   Each node has a local transform (relative to parent) and a world transform
///   (computed from the full hierarchy). Nodes form a tree: one parent, many children.
/// @example # Character with weapon and particle attachment points
///   class Character
///     def initialize(x, y)
///       @root = Node.new
///       @root.local_position = Vec2.new(x, y)
///
///       # Weapon mount point (offset from character center)
///       @weapon_node = Node.new
///       @weapon_node.local_position = Vec2.new(24, 8)  # Right side
///       @root.add_child(@weapon_node)
///
///       # Particle spawn point for effects (above head)
///       @particle_node = Node.new
///       @particle_node.local_position = Vec2.new(0, -32)
///       @root.add_child(@particle_node)
///
///       @sprite = GMR::Sprite.new(GMR::Graphics::Texture.load("assets/character.png"))
///     end
///
///     def update(dt)
///       # Move character
///       @root.local_position = Vec2.new(@root.local_position.x + @vx * dt,
///                                        @root.local_position.y + @vy * dt)
///       # Update all world transforms from root
///       Node.update_world_transforms(@root)
///     end
///
///     def draw
///       pos = @root.world_position
///       @sprite.x = pos.x
///       @sprite.y = pos.y
///       @sprite.draw
///     end
///
///     def weapon_position
///       @weapon_node.world_position  # Get world space position for weapon sprite
///     end
///
///     def spawn_particle_at_head
///       pos = @particle_node.world_position
///       ParticleSystem.spawn_at(pos.x, pos.y)
///     end
///   end
/// @example # Rotating turret with barrel
///   class Turret
///     def initialize(x, y)
///       @base = Node.new
///       @base.local_position = Vec2.new(x, y)
///
///       @barrel = Node.new
///       @barrel.local_position = Vec2.new(32, 0)  # Barrel extends right
///       @base.add_child(@barrel)
///
///       @muzzle = Node.new
///       @muzzle.local_position = Vec2.new(24, 0)  # Muzzle at end of barrel
///       @barrel.add_child(@muzzle)
///
///       @base_sprite = GMR::Sprite.new(GMR::Graphics::Texture.load("assets/turret_base.png"))
///       @barrel_sprite = GMR::Sprite.new(GMR::Graphics::Texture.load("assets/turret_barrel.png"))
///     end
///
///     def aim_at(target_x, target_y)
///       pos = @base.world_position
///       dx = target_x - pos.x
///       dy = target_y - pos.y
///       @barrel.local_rotation = Math.atan2(dy, dx) * 180 / Math::PI
///       Node.update_world_transforms(@base)
///     end
///
///     def fire
///       muzzle_pos = @muzzle.world_position
///       angle = @barrel.world_rotation
///       Projectile.spawn(muzzle_pos.x, muzzle_pos.y, angle)
///       GMR::Audio::Sound.play("assets/cannon_fire.wav")
///     end
///
///     def draw
///       @base_sprite.x = @base.world_position.x
///       @base_sprite.y = @base.world_position.y
///       @base_sprite.draw
///
///       @barrel_sprite.x = @barrel.world_position.x
///       @barrel_sprite.y = @barrel.world_position.y
///       @barrel_sprite.rotation = @barrel.world_rotation
///       @barrel_sprite.draw
///     end
///   end
/// @example # UI panel with child elements
///   class UIPanel
///     def initialize(x, y)
///       @panel = Node.new
///       @panel.local_position = Vec2.new(x, y)
///
///       @title = Node.new
///       @title.local_position = Vec2.new(10, 10)
///       @panel.add_child(@title)
///
///       @content = Node.new
///       @content.local_position = Vec2.new(10, 40)
///       @panel.add_child(@content)
///
///       @close_btn = Node.new
///       @close_btn.local_position = Vec2.new(180, 10)
///       @panel.add_child(@close_btn)
///     end
///
///     def show
///       @panel.active = true
///       # Slide in animation
///       GMR::Tween.to(@panel, :local_position, Vec2.new(@target_x, @target_y),
///                     duration: 0.3, ease: :out_back)
///     end
///
///     def hide
///       @panel.active = false
///     end
///
///     def draw
///       return unless @panel.active?
///       Node.update_world_transforms(@panel)
///       # Draw panel background at panel position
///       pos = @panel.world_position
///       GMR::Graphics.draw_rect(pos.x, pos.y, 200, 150, [40, 40, 60, 220])
///       # Draw title at title node position
///       title_pos = @title.world_position
///       GMR::Graphics.draw_text("Inventory", title_pos.x, title_pos.y, 20, [255, 255, 255])
///     end
///   end

// Degrees <-> Radians conversion
static constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
static constexpr float RAD_TO_DEG = 180.0f / 3.14159265358979323846f;

// Node Binding Data
struct NodeData {
    NodeHandle handle;
};

static void node_free(mrb_state* mrb, void* ptr) {
    NodeData* data = static_cast<NodeData*>(ptr);
    if (data) {
        NodeManager::instance().destroy(data->handle);
        mrb_free(mrb, data);
    }
}

static const mrb_data_type node_data_type = {
    "Node", node_free
};

static NodeData* get_node_data(mrb_state* mrb, mrb_value self) {
    return static_cast<NodeData*>(mrb_data_get_ptr(mrb, self, &node_data_type));
}

// Helper: Create Vec2 Ruby Object (GMR::Mathf::Vec2)
static mrb_value create_vec2(mrb_state* mrb, float x, float y) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* mathf = mrb_module_get_under(mrb, gmr, "Mathf");
    RClass* vec2_class = mrb_class_get_under(mrb, mathf, "Vec2");
    mrb_value args[2] = {mrb_float_value(mrb, x), mrb_float_value(mrb, y)};
    return mrb_obj_new(mrb, vec2_class, 2, args);
}

// Helper: Extract Vec2 from Ruby Value
static Vec2 extract_vec2(mrb_state* mrb, mrb_value val) {
    mrb_sym x_sym = mrb_intern_cstr(mrb, "x");
    mrb_sym y_sym = mrb_intern_cstr(mrb, "y");

    if (mrb_respond_to(mrb, val, x_sym) && mrb_respond_to(mrb, val, y_sym)) {
        mrb_value x = scripting::safe_method_call(mrb, val, "x");
        mrb_value y = scripting::safe_method_call(mrb, val, "y");
        return {static_cast<float>(mrb_as_float(mrb, x)),
                static_cast<float>(mrb_as_float(mrb, y))};
    }

    mrb_raise(mrb, E_TYPE_ERROR, "Expected Vec2 or object with x/y methods");
    return {0.0f, 0.0f};
}

// Store RClass pointer for creating Node objects from C
static RClass* node_class_ptr = nullptr;

// Helper: Create Ruby Node wrapper for an existing handle without calling initialize
// This avoids the wasteful pattern of: create -> destroy -> reassign
static mrb_value wrap_existing_node(mrb_state* mrb, NodeHandle handle) {
    if (!node_class_ptr || handle == INVALID_NODE_HANDLE) {
        return mrb_nil_value();
    }

    // Allocate Ruby object without calling initialize
    mrb_value obj = mrb_obj_value(mrb_obj_alloc(mrb, MRB_TT_CDATA, node_class_ptr));

    // Allocate and initialize wrapper data
    NodeData* data = static_cast<NodeData*>(mrb_malloc(mrb, sizeof(NodeData)));
    data->handle = handle;
    mrb_data_init(obj, data, &node_data_type);

    return obj;
}

// ============================================================================
// Constructor
// ============================================================================

/// @method initialize
/// @description Create a new Node with default transform.
/// @returns [Node] The new node
/// @example node = Node.new
static mrb_value mrb_node_initialize(mrb_state* mrb, mrb_value self) {
    NodeHandle handle = NodeManager::instance().create();

    NodeData* data = static_cast<NodeData*>(mrb_malloc(mrb, sizeof(NodeData)));
    data->handle = handle;
    mrb_data_init(self, data, &node_data_type);

    return self;
}

// ============================================================================
// Hierarchy Methods
// ============================================================================

/// @method add_child
/// @description Add a child node. Removes from previous parent if any.
///   Cycle detection prevents a node from becoming its own ancestor.
/// @param child [Node] The node to add as a child
/// @returns [Node] self for chaining
/// @example root.add_child(child)
static mrb_value mrb_node_add_child(mrb_state* mrb, mrb_value self) {
    mrb_value child_val;
    mrb_get_args(mrb, "o", &child_val);

    NodeData* parent_data = get_node_data(mrb, self);
    NodeData* child_data = get_node_data(mrb, child_val);

    if (!parent_data || !child_data) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Invalid node");
        return mrb_nil_value();
    }

    NodeManager::instance().add_child(parent_data->handle, child_data->handle);
    return self;
}

/// @method remove_child
/// @description Remove a child node. The child becomes a root node.
/// @param child [Node] The node to remove
/// @returns [Node] self for chaining
/// @example root.remove_child(child)
static mrb_value mrb_node_remove_child(mrb_state* mrb, mrb_value self) {
    mrb_value child_val;
    mrb_get_args(mrb, "o", &child_val);

    NodeData* parent_data = get_node_data(mrb, self);
    NodeData* child_data = get_node_data(mrb, child_val);

    if (!parent_data || !child_data) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Invalid node");
        return mrb_nil_value();
    }

    NodeManager::instance().remove_child(parent_data->handle, child_data->handle);
    return self;
}

/// @method parent
/// @description Get the parent node, or nil if this is a root.
/// @returns [Node, nil] The parent node
/// @example if node.parent then puts "has parent" end
static mrb_value mrb_node_parent(mrb_state* mrb, mrb_value self) {
    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    Node* node = NodeManager::instance().get(data->handle);
    if (!node || !node->parent) return mrb_nil_value();

    NodeHandle parent_handle = NodeManager::instance().get_handle(node->parent);
    if (parent_handle == INVALID_NODE_HANDLE) return mrb_nil_value();

    // Create Ruby wrapper for existing node (no wasteful create/destroy)
    return wrap_existing_node(mrb, parent_handle);
}

/// @method children
/// @description Get an array of child nodes.
/// @returns [Array<Node>] Array of children (empty if none)
/// @example node.children.each { |c| puts c.local_position }
static mrb_value mrb_node_children(mrb_state* mrb, mrb_value self) {
    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_ary_new(mrb);

    Node* node = NodeManager::instance().get(data->handle);
    if (!node) return mrb_ary_new(mrb);

    mrb_value children_array = mrb_ary_new_capa(mrb, node->child_count);

    for (int i = 0; i < node->child_count; ++i) {
        NodeHandle child_handle = NodeManager::instance().get_handle(node->children[i]);
        if (child_handle == INVALID_NODE_HANDLE) continue;

        // Create Ruby wrapper for existing node (no wasteful create/destroy)
        mrb_value child_obj = wrap_existing_node(mrb, child_handle);
        if (!mrb_nil_p(child_obj)) {
            mrb_ary_push(mrb, children_array, child_obj);
        }
    }

    return children_array;
}

/// @method child_count
/// @description Get the number of children.
/// @returns [Integer] Number of children
/// @example puts node.child_count
static mrb_value mrb_node_child_count(mrb_state* mrb, mrb_value self) {
    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_fixnum_value(0);

    Node* node = NodeManager::instance().get(data->handle);
    if (!node) return mrb_fixnum_value(0);

    return mrb_fixnum_value(node->child_count);
}

// ============================================================================
// Local Transform Getters
// ============================================================================

/// @method local_position
/// @description Get the local position relative to parent.
/// @returns [Vec2] The local position
/// @example pos = node.local_position
static mrb_value mrb_node_local_position(mrb_state* mrb, mrb_value self) {
    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    Node* node = NodeManager::instance().get(data->handle);
    if (!node) return mrb_nil_value();

    return create_vec2(mrb, node->local.position.x, node->local.position.y);
}

/// @method local_rotation
/// @description Get the local rotation in degrees.
/// @returns [Float] The local rotation
/// @example angle = node.local_rotation
static mrb_value mrb_node_local_rotation(mrb_state* mrb, mrb_value self) {
    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    Node* node = NodeManager::instance().get(data->handle);
    if (!node) return mrb_nil_value();

    return mrb_float_value(mrb, node->local.rotation * RAD_TO_DEG);
}

/// @method local_scale
/// @description Get the local scale.
/// @returns [Vec2] The local scale
/// @example scale = node.local_scale
static mrb_value mrb_node_local_scale(mrb_state* mrb, mrb_value self) {
    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    Node* node = NodeManager::instance().get(data->handle);
    if (!node) return mrb_nil_value();

    return create_vec2(mrb, node->local.scale.x, node->local.scale.y);
}

// ============================================================================
// Local Transform Setters
// ============================================================================

/// @method local_position=
/// @description Set the local position relative to parent.
/// @param value [Vec2] The new local position
/// @returns [Vec2] The value that was set
/// @example node.local_position = Vec2.new(100, 50)
static mrb_value mrb_node_set_local_position(mrb_state* mrb, mrb_value self) {
    mrb_value val;
    mrb_get_args(mrb, "o", &val);

    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    Node* node = NodeManager::instance().get(data->handle);
    if (!node) return mrb_nil_value();

    node->local.position = extract_vec2(mrb, val);
    return val;
}

/// @method local_rotation=
/// @description Set the local rotation in degrees.
/// @param value [Float] The new local rotation
/// @returns [Float] The value that was set
/// @example node.local_rotation = 45
static mrb_value mrb_node_set_local_rotation(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);

    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    Node* node = NodeManager::instance().get(data->handle);
    if (!node) return mrb_nil_value();

    node->local.rotation = static_cast<float>(val) * DEG_TO_RAD;
    return mrb_float_value(mrb, val);
}

/// @method local_scale=
/// @description Set the local scale.
/// @param value [Vec2] The new local scale
/// @returns [Vec2] The value that was set
/// @example node.local_scale = Vec2.new(2, 2)
static mrb_value mrb_node_set_local_scale(mrb_state* mrb, mrb_value self) {
    mrb_value val;
    mrb_get_args(mrb, "o", &val);

    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    Node* node = NodeManager::instance().get(data->handle);
    if (!node) return mrb_nil_value();

    node->local.scale = extract_vec2(mrb, val);
    return val;
}

// ============================================================================
// World Transform Getters (read-only)
// ============================================================================

/// @method world_position
/// @description Get the world position after hierarchy composition.
///   Call Node.update_world_transforms(root) first to ensure accuracy.
/// @returns [Vec2] The world position
/// @example pos = child.world_position
static mrb_value mrb_node_world_position(mrb_state* mrb, mrb_value self) {
    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    Node* node = NodeManager::instance().get(data->handle);
    if (!node) return mrb_nil_value();

    return create_vec2(mrb, node->world.position.x, node->world.position.y);
}

/// @method world_rotation
/// @description Get the world rotation in degrees after hierarchy composition.
/// @returns [Float] The world rotation
/// @example angle = child.world_rotation
static mrb_value mrb_node_world_rotation(mrb_state* mrb, mrb_value self) {
    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    Node* node = NodeManager::instance().get(data->handle);
    if (!node) return mrb_nil_value();

    return mrb_float_value(mrb, node->world.rotation * RAD_TO_DEG);
}

/// @method world_scale
/// @description Get the world scale after hierarchy composition.
/// @returns [Vec2] The world scale
/// @example scale = child.world_scale
static mrb_value mrb_node_world_scale(mrb_state* mrb, mrb_value self) {
    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    Node* node = NodeManager::instance().get(data->handle);
    if (!node) return mrb_nil_value();

    return create_vec2(mrb, node->world.scale.x, node->world.scale.y);
}

// ============================================================================
// Active State
// ============================================================================

/// @method active
/// @description Get the active flag of this node (ignores parent state).
/// @returns [Boolean] true if this node is active
/// @example if node.active then ... end
static mrb_value mrb_node_active(mrb_state* mrb, mrb_value self) {
    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_false_value();

    Node* node = NodeManager::instance().get(data->handle);
    if (!node) return mrb_false_value();

    return mrb_bool_value(node->active);
}

/// @method active=
/// @description Set the active flag of this node.
/// @param value [Boolean] true to activate, false to deactivate
/// @returns [Boolean] The value that was set
/// @example node.active = false
static mrb_value mrb_node_set_active(mrb_state* mrb, mrb_value self) {
    mrb_bool val;
    mrb_get_args(mrb, "b", &val);

    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_bool_value(val);

    Node* node = NodeManager::instance().get(data->handle);
    if (!node) return mrb_bool_value(val);

    node->active = val;
    return mrb_bool_value(val);
}

/// @method active?
/// @description Check if this node is active in the full hierarchy.
///   Returns false if this node or any ancestor is inactive.
/// @returns [Boolean] true if active in hierarchy
/// @example if node.active? then update(node) end
static mrb_value mrb_node_is_active(mrb_state* mrb, mrb_value self) {
    NodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_false_value();

    bool active = NodeManager::instance().is_active_in_hierarchy(data->handle);
    return mrb_bool_value(active);
}

// ============================================================================
// Class Methods
// ============================================================================

/// @method Node.update_world_transforms
/// @description Update world transforms for a node and all descendants.
///   Call this on the root before reading world_position/rotation/scale.
/// @param root [Node] The root node to start from
/// @returns [nil]
/// @example Node.update_world_transforms(root)
static mrb_value mrb_node_class_update_world_transforms(mrb_state* mrb, mrb_value self) {
    mrb_value root_val;
    mrb_get_args(mrb, "o", &root_val);

    NodeData* root_data = get_node_data(mrb, root_val);
    if (!root_data) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Invalid node");
        return mrb_nil_value();
    }

    NodeManager::instance().update_world_transforms(root_data->handle);
    return mrb_nil_value();
}

// ============================================================================
// Registration
// ============================================================================

void register_node(mrb_state* mrb) {
    // Node class under GMR::Core
    RClass* gmr = get_gmr_module(mrb);
    RClass* core = mrb_module_get_under(mrb, gmr, "Core");
    RClass* node_class = mrb_define_class_under(mrb, core, "Node", mrb->object_class);
    MRB_SET_INSTANCE_TT(node_class, MRB_TT_CDATA);
    node_class_ptr = node_class;

    // Constructor
    mrb_define_method(mrb, node_class, "initialize", mrb_node_initialize, MRB_ARGS_NONE());

    // Hierarchy
    mrb_define_method(mrb, node_class, "add_child", mrb_node_add_child, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, node_class, "remove_child", mrb_node_remove_child, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, node_class, "parent", mrb_node_parent, MRB_ARGS_NONE());
    mrb_define_method(mrb, node_class, "children", mrb_node_children, MRB_ARGS_NONE());
    mrb_define_method(mrb, node_class, "child_count", mrb_node_child_count, MRB_ARGS_NONE());

    // Local transform
    mrb_define_method(mrb, node_class, "local_position", mrb_node_local_position, MRB_ARGS_NONE());
    mrb_define_method(mrb, node_class, "local_position=", mrb_node_set_local_position, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, node_class, "local_rotation", mrb_node_local_rotation, MRB_ARGS_NONE());
    mrb_define_method(mrb, node_class, "local_rotation=", mrb_node_set_local_rotation, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, node_class, "local_scale", mrb_node_local_scale, MRB_ARGS_NONE());
    mrb_define_method(mrb, node_class, "local_scale=", mrb_node_set_local_scale, MRB_ARGS_REQ(1));

    // World transform (read-only)
    mrb_define_method(mrb, node_class, "world_position", mrb_node_world_position, MRB_ARGS_NONE());
    mrb_define_method(mrb, node_class, "world_rotation", mrb_node_world_rotation, MRB_ARGS_NONE());
    mrb_define_method(mrb, node_class, "world_scale", mrb_node_world_scale, MRB_ARGS_NONE());

    // Active state
    mrb_define_method(mrb, node_class, "active", mrb_node_active, MRB_ARGS_NONE());
    mrb_define_method(mrb, node_class, "active=", mrb_node_set_active, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, node_class, "active?", mrb_node_is_active, MRB_ARGS_NONE());

    // Class method for updating transforms
    mrb_define_class_method(mrb, node_class, "update_world_transforms",
                            mrb_node_class_update_world_transforms, MRB_ARGS_REQ(1));
}

} // namespace bindings
} // namespace gmr
