#include "gmr/bindings/ui.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/bindings/graphics.hpp"
#include "gmr/ui/ui_manager.hpp"
#include "gmr/scripting/helpers.hpp"
#include "gmr/state.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/variable.h>
#include <mruby/proc.h>
#include <mruby/string.h>
#include <mruby/array.h>

namespace gmr {
namespace bindings {

// ============================================================================
// Static Class Pointers
// ============================================================================

static RClass* ui_module = nullptr;
static RClass* ui_node_class = nullptr;
static RClass* ui_panel_class = nullptr;
static RClass* ui_label_class = nullptr;
static RClass* ui_button_class = nullptr;
static RClass* ui_builder_class = nullptr;
static RClass* ui_styles_module = nullptr;

// ============================================================================
// UI::Node Data Types
// ============================================================================

struct UINodeData {
    ui::UINodeHandle handle;
};

static void ui_node_free(mrb_state* mrb, void* ptr) {
    UINodeData* data = static_cast<UINodeData*>(ptr);
    if (data) {
        // Destroy the C++ node
        ui::UIManager::instance().destroy(mrb, data->handle);
        mrb_free(mrb, data);
    }
}

static const mrb_data_type ui_node_data_type = {
    "UI::Node", ui_node_free
};

static UINodeData* get_node_data(mrb_state* mrb, mrb_value self) {
    return static_cast<UINodeData*>(DATA_PTR(self));
}

// ============================================================================
// Builder Data Type
// ============================================================================

struct UIBuilderData {
    ui::UINodeHandle root_handle;
    ui::UINodeHandle current_parent;
    mrb_value root_ruby;  // Ruby object for root node
};

static void ui_builder_free(mrb_state* mrb, void* ptr) {
    UIBuilderData* data = static_cast<UIBuilderData*>(ptr);
    if (data) {
        mrb_free(mrb, data);
    }
}

static const mrb_data_type ui_builder_data_type = {
    "UI::Builder", ui_builder_free
};

static UIBuilderData* get_builder_data(mrb_state* mrb, mrb_value self) {
    return static_cast<UIBuilderData*>(DATA_PTR(self));
}

// ============================================================================
// Helper Functions
// ============================================================================

// Create a Ruby UI node object with C++ backing
static mrb_value create_node_object(mrb_state* mrb, RClass* klass, ui::UINodeHandle handle) {
    UINodeData* data = static_cast<UINodeData*>(mrb_malloc(mrb, sizeof(UINodeData)));
    data->handle = handle;

    mrb_value obj = mrb_obj_value(mrb_data_object_alloc(mrb, klass, data, &ui_node_data_type));

    // Store Ruby object reference in C++ node for callback invocation
    ui::UINodeState* state = ui::UIManager::instance().get(handle);
    if (state) {
        state->ruby_self = obj;
        mrb_gc_register(mrb, obj);
    }

    return obj;
}

// Parse common node options from kwargs
static void parse_node_options(mrb_state* mrb, mrb_value opts, ui::UINodeState* state) {
    if (mrb_nil_p(opts) || !mrb_hash_p(opts)) return;

    // Position
    mrb_value x_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "x")));
    if (!mrb_nil_p(x_val)) state->x = static_cast<float>(mrb_as_float(mrb, x_val));

    mrb_value y_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "y")));
    if (!mrb_nil_p(y_val)) state->y = static_cast<float>(mrb_as_float(mrb, y_val));

    // Size
    mrb_value w_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "width")));
    if (!mrb_nil_p(w_val)) state->width = static_cast<float>(mrb_as_float(mrb, w_val));

    mrb_value h_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "height")));
    if (!mrb_nil_p(h_val)) state->height = static_cast<float>(mrb_as_float(mrb, h_val));

    // Layout
    mrb_value layout_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "layout")));
    if (!mrb_nil_p(layout_val) && mrb_symbol_p(layout_val)) {
        mrb_sym layout_sym = mrb_symbol(layout_val);
        if (layout_sym == mrb_intern_lit(mrb, "vertical")) {
            state->layout = ui::LayoutMode::VERTICAL;
        } else if (layout_sym == mrb_intern_lit(mrb, "horizontal")) {
            state->layout = ui::LayoutMode::HORIZONTAL;
        } else {
            state->layout = ui::LayoutMode::NONE;
        }
    }

    // Padding
    mrb_value padding_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "padding")));
    if (!mrb_nil_p(padding_val)) {
        float p = static_cast<float>(mrb_as_float(mrb, padding_val));
        state->padding = ui::UISpacing(p);
    }

    // Spacing
    mrb_value spacing_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "spacing")));
    if (!mrb_nil_p(spacing_val)) {
        state->spacing = static_cast<float>(mrb_as_float(mrb, spacing_val));
    }

    // Anchor
    mrb_value anchor_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "anchor")));
    if (!mrb_nil_p(anchor_val) && mrb_symbol_p(anchor_val)) {
        mrb_sym anchor_sym = mrb_symbol(anchor_val);
        if (anchor_sym == mrb_intern_lit(mrb, "center")) {
            state->anchor = ui::Anchor::CENTER;
        } else if (anchor_sym == mrb_intern_lit(mrb, "top_center")) {
            state->anchor = ui::Anchor::TOP_CENTER;
        } else if (anchor_sym == mrb_intern_lit(mrb, "top_right")) {
            state->anchor = ui::Anchor::TOP_RIGHT;
        } else if (anchor_sym == mrb_intern_lit(mrb, "center_left")) {
            state->anchor = ui::Anchor::CENTER_LEFT;
        } else if (anchor_sym == mrb_intern_lit(mrb, "center_right")) {
            state->anchor = ui::Anchor::CENTER_RIGHT;
        } else if (anchor_sym == mrb_intern_lit(mrb, "bottom_left")) {
            state->anchor = ui::Anchor::BOTTOM_LEFT;
        } else if (anchor_sym == mrb_intern_lit(mrb, "bottom_center")) {
            state->anchor = ui::Anchor::BOTTOM_CENTER;
        } else if (anchor_sym == mrb_intern_lit(mrb, "bottom_right")) {
            state->anchor = ui::Anchor::BOTTOM_RIGHT;
        }
    }

    // Background color
    mrb_value bg_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "background_color")));
    if (!mrb_nil_p(bg_val)) {
        state->background_color = parse_color_value(mrb, bg_val, Color{0, 0, 0, 0});
    }

    // Hover background color
    mrb_value hover_bg_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "hover_background")));
    if (!mrb_nil_p(hover_bg_val)) {
        state->hover_background = parse_color_value(mrb, hover_bg_val, Color{0, 0, 0, 0});
    }

    // Pressed background color
    mrb_value pressed_bg_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "pressed_background")));
    if (!mrb_nil_p(pressed_bg_val)) {
        state->pressed_background = parse_color_value(mrb, pressed_bg_val, Color{0, 0, 0, 0});
    }

    // Text color
    mrb_value text_color_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "text_color")));
    if (!mrb_nil_p(text_color_val)) {
        state->text_color = parse_color_value(mrb, text_color_val, Color{255, 255, 255, 255});
    }

    // Font size
    mrb_value font_size_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "font_size")));
    if (!mrb_nil_p(font_size_val)) {
        state->font_size = static_cast<int>(mrb_as_int(mrb, font_size_val));
    }

    // Font (custom font object)
    mrb_value font_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "font")));
    if (!mrb_nil_p(font_val)) {
        FontData* font_data = get_font_data(mrb, font_val);
        if (font_data) {
            state->font = font_data->handle;
        }
    }

    // Border
    mrb_value border_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "border_width")));
    if (!mrb_nil_p(border_val)) {
        state->border_width = static_cast<float>(mrb_as_float(mrb, border_val));
    }

    mrb_value border_color_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "border_color")));
    if (!mrb_nil_p(border_color_val)) {
        state->border_color = parse_color_value(mrb, border_color_val, Color{0, 0, 0, 0});
    }

    // Corner radius
    mrb_value radius_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "corner_radius")));
    if (!mrb_nil_p(radius_val)) {
        state->corner_radius = static_cast<float>(mrb_as_float(mrb, radius_val));
    }
}

// Apply style from registry
static void apply_style(mrb_state* mrb, mrb_value opts, ui::UINodeState* state) {
    if (mrb_nil_p(opts) || !mrb_hash_p(opts)) return;

    mrb_value style_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "style")));
    if (mrb_nil_p(style_val) || !mrb_symbol_p(style_val)) return;

    // Get style from registry
    mrb_value styles_registry = mrb_iv_get(mrb, mrb_obj_value(ui_styles_module),
        mrb_intern_lit(mrb, "@registry"));
    if (mrb_nil_p(styles_registry) || !mrb_hash_p(styles_registry)) return;

    mrb_value style_hash = mrb_hash_get(mrb, styles_registry, style_val);
    if (!mrb_nil_p(style_hash) && mrb_hash_p(style_hash)) {
        parse_node_options(mrb, style_hash, state);
    }
}

// ============================================================================
// UI::Node Instance Methods
// ============================================================================

static mrb_value mrb_ui_node_x(mrb_state* mrb, mrb_value self) {
    UINodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    if (!state) return mrb_nil_value();

    return mrb_float_value(mrb, state->x);
}

static mrb_value mrb_ui_node_set_x(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);

    UINodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    if (state) {
        state->x = static_cast<float>(val);
        ui::UIManager::instance().mark_layout_dirty();
    }

    return mrb_float_value(mrb, val);
}

static mrb_value mrb_ui_node_y(mrb_state* mrb, mrb_value self) {
    UINodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    if (!state) return mrb_nil_value();

    return mrb_float_value(mrb, state->y);
}

static mrb_value mrb_ui_node_set_y(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);

    UINodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    if (state) {
        state->y = static_cast<float>(val);
        ui::UIManager::instance().mark_layout_dirty();
    }

    return mrb_float_value(mrb, val);
}

static mrb_value mrb_ui_node_width(mrb_state* mrb, mrb_value self) {
    UINodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    if (!state) return mrb_nil_value();

    return mrb_float_value(mrb, state->width);
}

static mrb_value mrb_ui_node_set_width(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);

    UINodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    if (state) {
        state->width = static_cast<float>(val);
        ui::UIManager::instance().mark_layout_dirty();
    }

    return mrb_float_value(mrb, val);
}

static mrb_value mrb_ui_node_height(mrb_state* mrb, mrb_value self) {
    UINodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    if (!state) return mrb_nil_value();

    return mrb_float_value(mrb, state->height);
}

static mrb_value mrb_ui_node_set_height(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);

    UINodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    if (state) {
        state->height = static_cast<float>(val);
        ui::UIManager::instance().mark_layout_dirty();
    }

    return mrb_float_value(mrb, val);
}

static mrb_value mrb_ui_node_visible(mrb_state* mrb, mrb_value self) {
    UINodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_false_value();

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    return state ? mrb_bool_value(state->visible) : mrb_false_value();
}

static mrb_value mrb_ui_node_set_visible(mrb_state* mrb, mrb_value self) {
    mrb_bool val;
    mrb_get_args(mrb, "b", &val);

    UINodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_nil_value();

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    if (state) {
        state->visible = val;
    }

    return mrb_bool_value(val);
}

static mrb_value mrb_ui_node_hovered(mrb_state* mrb, mrb_value self) {
    UINodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_false_value();

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    return state ? mrb_bool_value(state->hovered) : mrb_false_value();
}

static mrb_value mrb_ui_node_pressed(mrb_state* mrb, mrb_value self) {
    UINodeData* data = get_node_data(mrb, self);
    if (!data) return mrb_false_value();

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    return state ? mrb_bool_value(state->pressed) : mrb_false_value();
}

// ============================================================================
// UI::Node Event Callbacks
// ============================================================================

static mrb_value mrb_ui_node_on_click(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    UINodeData* data = get_node_data(mrb, self);
    if (!data) return self;

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    if (!state) return self;

    // Unregister old callback
    size_t idx = static_cast<size_t>(ui::UICallbackType::ON_CLICK);
    if (!mrb_nil_p(state->callbacks[idx])) {
        mrb_gc_unregister(mrb, state->callbacks[idx]);
    }

    // Register new callback
    state->callbacks[idx] = block;
    if (!mrb_nil_p(block)) {
        mrb_gc_register(mrb, block);
    }

    return self;
}

static mrb_value mrb_ui_node_on_hover_enter(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    UINodeData* data = get_node_data(mrb, self);
    if (!data) return self;

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    if (!state) return self;

    size_t idx = static_cast<size_t>(ui::UICallbackType::ON_HOVER_ENTER);
    if (!mrb_nil_p(state->callbacks[idx])) {
        mrb_gc_unregister(mrb, state->callbacks[idx]);
    }

    state->callbacks[idx] = block;
    if (!mrb_nil_p(block)) {
        mrb_gc_register(mrb, block);
    }

    return self;
}

static mrb_value mrb_ui_node_on_hover_exit(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    UINodeData* data = get_node_data(mrb, self);
    if (!data) return self;

    ui::UINodeState* state = ui::UIManager::instance().get(data->handle);
    if (!state) return self;

    size_t idx = static_cast<size_t>(ui::UICallbackType::ON_HOVER_EXIT);
    if (!mrb_nil_p(state->callbacks[idx])) {
        mrb_gc_unregister(mrb, state->callbacks[idx]);
    }

    state->callbacks[idx] = block;
    if (!mrb_nil_p(block)) {
        mrb_gc_register(mrb, block);
    }

    return self;
}

// ============================================================================
// UI::Builder DSL Methods
// ============================================================================

static mrb_value mrb_builder_panel(mrb_state* mrb, mrb_value self) {
    mrb_value opts = mrb_nil_value();
    mrb_value block = mrb_nil_value();
    mrb_get_args(mrb, "|H&", &opts, &block);

    UIBuilderData* builder = get_builder_data(mrb, self);
    if (!builder) return mrb_nil_value();

    // Create panel node
    ui::UINodeHandle handle = ui::UIManager::instance().create(ui::UINodeType::PANEL);
    ui::UINodeState* state = ui::UIManager::instance().get(handle);
    if (!state) return mrb_nil_value();

    // Default panel styling
    state->background_color = Color{40, 40, 50, 255};

    // Apply style first, then options (options override style)
    apply_style(mrb, opts, state);
    parse_node_options(mrb, opts, state);

    // Create Ruby object
    mrb_value panel = create_node_object(mrb, ui_panel_class, handle);

    // Add to current parent
    ui::UIManager::instance().set_parent(handle, builder->current_parent);

    // If block given, execute with this panel as parent
    if (!mrb_nil_p(block)) {
        ui::UINodeHandle old_parent = builder->current_parent;
        builder->current_parent = handle;

        scripting::safe_instance_exec(mrb, self, block);

        builder->current_parent = old_parent;
    }

    return panel;
}

static mrb_value mrb_builder_label(mrb_state* mrb, mrb_value self) {
    const char* text;
    mrb_value opts = mrb_nil_value();
    mrb_get_args(mrb, "z|H", &text, &opts);

    UIBuilderData* builder = get_builder_data(mrb, self);
    if (!builder) return mrb_nil_value();

    // Create label node
    ui::UINodeHandle handle = ui::UIManager::instance().create(ui::UINodeType::LABEL);
    ui::UINodeState* state = ui::UIManager::instance().get(handle);
    if (!state) return mrb_nil_value();

    state->text = text;
    state->text_color = Color{255, 255, 255, 255};
    state->font_size = 16;  // Default font size

    // Apply style then options
    apply_style(mrb, opts, state);
    parse_node_options(mrb, opts, state);

    // Set default height based on font size if not specified
    if (state->height <= 0) {
        state->height = static_cast<float>(state->font_size + 4);  // Font size + small padding
    }

    // Create Ruby object
    mrb_value label = create_node_object(mrb, ui_label_class, handle);

    // Add to current parent
    ui::UIManager::instance().set_parent(handle, builder->current_parent);

    return label;
}

static mrb_value mrb_builder_button(mrb_state* mrb, mrb_value self) {
    const char* text;
    mrb_value opts = mrb_nil_value();
    mrb_value block = mrb_nil_value();
    mrb_get_args(mrb, "z|H&", &text, &opts, &block);

    UIBuilderData* builder = get_builder_data(mrb, self);
    if (!builder) return mrb_nil_value();

    // Create button node
    ui::UINodeHandle handle = ui::UIManager::instance().create(ui::UINodeType::BUTTON);
    ui::UINodeState* state = ui::UIManager::instance().get(handle);
    if (!state) return mrb_nil_value();

    state->text = text;

    // Default button styling
    state->background_color = Color{60, 60, 80, 255};
    state->hover_background = Color{80, 80, 100, 255};
    state->pressed_background = Color{40, 40, 60, 255};
    state->text_color = Color{255, 255, 255, 255};
    state->padding = ui::UISpacing(8, 16, 8, 16);
    state->width = 200;
    state->height = 40;

    // Apply style then options
    apply_style(mrb, opts, state);
    parse_node_options(mrb, opts, state);

    // Create Ruby object
    mrb_value button = create_node_object(mrb, ui_button_class, handle);

    // Add to current parent
    ui::UIManager::instance().set_parent(handle, builder->current_parent);

    // If block given, execute with button as self for on_click etc.
    if (!mrb_nil_p(block)) {
        scripting::safe_instance_exec(mrb, button, block);
    }

    return button;
}

// ============================================================================
// Graphics.ui Entry Point
// ============================================================================

static mrb_value mrb_graphics_ui(mrb_state* mrb, mrb_value self) {
    mrb_value opts = mrb_nil_value();
    mrb_value block = mrb_nil_value();
    mrb_get_args(mrb, "|H&", &opts, &block);

    if (mrb_nil_p(block)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Graphics.ui requires a block");
        return mrb_nil_value();
    }

    // Create root panel
    ui::UINodeHandle root_handle = ui::UIManager::instance().create(ui::UINodeType::PANEL);
    ui::UINodeState* root_state = ui::UIManager::instance().get(root_handle);
    if (!root_state) return mrb_nil_value();

    // Root defaults to full screen (width=0, height=0 means use available space)
    // No explicit size so it automatically adapts to screen size changes
    root_state->width = 0;
    root_state->height = 0;

    // Apply options if provided
    parse_node_options(mrb, opts, root_state);

    // Create Ruby object for root
    mrb_value root = create_node_object(mrb, ui_panel_class, root_handle);

    // Create builder
    UIBuilderData* builder_data = static_cast<UIBuilderData*>(mrb_malloc(mrb, sizeof(UIBuilderData)));
    builder_data->root_handle = root_handle;
    builder_data->current_parent = root_handle;
    builder_data->root_ruby = root;

    mrb_value builder = mrb_obj_value(mrb_data_object_alloc(mrb, ui_builder_class, builder_data, &ui_builder_data_type));

    // Execute DSL block with builder as self
    scripting::safe_instance_exec(mrb, builder, block);

    // Set as active root for rendering
    ui::UIManager::instance().set_root(root_handle);

    return root;
}

// ============================================================================
// UI::Styles Module Methods
// ============================================================================

static mrb_value mrb_styles_register(mrb_state* mrb, mrb_value self) {
    mrb_sym name;
    mrb_value opts = mrb_nil_value();
    mrb_get_args(mrb, "n|H", &name, &opts);

    // Get or create registry
    mrb_value registry = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@registry"));
    if (mrb_nil_p(registry)) {
        registry = mrb_hash_new(mrb);
        mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@registry"), registry);
    }

    // Store style
    mrb_hash_set(mrb, registry, mrb_symbol_value(name), opts);

    return mrb_nil_value();
}

static mrb_value mrb_styles_get(mrb_state* mrb, mrb_value self) {
    mrb_sym name;
    mrb_get_args(mrb, "n", &name);

    mrb_value registry = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@registry"));
    if (mrb_nil_p(registry)) return mrb_nil_value();

    return mrb_hash_get(mrb, registry, mrb_symbol_value(name));
}

// ============================================================================
// UI Module Methods
// ============================================================================

static mrb_value mrb_ui_clear(mrb_state* mrb, mrb_value self) {
    ui::UIManager::instance().clear(mrb);
    return mrb_nil_value();
}

// ============================================================================
// Registration
// ============================================================================

void register_ui(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);

    // GMR::UI module
    ui_module = mrb_define_module_under(mrb, gmr, "UI");

    // GMR::UI::Node base class
    ui_node_class = mrb_define_class_under(mrb, ui_module, "Node", mrb->object_class);
    MRB_SET_INSTANCE_TT(ui_node_class, MRB_TT_CDATA);

    // Node instance methods
    mrb_define_method(mrb, ui_node_class, "x", mrb_ui_node_x, MRB_ARGS_NONE());
    mrb_define_method(mrb, ui_node_class, "x=", mrb_ui_node_set_x, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, ui_node_class, "y", mrb_ui_node_y, MRB_ARGS_NONE());
    mrb_define_method(mrb, ui_node_class, "y=", mrb_ui_node_set_y, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, ui_node_class, "width", mrb_ui_node_width, MRB_ARGS_NONE());
    mrb_define_method(mrb, ui_node_class, "width=", mrb_ui_node_set_width, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, ui_node_class, "height", mrb_ui_node_height, MRB_ARGS_NONE());
    mrb_define_method(mrb, ui_node_class, "height=", mrb_ui_node_set_height, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, ui_node_class, "visible", mrb_ui_node_visible, MRB_ARGS_NONE());
    mrb_define_method(mrb, ui_node_class, "visible=", mrb_ui_node_set_visible, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, ui_node_class, "hovered?", mrb_ui_node_hovered, MRB_ARGS_NONE());
    mrb_define_method(mrb, ui_node_class, "pressed?", mrb_ui_node_pressed, MRB_ARGS_NONE());

    // Event callbacks
    mrb_define_method(mrb, ui_node_class, "on_click", mrb_ui_node_on_click, MRB_ARGS_BLOCK());
    mrb_define_method(mrb, ui_node_class, "on_hover_enter", mrb_ui_node_on_hover_enter, MRB_ARGS_BLOCK());
    mrb_define_method(mrb, ui_node_class, "on_hover_exit", mrb_ui_node_on_hover_exit, MRB_ARGS_BLOCK());

    // GMR::UI::Panel (inherits from Node)
    ui_panel_class = mrb_define_class_under(mrb, ui_module, "Panel", ui_node_class);

    // GMR::UI::Label (inherits from Node)
    ui_label_class = mrb_define_class_under(mrb, ui_module, "Label", ui_node_class);

    // GMR::UI::Button (inherits from Node)
    ui_button_class = mrb_define_class_under(mrb, ui_module, "Button", ui_node_class);

    // GMR::UI::Builder (internal, for DSL)
    ui_builder_class = mrb_define_class_under(mrb, ui_module, "Builder", mrb->object_class);
    MRB_SET_INSTANCE_TT(ui_builder_class, MRB_TT_CDATA);

    mrb_define_method(mrb, ui_builder_class, "panel", mrb_builder_panel, MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
    mrb_define_method(mrb, ui_builder_class, "label", mrb_builder_label, MRB_ARGS_ARG(1, 1));
    mrb_define_method(mrb, ui_builder_class, "button", mrb_builder_button, MRB_ARGS_ARG(1, 1) | MRB_ARGS_BLOCK());

    // GMR::UI::Styles module
    ui_styles_module = mrb_define_module_under(mrb, ui_module, "Styles");
    mrb_define_module_function(mrb, ui_styles_module, "register", mrb_styles_register, MRB_ARGS_ARG(1, 1));
    mrb_define_module_function(mrb, ui_styles_module, "get", mrb_styles_get, MRB_ARGS_REQ(1));

    // Initialize style registry
    mrb_value registry = mrb_hash_new(mrb);
    mrb_iv_set(mrb, mrb_obj_value(ui_styles_module), mrb_intern_lit(mrb, "@registry"), registry);

    // Register default styles
    mrb_value primary_style = mrb_hash_new(mrb);
    mrb_value primary_bg = mrb_ary_new_capa(mrb, 3);
    mrb_ary_push(mrb, primary_bg, mrb_fixnum_value(60));
    mrb_ary_push(mrb, primary_bg, mrb_fixnum_value(120));
    mrb_ary_push(mrb, primary_bg, mrb_fixnum_value(200));
    mrb_hash_set(mrb, primary_style, mrb_symbol_value(mrb_intern_lit(mrb, "background_color")), primary_bg);

    mrb_value primary_hover = mrb_ary_new_capa(mrb, 3);
    mrb_ary_push(mrb, primary_hover, mrb_fixnum_value(80));
    mrb_ary_push(mrb, primary_hover, mrb_fixnum_value(140));
    mrb_ary_push(mrb, primary_hover, mrb_fixnum_value(220));
    mrb_hash_set(mrb, primary_style, mrb_symbol_value(mrb_intern_lit(mrb, "hover_background")), primary_hover);

    mrb_hash_set(mrb, registry, mrb_symbol_value(mrb_intern_lit(mrb, "primary")), primary_style);

    mrb_value danger_style = mrb_hash_new(mrb);
    mrb_value danger_bg = mrb_ary_new_capa(mrb, 3);
    mrb_ary_push(mrb, danger_bg, mrb_fixnum_value(200));
    mrb_ary_push(mrb, danger_bg, mrb_fixnum_value(60));
    mrb_ary_push(mrb, danger_bg, mrb_fixnum_value(60));
    mrb_hash_set(mrb, danger_style, mrb_symbol_value(mrb_intern_lit(mrb, "background_color")), danger_bg);

    mrb_value danger_hover = mrb_ary_new_capa(mrb, 3);
    mrb_ary_push(mrb, danger_hover, mrb_fixnum_value(220));
    mrb_ary_push(mrb, danger_hover, mrb_fixnum_value(80));
    mrb_ary_push(mrb, danger_hover, mrb_fixnum_value(80));
    mrb_hash_set(mrb, danger_style, mrb_symbol_value(mrb_intern_lit(mrb, "hover_background")), danger_hover);

    mrb_hash_set(mrb, registry, mrb_symbol_value(mrb_intern_lit(mrb, "danger")), danger_style);

    // UI module methods
    mrb_define_module_function(mrb, ui_module, "clear", mrb_ui_clear, MRB_ARGS_NONE());

    // Add Graphics.ui method
    RClass* graphics = get_gmr_submodule(mrb, "Graphics");
    mrb_define_module_function(mrb, graphics, "ui", mrb_graphics_ui, MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
}

} // namespace bindings
} // namespace gmr
