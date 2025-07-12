#pragma once
#include "../../imgui/imgui_impl_win32.h"
#include "../../imgui/imgui_internal.h"
#include "../../imgui/imgui.h"


namespace wiget {

	bool slider_float(const char* label, float* v, float v_min, float v_max)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *ImGui::GetCurrentContext();
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);

		float width = 140.0f;
		float height = 6.0f;

		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImVec2 label_pos = ImVec2(pos.x, pos.y + 2.0f);
		ImVec2 slider_pos = ImVec2(pos.x, pos.y + 22.0f);
		ImRect bb(slider_pos, ImVec2(slider_pos.x + width, slider_pos.y + height));
		ImGui::ItemSize(ImVec2(width, 22));
		if (!ImGui::ItemAdd(bb, id))
			return false;

		if (label && label[0] != '\0') {
			ImGui::GetWindowDrawList()->AddText(label_pos, IM_COL32(255, 255, 255, 255), label);
		}

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
		if (held) {
			float mouse_x = ImGui::GetIO().MousePos.x;
			float t = (mouse_x - bb.Min.x) / (bb.Max.x - bb.Min.x);
			t = ImClamp(t, 0.0f, 1.0f);
			*v = ImLerp(v_min, v_max, t);
		}

		float t = (*v - v_min) / (v_max - v_min);
		float filled_width = t * width;

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImU32 bg_col = IM_COL32(30, 30, 30, 255);
		ImU32 fill_col = IM_COL32(90, 120, 255, 255);

		draw_list->AddRectFilled(bb.Min, bb.Max, bg_col, 0.0f);
		draw_list->AddRectFilled(bb.Min, ImVec2(bb.Min.x + filled_width, bb.Max.y), fill_col, 0.0f);
		draw_list->AddRect(bb.Min, bb.Max, IM_COL32(120, 120, 120, 255), 0.0f);

		float handle_x = bb.Min.x + filled_width;
		float handle_size = height + 2.0f;
		ImVec2 handle_min(handle_x - handle_size * 0.5f, bb.Min.y - 1.0f);
		ImVec2 handle_max(handle_x + handle_size * 0.5f, bb.Max.y + 1.0f);
		draw_list->AddRectFilled(handle_min, handle_max, IM_COL32(90, 120, 255, 255), 0.0f);
		draw_list->AddRect(handle_min, handle_max, IM_COL32(255, 255, 255, 255), 0.0f);

		char buf[32];
		snprintf(buf, sizeof(buf), "%.1f", *v);
		ImVec2 val_text_size = ImGui::CalcTextSize(buf);
		float value_x = bb.Max.x + 12.0f;
		float value_y = bb.Min.y + (height * 0.5f) - (val_text_size.y * 0.5f);
		draw_list->AddText(ImVec2(value_x, value_y), IM_COL32(255, 255, 255, 255), buf);

		return held;
	}

	bool slider_int(const char* label, int* v, int v_min, int v_max, float width = 150.0f, float height = 6.0f)
	{
		float float_val = static_cast<float>(*v);
		bool changed = slider_float(label, &float_val, static_cast<float>(v_min), static_cast<float>(v_max));
		*v = static_cast<int>(float_val + 0.5f);
		return changed;
	}

	bool check_box(const char* label, bool* v, float boxSize = 20.0f)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *ImGui::GetCurrentContext();
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);

		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImRect box_bb(pos, ImVec2(pos.x + boxSize, pos.y + boxSize));
		ImGui::ItemSize(box_bb);
		if (!ImGui::ItemAdd(box_bb, id))
			return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(box_bb, id, &hovered, &held);
		if (pressed)
			*v = !*v;

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		ImU32 border_col = hovered ? IM_COL32(90, 120, 255, 255) : IM_COL32(80, 80, 80, 255);
		ImU32 fill_col = *v ? IM_COL32(90, 120, 255, 255) : IM_COL32(30, 30, 30, 255);

		draw_list->AddRectFilled(box_bb.Min, box_bb.Max, fill_col, 0.0f);
		draw_list->AddRect(box_bb.Min, box_bb.Max, border_col, 0.0f, 0, 2.0f);

		if (*v) {
			float pad = boxSize * 0.25f;
			ImVec2 p1 = ImVec2(box_bb.Min.x + pad, box_bb.Min.y + boxSize * 0.55f);
			ImVec2 p2 = ImVec2(box_bb.Min.x + boxSize * 0.45f, box_bb.Max.y - pad);
			ImVec2 p3 = ImVec2(box_bb.Max.x - pad, box_bb.Min.y + pad);
			draw_list->AddLine(p1, p2, IM_COL32(255, 255, 255, 255), 2.5f);
			draw_list->AddLine(p2, p3, IM_COL32(255, 255, 255, 255), 2.5f);
		}

		if (label && label[0] != '\0') {
			ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
			ImGui::TextUnformatted(label);
		}

		return pressed;
	}

	bool color_picker(const char* label, float color[3], float box_size = 16.0f)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		const ImGuiID id = window->GetID(label);
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		ImVec2 box_min = pos;
		ImVec2 box_max = ImVec2(pos.x + box_size, pos.y + box_size);

		ImRect bb(box_min, box_max);
		ImGui::ItemSize(bb);
		if (!ImGui::ItemAdd(bb, id))
			return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
		bool changed = false;

		ImU32 col = IM_COL32(
			(int)(color[0] * 255),
			(int)(color[1] * 255),
			(int)(color[2] * 255),
			255
		);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(box_min, box_max, col, 3.0f);
		draw_list->AddRect(box_min, box_max, hovered ? IM_COL32(90, 120, 255, 255) : IM_COL32(100, 100, 100, 255), 3.0f);

		ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
		ImGui::TextUnformatted(label);

		if (pressed)
			ImGui::OpenPopup(label);

		if (ImGui::BeginPopup(label)) {
			changed |= ImGui::ColorPicker3("##picker", color,
				ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_NoInputs);
			ImGui::EndPopup();
		}

		return changed;
	}

	bool combo_box(const char* label, int* current_item, const char* const items[], int items_count, float width = 150.0f)
	{
		ImVec4 blue = ImVec4(0.353f, 0.471f, 1.0f, 1.0f);

		ImGui::GetStyle().Colors[ImGuiCol_Header] = blue;
		ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered] = ImVec4(0.4f, 0.52f, 1.0f, 1.0f);
		ImGui::GetStyle().Colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.57f, 1.0f, 1.0f);
		ImGui::GetStyle().Colors[ImGuiCol_Button] = blue;
		ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4f, 0.52f, 1.0f, 1.0f);
		ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.57f, 1.0f, 1.0f);
		ImGui::GetStyle().Colors[ImGuiCol_FrameBgHovered] = blue;
		ImGui::GetStyle().Colors[ImGuiCol_FrameBgActive] = ImVec4(0.45f, 0.57f, 1.0f, 1.0f);

		if (label && label[0] != '\0') {
			ImGui::TextUnformatted(label);
		}

		ImGui::SetNextItemWidth(width);
		bool changed = false;
		if (ImGui::BeginCombo("##combo", items[*current_item])) {
			for (int i = 0; i < items_count; ++i) {
				bool is_selected = (i == *current_item);
				if (ImGui::Selectable(items[i], is_selected)) {
					*current_item = i;
					changed = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		return changed;
	}


}

namespace draw {


	void AddHexagon(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& p5, const ImVec2& p6, ImU32 col, float thickness)
	{
		if ((col & IM_COL32_A_MASK) == 0)
			return;

		ImGui::GetBackgroundDrawList()->PathLineTo(p1);
		ImGui::GetBackgroundDrawList()->PathLineTo(p2);
		ImGui::GetBackgroundDrawList()->PathLineTo(p3);
		ImGui::GetBackgroundDrawList()->PathLineTo(p4);
		ImGui::GetBackgroundDrawList()->PathLineTo(p5);
		ImGui::GetBackgroundDrawList()->PathLineTo(p6);

		ImGui::GetBackgroundDrawList()->PathStroke(col, true, thickness);
	}

	void AddHexagonFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& p5, const ImVec2& p6, ImU32 col)
	{
		if ((col & IM_COL32_A_MASK) == 0)
			return;

		ImGui::GetBackgroundDrawList()->PathLineTo(p1);
		ImGui::GetBackgroundDrawList()->PathLineTo(p2);
		ImGui::GetBackgroundDrawList()->PathLineTo(p3);
		ImGui::GetBackgroundDrawList()->PathLineTo(p4);
		ImGui::GetBackgroundDrawList()->PathLineTo(p5);
		ImGui::GetBackgroundDrawList()->PathLineTo(p6);
		ImGui::GetBackgroundDrawList()->PathFillConvex(col);
	}

	void DrawQuadFilled(ImVec2 p1, ImVec2 p2, ImVec2 p3, ImVec2 p4, ImColor color) {
		ImGui::GetBackgroundDrawList()->AddQuadFilled(p1, p2, p3, p4, color);
	}

	void DrawHexagon(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& p5, const ImVec2& p6, ImU32 col, float thickness)
	{
		AddHexagon(p1, p2, p3, p4, p5, p6, col, thickness);
	}

	void DrawHexagonFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& p5, const ImVec2& p6, ImU32 col)
	{
		AddHexagonFilled(p1, p2, p3, p4, p5, p6, col);
	}



	void DrawRectFilled(const ImVec2& min, const ImVec2& max, ImColor color) {
		ImDrawList* drawList = ImGui::GetBackgroundDrawList(); 
		drawList->AddRectFilled(min, max, color);
	}
	ImColor GetShieldColor(int armorType) {
		switch (armorType) {
		case 1: return ImColor(247, 247, 247);
		case 2: return ImColor(39, 178, 255);
		case 3: return ImColor(206, 59, 255);
		case 4: return ImColor(255, 255, 79);
		case 5: return ImColor(219, 2, 2);
		default: return ImColor(247, 247, 247);
		}
	}
	void SeerHealth(ImDrawList* dl, float x, float y, int shield, int max_shield, int armorType, int health,
		float width = 80.0f, float height = 14.0f)
	{
		const float max_health = 100.0f;
		const float shield_step = 25.0f;
		const int segments = 5;
		const float rounding = 5.0f;
		const float padding = 3.0f;

		float shield_height = height * 0.35f;
		float health_height = height - shield_height - padding * 0.5f;

		// Background with subtle outer glow (larger transparent rect)
		ImVec2 bg_min(x - width / 2 - 2, y - height - 2);
		ImVec2 bg_max(x + width / 2 + 2, y + 2);
		dl->AddRectFilled(bg_min, bg_max, IM_COL32(50, 50, 50, 40), rounding + 2);

		// Main background (dark, rounded)
		ImVec2 main_bg_min(x - width / 2, y - height);
		ImVec2 main_bg_max(x + width / 2, y);
		dl->AddRectFilled(main_bg_min, main_bg_max, IM_COL32(15, 15, 15, 230), rounding);

		// Health background (darker blue)
		ImVec2 health_bg_min(main_bg_min.x + padding, main_bg_min.y + padding + shield_height);
		ImVec2 health_bg_max(main_bg_max.x - padding, main_bg_max.y - padding);
		dl->AddRectFilled(health_bg_min, health_bg_max, IM_COL32(25, 30, 60, 180), rounding);

		// Health fill with smooth gradient color
		float health_ratio = ImClamp((float)health / max_health, 0.0f, 1.0f);
		float health_width = health_ratio * (width - 2 * padding);
		if (health_width > 0.0f) {
			ImVec2 health_min(health_bg_min.x, health_bg_min.y);
			ImVec2 health_max(health_bg_min.x + health_width, health_bg_max.y);

			// Smooth color blend from red (low) → yellow (mid) → green (high)
			ImU32 health_color_start = IM_COL32(255, 60, 60, 255);
			ImU32 health_color_mid = IM_COL32(255, 215, 100, 255);
			ImU32 health_color_end = IM_COL32(80, 220, 100, 255);

			// Calculate interpolation factor for mid color point
			float mid_point = 0.5f;
			ImU32 col_left, col_right;
			if (health_ratio < mid_point) {
				float t = health_ratio / mid_point;
				// lerp red → yellow
				col_left = health_color_start;
				col_right = IM_COL32(
					(int)(255 * (1 - t) + 255 * t),
					(int)(60 * (1 - t) + 215 * t),
					(int)(60 * (1 - t) + 100 * t),
					255
				);
			}
			else {
				float t = (health_ratio - mid_point) / (1 - mid_point);
				// lerp yellow → green
				col_left = IM_COL32(
					(int)(255 * (1 - t) + 80 * t),
					(int)(215 * (1 - t) + 220 * t),
					(int)(100 * (1 - t) + 100 * t),
					255
				);
				col_right = health_color_end;
			}

			dl->AddRectFilledMultiColor(health_min, health_max, col_left, col_right, col_right, col_left);

			// Inner shadow (dark fade at top)
			ImU32 shadow_col = IM_COL32(0, 0, 0, 90);
			ImVec2 shadow_min = health_min;
			ImVec2 shadow_max = ImVec2(health_max.x, health_max.y - (health_bg_max.y - health_bg_min.y) * 0.5f);
			dl->AddRectFilled(shadow_min, shadow_max, shadow_col, rounding);
		}

		// Glossy highlight gradient (top fade)
		ImVec2 highlight_min = ImVec2(health_bg_min.x, health_bg_min.y);
		ImVec2 highlight_max = ImVec2(health_bg_min.x + health_width, health_bg_min.y + (health_bg_max.y - health_bg_min.y) * 0.4f);
		dl->AddRectFilledMultiColor(
			highlight_min,
			highlight_max,
			IM_COL32(255, 255, 255, 40),
			IM_COL32(255, 255, 255, 0),
			IM_COL32(255, 255, 255, 0),
			IM_COL32(255, 255, 255, 40)
		);

		// Shield cracked background color
		ImU32 shieldCracked = IM_COL32(50, 50, 50, 220);

		// Shield color
		ImU32 shieldCol = GetShieldColor(armorType);

		int shieldRemaining = shield;
		float gap = 1.5f;
		float segmentWidth = (width - 2 * padding - (segments - 1) * gap) / segments;

		// Draw shield segments with subtle gaps
		for (int i = 0; i < segments; ++i) {
			ImVec2 seg_min(health_bg_min.x + i * (segmentWidth + gap), health_bg_min.y - shield_height - padding * 0.5f);
			ImVec2 seg_max(seg_min.x + segmentWidth, seg_min.y + shield_height);

			dl->AddRectFilled(seg_min, seg_max, shieldCracked, rounding);

			if (shieldRemaining > 0) {
				float shieldWidth = (float)min(shieldRemaining, 25) / shield_step * segmentWidth;
				ImVec2 shield_max(seg_min.x + shieldWidth, seg_max.y);
				dl->AddRectFilled(seg_min, shield_max, shieldCol, rounding);

				// Small shine reflection line on shield segment (top)
				ImVec2 shine_start(seg_min.x + 2, seg_min.y + 2);
				ImVec2 shine_end(seg_min.x + shieldWidth - 2, seg_min.y + 4);
				dl->AddLine(shine_start, shine_end, IM_COL32(255, 255, 255, 60), 1.0f);
			}

			shieldRemaining -= 25;
		}

		// Outline main bar
		dl->AddRect(main_bg_min, main_bg_max, IM_COL32(255, 255, 255, 60), rounding, 0, 1.25f);
	}

	void DrawTextWithOutline(
		ImDrawList* drawList,
		ImFont* font,
		float fontSize,
		ImVec2 pos,
		ImU32 textColor,
		ImU32 outlineColor,
		const char* text,
		bool outline = true,
		float thickness = 1.0f,
		bool shadow = false
	) {
		if (outline) {
			// Draw outline by rendering text around the main text in 8 directions
			const float offs = thickness;
			drawList->AddText(font, fontSize, ImVec2(pos.x - offs, pos.y), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x + offs, pos.y), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x, pos.y - offs), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x, pos.y + offs), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x - offs, pos.y - offs), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x + offs, pos.y - offs), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x - offs, pos.y + offs), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x + offs, pos.y + offs), outlineColor, text);
		}

		if (shadow) {
			// Optional subtle shadow below text
			constexpr float shadowOffset = 1.5f;
			ImU32 shadowColor = IM_COL32(0, 0, 0, 100);
			drawList->AddText(font, fontSize, ImVec2(pos.x + shadowOffset, pos.y + shadowOffset), shadowColor, text);
		}

		// Draw main text on top
		drawList->AddText(font, fontSize, pos, textColor, text);
	}

	void DrawTextitemesp(
		ImDrawList* drawList,
		ImFont* font,
		float fontSize,
		ImVec2 pos,
		ImU32 textColor,
		ImU32 outlineColor,
		const char* text,
		bool outline = true,
		float thickness = 1.0f,
		bool shadow = false,
		bool drawBackground = true,           // New param: whether to draw background
		ImU32 backgroundColor = IM_COL32(0, 0, 0, 150), // Semi-transparent black
		float backgroundRounding = 4.0f,
		ImVec2 padding = ImVec2(4, 2)         // Padding around text inside bg
	) {
		// Calculate text size
		ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);

		if (drawBackground) {
			// Draw rounded rectangle background behind text
			ImVec2 bgMin = ImVec2(pos.x - padding.x, pos.y - padding.y);
			ImVec2 bgMax = ImVec2(pos.x + textSize.x + padding.x, pos.y + textSize.y + padding.y);
			drawList->AddRectFilled(bgMin, bgMax, backgroundColor, backgroundRounding);
		}

		if (outline) {
			// Draw outline by rendering text around the main text in 8 directions
			const float offs = thickness;
			drawList->AddText(font, fontSize, ImVec2(pos.x - offs, pos.y), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x + offs, pos.y), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x, pos.y - offs), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x, pos.y + offs), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x - offs, pos.y - offs), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x + offs, pos.y - offs), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x - offs, pos.y + offs), outlineColor, text);
			drawList->AddText(font, fontSize, ImVec2(pos.x + offs, pos.y + offs), outlineColor, text);
		}

		if (shadow) {
			// Optional subtle shadow below text
			constexpr float shadowOffset = 1.5f;
			ImU32 shadowColor = IM_COL32(0, 0, 0, 100);
			drawList->AddText(font, fontSize, ImVec2(pos.x + shadowOffset, pos.y + shadowOffset), shadowColor, text);
		}

		// Draw main text on top
		drawList->AddText(font, fontSize, pos, textColor, text);
	}



	void DrawRotatedImageWithBoxColor(ImTextureID textureID, float x, float y, float width, float height, ImColor tintColor)
	{
		ImVec2 center = ImVec2(x + width * 0.5f, y + height * 0.5f);

		float angle = 3.14159265359f;

		ImVec2 rotationMatrix[2] = {
			ImVec2(cos(angle), -sin(angle)),
			ImVec2(sin(angle), cos(angle))
		};

		auto calculateRotatedCoordinates = [&](ImVec2 point) -> ImVec2 {
			return ImVec2(
				center.x + rotationMatrix[0].x * (point.x - center.x) + rotationMatrix[0].y * (point.y - center.y),
				center.y + rotationMatrix[1].x * (point.x - center.x) + rotationMatrix[1].y * (point.y - center.y)
			);
			};

		ImVec2 topLeft = ImVec2(x, y);
		ImVec2 bottomRight = ImVec2(x + width, y + height);

		ImVec2 rotatedTopLeft = calculateRotatedCoordinates(topLeft);
		ImVec2 rotatedBottomRight = calculateRotatedCoordinates(bottomRight);

		ImVec2 offset[8] = {
		ImVec2(-1, -1), ImVec2(1, -1), ImVec2(-1, 1), ImVec2(1, 1),
		ImVec2(-1, 0), ImVec2(1, 0), ImVec2(0, -1), ImVec2(0, 1)
		};

		for (int i = 0; i < 8; i++) {
			ImVec2 outlineTopLeft = ImVec2(rotatedTopLeft.x + offset[i].x, rotatedTopLeft.y + offset[i].y);
			ImVec2 outlineBottomRight = ImVec2(rotatedBottomRight.x + offset[i].x, rotatedBottomRight.y + offset[i].y);
			ImGui::GetBackgroundDrawList()->AddImage(textureID, outlineTopLeft, outlineBottomRight, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(0, 0, 0, 255));
		}
		ImGui::GetBackgroundDrawList()->AddImage(textureID, rotatedTopLeft, rotatedBottomRight, ImVec2(0, 0), ImVec2(1, 1), ImGui::ColorConvertFloat4ToU32(tintColor));

	}

	void draw_normal_box(float x, float y, float w, float h, ImU32 color, float thickness = 1.0f)
	{
		auto* draw = ImGui::GetBackgroundDrawList();

		// Draw a simple rectangle outline
		draw->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color, 0.0f, 0, thickness);
	}

	void draw_text(float x, float y, ImColor color, const char* string, ...) {
		char buf[512];
		va_list arg_list;

		ZeroMemory(buf, sizeof(buf));

		va_start(arg_list, string);
		vsnprintf(buf, sizeof(buf), string, arg_list);
		va_end(arg_list);

		ImGui::GetBackgroundDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(x, y), color, buf, 0, 0, 0);
		return;
	}

	void draw_line(float x, float y, float x2, float y2, ImColor color, float thickness) {
		if (x < 0)
			return;

		if (y < 0)
			return;

		if (x2 > (float)Width)
			return;

		if (y2 > (float)Height)
			return;

		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(x, y), ImVec2(x2, y2), color, thickness);
		return;
	}

	void draw_rectangle(float x, float y, float x2, float y2, ImColor color, float thickness) {
		ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + x2, y + y2), color, 0, -1, thickness);
		return;
	}

	void draw_cornered_box(int x, int y, int w, int h, const ImColor color, int thickness)
	{

		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x, y), ImVec2(x, y + (h / 3)), ImColor(0, 0, 0), (thickness * 3));
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x, y), ImVec2(x + (w / 3), y), ImColor(0, 0, 0), (thickness * 3));
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x + w - (w / 3), y), ImVec2(x + w, y), ImColor(0, 0, 0), (thickness * 3));
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x + w, y), ImVec2(x + w, y + (h / 3)), ImColor(0, 0, 0), (thickness * 3));
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x, y + h - (h / 3)), ImVec2(x, y + h), ImColor(0, 0, 0), (thickness * 3));
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x, y + h), ImVec2(x + (w / 3), y + h), ImColor(0, 0, 0), (thickness * 3));
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x + w - (w / 3), y + h), ImVec2(x + w, y + h), ImColor(0, 0, 0), (thickness * 3));
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x + w, y + h - (h / 3)), ImVec2(x + w, y + h), ImColor(0, 0, 0), (thickness * 3));

		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x, y), ImVec2(x, y + (h / 3)), color, thickness);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x, y), ImVec2(x + (w / 3), y), color, thickness);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x + w - (w / 3), y), ImVec2(x + w, y), color, thickness);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x + w, y), ImVec2(x + w, y + (h / 3)), color, thickness);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x, y + h - (h / 3)), ImVec2(x, y + h), color, thickness);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x, y + h), ImVec2(x + (w / 3), y + h), color, thickness);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x + w - (w / 3), y + h), ImVec2(x + w, y + h), color, thickness);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(x + w, y + h - (h / 3)), ImVec2(x + w, y + h), color, thickness);
	}

	void draw_rectangle_filled(float x, float y, float x2, float y2, ImColor color) {
		ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + x2, y + y2), color);
		return;
	}

	void draw_rectangle_filled_multicolor(float x, float y, float x2, float y2, ImColor color, ImColor color2, ImColor color3, ImColor color4) {
		ImGui::GetBackgroundDrawList()->AddRectFilledMultiColor(ImVec2(x, y), ImVec2(x + x2, y + y2), color, color2, color3, color4);
		return;
	}

	void draw_corner_box(float x, float y, float x2, float y2, ImColor color, float thickness) {
		float box_width = x2 - x;
		float box_height = y2 - y;

		draw_line(x, y, x + box_width / 4, y, color, thickness);
		draw_line(x2, y, x2 - box_width / 4, y, color, thickness);

		draw_line(x, y, x, y + box_height / 3, color, thickness);
		draw_line(x, y2, x, y2 - box_height / 3, color, thickness);

		draw_line(x2, y, x2, y + box_height / 3, color, thickness);
		draw_line(x2, y2, x2, y2 - box_height / 3, color, thickness);

		draw_line(x, y2, x + box_width / 4, y2, color, thickness);
		draw_line(x2, y2, x2 - box_width / 4, y2, color, thickness);

		return;
	}

	void draw_circle(float x, float y, ImColor color, float radius) {
		ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(x, y), radius, color);
		return;
	}

	void draw_filled_circle(float x, float y, ImColor color, float radius) {
		ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(x, y), radius, color);
		return;
	}

	inline void DrawModernNametag(float x, float y, const char* name)
	{
		ImDrawList* drawList = ImGui::GetBackgroundDrawList();
		ImFont* font = ImGui::GetFont();
		float fontSize = ImGui::GetFontSize();

		ImVec2 textSize = ImGui::CalcTextSize(name);
		const float paddingX = 8.0f;
		const float paddingY = 4.0f;
		const float rounding = 2.0f;

		ImVec2 boxMin(x - textSize.x / 2.0f - paddingX, y - textSize.y / 2.0f - paddingY);
		ImVec2 boxMax(x + textSize.x / 2.0f + paddingX, y + textSize.y / 2.0f + paddingY);

		ImVec2 shadowMin = ImVec2(boxMin.x - 2, boxMin.y - 2);
		ImVec2 shadowMax = ImVec2(boxMax.x + 2, boxMax.y + 2);

		// 🔲 Drop Shadow (soft)
		drawList->AddRectFilled(shadowMin, shadowMax, IM_COL32(0, 0, 0, 60), rounding);

		// 🔳 Gradient Background
		drawList->AddRectFilledMultiColor(
			boxMin, boxMax,
			IM_COL32(40, 40, 50, 200),  // Top-left
			IM_COL32(40, 40, 50, 200),  // Top-right
			IM_COL32(20, 20, 25, 200),  // Bottom-right
			IM_COL32(20, 20, 25, 200)
		);

		// ✨ Inner highlight (top gloss)
		ImVec2 glossMax = ImVec2(boxMax.x, boxMin.y + (boxMax.y - boxMin.y) * 0.4f);
		drawList->AddRectFilled(boxMin, glossMax, IM_COL32(255, 255, 255, 15), rounding);

		// 🔲 Border
		drawList->AddRect(boxMin, boxMax, IM_COL32(255, 255, 255, 60), rounding, 0, 1.2f);

		// 🅰️ Text (centered)
		drawList->AddText(font, fontSize, ImVec2(x - textSize.x / 2.0f, y - textSize.y / 2.0f), IM_COL32(255, 255, 255, 240), name);
	}


	inline void draw_arrow(const ImVec2& pos, float angle, float size, ImColor color) {
		ImDrawList* draw_list = ImGui::GetForegroundDrawList();

		ImVec2 tip = ImVec2(
			pos.x + cosf(angle) * size,
			pos.y + sinf(angle) * size
		);
		ImVec2 left = ImVec2(
			pos.x + cosf(angle + 2.5f) * (size * 0.5f),
			pos.y + sinf(angle + 2.5f) * (size * 0.5f)
		);
		ImVec2 right = ImVec2(
			pos.x + cosf(angle - 2.5f) * (size * 0.5f),
			pos.y + sinf(angle - 2.5f) * (size * 0.5f)
		);

		draw_list->AddTriangleFilled(tip, left, right, color);
		draw_list->AddTriangle(tip, left, right, ImColor(0, 0, 0, 255), 2.0f);
	}
}