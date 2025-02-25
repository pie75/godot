/*************************************************************************/
/*  color_picker.cpp                                                     */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "color_picker.h"

#include "core/input/input.h"
#include "core/math/color.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "scene/gui/color_mode.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_settings.h"
#endif

#include "thirdparty/misc/ok_color.h"
#include "thirdparty/misc/ok_color_shader.h"

List<Color> ColorPicker::preset_cache;
List<Color> ColorPicker::recent_preset_cache;

void ColorPicker::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_update_color();
#ifdef TOOLS_ENABLED
			if (Engine::get_singleton()->is_editor_hint()) {
				if (preset_cache.is_empty()) {
					PackedColorArray saved_presets = EditorSettings::get_singleton()->get_project_metadata("color_picker", "presets", PackedColorArray());
					for (int i = 0; i < saved_presets.size(); i++) {
						preset_cache.push_back(saved_presets[i]);
					}
				}

				for (int i = 0; i < preset_cache.size(); i++) {
					presets.push_back(preset_cache[i]);
				}

				if (recent_preset_cache.is_empty()) {
					PackedColorArray saved_recent_presets = EditorSettings::get_singleton()->get_project_metadata("color_picker", "recent_presets", PackedColorArray());
					for (int i = 0; i < saved_recent_presets.size(); i++) {
						recent_preset_cache.push_back(saved_recent_presets[i]);
					}
				}

				for (int i = 0; i < recent_preset_cache.size(); i++) {
					recent_presets.push_back(recent_preset_cache[i]);
				}
			}
#endif
			[[fallthrough]];
		}
		case NOTIFICATION_THEME_CHANGED: {
			btn_pick->set_icon(get_theme_icon(SNAME("screen_picker"), SNAME("ColorPicker")));
			_update_drop_down_arrow(btn_preset->is_pressed(), btn_preset);
			_update_drop_down_arrow(btn_recent_preset->is_pressed(), btn_recent_preset);
			btn_add_preset->set_icon(get_theme_icon(SNAME("add_preset")));

			btn_pick->set_custom_minimum_size(Size2(28 * get_theme_default_base_scale(), 0));
			btn_shape->set_custom_minimum_size(Size2(28 * get_theme_default_base_scale(), 0));
			btn_mode->set_custom_minimum_size(Size2(28 * get_theme_default_base_scale(), 0));

			uv_edit->set_custom_minimum_size(Size2(get_theme_constant(SNAME("sv_width")), get_theme_constant(SNAME("sv_height"))));
			w_edit->set_custom_minimum_size(Size2(get_theme_constant(SNAME("h_width")), 0));

			wheel_edit->set_custom_minimum_size(Size2(get_theme_constant(SNAME("sv_width")), get_theme_constant(SNAME("sv_height"))));
			wheel_margin->add_theme_constant_override("margin_bottom", 8 * get_theme_default_base_scale());

			for (int i = 0; i < SLIDER_COUNT; i++) {
				labels[i]->set_custom_minimum_size(Size2(get_theme_constant(SNAME("label_width")), 0));
				set_offset((Side)i, get_offset((Side)i) + get_theme_constant(SNAME("margin")));
			}
			alpha_label->set_custom_minimum_size(Size2(get_theme_constant(SNAME("label_width")), 0));
			set_offset((Side)0, get_offset((Side)0) + get_theme_constant(SNAME("margin")));

			_reset_theme();

			if (Engine::get_singleton()->is_editor_hint()) {
				// Adjust for the width of the "Script" icon.
				text_type->set_custom_minimum_size(Size2(28 * get_theme_default_base_scale(), 0));
			}

			_update_presets();
			_update_recent_presets();
			_update_controls();
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			Popup *p = Object::cast_to<Popup>(get_parent());
			if (p && is_visible_in_tree()) {
				p->set_size(Size2(get_combined_minimum_size().width + get_theme_constant(SNAME("margin")) * 2, get_combined_minimum_size().height + get_theme_constant(SNAME("margin")) * 2));
			}
		} break;

		case NOTIFICATION_WM_CLOSE_REQUEST: {
			if (screen != nullptr && screen->is_visible()) {
				screen->hide();
			}
		} break;
	}
}

Ref<Shader> ColorPicker::wheel_shader;
Ref<Shader> ColorPicker::circle_shader;
Ref<Shader> ColorPicker::circle_ok_color_shader;

void ColorPicker::init_shaders() {
	wheel_shader.instantiate();
	wheel_shader->set_code(R"(
// ColorPicker wheel shader.

shader_type canvas_item;

void fragment() {
	float x = UV.x - 0.5;
	float y = UV.y - 0.5;
	float a = atan(y, x);
	x += 0.001;
	y += 0.001;
	float b = float(sqrt(x * x + y * y) < 0.5) * float(sqrt(x * x + y * y) > 0.42);
	x -= 0.002;
	float b2 = float(sqrt(x * x + y * y) < 0.5) * float(sqrt(x * x + y * y) > 0.42);
	y -= 0.002;
	float b3 = float(sqrt(x * x + y * y) < 0.5) * float(sqrt(x * x + y * y) > 0.42);
	x += 0.002;
	float b4 = float(sqrt(x * x + y * y) < 0.5) * float(sqrt(x * x + y * y) > 0.42);

	COLOR = vec4(clamp((abs(fract(((a - TAU) / TAU) + vec3(3.0, 2.0, 1.0) / 3.0) * 6.0 - 3.0) - 1.0), 0.0, 1.0), (b + b2 + b3 + b4) / 4.00);
}
)");

	circle_shader.instantiate();
	circle_shader->set_code(R"(
// ColorPicker circle shader.

shader_type canvas_item;

uniform float v = 1.0;

void fragment() {
	float x = UV.x - 0.5;
	float y = UV.y - 0.5;
	float a = atan(y, x);
	x += 0.001;
	y += 0.001;
	float b = float(sqrt(x * x + y * y) < 0.5);
	x -= 0.002;
	float b2 = float(sqrt(x * x + y * y) < 0.5);
	y -= 0.002;
	float b3 = float(sqrt(x * x + y * y) < 0.5);
	x += 0.002;
	float b4 = float(sqrt(x * x + y * y) < 0.5);

	COLOR = vec4(mix(vec3(1.0), clamp(abs(fract(vec3((a - TAU) / TAU) + vec3(1.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - vec3(3.0)) - vec3(1.0), 0.0, 1.0), ((float(sqrt(x * x + y * y)) * 2.0)) / 1.0) * vec3(v), (b + b2 + b3 + b4) / 4.00);
})");

	circle_ok_color_shader.instantiate();
	circle_ok_color_shader->set_code(OK_COLOR_SHADER + R"(
// ColorPicker ok color hsv circle shader.

uniform float v = 1.0;

void fragment() {
	float x = UV.x - 0.5;
	float y = UV.y - 0.5;
	float h = atan(y, x) / (2.0 * M_PI);
	float s = sqrt(x * x + y * y) * 2.0;
	vec3 col = okhsl_to_srgb(vec3(h, s, v));
	x += 0.001;
	y += 0.001;
	float b = float(sqrt(x * x + y * y) < 0.5);
	x -= 0.002;
	float b2 = float(sqrt(x * x + y * y) < 0.5);
	y -= 0.002;
	float b3 = float(sqrt(x * x + y * y) < 0.5);
	x += 0.002;
	float b4 = float(sqrt(x * x + y * y) < 0.5);
	COLOR = vec4(col, (b + b2 + b3 + b4) / 4.00);
})");
}

void ColorPicker::finish_shaders() {
	wheel_shader.unref();
	circle_shader.unref();
	circle_ok_color_shader.unref();
}

void ColorPicker::set_focus_on_line_edit() {
	c_text->call_deferred(SNAME("grab_focus"));
}

void ColorPicker::_update_controls() {
	int mode_sliders_count = modes[current_mode]->get_slider_count();

	for (int i = current_slider_count; i < mode_sliders_count; i++) {
		sliders[i]->show();
		labels[i]->show();
		values[i]->show();
	}
	for (int i = mode_sliders_count; i < current_slider_count; i++) {
		sliders[i]->hide();
		labels[i]->hide();
		values[i]->hide();
	}
	current_slider_count = mode_sliders_count;

	for (int i = 0; i < current_slider_count; i++) {
		labels[i]->set_text(modes[current_mode]->get_slider_label(i));
	}
	alpha_label->set_text("A");

	slider_theme_modified = modes[current_mode]->apply_theme();

	if (edit_alpha) {
		alpha_value->show();
		alpha_slider->show();
		alpha_label->show();
	} else {
		alpha_value->hide();
		alpha_slider->hide();
		alpha_label->hide();
	}

	switch (_get_actual_shape()) {
		case SHAPE_HSV_RECTANGLE:
			wheel_edit->hide();
			w_edit->show();
			uv_edit->show();
			break;
		case SHAPE_HSV_WHEEL:
			wheel_edit->show();
			w_edit->hide();
			uv_edit->hide();

			wheel->set_material(wheel_mat);
			break;
		case SHAPE_VHS_CIRCLE:
			wheel_edit->show();
			w_edit->show();
			uv_edit->hide();
			wheel->set_material(circle_mat);
			circle_mat->set_shader(circle_shader);
			break;
		case SHAPE_OKHSL_CIRCLE:
			wheel_edit->show();
			w_edit->show();
			uv_edit->hide();
			wheel->set_material(circle_mat);
			circle_mat->set_shader(circle_ok_color_shader);
			break;
		default: {
		}
	}
}

void ColorPicker::_set_pick_color(const Color &p_color, bool p_update_sliders) {
	if (text_changed) {
		add_recent_preset(color);
		text_changed = false;
	}

	color = p_color;
	if (color != last_color) {
		_copy_color_to_hsv();
		last_color = color;
	}

	if (!is_inside_tree()) {
		return;
	}

	_update_color(p_update_sliders);
}

void ColorPicker::set_pick_color(const Color &p_color) {
	_set_pick_color(p_color, true); //because setters can't have more arguments
}

void ColorPicker::set_old_color(const Color &p_color) {
	old_color = p_color;
}

void ColorPicker::set_display_old_color(bool p_enabled) {
	display_old_color = p_enabled;
}

bool ColorPicker::is_displaying_old_color() const {
	return display_old_color;
}

void ColorPicker::set_edit_alpha(bool p_show) {
	if (edit_alpha == p_show) {
		return;
	}
	edit_alpha = p_show;
	_update_controls();

	if (!is_inside_tree()) {
		return;
	}

	_update_color();
	sample->queue_redraw();
}

bool ColorPicker::is_editing_alpha() const {
	return edit_alpha;
}

void ColorPicker::_value_changed(double) {
	if (updating) {
		return;
	}

	color = modes[current_mode]->get_color();

	if (current_mode == MODE_HSV || current_mode == MODE_OKHSL) {
		h = sliders[0]->get_value() / 360.0;
		s = sliders[1]->get_value() / 100.0;
		v = sliders[2]->get_value() / 100.0;
		last_color = color;
	}

	_set_pick_color(color, false);
	emit_signal(SNAME("color_changed"), color);
}

void ColorPicker::add_mode(ColorMode *p_mode) {
	modes.push_back(p_mode);
}

void ColorPicker::create_slider(GridContainer *gc, int idx) {
	Label *lbl = memnew(Label());
	lbl->set_v_size_flags(SIZE_SHRINK_CENTER);
	gc->add_child(lbl);

	HSlider *slider = memnew(HSlider);
	slider->set_v_size_flags(SIZE_SHRINK_CENTER);
	slider->set_focus_mode(FOCUS_NONE);
	gc->add_child(slider);

	SpinBox *val = memnew(SpinBox);
	slider->share(val);
	val->set_select_all_on_focus(true);
	gc->add_child(val);

	LineEdit *vle = val->get_line_edit();
	vle->connect("text_changed", callable_mp(this, &ColorPicker::_text_changed));
	vle->connect("gui_input", callable_mp(this, &ColorPicker::_line_edit_input));
	vle->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);

	val->connect("gui_input", callable_mp(this, &ColorPicker::_slider_or_spin_input));

	slider->set_h_size_flags(SIZE_EXPAND_FILL);

	slider->connect("value_changed", callable_mp(this, &ColorPicker::_value_changed));
	slider->connect("draw", callable_mp(this, &ColorPicker::_slider_draw).bind(idx));
	slider->connect("gui_input", callable_mp(this, &ColorPicker::_slider_or_spin_input));

	if (idx < SLIDER_COUNT) {
		sliders[idx] = slider;
		values[idx] = val;
		labels[idx] = lbl;
	} else {
		alpha_slider = slider;
		alpha_value = val;
		alpha_label = lbl;
	}
}

HSlider *ColorPicker::get_slider(int p_idx) {
	if (p_idx < SLIDER_COUNT) {
		return sliders[p_idx];
	}
	return alpha_slider;
}

Vector<float> ColorPicker::get_active_slider_values() {
	Vector<float> cur_values;
	for (int i = 0; i < current_slider_count; i++) {
		cur_values.push_back(sliders[i]->get_value());
	}
	cur_values.push_back(alpha_slider->get_value());
	return cur_values;
}

void ColorPicker::_copy_color_to_hsv() {
	if (_get_actual_shape() == SHAPE_OKHSL_CIRCLE) {
		h = color.get_ok_hsl_h();
		s = color.get_ok_hsl_s();
		v = color.get_ok_hsl_l();
	} else {
		h = color.get_h();
		s = color.get_s();
		v = color.get_v();
	}
}

void ColorPicker::_copy_hsv_to_color() {
	if (_get_actual_shape() == SHAPE_OKHSL_CIRCLE) {
		color.set_ok_hsl(h, s, v, color.a);
	} else {
		color.set_hsv(h, s, v, color.a);
	}
}

void ColorPicker::_select_from_preset_container(const Color &p_color) {
	if (preset_group->get_pressed_button()) {
		preset_group->get_pressed_button()->set_pressed(false);
	}

	for (int i = 1; i < preset_container->get_child_count(); i++) {
		ColorPresetButton *current_btn = Object::cast_to<ColorPresetButton>(preset_container->get_child(i));
		if (current_btn && p_color == current_btn->get_preset_color()) {
			current_btn->set_pressed(true);
			break;
		}
	}
}

bool ColorPicker::_select_from_recent_preset_hbc(const Color &p_color) {
	for (int i = 0; i < recent_preset_hbc->get_child_count(); i++) {
		ColorPresetButton *current_btn = Object::cast_to<ColorPresetButton>(recent_preset_hbc->get_child(i));
		if (current_btn && p_color == current_btn->get_preset_color()) {
			current_btn->set_pressed(true);
			return true;
		}
	}
	return false;
}

ColorPicker::PickerShapeType ColorPicker::_get_actual_shape() const {
	return modes[current_mode]->get_shape_override() != SHAPE_MAX ? modes[current_mode]->get_shape_override() : current_shape;
}

void ColorPicker::_reset_theme() {
	Ref<StyleBoxFlat> style_box_flat(memnew(StyleBoxFlat));
	style_box_flat->set_default_margin(SIDE_TOP, 16 * get_theme_default_base_scale());
	style_box_flat->set_bg_color(Color(0.2, 0.23, 0.31).lerp(Color(0, 0, 0, 1), 0.3).clamp());
	for (int i = 0; i < SLIDER_COUNT; i++) {
		sliders[i]->add_theme_icon_override("grabber", get_theme_icon(SNAME("bar_arrow"), SNAME("ColorPicker")));
		sliders[i]->add_theme_icon_override("grabber_highlight", get_theme_icon(SNAME("bar_arrow"), SNAME("ColorPicker")));
		sliders[i]->add_theme_constant_override("grabber_offset", 8 * get_theme_default_base_scale());
		if (!colorize_sliders) {
			sliders[i]->add_theme_style_override("slider", style_box_flat);
		}
	}
	alpha_slider->add_theme_icon_override("grabber", get_theme_icon(SNAME("bar_arrow"), SNAME("ColorPicker")));
	alpha_slider->add_theme_icon_override("grabber_highlight", get_theme_icon(SNAME("bar_arrow"), SNAME("ColorPicker")));
	alpha_slider->add_theme_constant_override("grabber_offset", 8 * get_theme_default_base_scale());
	if (!colorize_sliders) {
		alpha_slider->add_theme_style_override("slider", style_box_flat);
	}
}

void ColorPicker::_html_submitted(const String &p_html) {
	if (updating || text_is_constructor || !c_text->is_visible()) {
		return;
	}

	Color previous_color = color;
	color = Color::html(p_html);
	if (!is_editing_alpha()) {
		color.a = previous_color.a;
	}

	if (color == previous_color) {
		return;
	}
	if (!is_inside_tree()) {
		return;
	}

	set_pick_color(color);
	emit_signal(SNAME("color_changed"), color);
}

void ColorPicker::_update_color(bool p_update_sliders) {
	updating = true;

	if (p_update_sliders) {
		float step = modes[current_mode]->get_slider_step();
		for (int i = 0; i < current_slider_count; i++) {
			sliders[i]->set_max(modes[current_mode]->get_slider_max(i));
			sliders[i]->set_step(step);
			sliders[i]->set_value(modes[current_mode]->get_slider_value(i));
		}
		alpha_slider->set_max(modes[current_mode]->get_slider_max(current_slider_count));
		alpha_slider->set_step(step);
		alpha_slider->set_value(modes[current_mode]->get_slider_value(current_slider_count));
	}

	_update_text_value();

	sample->queue_redraw();
	uv_edit->queue_redraw();
	w_edit->queue_redraw();
	for (int i = 0; i < current_slider_count; i++) {
		sliders[i]->queue_redraw();
	}
	alpha_slider->queue_redraw();
	wheel->queue_redraw();
	wheel_uv->queue_redraw();
	updating = false;
}

void ColorPicker::_update_presets() {
	int preset_size = _get_preset_size();
	// Only update the preset button size if it has changed.
	if (preset_size != prev_preset_size) {
		prev_preset_size = preset_size;
		btn_add_preset->set_custom_minimum_size(Size2(preset_size, preset_size));
		for (int i = 1; i < preset_container->get_child_count(); i++) {
			ColorPresetButton *cpb = Object::cast_to<ColorPresetButton>(preset_container->get_child(i));
			cpb->set_custom_minimum_size(Size2(preset_size, preset_size));
		}
	}

#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		// Only load preset buttons when the only child is the add-preset button.
		if (preset_container->get_child_count() == 1) {
			for (int i = 0; i < preset_cache.size(); i++) {
				_add_preset_button(preset_size, preset_cache[i]);
			}
			_notification(NOTIFICATION_VISIBILITY_CHANGED);
		}
	}
#endif
}

void ColorPicker::_update_recent_presets() {
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		int recent_preset_count = recent_preset_hbc->get_child_count();
		for (int i = 0; i < recent_preset_count; i++) {
			memdelete(recent_preset_hbc->get_child(0));
		}

		recent_presets.clear();
		for (int i = 0; i < recent_preset_cache.size(); i++) {
			recent_presets.push_back(recent_preset_cache[i]);
		}

		int preset_size = _get_preset_size();
		for (int i = 0; i < recent_presets.size(); i++) {
			_add_recent_preset_button(preset_size, recent_presets[i]);
		}

		_notification(NOTIFICATION_VISIBILITY_CHANGED);
	}
#endif
}

void ColorPicker::_text_type_toggled() {
	text_is_constructor = !text_is_constructor;
	if (text_is_constructor) {
		text_type->set_text("");
		text_type->set_icon(get_theme_icon(SNAME("Script"), SNAME("EditorIcons")));

		c_text->set_editable(false);
		c_text->set_h_size_flags(SIZE_EXPAND_FILL);
	} else {
		text_type->set_text("#");
		text_type->set_icon(nullptr);

		c_text->set_editable(true);
		c_text->set_h_size_flags(SIZE_FILL);
	}
	_update_color();
}

Color ColorPicker::get_pick_color() const {
	return color;
}

void ColorPicker::set_picker_shape(PickerShapeType p_shape) {
	ERR_FAIL_INDEX(p_shape, SHAPE_MAX);
	if (p_shape == current_shape) {
		return;
	}
	shape_popup->set_item_checked(current_shape, false);
	shape_popup->set_item_checked(p_shape, true);

	btn_shape->set_icon(shape_popup->get_item_icon(p_shape));

	current_shape = p_shape;

	_copy_color_to_hsv();

	_update_controls();
	_update_color();
}

ColorPicker::PickerShapeType ColorPicker::get_picker_shape() const {
	return current_shape;
}

inline int ColorPicker::_get_preset_size() {
	return (int(get_minimum_size().width) - (preset_container->get_theme_constant(SNAME("h_separation")) * (PRESET_COLUMN_COUNT - 1))) / PRESET_COLUMN_COUNT;
}

void ColorPicker::_add_preset_button(int p_size, const Color &p_color) {
	ColorPresetButton *btn_preset_new = memnew(ColorPresetButton(p_color, p_size));
	btn_preset_new->set_tooltip_text(vformat(RTR("Color: #%s\nLMB: Apply color\nRMB: Remove preset"), p_color.to_html(p_color.a < 1)));
	btn_preset_new->set_drag_forwarding(this);
	btn_preset_new->set_button_group(preset_group);
	preset_container->add_child(btn_preset_new);
	btn_preset_new->set_pressed(true);
	btn_preset_new->connect("gui_input", callable_mp(this, &ColorPicker::_preset_input).bind(p_color));
}

void ColorPicker::_add_recent_preset_button(int p_size, const Color &p_color) {
	ColorPresetButton *btn_preset_new = memnew(ColorPresetButton(p_color, p_size));
	btn_preset_new->set_tooltip_text(vformat(RTR("Color: #%s\nLMB: Apply color"), p_color.to_html(p_color.a < 1)));
	btn_preset_new->set_button_group(recent_preset_group);
	recent_preset_hbc->add_child(btn_preset_new);
	recent_preset_hbc->move_child(btn_preset_new, 0);
	btn_preset_new->set_pressed(true);
	btn_preset_new->connect("toggled", callable_mp(this, &ColorPicker::_recent_preset_pressed).bind(btn_preset_new));
}

void ColorPicker::_show_hide_preset(const bool &p_is_btn_pressed, Button *p_btn_preset, Container *p_preset_container) {
	if (p_is_btn_pressed) {
		p_preset_container->show();
	} else {
		p_preset_container->hide();
	}
	_update_drop_down_arrow(p_is_btn_pressed, p_btn_preset);
}

void ColorPicker::_update_drop_down_arrow(const bool &p_is_btn_pressed, Button *p_btn_preset) {
	if (p_is_btn_pressed) {
		p_btn_preset->set_icon(get_theme_icon(SNAME("expanded_arrow"), SNAME("ColorPicker")));
	} else {
		p_btn_preset->set_icon(get_theme_icon(SNAME("folded_arrow"), SNAME("ColorPicker")));
	}
}

void ColorPicker::_set_mode_popup_value(ColorModeType p_mode) {
	ERR_FAIL_INDEX(p_mode, MODE_MAX + 1);

	if (p_mode == MODE_MAX) {
		set_colorize_sliders(!colorize_sliders);
	} else {
		set_color_mode(p_mode);
	}
}

Variant ColorPicker::_get_drag_data_fw(const Point2 &p_point, Control *p_from_control) {
	ColorPresetButton *dragged_preset_button = Object::cast_to<ColorPresetButton>(p_from_control);

	if (!dragged_preset_button) {
		return Variant();
	}

	ColorPresetButton *drag_preview = memnew(ColorPresetButton(dragged_preset_button->get_preset_color(), _get_preset_size()));
	set_drag_preview(drag_preview);

	Dictionary drag_data;
	drag_data["type"] = "color_preset";
	drag_data["color_preset"] = dragged_preset_button->get_index();

	return drag_data;
}

bool ColorPicker::_can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from_control) const {
	Dictionary d = p_data;
	if (!d.has("type") || String(d["type"]) != "color_preset") {
		return false;
	}
	return true;
}

void ColorPicker::_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from_control) {
	Dictionary d = p_data;
	if (!d.has("type")) {
		return;
	}

	if (String(d["type"]) == "color_preset") {
		int preset_from_id = d["color_preset"];
		int hover_now = p_from_control->get_index();

		if (preset_from_id == hover_now || hover_now == -1) {
			return;
		}
		preset_container->move_child(preset_container->get_child(preset_from_id), hover_now);
	}
}

void ColorPicker::add_preset(const Color &p_color) {
	List<Color>::Element *e = presets.find(p_color);
	if (e) {
		presets.move_to_back(e);
		preset_cache.move_to_back(preset_cache.find(p_color));

		preset_container->move_child(preset_group->get_pressed_button(), preset_container->get_child_count() - 1);
	} else {
		presets.push_back(p_color);
		preset_cache.push_back(p_color);

		_add_preset_button(_get_preset_size(), p_color);
	}

#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		PackedColorArray arr_to_save = get_presets();
		EditorSettings::get_singleton()->set_project_metadata("color_picker", "presets", arr_to_save);
	}
#endif
}

void ColorPicker::add_recent_preset(const Color &p_color) {
	if (!_select_from_recent_preset_hbc(p_color)) {
		if (recent_preset_hbc->get_child_count() >= PRESET_COLUMN_COUNT) {
			recent_preset_cache.pop_front();
			recent_presets.pop_front();
			recent_preset_hbc->get_child(PRESET_COLUMN_COUNT - 1)->queue_free();
		}
		recent_presets.push_back(p_color);
		recent_preset_cache.push_back(p_color);
		_add_recent_preset_button(_get_preset_size(), p_color);
	}
	_select_from_preset_container(p_color);

#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		PackedColorArray arr_to_save = get_recent_presets();
		EditorSettings::get_singleton()->set_project_metadata("color_picker", "recent_presets", arr_to_save);
	}
#endif
}

void ColorPicker::erase_preset(const Color &p_color) {
	List<Color>::Element *e = presets.find(p_color);
	if (e) {
		presets.erase(e);
		preset_cache.erase(preset_cache.find(p_color));

		// Find preset button to remove.
		for (int i = 1; i < preset_container->get_child_count(); i++) {
			ColorPresetButton *current_btn = Object::cast_to<ColorPresetButton>(preset_container->get_child(i));
			if (current_btn && p_color == current_btn->get_preset_color()) {
				current_btn->queue_free();
				break;
			}
		}

#ifdef TOOLS_ENABLED
		if (Engine::get_singleton()->is_editor_hint()) {
			PackedColorArray arr_to_save = get_presets();
			EditorSettings::get_singleton()->set_project_metadata("color_picker", "presets", arr_to_save);
		}
#endif
	}
}

void ColorPicker::erase_recent_preset(const Color &p_color) {
	List<Color>::Element *e = recent_presets.find(p_color);
	if (e) {
		recent_presets.erase(e);
		recent_preset_cache.erase(recent_preset_cache.find(p_color));

		// Find recent preset button to remove.
		for (int i = 1; i < recent_preset_hbc->get_child_count(); i++) {
			ColorPresetButton *current_btn = Object::cast_to<ColorPresetButton>(recent_preset_hbc->get_child(i));
			if (current_btn && p_color == current_btn->get_preset_color()) {
				current_btn->queue_free();
				break;
			}
		}

#ifdef TOOLS_ENABLED
		if (Engine::get_singleton()->is_editor_hint()) {
			PackedColorArray arr_to_save = get_recent_presets();
			EditorSettings::get_singleton()->set_project_metadata("color_picker", "recent_presets", arr_to_save);
		}
#endif
	}
}

PackedColorArray ColorPicker::get_presets() const {
	PackedColorArray arr;
	arr.resize(presets.size());
	for (int i = 0; i < presets.size(); i++) {
		arr.set(i, presets[i]);
	}
	return arr;
}

PackedColorArray ColorPicker::get_recent_presets() const {
	PackedColorArray arr;
	arr.resize(recent_presets.size());
	for (int i = 0; i < recent_presets.size(); i++) {
		arr.set(i, recent_presets[i]);
	}
	return arr;
}

void ColorPicker::set_color_mode(ColorModeType p_mode) {
	ERR_FAIL_INDEX(p_mode, MODE_MAX);

	if (current_mode == p_mode) {
		return;
	}

	if (slider_theme_modified) {
		_reset_theme();
	}

	mode_popup->set_item_checked(current_mode, false);
	mode_popup->set_item_checked(p_mode, true);

	if (p_mode < MODE_BUTTON_COUNT) {
		mode_btns[p_mode]->set_pressed(true);
	} else if (current_mode < MODE_BUTTON_COUNT) {
		mode_btns[current_mode]->set_pressed(false);
	}

	current_mode = p_mode;

	if (!is_inside_tree()) {
		return;
	}

	_update_controls();
	_update_color();
}

ColorPicker::ColorModeType ColorPicker::get_color_mode() const {
	return current_mode;
}

void ColorPicker::set_colorize_sliders(bool p_colorize_sliders) {
	if (colorize_sliders == p_colorize_sliders) {
		return;
	}

	colorize_sliders = p_colorize_sliders;
	mode_popup->set_item_checked(MODE_MAX + 1, colorize_sliders);

	if (colorize_sliders) {
		Ref<StyleBoxEmpty> style_box_empty(memnew(StyleBoxEmpty));

		if (!slider_theme_modified) {
			for (int i = 0; i < SLIDER_COUNT; i++) {
				sliders[i]->add_theme_style_override("slider", style_box_empty);
			}
		}
		alpha_slider->add_theme_style_override("slider", style_box_empty);
	} else {
		Ref<StyleBoxFlat> style_box_flat(memnew(StyleBoxFlat));
		style_box_flat->set_default_margin(SIDE_TOP, 16 * get_theme_default_base_scale());
		style_box_flat->set_bg_color(Color(0.2, 0.23, 0.31).lerp(Color(0, 0, 0, 1), 0.3).clamp());

		if (!slider_theme_modified) {
			for (int i = 0; i < SLIDER_COUNT; i++) {
				sliders[i]->add_theme_style_override("slider", style_box_flat);
			}
		}
		alpha_slider->add_theme_style_override("slider", style_box_flat);
	}
}

bool ColorPicker::is_colorizing_sliders() const {
	return colorize_sliders;
}

void ColorPicker::set_deferred_mode(bool p_enabled) {
	deferred_mode_enabled = p_enabled;
}

bool ColorPicker::is_deferred_mode() const {
	return deferred_mode_enabled;
}

void ColorPicker::_update_text_value() {
	bool text_visible = true;
	if (text_is_constructor) {
		String t = "Color(" + String::num(color.r) + ", " + String::num(color.g) + ", " + String::num(color.b);
		if (edit_alpha && color.a < 1) {
			t += ", " + String::num(color.a) + ")";
		} else {
			t += ")";
		}
		c_text->set_text(t);
	}

	if (color.r > 1 || color.g > 1 || color.b > 1 || color.r < 0 || color.g < 0 || color.b < 0) {
		text_visible = false;
	} else if (!text_is_constructor) {
		c_text->set_text(color.to_html(edit_alpha && color.a < 1));
	}

	text_type->set_visible(text_visible);
	c_text->set_visible(text_visible);
}

void ColorPicker::_sample_input(const Ref<InputEvent> &p_event) {
	const Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
		const Rect2 rect_old = Rect2(Point2(), Size2(sample->get_size().width * 0.5, sample->get_size().height * 0.95));
		if (rect_old.has_point(mb->get_position())) {
			// Revert to the old color when left-clicking the old color sample.
			set_pick_color(old_color);
			emit_signal(SNAME("color_changed"), color);
		}
	}
}

void ColorPicker::_sample_draw() {
	// Covers the right half of the sample if the old color is being displayed,
	// or the whole sample if it's not being displayed.
	Rect2 rect_new;

	if (display_old_color) {
		rect_new = Rect2(Point2(sample->get_size().width * 0.5, 0), Size2(sample->get_size().width * 0.5, sample->get_size().height * 0.95));

		// Draw both old and new colors for easier comparison (only if spawned from a ColorPickerButton).
		const Rect2 rect_old = Rect2(Point2(), Size2(sample->get_size().width * 0.5, sample->get_size().height * 0.95));

		if (display_old_color && old_color.a < 1.0) {
			sample->draw_texture_rect(get_theme_icon(SNAME("sample_bg"), SNAME("ColorPicker")), rect_old, true);
		}

		sample->draw_rect(rect_old, old_color);

		if (old_color.r > 1 || old_color.g > 1 || old_color.b > 1) {
			// Draw an indicator to denote that the old color is "overbright" and can't be displayed accurately in the preview.
			sample->draw_texture(get_theme_icon(SNAME("overbright_indicator"), SNAME("ColorPicker")), Point2());
		}
	} else {
		rect_new = Rect2(Point2(), Size2(sample->get_size().width, sample->get_size().height * 0.95));
	}

	if (color.a < 1.0) {
		sample->draw_texture_rect(get_theme_icon(SNAME("sample_bg"), SNAME("ColorPicker")), rect_new, true);
	}

	sample->draw_rect(rect_new, color);

	if (color.r > 1 || color.g > 1 || color.b > 1) {
		// Draw an indicator to denote that the new color is "overbright" and can't be displayed accurately in the preview.
		sample->draw_texture(get_theme_icon(SNAME("overbright_indicator"), SNAME("ColorPicker")), Point2(uv_edit->get_size().width * 0.5, 0));
	}
}

void ColorPicker::_hsv_draw(int p_which, Control *c) {
	if (!c) {
		return;
	}

	PickerShapeType actual_shape = _get_actual_shape();
	if (p_which == 0) {
		Vector<Point2> points;
		Vector<Color> colors;
		Vector<Color> colors2;
		Color col = color;
		Vector2 center = c->get_size() / 2.0;

		switch (actual_shape) {
			case SHAPE_HSV_WHEEL: {
				points.resize(4);
				colors.resize(4);
				colors2.resize(4);
				real_t ring_radius_x = Math_SQRT12 * c->get_size().width * 0.42;
				real_t ring_radius_y = Math_SQRT12 * c->get_size().height * 0.42;

				points.set(0, center - Vector2(ring_radius_x, ring_radius_y));
				points.set(1, center + Vector2(ring_radius_x, -ring_radius_y));
				points.set(2, center + Vector2(ring_radius_x, ring_radius_y));
				points.set(3, center + Vector2(-ring_radius_x, ring_radius_y));
				colors.set(0, Color(1, 1, 1, 1));
				colors.set(1, Color(1, 1, 1, 1));
				colors.set(2, Color(0, 0, 0, 1));
				colors.set(3, Color(0, 0, 0, 1));
				c->draw_polygon(points, colors);

				col.set_hsv(h, 1, 1);
				col.a = 0;
				colors2.set(0, col);
				col.a = 1;
				colors2.set(1, col);
				col.set_hsv(h, 1, 0);
				colors2.set(2, col);
				col.a = 0;
				colors2.set(3, col);
				c->draw_polygon(points, colors2);
				break;
			}
			case SHAPE_HSV_RECTANGLE: {
				points.resize(4);
				colors.resize(4);
				colors2.resize(4);
				points.set(0, Vector2());
				points.set(1, Vector2(c->get_size().x, 0));
				points.set(2, c->get_size());
				points.set(3, Vector2(0, c->get_size().y));
				colors.set(0, Color(1, 1, 1, 1));
				colors.set(1, Color(1, 1, 1, 1));
				colors.set(2, Color(0, 0, 0, 1));
				colors.set(3, Color(0, 0, 0, 1));
				c->draw_polygon(points, colors);
				col = color;
				col.set_hsv(h, 1, 1);
				col.a = 0;
				colors2.set(0, col);
				col.a = 1;
				colors2.set(1, col);
				col.set_hsv(h, 1, 0);
				colors2.set(2, col);
				col.a = 0;
				colors2.set(3, col);
				c->draw_polygon(points, colors2);
				break;
			}
			default: {
			}
		}
		Ref<Texture2D> cursor = get_theme_icon(SNAME("picker_cursor"), SNAME("ColorPicker"));
		int x;
		int y;
		if (actual_shape == SHAPE_VHS_CIRCLE || actual_shape == SHAPE_OKHSL_CIRCLE) {
			x = center.x + (center.x * Math::cos(h * Math_TAU) * s) - (cursor->get_width() / 2);
			y = center.y + (center.y * Math::sin(h * Math_TAU) * s) - (cursor->get_height() / 2);
		} else {
			real_t corner_x = (c == wheel_uv) ? center.x - Math_SQRT12 * c->get_size().width * 0.42 : 0;
			real_t corner_y = (c == wheel_uv) ? center.y - Math_SQRT12 * c->get_size().height * 0.42 : 0;

			Size2 real_size(c->get_size().x - corner_x * 2, c->get_size().y - corner_y * 2);
			x = CLAMP(real_size.x * s, 0, real_size.x) + corner_x - (cursor->get_width() / 2);
			y = CLAMP(real_size.y - real_size.y * v, 0, real_size.y) + corner_y - (cursor->get_height() / 2);
		}
		c->draw_texture(cursor, Point2(x, y));

		col.set_hsv(h, 1, 1);
		if (actual_shape == SHAPE_HSV_WHEEL) {
			points.resize(4);
			double h1 = h - (0.5 / 360);
			double h2 = h + (0.5 / 360);
			points.set(0, Point2(center.x + (center.x * Math::cos(h1 * Math_TAU)), center.y + (center.y * Math::sin(h1 * Math_TAU))));
			points.set(1, Point2(center.x + (center.x * Math::cos(h1 * Math_TAU) * 0.84), center.y + (center.y * Math::sin(h1 * Math_TAU) * 0.84)));
			points.set(2, Point2(center.x + (center.x * Math::cos(h2 * Math_TAU)), center.y + (center.y * Math::sin(h2 * Math_TAU))));
			points.set(3, Point2(center.x + (center.x * Math::cos(h2 * Math_TAU) * 0.84), center.y + (center.y * Math::sin(h2 * Math_TAU) * 0.84)));
			c->draw_multiline(points, col.inverted());
		}

	} else if (p_which == 1) {
		if (actual_shape == SHAPE_HSV_RECTANGLE) {
			Ref<Texture2D> hue = get_theme_icon(SNAME("color_hue"), SNAME("ColorPicker"));
			c->draw_texture_rect(hue, Rect2(Point2(), c->get_size()));
			int y = c->get_size().y - c->get_size().y * (1.0 - h);
			Color col;
			col.set_hsv(h, 1, 1);
			c->draw_line(Point2(0, y), Point2(c->get_size().x, y), col.inverted());
		} else if (actual_shape == SHAPE_OKHSL_CIRCLE) {
			Vector<Point2> points;
			Vector<Color> colors;
			Color col;
			col.set_ok_hsl(h, s, 1);
			Color col2;
			col2.set_ok_hsl(h, s, 0.5);
			Color col3;
			col3.set_ok_hsl(h, s, 0);
			points.resize(6);
			colors.resize(6);
			points.set(0, Vector2(c->get_size().x, 0));
			points.set(1, Vector2(c->get_size().x, c->get_size().y * 0.5));
			points.set(2, c->get_size());
			points.set(3, Vector2(0, c->get_size().y));
			points.set(4, Vector2(0, c->get_size().y * 0.5));
			points.set(5, Vector2());
			colors.set(0, col);
			colors.set(1, col2);
			colors.set(2, col3);
			colors.set(3, col3);
			colors.set(4, col2);
			colors.set(5, col);
			c->draw_polygon(points, colors);
			int y = c->get_size().y - c->get_size().y * CLAMP(v, 0, 1);
			col.set_ok_hsl(h, 1, v);
			c->draw_line(Point2(0, y), Point2(c->get_size().x, y), col.inverted());
		} else if (actual_shape == SHAPE_VHS_CIRCLE) {
			Vector<Point2> points;
			Vector<Color> colors;
			Color col;
			col.set_hsv(h, s, 1);
			points.resize(4);
			colors.resize(4);
			points.set(0, Vector2());
			points.set(1, Vector2(c->get_size().x, 0));
			points.set(2, c->get_size());
			points.set(3, Vector2(0, c->get_size().y));
			colors.set(0, col);
			colors.set(1, col);
			colors.set(2, Color(0, 0, 0));
			colors.set(3, Color(0, 0, 0));
			c->draw_polygon(points, colors);
			int y = c->get_size().y - c->get_size().y * CLAMP(v, 0, 1);
			col.set_hsv(h, 1, v);
			c->draw_line(Point2(0, y), Point2(c->get_size().x, y), col.inverted());
		}
	} else if (p_which == 2) {
		c->draw_rect(Rect2(Point2(), c->get_size()), Color(1, 1, 1));
		if (actual_shape == SHAPE_VHS_CIRCLE || actual_shape == SHAPE_OKHSL_CIRCLE) {
			circle_mat->set_shader_parameter("v", v);
		}
	}
}

void ColorPicker::_slider_draw(int p_which) {
	if (colorize_sliders) {
		modes[current_mode]->slider_draw(p_which);
	}
}

void ColorPicker::_uv_input(const Ref<InputEvent> &p_event, Control *c) {
	Ref<InputEventMouseButton> bev = p_event;
	PickerShapeType actual_shape = _get_actual_shape();

	if (bev.is_valid()) {
		if (bev->is_pressed() && bev->get_button_index() == MouseButton::LEFT) {
			Vector2 center = c->get_size() / 2.0;
			if (actual_shape == SHAPE_VHS_CIRCLE || actual_shape == SHAPE_OKHSL_CIRCLE) {
				real_t dist = center.distance_to(bev->get_position());
				if (dist <= center.x) {
					real_t rad = center.angle_to_point(bev->get_position());
					h = ((rad >= 0) ? rad : (Math_TAU + rad)) / Math_TAU;
					s = CLAMP(dist / center.x, 0, 1);
				} else {
					return;
				}
			} else {
				real_t corner_x = (c == wheel_uv) ? center.x - Math_SQRT12 * c->get_size().width * 0.42 : 0;
				real_t corner_y = (c == wheel_uv) ? center.y - Math_SQRT12 * c->get_size().height * 0.42 : 0;
				Size2 real_size(c->get_size().x - corner_x * 2, c->get_size().y - corner_y * 2);

				if (bev->get_position().x < corner_x || bev->get_position().x > c->get_size().x - corner_x ||
						bev->get_position().y < corner_y || bev->get_position().y > c->get_size().y - corner_y) {
					{
						real_t dist = center.distance_to(bev->get_position());

						if (dist >= center.x * 0.84 && dist <= center.x) {
							real_t rad = center.angle_to_point(bev->get_position());
							h = ((rad >= 0) ? rad : (Math_TAU + rad)) / Math_TAU;
							spinning = true;
						} else {
							return;
						}
					}
				}

				if (!spinning) {
					real_t x = CLAMP(bev->get_position().x, corner_x, c->get_size().x - corner_x);
					real_t y = CLAMP(bev->get_position().y, corner_x, c->get_size().y - corner_y);

					s = (x - c->get_position().x - corner_x) / real_size.x;
					v = 1.0 - (y - c->get_position().y - corner_y) / real_size.y;
				}
			}

			changing_color = true;

			_copy_hsv_to_color();
			last_color = color;
			set_pick_color(color);
			_update_color();

			if (!deferred_mode_enabled) {
				emit_signal(SNAME("color_changed"), color);
			}
		} else if (!bev->is_pressed() && bev->get_button_index() == MouseButton::LEFT) {
			if (deferred_mode_enabled) {
				emit_signal(SNAME("color_changed"), color);
			}
			add_recent_preset(color);
			changing_color = false;
			spinning = false;
		} else {
			changing_color = false;
			spinning = false;
		}
	}

	Ref<InputEventMouseMotion> mev = p_event;

	if (mev.is_valid()) {
		if (!changing_color) {
			return;
		}

		Vector2 center = c->get_size() / 2.0;
		if (actual_shape == SHAPE_VHS_CIRCLE || actual_shape == SHAPE_OKHSL_CIRCLE) {
			real_t dist = center.distance_to(mev->get_position());
			real_t rad = center.angle_to_point(mev->get_position());
			h = ((rad >= 0) ? rad : (Math_TAU + rad)) / Math_TAU;
			s = CLAMP(dist / center.x, 0, 1);
		} else {
			if (spinning) {
				real_t rad = center.angle_to_point(mev->get_position());
				h = ((rad >= 0) ? rad : (Math_TAU + rad)) / Math_TAU;
			} else {
				real_t corner_x = (c == wheel_uv) ? center.x - Math_SQRT12 * c->get_size().width * 0.42 : 0;
				real_t corner_y = (c == wheel_uv) ? center.y - Math_SQRT12 * c->get_size().height * 0.42 : 0;
				Size2 real_size(c->get_size().x - corner_x * 2, c->get_size().y - corner_y * 2);

				real_t x = CLAMP(mev->get_position().x, corner_x, c->get_size().x - corner_x);
				real_t y = CLAMP(mev->get_position().y, corner_x, c->get_size().y - corner_y);

				s = (x - corner_x) / real_size.x;
				v = 1.0 - (y - corner_y) / real_size.y;
			}
		}

		_copy_hsv_to_color();
		last_color = color;
		set_pick_color(color);
		_update_color();

		if (!deferred_mode_enabled) {
			emit_signal(SNAME("color_changed"), color);
		}
	}
}

void ColorPicker::_w_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> bev = p_event;
	PickerShapeType actual_shape = _get_actual_shape();

	if (bev.is_valid()) {
		if (bev->is_pressed() && bev->get_button_index() == MouseButton::LEFT) {
			changing_color = true;
			float y = CLAMP((float)bev->get_position().y, 0, w_edit->get_size().height);
			if (actual_shape == SHAPE_VHS_CIRCLE || actual_shape == SHAPE_OKHSL_CIRCLE) {
				v = 1.0 - (y / w_edit->get_size().height);
			} else {
				h = y / w_edit->get_size().height;
			}
		} else {
			changing_color = false;
		}

		_copy_hsv_to_color();
		last_color = color;
		set_pick_color(color);
		_update_color();

		if (!bev->is_pressed() && bev->get_button_index() == MouseButton::LEFT) {
			add_recent_preset(color);
			emit_signal(SNAME("color_changed"), color);
		} else if (!deferred_mode_enabled) {
			emit_signal(SNAME("color_changed"), color);
		}
	}

	Ref<InputEventMouseMotion> mev = p_event;

	if (mev.is_valid()) {
		if (!changing_color) {
			return;
		}
		float y = CLAMP((float)mev->get_position().y, 0, w_edit->get_size().height);
		if (actual_shape == SHAPE_VHS_CIRCLE || actual_shape == SHAPE_OKHSL_CIRCLE) {
			v = 1.0 - (y / w_edit->get_size().height);
		} else {
			h = y / w_edit->get_size().height;
		}

		_copy_hsv_to_color();
		last_color = color;
		set_pick_color(color);
		_update_color();

		if (!deferred_mode_enabled) {
			emit_signal(SNAME("color_changed"), color);
		}
	}
}

void ColorPicker::_slider_or_spin_input(const Ref<InputEvent> &p_event) {
	if (line_edit_mouse_release) {
		line_edit_mouse_release = false;
		return;
	}
	Ref<InputEventMouseButton> bev = p_event;
	if (bev.is_valid() && !bev->is_pressed() && bev->get_button_index() == MouseButton::LEFT) {
		add_recent_preset(color);
	}
}

void ColorPicker::_line_edit_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> bev = p_event;
	if (bev.is_valid() && !bev->is_pressed() && bev->get_button_index() == MouseButton::LEFT) {
		line_edit_mouse_release = true;
	}
}

void ColorPicker::_preset_input(const Ref<InputEvent> &p_event, const Color &p_color) {
	Ref<InputEventMouseButton> bev = p_event;

	if (bev.is_valid()) {
		if (bev->is_pressed() && bev->get_button_index() == MouseButton::LEFT) {
			set_pick_color(p_color);
			add_recent_preset(color);
			emit_signal(SNAME("color_changed"), p_color);
		} else if (bev->is_pressed() && bev->get_button_index() == MouseButton::RIGHT && presets_enabled) {
			erase_preset(p_color);
			emit_signal(SNAME("preset_removed"), p_color);
		}
	}
}

void ColorPicker::_recent_preset_pressed(const bool p_pressed, ColorPresetButton *p_preset) {
	if (!p_pressed) {
		return;
	}
	set_pick_color(p_preset->get_preset_color());

	recent_presets.move_to_back(recent_presets.find(p_preset->get_preset_color()));
	List<Color>::Element *e = recent_preset_cache.find(p_preset->get_preset_color());
	if (e) {
		recent_preset_cache.move_to_back(e);
	}

	recent_preset_hbc->move_child(p_preset, 0);
	emit_signal(SNAME("color_changed"), p_preset->get_preset_color());
}

void ColorPicker::_screen_input(const Ref<InputEvent> &p_event) {
	if (!is_inside_tree()) {
		return;
	}

	Ref<InputEventMouseButton> bev = p_event;
	if (bev.is_valid() && bev->get_button_index() == MouseButton::LEFT && !bev->is_pressed()) {
		emit_signal(SNAME("color_changed"), color);
		screen->hide();
	}

	Ref<InputEventMouseMotion> mev = p_event;
	if (mev.is_valid()) {
		Viewport *r = get_tree()->get_root();
		if (!r->get_visible_rect().has_point(mev->get_global_position())) {
			return;
		}

		Ref<Image> img = r->get_texture()->get_image();
		if (img.is_valid() && !img->is_empty()) {
			Vector2 ofs = mev->get_global_position();
			Color c = img->get_pixel(ofs.x, ofs.y);

			set_pick_color(c);
		}
	}
}

void ColorPicker::_text_changed(const String &) {
	text_changed = true;
}

void ColorPicker::_add_preset_pressed() {
	add_preset(color);
	emit_signal(SNAME("preset_added"), color);
}

void ColorPicker::_screen_pick_pressed() {
	if (!is_inside_tree()) {
		return;
	}

	Viewport *r = get_tree()->get_root();
	if (!screen) {
		screen = memnew(Control);
		r->add_child(screen);
		screen->set_as_top_level(true);
		screen->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		screen->set_default_cursor_shape(CURSOR_POINTING_HAND);
		screen->connect("gui_input", callable_mp(this, &ColorPicker::_screen_input));
		// It immediately toggles off in the first press otherwise.
		screen->call_deferred(SNAME("connect"), "hidden", Callable(btn_pick, "set_pressed").bind(false));
	} else {
		screen->show();
	}
	screen->move_to_front();
	// TODO: show modal no longer works, needs to be converted to a popup.
	//screen->show_modal();
}

void ColorPicker::_html_focus_exit() {
	if (c_text->is_menu_visible()) {
		return;
	}
	_html_submitted(c_text->get_text());
}

void ColorPicker::set_presets_enabled(bool p_enabled) {
	if (presets_enabled == p_enabled) {
		return;
	}
	presets_enabled = p_enabled;
	if (!p_enabled) {
		btn_add_preset->set_disabled(true);
		btn_add_preset->set_focus_mode(FOCUS_NONE);
	} else {
		btn_add_preset->set_disabled(false);
		btn_add_preset->set_focus_mode(FOCUS_ALL);
	}
}

bool ColorPicker::are_presets_enabled() const {
	return presets_enabled;
}

void ColorPicker::set_presets_visible(bool p_visible) {
	if (presets_visible == p_visible) {
		return;
	}
	presets_visible = p_visible;
	preset_container->set_visible(p_visible);
}

bool ColorPicker::are_presets_visible() const {
	return presets_visible;
}

void ColorPicker::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_pick_color", "color"), &ColorPicker::set_pick_color);
	ClassDB::bind_method(D_METHOD("get_pick_color"), &ColorPicker::get_pick_color);
	ClassDB::bind_method(D_METHOD("set_deferred_mode", "mode"), &ColorPicker::set_deferred_mode);
	ClassDB::bind_method(D_METHOD("is_deferred_mode"), &ColorPicker::is_deferred_mode);
	ClassDB::bind_method(D_METHOD("set_color_mode", "color_mode"), &ColorPicker::set_color_mode);
	ClassDB::bind_method(D_METHOD("get_color_mode"), &ColorPicker::get_color_mode);
	ClassDB::bind_method(D_METHOD("set_edit_alpha", "show"), &ColorPicker::set_edit_alpha);
	ClassDB::bind_method(D_METHOD("is_editing_alpha"), &ColorPicker::is_editing_alpha);
	ClassDB::bind_method(D_METHOD("set_presets_enabled", "enabled"), &ColorPicker::set_presets_enabled);
	ClassDB::bind_method(D_METHOD("are_presets_enabled"), &ColorPicker::are_presets_enabled);
	ClassDB::bind_method(D_METHOD("set_presets_visible", "visible"), &ColorPicker::set_presets_visible);
	ClassDB::bind_method(D_METHOD("are_presets_visible"), &ColorPicker::are_presets_visible);
	ClassDB::bind_method(D_METHOD("add_preset", "color"), &ColorPicker::add_preset);
	ClassDB::bind_method(D_METHOD("erase_preset", "color"), &ColorPicker::erase_preset);
	ClassDB::bind_method(D_METHOD("get_presets"), &ColorPicker::get_presets);
	ClassDB::bind_method(D_METHOD("add_recent_preset", "color"), &ColorPicker::add_recent_preset);
	ClassDB::bind_method(D_METHOD("erase_recent_preset", "color"), &ColorPicker::erase_recent_preset);
	ClassDB::bind_method(D_METHOD("get_recent_presets"), &ColorPicker::get_recent_presets);
	ClassDB::bind_method(D_METHOD("set_picker_shape", "shape"), &ColorPicker::set_picker_shape);
	ClassDB::bind_method(D_METHOD("get_picker_shape"), &ColorPicker::get_picker_shape);

	ClassDB::bind_method(D_METHOD("_get_drag_data_fw"), &ColorPicker::_get_drag_data_fw);
	ClassDB::bind_method(D_METHOD("_can_drop_data_fw"), &ColorPicker::_can_drop_data_fw);
	ClassDB::bind_method(D_METHOD("_drop_data_fw"), &ColorPicker::_drop_data_fw);

	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_pick_color", "get_pick_color");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "edit_alpha"), "set_edit_alpha", "is_editing_alpha");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "color_mode", PROPERTY_HINT_ENUM, "RGB,HSV,RAW,OKHSL"), "set_color_mode", "get_color_mode");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "deferred_mode"), "set_deferred_mode", "is_deferred_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "picker_shape", PROPERTY_HINT_ENUM, "HSV Rectangle,HSV Rectangle Wheel,VHS Circle,OKHSL Circle"), "set_picker_shape", "get_picker_shape");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "presets_enabled"), "set_presets_enabled", "are_presets_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "presets_visible"), "set_presets_visible", "are_presets_visible");

	ADD_SIGNAL(MethodInfo("color_changed", PropertyInfo(Variant::COLOR, "color")));
	ADD_SIGNAL(MethodInfo("preset_added", PropertyInfo(Variant::COLOR, "color")));
	ADD_SIGNAL(MethodInfo("preset_removed", PropertyInfo(Variant::COLOR, "color")));

	BIND_ENUM_CONSTANT(MODE_RGB);
	BIND_ENUM_CONSTANT(MODE_HSV);
	BIND_ENUM_CONSTANT(MODE_RAW);
	BIND_ENUM_CONSTANT(MODE_OKHSL);

	BIND_ENUM_CONSTANT(SHAPE_HSV_RECTANGLE);
	BIND_ENUM_CONSTANT(SHAPE_HSV_WHEEL);
	BIND_ENUM_CONSTANT(SHAPE_VHS_CIRCLE);
	BIND_ENUM_CONSTANT(SHAPE_OKHSL_CIRCLE);
}

ColorPicker::ColorPicker() :
		BoxContainer(true) {
	HBoxContainer *hb_edit = memnew(HBoxContainer);
	add_child(hb_edit, false, INTERNAL_MODE_FRONT);
	hb_edit->set_v_size_flags(SIZE_SHRINK_BEGIN);

	uv_edit = memnew(Control);
	hb_edit->add_child(uv_edit);
	uv_edit->connect("gui_input", callable_mp(this, &ColorPicker::_uv_input).bind(uv_edit));
	uv_edit->set_mouse_filter(MOUSE_FILTER_PASS);
	uv_edit->set_h_size_flags(SIZE_EXPAND_FILL);
	uv_edit->set_v_size_flags(SIZE_EXPAND_FILL);
	uv_edit->connect("draw", callable_mp(this, &ColorPicker::_hsv_draw).bind(0, uv_edit));

	HBoxContainer *hb_smpl = memnew(HBoxContainer);
	add_child(hb_smpl, false, INTERNAL_MODE_FRONT);

	btn_pick = memnew(Button);
	hb_smpl->add_child(btn_pick);
	btn_pick->set_toggle_mode(true);
	btn_pick->set_tooltip_text(RTR("Pick a color from the editor window."));
	btn_pick->connect("pressed", callable_mp(this, &ColorPicker::_screen_pick_pressed));

	sample = memnew(TextureRect);
	hb_smpl->add_child(sample);
	sample->set_h_size_flags(SIZE_EXPAND_FILL);
	sample->connect("gui_input", callable_mp(this, &ColorPicker::_sample_input));
	sample->connect("draw", callable_mp(this, &ColorPicker::_sample_draw));

	btn_shape = memnew(MenuButton);
	btn_shape->set_flat(false);
	hb_smpl->add_child(btn_shape);
	btn_shape->set_toggle_mode(true);
	btn_shape->set_tooltip_text(RTR("Select a picker shape."));

	current_shape = SHAPE_HSV_RECTANGLE;

	shape_popup = btn_shape->get_popup();
	shape_popup->add_icon_radio_check_item(get_theme_icon(SNAME("shape_rect"), SNAME("ColorPicker")), "HSV Rectangle", SHAPE_HSV_RECTANGLE);
	shape_popup->add_icon_radio_check_item(get_theme_icon(SNAME("shape_rect_wheel"), SNAME("ColorPicker")), "HSV Wheel", SHAPE_HSV_WHEEL);
	shape_popup->add_icon_radio_check_item(get_theme_icon(SNAME("shape_circle"), SNAME("ColorPicker")), "VHS Circle", SHAPE_VHS_CIRCLE);
	shape_popup->add_icon_radio_check_item(get_theme_icon(SNAME("shape_circle"), SNAME("ColorPicker")), "OKHSL Circle", SHAPE_OKHSL_CIRCLE);
	shape_popup->set_item_checked(current_shape, true);
	shape_popup->connect("id_pressed", callable_mp(this, &ColorPicker::set_picker_shape));

	btn_shape->set_icon(shape_popup->get_item_icon(current_shape));

	add_mode(new ColorModeRGB(this));
	add_mode(new ColorModeHSV(this));
	add_mode(new ColorModeRAW(this));
	add_mode(new ColorModeOKHSL(this));

	HBoxContainer *mode_hbc = memnew(HBoxContainer);
	add_child(mode_hbc, false, INTERNAL_MODE_FRONT);

	mode_group.instantiate();

	for (int i = 0; i < MODE_BUTTON_COUNT; i++) {
		mode_btns[i] = memnew(Button);
		mode_hbc->add_child(mode_btns[i]);
		mode_btns[i]->set_focus_mode(FOCUS_NONE);
		mode_btns[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		mode_btns[i]->add_theme_style_override("pressed", get_theme_stylebox("tab_selected", "TabContainer"));
		mode_btns[i]->add_theme_style_override("normal", get_theme_stylebox("tab_unselected", "TabContainer"));
		mode_btns[i]->add_theme_style_override("hover", get_theme_stylebox("tab_selected", "TabContainer"));
		mode_btns[i]->set_toggle_mode(true);
		mode_btns[i]->set_text(modes[i]->get_name());
		mode_btns[i]->set_button_group(mode_group);
		mode_btns[i]->connect("pressed", callable_mp(this, &ColorPicker::set_color_mode).bind((ColorModeType)i));
	}
	mode_btns[0]->set_pressed(true);

	btn_mode = memnew(MenuButton);
	btn_mode->set_text("...");
	btn_mode->set_flat(false);
	mode_hbc->add_child(btn_mode);
	btn_mode->set_toggle_mode(true);
	btn_mode->set_tooltip_text(RTR("Select a picker mode."));

	current_mode = MODE_RGB;

	mode_popup = btn_mode->get_popup();
	for (int i = 0; i < modes.size(); i++) {
		mode_popup->add_radio_check_item(modes[i]->get_name(), i);
	}
	mode_popup->add_separator();
	mode_popup->add_check_item("Colorized Sliders", MODE_MAX);
	mode_popup->set_item_checked(current_mode, true);
	mode_popup->set_item_checked(MODE_MAX + 1, true);
	mode_popup->connect("id_pressed", callable_mp(this, &ColorPicker::_set_mode_popup_value));
	VBoxContainer *vbl = memnew(VBoxContainer);
	add_child(vbl, false, INTERNAL_MODE_FRONT);

	VBoxContainer *vbr = memnew(VBoxContainer);

	add_child(vbr, false, INTERNAL_MODE_FRONT);
	vbr->set_h_size_flags(SIZE_EXPAND_FILL);

	GridContainer *gc = memnew(GridContainer);

	vbr->add_child(gc);
	gc->set_h_size_flags(SIZE_EXPAND_FILL);
	gc->set_columns(3);

	for (int i = 0; i < SLIDER_COUNT + 1; i++) {
		create_slider(gc, i);
	}

	alpha_label->set_text("A");

	HBoxContainer *hhb = memnew(HBoxContainer);
	hhb->set_alignment(ALIGNMENT_BEGIN);
	vbr->add_child(hhb);

	hhb->add_child(memnew(Label("Hex")));

	text_type = memnew(Button);
	hhb->add_child(text_type);
	text_type->set_text("#");
	text_type->set_tooltip_text(RTR("Switch between hexadecimal and code values."));
	if (Engine::get_singleton()->is_editor_hint()) {
		text_type->connect("pressed", callable_mp(this, &ColorPicker::_text_type_toggled));
	} else {
		text_type->set_flat(true);
		text_type->set_mouse_filter(MOUSE_FILTER_IGNORE);
	}

	c_text = memnew(LineEdit);
	hhb->add_child(c_text);
	c_text->set_select_all_on_focus(true);
	c_text->connect("text_submitted", callable_mp(this, &ColorPicker::_html_submitted));
	c_text->connect("text_changed", callable_mp(this, &ColorPicker::_text_changed));
	c_text->connect("focus_exited", callable_mp(this, &ColorPicker::_html_focus_exit));

	wheel_edit = memnew(AspectRatioContainer);
	wheel_edit->set_h_size_flags(SIZE_EXPAND_FILL);
	wheel_edit->set_v_size_flags(SIZE_EXPAND_FILL);
	hb_edit->add_child(wheel_edit);

	wheel_mat.instantiate();
	wheel_mat->set_shader(wheel_shader);
	circle_mat.instantiate();
	circle_mat->set_shader(circle_shader);

	wheel_margin = memnew(MarginContainer);
	wheel_margin->add_theme_constant_override("margin_bottom", 8);
	wheel_edit->add_child(wheel_margin);

	wheel = memnew(Control);
	wheel_margin->add_child(wheel);
	wheel->set_mouse_filter(MOUSE_FILTER_PASS);
	wheel->connect("draw", callable_mp(this, &ColorPicker::_hsv_draw).bind(2, wheel));

	wheel_uv = memnew(Control);
	wheel_margin->add_child(wheel_uv);
	wheel_uv->connect("gui_input", callable_mp(this, &ColorPicker::_uv_input).bind(wheel_uv));
	wheel_uv->connect("draw", callable_mp(this, &ColorPicker::_hsv_draw).bind(0, wheel_uv));

	w_edit = memnew(Control);
	hb_edit->add_child(w_edit);
	w_edit->set_h_size_flags(SIZE_FILL);
	w_edit->set_v_size_flags(SIZE_EXPAND_FILL);
	w_edit->connect("gui_input", callable_mp(this, &ColorPicker::_w_input));
	w_edit->connect("draw", callable_mp(this, &ColorPicker::_hsv_draw).bind(1, w_edit));

	_update_controls();
	updating = false;

	preset_container = memnew(GridContainer);
	preset_container->set_h_size_flags(SIZE_EXPAND_FILL);
	preset_container->set_columns(PRESET_COLUMN_COUNT);
	preset_container->hide();

	preset_group.instantiate();

	btn_preset = memnew(Button);
	btn_preset->set_text("Swatches");
	btn_preset->set_flat(true);
	btn_preset->set_toggle_mode(true);
	btn_preset->set_focus_mode(FOCUS_NONE);
	btn_preset->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	btn_preset->connect("toggled", callable_mp(this, &ColorPicker::_show_hide_preset).bind(btn_preset, preset_container));
	add_child(btn_preset, false, INTERNAL_MODE_FRONT);

	add_child(preset_container, false, INTERNAL_MODE_FRONT);

	recent_preset_hbc = memnew(HBoxContainer);
	recent_preset_hbc->set_v_size_flags(SIZE_SHRINK_BEGIN);
	recent_preset_hbc->hide();

	recent_preset_group.instantiate();

	btn_recent_preset = memnew(Button);
	btn_recent_preset->set_text("Recent Colors");
	btn_recent_preset->set_flat(true);
	btn_recent_preset->set_toggle_mode(true);
	btn_recent_preset->set_focus_mode(FOCUS_NONE);
	btn_recent_preset->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	btn_recent_preset->connect("toggled", callable_mp(this, &ColorPicker::_show_hide_preset).bind(btn_recent_preset, recent_preset_hbc));
	add_child(btn_recent_preset, false, INTERNAL_MODE_FRONT);

	add_child(recent_preset_hbc, false, INTERNAL_MODE_FRONT);

	set_pick_color(Color(1, 1, 1));

	btn_add_preset = memnew(Button);
	btn_add_preset->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	btn_add_preset->set_tooltip_text(RTR("Add current color as a preset."));
	btn_add_preset->connect("pressed", callable_mp(this, &ColorPicker::_add_preset_pressed));
	preset_container->add_child(btn_add_preset);
}

ColorPicker::~ColorPicker() {
	for (int i = 0; i < modes.size(); i++) {
		delete modes[i];
	}
}

/////////////////

void ColorPickerButton::_about_to_popup() {
	set_pressed(true);
	if (picker) {
		picker->set_old_color(color);
	}
}

void ColorPickerButton::_color_changed(const Color &p_color) {
	color = p_color;
	queue_redraw();
	emit_signal(SNAME("color_changed"), color);
}

void ColorPickerButton::_modal_closed() {
	emit_signal(SNAME("popup_closed"));
	set_pressed(false);
}

void ColorPickerButton::pressed() {
	_update_picker();

	Size2 size = get_size() * get_viewport()->get_canvas_transform().get_scale();

	popup->reset_size();
	picker->_update_presets();
	picker->_update_recent_presets();

	Rect2i usable_rect = popup->get_usable_parent_rect();
	//let's try different positions to see which one we can use

	Rect2i cp_rect(Point2i(), popup->get_size());
	for (int i = 0; i < 4; i++) {
		if (i > 1) {
			cp_rect.position.y = get_screen_position().y - cp_rect.size.y;
		} else {
			cp_rect.position.y = get_screen_position().y + size.height;
		}

		if (i & 1) {
			cp_rect.position.x = get_screen_position().x;
		} else {
			cp_rect.position.x = get_screen_position().x - MAX(0, (cp_rect.size.x - size.x));
		}

		if (usable_rect.encloses(cp_rect)) {
			break;
		}
	}
	popup->set_position(cp_rect.position);
	popup->popup();
	picker->set_focus_on_line_edit();
}

void ColorPickerButton::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			const Ref<StyleBox> normal = get_theme_stylebox(SNAME("normal"));
			const Rect2 r = Rect2(normal->get_offset(), get_size() - normal->get_minimum_size());
			draw_texture_rect(Control::get_theme_icon(SNAME("bg"), SNAME("ColorPickerButton")), r, true);
			draw_rect(r, color);

			if (color.r > 1 || color.g > 1 || color.b > 1) {
				// Draw an indicator to denote that the color is "overbright" and can't be displayed accurately in the preview
				draw_texture(Control::get_theme_icon(SNAME("overbright_indicator"), SNAME("ColorPicker")), normal->get_offset());
			}
		} break;

		case NOTIFICATION_WM_CLOSE_REQUEST: {
			if (popup) {
				popup->hide();
			}
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (popup && !is_visible_in_tree()) {
				popup->hide();
			}
		} break;
	}
}

void ColorPickerButton::set_pick_color(const Color &p_color) {
	if (color == p_color) {
		return;
	}
	color = p_color;
	if (picker) {
		picker->set_pick_color(p_color);
	}

	queue_redraw();
}

Color ColorPickerButton::get_pick_color() const {
	return color;
}

void ColorPickerButton::set_edit_alpha(bool p_show) {
	if (edit_alpha == p_show) {
		return;
	}
	edit_alpha = p_show;
	if (picker) {
		picker->set_edit_alpha(p_show);
	}
}

bool ColorPickerButton::is_editing_alpha() const {
	return edit_alpha;
}

ColorPicker *ColorPickerButton::get_picker() {
	_update_picker();
	return picker;
}

PopupPanel *ColorPickerButton::get_popup() {
	_update_picker();
	return popup;
}

void ColorPickerButton::_update_picker() {
	if (!picker) {
		popup = memnew(PopupPanel);
		popup->set_wrap_controls(true);
		picker = memnew(ColorPicker);
		picker->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		popup->add_child(picker);
		add_child(popup, false, INTERNAL_MODE_FRONT);
		picker->connect("color_changed", callable_mp(this, &ColorPickerButton::_color_changed));
		popup->connect("about_to_popup", callable_mp(this, &ColorPickerButton::_about_to_popup));
		popup->connect("popup_hide", callable_mp(this, &ColorPickerButton::_modal_closed));
		picker->connect("minimum_size_changed", callable_mp((Window *)popup, &Window::reset_size));
		picker->set_pick_color(color);
		picker->set_edit_alpha(edit_alpha);
		picker->set_display_old_color(true);
		emit_signal(SNAME("picker_created"));
	}
}

void ColorPickerButton::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_pick_color", "color"), &ColorPickerButton::set_pick_color);
	ClassDB::bind_method(D_METHOD("get_pick_color"), &ColorPickerButton::get_pick_color);
	ClassDB::bind_method(D_METHOD("get_picker"), &ColorPickerButton::get_picker);
	ClassDB::bind_method(D_METHOD("get_popup"), &ColorPickerButton::get_popup);
	ClassDB::bind_method(D_METHOD("set_edit_alpha", "show"), &ColorPickerButton::set_edit_alpha);
	ClassDB::bind_method(D_METHOD("is_editing_alpha"), &ColorPickerButton::is_editing_alpha);
	ClassDB::bind_method(D_METHOD("_about_to_popup"), &ColorPickerButton::_about_to_popup);

	ADD_SIGNAL(MethodInfo("color_changed", PropertyInfo(Variant::COLOR, "color")));
	ADD_SIGNAL(MethodInfo("popup_closed"));
	ADD_SIGNAL(MethodInfo("picker_created"));
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_pick_color", "get_pick_color");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "edit_alpha"), "set_edit_alpha", "is_editing_alpha");
}

ColorPickerButton::ColorPickerButton(const String &p_text) :
		Button(p_text) {
	set_toggle_mode(true);
}

/////////////////

void ColorPresetButton::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			const Rect2 r = Rect2(Point2(0, 0), get_size());
			Ref<StyleBox> sb_raw = get_theme_stylebox(SNAME("preset_fg"), SNAME("ColorPresetButton"))->duplicate();
			Ref<StyleBoxFlat> sb_flat = sb_raw;
			Ref<StyleBoxTexture> sb_texture = sb_raw;

			if (sb_flat.is_valid()) {
				sb_flat->set_border_width(SIDE_BOTTOM, 2);
				if (get_draw_mode() == DRAW_PRESSED || get_draw_mode() == DRAW_HOVER_PRESSED) {
					sb_flat->set_border_color(Color(1, 1, 1, 1));
				} else {
					sb_flat->set_border_color(Color(0, 0, 0, 1));
				}

				if (preset_color.a < 1) {
					// Draw a background pattern when the color is transparent.
					sb_flat->set_bg_color(Color(1, 1, 1));
					sb_flat->draw(get_canvas_item(), r);

					Rect2 bg_texture_rect = r.grow_side(SIDE_LEFT, -sb_flat->get_margin(SIDE_LEFT));
					bg_texture_rect = bg_texture_rect.grow_side(SIDE_RIGHT, -sb_flat->get_margin(SIDE_RIGHT));
					bg_texture_rect = bg_texture_rect.grow_side(SIDE_TOP, -sb_flat->get_margin(SIDE_TOP));
					bg_texture_rect = bg_texture_rect.grow_side(SIDE_BOTTOM, -sb_flat->get_margin(SIDE_BOTTOM));

					draw_texture_rect(get_theme_icon(SNAME("preset_bg"), SNAME("ColorPresetButton")), bg_texture_rect, true);
					sb_flat->set_bg_color(preset_color);
				}
				sb_flat->set_bg_color(preset_color);
				sb_flat->draw(get_canvas_item(), r);
			} else if (sb_texture.is_valid()) {
				if (preset_color.a < 1) {
					// Draw a background pattern when the color is transparent.
					bool use_tile_texture = (sb_texture->get_h_axis_stretch_mode() == StyleBoxTexture::AxisStretchMode::AXIS_STRETCH_MODE_TILE) || (sb_texture->get_h_axis_stretch_mode() == StyleBoxTexture::AxisStretchMode::AXIS_STRETCH_MODE_TILE_FIT);
					draw_texture_rect(get_theme_icon(SNAME("preset_bg"), SNAME("ColorPresetButton")), r, use_tile_texture);
				}
				sb_texture->set_modulate(preset_color);
				sb_texture->draw(get_canvas_item(), r);
			} else {
				WARN_PRINT("Unsupported StyleBox used for ColorPresetButton. Use StyleBoxFlat or StyleBoxTexture instead.");
			}
			if (preset_color.r > 1 || preset_color.g > 1 || preset_color.b > 1) {
				// Draw an indicator to denote that the color is "overbright" and can't be displayed accurately in the preview
				draw_texture(Control::get_theme_icon(SNAME("overbright_indicator"), SNAME("ColorPresetButton")), Vector2(0, 0));
			}

		} break;
	}
}

void ColorPresetButton::set_preset_color(const Color &p_color) {
	preset_color = p_color;
}

Color ColorPresetButton::get_preset_color() const {
	return preset_color;
}

ColorPresetButton::ColorPresetButton(Color p_color, int p_size) {
	preset_color = p_color;
	set_toggle_mode(true);
	set_custom_minimum_size(Size2(p_size, p_size));
}

ColorPresetButton::~ColorPresetButton() {
}
