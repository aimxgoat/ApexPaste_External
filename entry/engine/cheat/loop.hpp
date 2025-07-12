#include <chrono>
#include <mutex>
#include "../game/render.h"
#include "../game/sdk.hpp"
#include "../game/keys.h"
#include "../../protection/import.h"
#include "../../protection/oxorany_include.h"
#include <set>
#include "config.h"
#include <string>
#include <vector>
#include "../../imgui/imgui_impl_dx11.h"


static enum AimPart
{
	HEAD,
	NECK,
	CHEST,
	PELVIS
};
AimPart aimpart = HEAD;

typedef struct _Entity_Loot {
	c_player player;
	int distance;
	int index;
	std::string ballsack;
} Entity_Loot;

typedef struct _Entity {
	c_player player;
	int distance;
	std::string name;
	std::string op;
	int health;
	int shield;
	int visible;
	std::string weapon;
	bool is_visible = false;

} Entity;

std::vector<Entity> tempList;
std::vector<Entity_Loot> tempList_loot;
Entity AimbotPlayer;
std::vector<Entity> EntityList;
std::vector<Entity_Loot> EntityList_Loot;

static c_player* lockedAimbotTarget = nullptr;
static int lockedAimbotIndex = -1;

struct GamePointers {
	c_player local_player;
	int size;
}; static GamePointers* Pointers = new GamePointers();

std::set<std::string> unique_signifiers;

void pointer_reading_work() {
	while (true) {
		try {
			auto LevelName = mem::readString(baseAddress + off::lastVisibleTime);

			Pointers->local_player = c_player(mem::Read1<DWORD_PTR>(baseAddress + off::localPlayer));

			if (LevelName.find("mp_rr_canyonlands_staging_mu1") != std::string::npos)
				Pointers->size = 20000;
			else
				Pointers->size = 20000;
		}
		catch (const std::exception& e) {
			printf("[PointerReader] Exception: %s\n", e.what());
		}
		catch (...) {
			printf("[PointerReader] Unknown exception\n");
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
}

struct HighlightFunctionBits
{
	uint8_t functionBits[4];
};

struct HighlightParameter
{
	float parameter[3];
};


namespace weapon
{

	int index = 0;
	int WeaponGlowIndex = 0;
}


void weapon_glow()
{
	if (settings::weapon_glow && Pointers->local_player.valid()) {
		uint64_t actWeaponID = mem::Read1<uint64_t>(Pointers->local_player.get_address() + off::viewModel) & 0xFFFF;
		if (!actWeaponID) return;
		uint64_t currentWeapon = mem::Read1<uint64_t>(baseAddress + off::entityList + (actWeaponID * 0x20));
		if (!currentWeapon) return;
		mem::write<uint8_t>(currentWeapon + off::glow_enable,
			1);
		mem::write<uint8_t>(currentWeapon + off::glow_highlight_id,
			weapon::index);

		if (0 == 5 || 0 == 6)
		{
			auto highlightsettings = mem::Read1<uint64_t>(baseAddress + off::glow_highlights);
			
			static int cases = 0;
			static float r = 1.00f, g = 0.00f, b = 1.00f;
			switch (cases)
			{
			case 0: { r -= 0.003f; if (r <= 0) cases += 1; break; }
			case 1: { g += 0.003f; b -= 0.003f; if (g >= 1) cases += 1; break; }
			case 2: { r += 0.003f; if (r >= 1) cases += 1; break; }
			case 3: { b += 0.003f; g -= 0.003f; if (b >= 1) cases = 0; break; }
			default: { r = 1.00f; g = 0.00f; b = 1.00f; break; }
			}
			mem::write<HighlightFunctionBits>(highlightsettings + (0x38 * 46) + 0, { 137,138,35,127 });
			mem::write<HighlightParameter>(highlightsettings + 0x34 * uint64_t(weapon::index) + 8, { r / 4.25f,g / 4.25f,b / 4.25f });
		}
	}
}

void player_glow(Entity* entity, std::unordered_map<uintptr_t, int>& glow_map)
{
	if (!settings::glow || !Pointers->local_player.valid())
		return;
	uintptr_t entityAddress = entity->player.get_address();
	int desiredGlowType = settings::glowtype;

	if (glow_map[entityAddress] == desiredGlowType)
		return;

	mem::write<int>(entityAddress + off::glow_enable, 1);
	mem::write<int>(entityAddress + off::glow_highlight_id, desiredGlowType);
	mem::write<int>(entityAddress + off::glow_throuh_wall, 2);

	mem::write<int>(entityAddress + off::glow_fix, 300.0f);
	mem::write<uint8_t>(entityAddress + off::glow_fix, 2);

	glow_map[entityAddress] = desiredGlowType;
}

void stop_glow(uintptr_t entity)
{
	mem::write<int>(entity + off::glow_highlight_id, 0);
	mem::write<int>(entity + off::glow_throuh_wall, 0);
	mem::write<float>(entity + off::glow_dist, 0.f);
	uint64_t settings = mem::Read1<uint64_t>(entity + off::glow_highlights);
	mem::write<int>(entity + off::glow_highlight_id + 0, 0);
}

namespace debugger
{
	bool yes = false;
}

std::mutex EntityListMutex;

void player_reading_work() {
	while (true) {
		Pointers->local_player = c_player(mem::Read1<DWORD_PTR>(baseAddress + off::localPlayer));

		tempList.clear();
		tempList_loot.clear();

		Pointers->size = 20000;

		tempList.reserve(Pointers->size);
		tempList_loot.reserve(Pointers->size);


		for (int i = 0; i <= Pointers->size; i++) {
			const auto entity_addr = mem::Read1<DWORD_PTR>(baseAddress + off::entityList + (i * 0x20));
			if (!entity_addr) continue;

			c_player player(entity_addr);
			if (player.get_address() == 0 || player.get_address() == Pointers->local_player.get_address())
				continue;

			const auto signifier = class_signifier(entity_addr);

			if (debugger::yes && unique_signifiers.insert(signifier).second)
				printf("[Signifier] %s\n", signifier.c_str());

			if (signifier.find("prop_survival") != std::string::npos || signifier.find("prop_death_box") != std::string::npos) {
				const float distance = player.distance() / 100.0f;
				if (distance > 20.0f) continue;

				const int index = mem::Read1<int>(entity_addr + 0x15d4); // m_customScriptInt
				tempList_loot.emplace_back(Entity_Loot{
					.player = player,
					.distance = static_cast<int>(distance),
					.index = index,
					.ballsack = signifier
					});
				continue;
			}

			if (signifier.find("player_vehicle") != std::string::npos) continue;
			if (signifier.find("player") == std::string::npos && signifier.find("npc_dummie") == std::string::npos)
				continue;

			const float health = player.i_health();
			if (health <= 0.1f) continue;
			if (player.i_team() == Pointers->local_player.i_team()) continue;

			const float distance = player.distance() / 100.0f;
			if (distance > 200.0f) continue;

			tempList.emplace_back(Entity{
				.player = player,
				.distance = static_cast<int>(distance),
				.name = player.s_name(),
				.op = player.GetOperator(),
				.health = static_cast<int>(health),
				.shield = player.i_shield(),
				.visible = i ,
				.weapon = get_weapon_name(entity_addr),
				});
		}

		{
			std::scoped_lock lock(EntityListMutex);
			EntityList.swap(tempList);
			EntityList_Loot.swap(tempList_loot);
		}
		//gurt: sybau
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
}

struct Point {
	float x, y;
	//gurt: sybau
};

Point deCasteljau(const std::vector<Point>& controls, float t) {
	std::vector<Point> points = controls;
	for (size_t i = 1; i < controls.size(); ++i) {
		for (size_t j = 0; j < controls.size() - i; ++j) {
			points[j].x = (1 - t) * points[j].x + t * points[j + 1].x;
			points[j].y = (1 - t) * points[j].y + t * points[j + 1].y;
		}
	}
	return points[0];
}

std::vector<Point> generateControlPoints(Point start, Point end, int numPoints) {
	std::vector<Point> controls;
	controls.push_back(start);

	float dx = end.x - start.x;
	float dy = end.y - start.y;
	float distance = std::hypot(dx, dy);

	float archSign = ((dx >= 0) ? 1.0f : -1.0f) * ((dy >= 0) ? 1.0f : -1.0f);

	for (int i = 1; i < numPoints - 1; ++i) {
		float t = static_cast<float>(i) / (numPoints - 1);
		Point p{
			start.x + dx * t,
			start.y + dy * t
		};

		float perpX = dy;
		float perpY = -dx;
		float norm = std::hypot(perpX, perpY);
		if (norm != 0) {
			perpX /= norm;
			perpY /= norm;
		}

		float offset = std::sin(t * 3.14159f) * distance * 0.1f * archSign;
		p.x += perpX * offset;
		p.y += perpY * offset;

		controls.push_back(p);
	}

	controls.push_back(end);
	return controls;
}

void draw_aimbot_curve(const std::vector<Point>& controls) {
	ImDrawList* draw_list = ImGui::GetForegroundDrawList();
	const int segments = 50;
	Point prev = deCasteljau(controls, 0.0f);
	for (int i = 1; i <= segments; ++i) {
		float t = i / (float)segments;
		Point curr = deCasteljau(controls, t);
		//draw_list->AddLine(ImVec2(prev.x, prev.y), ImVec2(curr.x, curr.y), ImColor(0, 0, 0), 3.5f);
		//draw_list->AddLine(ImVec2(prev.x, prev.y), ImVec2(curr.x, curr.y), ImColor(settings::target_color[0], settings::target_color[1], settings::target_color[2]), 1);
		prev = curr;
	}
}

static std::vector<Point> last_aimbot_curve;

void aimbot(int x, int y) {
	static float t = 0.0f;
	static std::vector<Point> controls;
	static Point prevTarget{ 0, 0 };
	static Point prevDesired{ 0, 0 };

	float centerX = Width / 2.0f;
	float centerY = Height / 2.0f;
	Point currentTarget{ static_cast<float>(x), static_cast<float>(y) };

	// Reset control points if target changed or completed curve
	if (currentTarget.x != prevTarget.x || currentTarget.y != prevTarget.y || t >= 1.0f) {
		t = 0.0f;
		prevTarget = currentTarget;
		Point start{ centerX, centerY };
		controls = generateControlPoints(start, currentTarget, 30);
		prevDesired = start;
	}

	// Clamp smoothing to avoid div by zero
	float smoothing = (settings::smoothing > 0) ? settings::smoothing : 1.0f;

	// Interpolation step based on smoothing
	float step = 1.0f / smoothing;

	// Calculate the desired next point on the curve
	Point desired = deCasteljau(controls, t);

	// Calculate movement delta from previous frame
	float moveDx = desired.x - prevDesired.x;
	float moveDy = desired.y - prevDesired.y;

	// Move mouse by delta
	mem::MoveMouse(moveDx, moveDy);

	// Update previous position
	prevDesired = desired;

	// Advance t with clamp
	t = min(t + step, 1.0f);
}

void drawmenu()
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 8.0f;
	style.FrameRounding = 6.0f;
	style.WindowBorderSize = 0.0f;
	style.FrameBorderSize = 1.0f;
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 0.95f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
	style.Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.3f, 0.5f, 0.9f, 1.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.23f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.3f, 0.35f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.4f, 0.5f, 1.0f);

	ImGui::SetNextWindowSize(ImVec2(520, 420), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.95f);

	ImGui::Begin("gurt: sybau private", nullptr,
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 win_pos = ImGui::GetWindowPos();
	ImVec2 win_size = ImGui::GetWindowSize();

	// --- Top bar background ---
	float topBarHeight = 36.0f;
	draw_list->AddRectFilled(win_pos, ImVec2(win_pos.x + win_size.x, win_pos.y + topBarHeight), IM_COL32(20, 20, 30, 255), 8.0f);

	// --- Title text ---
	draw_list->AddText(ImVec2(win_pos.x + 16, win_pos.y + 10), IM_COL32(100, 140, 255, 255), "gurt: sybau private");

	// --- Tabs ---
	static int activeTab = 0;
	const char* tabs[] = { "aimbot", "visuals", "colors", "world", "configs" };
	int numTabs = IM_ARRAYSIZE(tabs);

	float tabX = win_pos.x + 180;
	for (int i = 0; i < numTabs; ++i)
	{
		ImVec2 tabTextSize = ImGui::CalcTextSize(tabs[i]);
		ImVec2 tabPos = ImVec2(tabX, win_pos.y + 8);
		ImRect tabRect(tabPos, ImVec2(tabPos.x + tabTextSize.x + 20, tabPos.y + 28));

		// Background for active tab
		ImU32 bgColor = (activeTab == i) ? IM_COL32(70, 90, 200, 220) : IM_COL32(40, 40, 50, 150);
		draw_list->AddRectFilled(tabRect.Min, tabRect.Max, bgColor, 8.0f);

		// Text color with hover highlight
		bool hovered = ImGui::IsMouseHoveringRect(tabRect.Min, tabRect.Max);
		ImU32 textColor = (activeTab == i) ? IM_COL32(230, 230, 230, 255) : (hovered ? IM_COL32(180, 180, 220, 255) : IM_COL32(150, 150, 160, 255));

		draw_list->AddText(ImVec2(tabPos.x + 10, tabPos.y + 6), textColor, tabs[i]);

		if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			activeTab = i;

		tabX += tabTextSize.x + 36;
	}

	// --- Content area ---
	ImGui::SetCursorScreenPos(ImVec2(win_pos.x + 14, win_pos.y + topBarHeight + 14));
	ImGui::BeginChild("MainContent", ImVec2(win_size.x - 28, win_size.y - topBarHeight - 56), false, ImGuiWindowFlags_NoScrollbar);
	ImGui::Columns(2, nullptr, false);
	ImGui::SetColumnWidth(0, 250);

	if (activeTab == 0) // Aimbot
	{
		ImGui::BeginChild("AimbotLeft", ImVec2(240, 280), true);
		ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.9f, 1.0f), "aimbot");
		ImGui::Separator();
		wiget::check_box("aimbot", &settings::aimbot);
		wiget::check_box("fov outline", &settings::showfov);
		wiget::check_box("vis check", &settings::vischeck);

		const char* parts[] = { "head", "neck", "chest", "pelvis" };
		wiget::combo_box("aim bone", (int*)&aimpart, parts, IM_ARRAYSIZE(parts));
		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::BeginChild("AimbotRight", ImVec2(240, 280), true);
		ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.9f, 1.0f), "settings");
		ImGui::Separator();
		wiget::slider_int("fov", &settings::fov, 0, 500);
		wiget::slider_float("smoothing", &settings::smoothing, 1.1f, 50.0f);
		ImGui::EndChild();
	}
	else if (activeTab == 1) // Visuals
	{
		ImGui::BeginChild("VisualsLeft", ImVec2(240, 280), true);
		ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.9f, 1.0f), "esp");
		ImGui::Separator();

		wiget::check_box("corner box", &settings::box);
		wiget::check_box("2d box", &settings::normal_box);
		wiget::check_box("player name", &settings::name);
		wiget::check_box("health bar", &settings::health_shield);
		wiget::check_box("skeleton", &settings::skeleton);
		wiget::check_box("operator", &settings::op);
		wiget::check_box("weapon", &settings::weapon);
		wiget::check_box("weapon Icon", &settings::weapon_icon);
		wiget::check_box("distance", &settings::distance);
		wiget::check_box("radar", &settings::radar);
		wiget::slider_int("max distance", &settings::playerDistance, 0, 500);

		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::BeginChild("VisualsRight", ImVec2(240, 280), true);
		ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.9f, 1.0f), "other");
		ImGui::Separator();

		wiget::check_box("glow", &settings::glow);
		// Glow Color Combo Box
		{
			static int glowIndex = 0;
			const char* glowColors[] = { "blue", "green", "red", "blue Alt" };
			int glowValues[] = { 26, 16, 76, 83 };
			for (int i = 0; i < 4; ++i) {
				if (settings::glowtype == glowValues[i])
					glowIndex = i;
			}
			if (wiget::combo_box("Glow Color", &glowIndex, glowColors, IM_ARRAYSIZE(glowColors))) {
				settings::glowtype = glowValues[glowIndex];
			}
		}
		wiget::check_box("weapon glow", &settings::weapon_glow);

		// Weapon Glow Color Combo Box
		{
			static int weaponGlowIndex = 0;
			const char* weaponGlowOptions[] = { "green array", "white", "pulse", "solid green", "purple", "red outline", "half orange" };
			int weaponGlowValues[] = { 2, 62, 77, 50, 52, 0, 1 };
			for (int i = 0; i < 7; ++i) {
				if (settings::weapon_glow_index == weaponGlowValues[i])
					weaponGlowIndex = i;
			}
			if (ImGui::Combo("weapon glow color", &weaponGlowIndex, weaponGlowOptions, 7)) {
				settings::weapon_glow_index = weaponGlowValues[weaponGlowIndex];
			}
		}

		ImGui::EndChild();
	}
	else if (activeTab == 2) // Colors
	{
		ImGui::BeginChild("ColorsLeft", ImVec2(240, 280), true);
		ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.9f, 1.0f), "colors");
		ImGui::Separator();

		wiget::color_picker("fov color", settings::fov_color);
		wiget::color_picker("box color, visible", settings::box_color_visible);
		wiget::color_picker("box color, not visible", settings::box_color_not_visible);
		wiget::color_picker("2d box color, visible", settings::normal_box_color_visible);
		wiget::color_picker("2d box color, not visible", settings::normal_box_color_not_visible);
		wiget::color_picker("skeleton color, visible", settings::skel_color_visible);
		wiget::color_picker("skeleton color, not visible", settings::skel_color_not_visible);

		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::BeginChild("ColorsRight", ImVec2(240, 280), true);
		ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.9f, 1.0f), "others");
		ImGui::Separator();

		wiget::color_picker("name color, visible", settings::name_color_visible);
		wiget::color_picker("name color, not visible", settings::name_color_not_visible);
		wiget::color_picker("distance color, visible", settings::distance_color_visible);
		wiget::color_picker("distance color, not visible", settings::distance_color_not_visible);
		wiget::color_picker("operator color, visible", settings::operator_color_visible);
		wiget::color_picker("operator color, not visible", settings::operator_color_not_visible);
		wiget::color_picker("weapon color, visible", settings::weapon_color_visible);
		wiget::color_picker("weapon color, not visible", settings::weapon_color_not_visible);
		wiget::color_picker("weapon icon color, visible", settings::weapon_icon_color_visible);
		wiget::color_picker("weapon icon color, not visible", settings::weapon_icon_color_not_visible);

		ImGui::EndChild();
	}
	else if (activeTab == 3) // World
	{
		ImGui::BeginChild("WorldLeft", ImVec2(240, 280), true);
		ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.9f, 1.0f), "loot");
		ImGui::Separator();

		wiget::check_box("icon", &settings::Icon);
		wiget::check_box("text", &settings::Text);
		wiget::check_box("shield", &settings::Shield);
		wiget::check_box("backPack", &settings::BackPack);
		wiget::check_box("knocked", &settings::Knocked);
		wiget::check_box("helmet", &settings::Helmet);
		wiget::check_box("light ammo", &settings::LightAmmo);
		wiget::check_box("shotgun ammo", &settings::ShotgunAmmo);
		wiget::check_box("sniper ammo", &settings::SniperAmmo);
		wiget::check_box("heavy ammo", &settings::HeavyAmmo);
		wiget::check_box("energy ammo", &settings::EnergyAmmo);
		wiget::check_box("light weapon", &settings::LightWeapon);
		wiget::check_box("sniper", &settings::Sniper);
		wiget::check_box("legendary", &settings::Legendary);
		wiget::check_box("energy", &settings::Energy);
		wiget::check_box("heavy", &settings::Heavy);
		wiget::check_box("shotgun", &settings::Shotgun);
		wiget::check_box("medic", &settings::Medic);

		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::BeginChild("WorldRight", ImVec2(240, 280), true);
		ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.9f, 1.0f), "others");
		ImGui::Separator();

		wiget::slider_int("loot distance", &settings::lootDistance, 0, 100);

		ImGui::EndChild();
	}
	else if (activeTab == 4) // Configs
	{
		static char newConfigName[64] = "";
		static int selectedConfig = -1;
		static std::vector<std::string> configNames;
		static float lastRefresh = 0.0f;
		static char statusMsg[128] = "";
		float now = ImGui::GetTime();

		if (lastRefresh == 0.0f || now - lastRefresh > 1.0f) {
			configNames = config::List();
			lastRefresh = now;
			if (selectedConfig >= (int)configNames.size()) selectedConfig = -1;
		}

		ImGui::BeginChild("ConfigLeft", ImVec2(240, 280), true);
		ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.9f, 1.0f), "config");
		ImGui::Separator();

		std::vector<const char*> configNamesCStr;
		for (const auto& n : configNames)
			configNamesCStr.push_back(n.c_str());

		ImGui::Text("configs:");
		if (!configNamesCStr.empty())
			ImGui::ListBox("##ConfigList", &selectedConfig, configNamesCStr.data(), (int)configNamesCStr.size(), 8);
		else
			ImGui::TextDisabled("No configs.");

		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::BeginChild("ConfigRight", ImVec2(240, 280), true);
		ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.9f, 1.0f), "actions");
		ImGui::Separator();

		if (ImGui::Button("save as new config"))
		{
			int maxCfig = 0;
			for (const auto& n : configNames) {
				if (n.rfind("cfig", 0) == 0) {
					int num = atoi(n.c_str() + 4);
					if (num > maxCfig)
						maxCfig = num;
				}
			}
			int nextCfig = maxCfig + 1;
			snprintf(newConfigName, sizeof(newConfigName), "cfig%d", nextCfig);
			if (config::Save(newConfigName)) {
				snprintf(statusMsg, sizeof(statusMsg), "saved as config: %s", newConfigName);
				lastRefresh = 0.0f;
			}
			else {
				snprintf(statusMsg, sizeof(statusMsg), "failed to save: %s", newConfigName);
			}
		}
		if (ImGui::Button("load selected") && selectedConfig >= 0 && selectedConfig < (int)configNames.size())
		{
			if (config::Load(configNames[selectedConfig])) {
				snprintf(statusMsg, sizeof(statusMsg), "loaded config: %s", configNames[selectedConfig].c_str());
			}
			else {
				snprintf(statusMsg, sizeof(statusMsg), "failed to load config: %s", configNames[selectedConfig].c_str());
			}
		}
		if (ImGui::Button("delete selected") && selectedConfig >= 0 && selectedConfig < (int)configNames.size())
		{
			if (config::Delete(configNames[selectedConfig])) {
				snprintf(statusMsg, sizeof(statusMsg), "deleted config: %s", configNames[selectedConfig].c_str());
				selectedConfig = -1;
				lastRefresh = 0.0f;
			}
			else {
				snprintf(statusMsg, sizeof(statusMsg), "failed to delete config: %s", configNames[selectedConfig].c_str());
			}
		}

		if (strlen(statusMsg) > 0) {
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", statusMsg);
		}

		ImGui::EndChild();
	}

	ImGui::Columns(1);
	ImGui::EndChild();

	// --- Bottom bar ---
	float barHeight = 28.0f;
	ImVec2 barPos = ImVec2(win_pos.x, win_pos.y + win_size.y - barHeight);
	draw_list->AddRectFilled(barPos, ImVec2(barPos.x + win_size.x, barPos.y + barHeight), IM_COL32(20, 20, 30, 255), 8.0f);

	draw_list->AddText(ImVec2(barPos.x + 14, barPos.y + 6), IM_COL32(100, 140, 255, 255), "@vrailn");
	const char* version = "v1.3.0";
	ImVec2 versionSize = ImGui::CalcTextSize(version);
	draw_list->AddText(ImVec2(barPos.x + win_size.x - versionSize.x - 14, barPos.y + 6), IM_COL32(180, 180, 180, 255), version);

	ImGui::End();
}

inline std::vector<c_player> player_list;


bool InRect(double Radius, FVector2D ScreenLocation)
{
	return (Width / 2) >= ((Width / 2) - Radius) && (Width / 2) <= ((Width / 2) + Radius) &&
		ScreenLocation.X >= (ScreenLocation.Y - Radius) && ScreenLocation.Y <= (ScreenLocation.X + Radius);
}

bool InCircle(double Radius, FVector2D  ScreenLocation)
{
	if (InRect(Radius, ScreenLocation))
	{
		double dx = (Width / 2) - ScreenLocation.X; dx *= dx;
		double dy = (Height / 2) - ScreenLocation.Y; dy *= dy;
		return dx + dy <= Radius * Radius;
	} return false;
}

double Vector_Distance2D(const FVector2D& a, const FVector2D& b) {
	double dx = static_cast<double>(a.X) - static_cast<double>(b.X);
	double dy = static_cast<double>(a.Y) - static_cast<double>(b.Y);
	return std::sqrt(dx * dx + dy * dy);
}


bool is_within_screen_bounds(const ImVec2& screen_location) {
	if (screen_location.x > 0 && screen_location.x < Width && screen_location.y > 0 && screen_location.y < Height)
		return true;
	else
		return false;
}



void loop_loot()
{
	if (!settings::Text){
		return;
	}
	if (!Pointers->local_player.valid()) {
		return;
	}



	for (auto& loot_entity : EntityList_Loot) {

		int LootIndex = loot_entity.index;
		auto Pos = loot_entity.player.vec_origin();

		vector3 HeadPos = Pos;
		HeadPos.z += 25;
		float Height = HeadPos.y - Pos.y;
		float Width = Height / 2.0f;
		vector3 Spair = world_to_screen(Pos);

		if (is_within_screen_bounds(ImVec2(Spair.x, Spair.y)))
		{


			if (settings::Medic)
			{
				for (size_t MedicIndex = 0; MedicIndex < MedList.size(); MedicIndex++)
				{
					auto MedicLoot = MedList[MedicIndex];

					if (MedicLoot.first == LootIndex)
					{
						std::string Text = MedicLoot.second;

						ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

						if (settings::Text)
						{
							draw::DrawTextitemesp(ImGui::GetBackgroundDrawList(), ImGui::GetFont(), 13, ImVec2(
								Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
								ImColor(
									settings::MedicaColor[0],
									settings::MedicaColor[1],
									settings::MedicaColor[2], 255.f), ImColor(0, 0, 0),

								Text.c_str(), 1);




						}
					}
				}
			}
			if (settings::LightWeapon)
			{
				for (size_t LightGunIndex = 0; LightGunIndex < LightGunList.size(); LightGunIndex++)
				{
					auto LightGun = LightGunList[LightGunIndex];

					if (LightGun.first == LootIndex)
					{
						std::string Text = LightGun.second;
						ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

						if (settings::Text)
						{
							draw::DrawTextitemesp(
								ImGui::GetBackgroundDrawList(),
								ImGui::GetFont(),
								13,
								ImVec2(Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
								ImColor(255, 178, 102, 255), // Apex-style light orange
								ImColor(0, 0, 0),            // Black outline
								Text.c_str(),
								true
							);
						}
					}
				}
			}

			if (settings::Sniper)
			{
				for (size_t LightGunIndex = 0; LightGunIndex < SniperGunList.size(); LightGunIndex++)
				{
					auto LightGun = SniperGunList[LightGunIndex];

					if (LightGun.first == LootIndex)
					{
						std::string Text = LightGun.second;
						ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

						if (settings::Text)
						{
							draw::DrawTextitemesp(
								ImGui::GetBackgroundDrawList(),
								ImGui::GetFont(),
								13,
								ImVec2(Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
								ImColor(120, 190, 255, 255),  // Light sniper blue
								ImColor(0, 0, 0),  // Black outline
								Text.c_str(),
								true
							);
						}
					}
				}
			}

			if (settings::Legendary)
			{
				for (size_t LightGunIndex = 0; LightGunIndex < LegendaryList.size(); LightGunIndex++)
				{
					auto LightGun = LegendaryList[LightGunIndex];

					if (LightGun.first == LootIndex)
					{
						std::string Text = LightGun.second;

						ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

						if (settings::Text)
						{
							draw::DrawTextitemesp(
								ImGui::GetBackgroundDrawList(),
								ImGui::GetFont(),
								13,
								ImVec2(Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
								ImColor(255, 120, 120, 180),  // Light light red (pastel)
								ImColor(0, 0, 0, 150),        // Soft black outline for subtle contrast
								Text.c_str(),
								1);
						}
					}
				}
			}

			if (settings::Energy)
			{
				for (size_t LightGunIndex = 0; LightGunIndex < EnergyList.size(); LightGunIndex++)
				{
					auto LightGun = EnergyList[LightGunIndex];

					if (LightGun.first == LootIndex)
					{
						std::string Text = LightGun.second;
						ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

						if (settings::Text)
						{
							draw::DrawTextitemesp(
								ImGui::GetBackgroundDrawList(),
								ImGui::GetFont(),
								13.0f,
								ImVec2(Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
								ImColor(130, 255, 100, 255), // Neon green
								ImColor(0, 0, 0),
								Text.c_str(),
								true
							);
						}
					}
				}
			}
			if (settings::Heavy)
			{
				for (size_t LightGunIndex = 0; LightGunIndex < HeavyList.size(); LightGunIndex++)
				{
					auto LightGun = HeavyList[LightGunIndex];

					if (LightGun.first == LootIndex)
					{
						std::string Text = LightGun.second;
						ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

						if (settings::Text)
						{
							draw::DrawTextitemesp(
								ImGui::GetBackgroundDrawList(),
								ImGui::GetFont(),
								13.0f,
								ImVec2(Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
								ImColor(59, 201, 173, 255), // Heavy weapon color (same as Heavy Ammo)
								ImColor(0, 0, 0),
								Text.c_str(),
								true
							);
						}
					}
				}
			}
			if (settings::Shotgun)
			{
				for (size_t ShotgunIndex = 0; ShotgunIndex < ShotGunlist.size(); ShotgunIndex++)
				{
					auto Shotgun = ShotGunlist[ShotgunIndex];

					if (Shotgun.first == LootIndex)
					{
						std::string Text = Shotgun.second;
						ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

						if (settings::Text)
						{
							draw::DrawTextitemesp(
								ImGui::GetBackgroundDrawList(),
								ImGui::GetFont(),
								13,
								ImVec2(Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
								ImColor(255, 80, 80, 255),  // Light red (Apex style)
								ImColor(0, 0, 0),
								Text.c_str(),
								true
							);
						}
					}
				}
			}

			if (settings::Helmet)
			{
				for (size_t ShotgunIndex = 0; ShotgunIndex < HelmetList.size(); ShotgunIndex++)
				{
					auto Shotgun = HelmetList[ShotgunIndex];

					if (Shotgun.first == LootIndex)
					{
						std::string Text = Shotgun.second;

						ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

						if (settings::Text)
						{
							draw::DrawTextitemesp(ImGui::GetBackgroundDrawList(), ImGui::GetFont(), 13, ImVec2(
								Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
								ImColor(
									settings::HelmetColor[0],
									settings::HelmetColor[1],
									settings::HelmetColor[2], 255.f), ImColor(0, 0, 0),
								Text.c_str(), 1);
						}
					}
				}
			}
			if (settings::Shield)
			{
				for (size_t ShotgunIndex = 0; ShotgunIndex < ShieldList.size(); ShotgunIndex++)
				{
					auto Shotgun = ShieldList[ShotgunIndex];

					if (Shotgun.first == LootIndex)
					{
						std::string Text = Shotgun.second;

						ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

						if (settings::Text)
						{
							draw::DrawTextitemesp(ImGui::GetBackgroundDrawList(), ImGui::GetFont(), 13, ImVec2(
								Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
								ImColor(
									settings::ShieldColor[0],
									settings::ShieldColor[1],
									settings::ShieldColor[2], 255.f), ImColor(0, 0, 0),
								Text.c_str(), 1);
						}
					}
				}
			}
			if (settings::BackPack)
			{
				for (size_t ShotgunIndex = 0; ShotgunIndex < BackList.size(); ShotgunIndex++)
				{
					auto Shotgun = BackList[ShotgunIndex];

					if (Shotgun.first == LootIndex)
					{
						std::string Text = Shotgun.second;

						ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

						if (settings::Text)
						{
							draw::DrawTextitemesp(ImGui::GetBackgroundDrawList(), ImGui::GetFont(), 13, ImVec2(
								Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
								ImColor(
									settings::BackpackColor[0],
									settings::BackpackColor[1],
									settings::BackpackColor[2], 255.f), ImColor(0, 0, 0),
								Text.c_str(), 1);
						}
					}
				}
			}
			if (settings::Knocked)
			{
				for (size_t ShotgunIndex = 0; ShotgunIndex < knockedShield.size(); ShotgunIndex++)
				{
					auto Shotgun = knockedShield[ShotgunIndex];

					if (Shotgun.first == LootIndex)
					{
						std::string Text = Shotgun.second;

						ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

						if (settings::Text)
						{
							draw::DrawTextitemesp(ImGui::GetBackgroundDrawList(), ImGui::GetFont(), 13, ImVec2(
								Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
								ImColor(
									settings::knockedShieldColor[0],
									settings::knockedShieldColor[1],
									settings::knockedShieldColor[2], 255.f), ImColor(0, 0, 0),
								Text.c_str(), 1);
						}
					}
				}
			}

			if (settings::EnergyAmmo && LootIndex == 163)
			{
				{
					std::string Text = "Energy Ammo";

					ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

					if (settings::Text)
					{
						draw::DrawTextitemesp(
							ImGui::GetBackgroundDrawList(),
							ImGui::GetFont(),
							13.0f,
							ImVec2(Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
							ImColor(130, 255, 100, 255), // Energy ammo neon green
							ImColor(0, 0, 0),
							Text.c_str(),
							true
						);
					}
				}
			}
			if (settings::ShotgunAmmo && LootIndex == 164)
			{
				std::string Text = "Shotgun Ammo";
				ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

				if (settings::Text)
				{
					ImVec2 textPos = ImVec2(Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f));

					draw::DrawTextitemesp(
						ImGui::GetBackgroundDrawList(),
						ImGui::GetFont(),
						13.0f,
						textPos,
						ImColor(255, 80, 80, 255), // Light red (Apex style)
						ImColor(0, 0, 0),          // Black outline
						Text.c_str(),
						true
					);
				}
			}

			if (settings::LightAmmo && LootIndex == 162)
			{
				std::string Text = "Light Ammo";
				ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

				if (settings::Text)
				{
					ImVec2 textPos = ImVec2(Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f));

					draw::DrawTextitemesp(
						ImGui::GetBackgroundDrawList(),
						ImGui::GetFont(),
						13.0f,
						textPos,
						ImColor(255, 178, 102, 255), // Apex-style light orange
						ImColor(0, 0, 0),            // Outline color
						Text.c_str(),
						true
					);
				}
			}

			if (settings::SniperAmmo && LootIndex == 166)
			{
				std::string Text = "Sniper Ammo";
				ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

				if (settings::Text)
				{
					ImVec2 textPos = ImVec2(Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f));

					draw::DrawTextitemesp(
						ImGui::GetBackgroundDrawList(),
						ImGui::GetFont(),
						13.0f,
						textPos,
						ImColor(120, 190, 255, 255), // Light sniper blue
						ImColor(0, 0, 0),            // Outline
						Text.c_str(),
						true
					);
				}
			}

			if (settings::HeavyAmmo && LootIndex == 165)
			{
				{
					std::string Text = "Heavy Ammo";

					ImVec2 Size = ImGui::CalcTextSize(Text.c_str());
					if (settings::Text)
					{

						draw::DrawTextitemesp(
							ImGui::GetBackgroundDrawList(),
							ImGui::GetFont(),
							13.0f,
							ImVec2(Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
							ImColor(59, 201, 173, 255), // Correct heavy ammo color
							ImColor(0, 0, 0),
							Text.c_str(),
							true
						);
					}
				}

				if (settings::HeavyAmmo && LootIndex == 153)
				{
					{
						std::string Text = "Heavy Ammo";

						ImVec2 Size = ImGui::CalcTextSize(Text.c_str());

						if (settings::Text)
						{

							draw::DrawTextitemesp(
								ImGui::GetBackgroundDrawList(),
								ImGui::GetFont(),
								13.0f,
								ImVec2(Spair.x - (Size.x / 2.0f), Spair.y - (Size.y / 2.0f)),
								ImColor(59, 201, 173, 255), // Correct heavy ammo color
								ImColor(0, 0, 0),
								Text.c_str(),
								true
							);
						}
					}
				}
			}
		}
	}

}

void triggerShot() {
	static const uintptr_t attackAddress = baseAddress + off::inAttack + 0x8;
	mem::write<uint32_t>(attackAddress, 4);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	mem::write<uint32_t>(attackAddress, 5);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	mem::write<uint32_t>(attackAddress, 4);
}

enum class HitboxType {
	Head = 0,
	Neck = 1,
	UpperChest = 2,
	LowerChest = 3,
	Stomach = 4,
	Hip = 5,
	Leftshoulder = 6,
	Leftelbow = 7,
	Lefthand = 8,
	Rightshoulder = 9,
	RightelbowBone = 10,
	Righthand = 11,
	LeftThighs = 12,
	Leftknees = 13,
	Leftleg = 14,
	RightThighs = 16,
	Rightknees = 17,
	Rightleg = 18,
};


inline void DrawLine(ImDrawList* canvas, const Vector2& start, const Vector2& end, float thickness, const ImColor& color) {
	canvas->AddLine((const ImVec2&)start, (const ImVec2&)end, ImColor(color), thickness);
}

inline void DrawCorneredBox(ImDrawList* canvas, float X, float Y, float W, float H, ImColor color, float thickness) {
	float lineW = (W / 4);
	float lineH = (H / 5.5);
	float lineT = -thickness;
	// Corners
	DrawLine(canvas, Vector2(X, Y + thickness / 2), Vector2(X, Y + lineH), thickness, color); // bot right vert
	DrawLine(canvas, Vector2(X + thickness / 2, Y), Vector2(X + lineW, Y), thickness, color);
	DrawLine(canvas, Vector2(X + W - lineW, Y), Vector2(X + W - thickness / 2, Y), thickness, color); // bot left hor
	DrawLine(canvas, Vector2(X + W, Y + thickness / 2), Vector2(X + W, Y + lineH), thickness, color);
	DrawLine(canvas, Vector2(X, Y + H - lineH), Vector2(X, Y + H - (thickness / 2)), thickness, color); // top right vert
	DrawLine(canvas, Vector2(X + thickness / 2, Y + H), Vector2(X + lineW, Y + H), thickness, color);
	DrawLine(canvas, Vector2(X + W - lineW, Y + H), Vector2(X + W - thickness / 2, Y + H), thickness, color); // top left hor
	DrawLine(canvas, Vector2(X + W, Y + H - lineH), Vector2(X + W, Y + H - (thickness / 2)), thickness, color);
	/*if (Outline) {
		ImColor Black = ImColor(0, 0, 0);
		DrawLine(canvas,Vector2D(X - lineT, Y - lineT +thickness/2), Vector2D(X - lineT, Y + lineH), thickness , Black); //bot right vert
		DrawLine(canvas,Vector2D(X - lineT +thickness/2, Y - lineT), Vector2D(X + lineW, Y - lineT), thickness , Black);
		DrawLine(canvas,Vector2D(X + W - lineW, Y - lineT), Vector2D(X + W - (thickness/ 2) + lineT, Y - lineT), thickness , Black); //bot left hor
		DrawLine(canvas,Vector2D(X + W + lineT, Y - lineT +thickness/2), Vector2D(X + W + lineT, Y + lineH), thickness , Black);
		DrawLine(canvas,Vector2D(X - lineT, Y + H - lineH), Vector2D(X - lineT, Y + H -(thickness/2) + lineT ), thickness , Black); //top right vert
		DrawLine(canvas,Vector2D(X +thickness/2 - lineT, Y + H + lineT), Vector2D(X + lineW, Y + H + lineT), thickness , Black); //top right hor
		DrawLine(canvas,Vector2D(X + W - lineW, Y + H + lineT), Vector2D(X + W - (thickness/ 2) + lineT , Y + H + lineT), thickness , Black); //top left hor
		DrawLine(canvas,Vector2D(X + W + lineT, Y + H - lineH), Vector2D(X + W + lineT, Y + H + lineT - (thickness/ 2)), thickness , Black);
	}*/
}

// DrawBox
inline void Draw2DBox(ImDrawList* canvas, int Type, int Style, bool Outline, Vector2& foot, const Vector2& head, const ImColor& color2D, const ImColor& Filledcolor, float thickness) {
	// Type = 2D, 2D Filled
	// Style = 1 or 2, idk what to call them (for now)
	if (Type == 0) { // 2D Box
		if (Style == 0) {
			float height = head.y - foot.y;
			float width = height / 2.0f;
			if (Outline) {
				canvas->AddRect(ImVec2(foot.x - (width / 2), foot.y), ImVec2(head.x + (width / 2), head.y + (height * 0.2)), ImColor(0, 0, 0), 0.0f, 0, thickness + 1);
			}
			canvas->AddRect(ImVec2(foot.x - (width / 2), foot.y), ImVec2(head.x + (width / 2), head.y + (height * 0.2)), color2D, 0.0f, 0, thickness);
		}
		if (Style == 1) {
			float Height = (head.y - foot.y);
			Vector2 rectTop = Vector2(head.x - Height / 3, head.y);
			Vector2 rectBottom = Vector2(foot.x + Height / 3, foot.y);
			if (Outline) {
				canvas->AddRect(ImVec2(rectBottom.x, rectBottom.y), ImVec2(rectTop.x, rectTop.y + (Height * 0.2)), ImColor(0, 0, 0), 0.0f, 0, thickness + 1);
			}
			canvas->AddRect(ImVec2(rectBottom.x, rectBottom.y), ImVec2(rectTop.x, rectTop.y + (Height * 0.2)), color2D, 0.0f, 0, thickness);
		}
	}
	if (Type == 1) { // 2D Box + 2D Filled Box
		if (Style == 0) {
			float height = head.y - foot.y;
			float width = height / 2.0f;
			if (Outline) {
				canvas->AddRect(ImVec2(foot.x - (width / 2), foot.y), ImVec2(head.x + (width / 2), head.y + (height * 0.2)), ImColor(0, 0, 0), 0.0f, 0, thickness + 1);
			}
			canvas->AddRect(ImVec2(foot.x - (width / 2), foot.y), ImVec2(head.x + (width / 2), head.y + (height * 0.2)), color2D, 0.0f, 0, thickness);
			canvas->AddRectFilled(ImVec2(foot.x - (width / 2), foot.y), ImVec2(head.x + (width / 2), head.y + (height * 0.2)), Filledcolor, 0.0f, 0);
		}
		if (Style == 1) {
			float Height = (head.y - foot.y);
			Vector2 rectTop = Vector2(head.x - Height / 3, head.y);
			Vector2 rectBottom = Vector2(foot.x + Height / 3, foot.y);
			if (Outline) {
				canvas->AddRect(ImVec2(rectBottom.x, rectBottom.y), ImVec2(rectTop.x, rectTop.y + (Height * 0.2)), ImColor(0, 0, 0), 0.0f, 0, thickness + 1);
			}
			canvas->AddRect(ImVec2(rectBottom.x, rectBottom.y), ImVec2(rectTop.x, rectTop.y + (Height * 0.2)), color2D, 0.0f, 0, thickness);
			canvas->AddRectFilled(ImVec2(rectBottom.x, rectBottom.y), ImVec2(rectTop.x, rectTop.y + (Height * 0.2)), Filledcolor, thickness);
		}
	}
	if (Type == 2) { // 2D Corners
		float height = head.y - foot.y;
		float width = height / 2.0f;
		float x = foot.x - (width / 2.f);

		if (Outline) {
			DrawCorneredBox(canvas, x, foot.y, width, height + (height * 0.2), ImColor(0, 0, 0), thickness + 1);
		}
		DrawCorneredBox(canvas, x, foot.y, width, height + (height * 0.2), color2D, thickness);
	}
}

struct Color {

	static const int size;

	float r;
	float g;
	float b;
	float a;

	Color() : r(1.0f), g(1.0f), b(1.0f) {}

	Color(float r, float g, float b) : r(r), g(g), b(b) {
		clamp();
	}

	Color operator*(const float& scalar) const {
		return Color(r * scalar, g * scalar, b * scalar).clamp();
	}

	Color& operator*=(const float& scalar) {
		r *= scalar;
		g *= scalar;
		b *= scalar;
		return clamp();
	}

	Color operator+(const Color& other) const {
		return Color(r + other.r, g + other.g, b + other.b).clamp();
	}

	static Color lerp(Color startColor, Color endColor, float t) {
		t = std::clamp(t, 0.0f, 1.0f);

		startColor *= (1.0f - t);
		endColor *= t;
		auto result = startColor + endColor;

		return result.clamp();
	}

	Color& clamp() {
		r = std::clamp(r, 0.0f, 1.0f);
		g = std::clamp(g, 0.0f, 1.0f);
		b = std::clamp(b, 0.0f, 1.0f);
		return *this;
	}


	bool operator==(const Color& other) const {
		return r == other.r &&
			g == other.g &&
			b == other.b;
	}

	Color& roundColor() {
		r = round(r * 100) / 100;
		g = round(g * 100) / 100;
		b = round(b * 100) / 100;
		return *this;
	}
};

static void DrawRectFilled(ImDrawList* canvas, float x, float y, float x2, float y2, ImColor color, float rounding, int rounding_corners_flags) {
	canvas->AddRectFilled(ImVec2(x, y), ImVec2(x2, y2), color, rounding, rounding_corners_flags);
}

static void DrawProgressBar(ImDrawList* canvas, float x, float y, float w, float h, int value, int v_max, ImColor barColor) {
	DrawRectFilled(canvas, x, y, x + w, y - ((h / float(v_max)) * (float)value), barColor, 0.0f, 0);
	canvas->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y - h + 1), ImColor(0, 0, 0), 0, 1);
}

inline ImU32 ToImColor(const Color& col) {
	return IM_COL32(
		static_cast<int>(col.r * 255.f),
		static_cast<int>(col.g * 255.f),
		static_cast<int>(col.b * 255.f),
		static_cast<int>(col.a * 255.f)
	);
}

static void drawFilledRectagle(Vector2 position, Vector2 size, Color fillColor) {
	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	draw_list->AddRectFilled(
		ImVec2(position.x, position.y),
		ImVec2(position.x + size.x, position.y + size.y),
		ToImColor(fillColor)
	);
}

static void drawRectangleOutline(Vector2 position, Vector2 size, Color borderColor, float lineWidth) {
	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	draw_list->AddRect(
		ImVec2(position.x, position.y),
		ImVec2(position.x + size.x, position.y + size.y),
		ToImColor(borderColor),
		0.0f, // rounding
		0,    // flags
		lineWidth
	);
}

static void drawBorderedFillRectangle(Vector2 position, Vector2 size, Color fillColor, Color borderColor, float lineWidth, float fill) {
	drawFilledRectagle(position, Vector2(size.x, size.y), Color(0, 0, 0));
	drawFilledRectagle(position, Vector2(size.x * fill, size.y), fillColor);
	drawRectangleOutline(position, size, borderColor, lineWidth);
}

static void Draw2DBarTest(ImDrawList* canvas, bool DrawHealth, bool DrawShield, int BarStyle, int ColorMode, Vector2& Foot, Vector2& Head, int health, int maxHealth, int shield, int maxShield, float thickness, float thickness2, float BarWidth, float BarHeight) {
	// Shield Color
	ImColor shieldBarColor;
	if (ColorMode == 0) { // MaxShield
		if (maxShield == 50) { // white
			shieldBarColor = ImColor(168, 168, 168, 255);
		}
		else if (maxShield == 75) { // blue
			shieldBarColor = ImColor(39, 178, 255, 255);
		}
		else if (maxShield == 100) { // purple
			shieldBarColor = ImColor(206, 59, 255, 255);
		}
		else if (maxShield == 125) { // red
			shieldBarColor = ImColor(219, 2, 2, 255);
		}
	}

	if (ColorMode == 1) { // Current Shield
		if (shield <= 50) { // white
			shieldBarColor = ImColor(168, 168, 168, 255);
		}
		else if (shield <= 75) { // blue
			shieldBarColor = ImColor(39, 178, 255, 255);
		}
		else if (shield <= 100) { // purple
			shieldBarColor = ImColor(206, 59, 255, 255);
		}
		else if (shield <= 125) { // red
			shieldBarColor = ImColor(219, 2, 2, 255);
		}
	}

	if (BarStyle == 0) { // Sides
		float height = Head.y - Foot.y;
		float width = height / 2.f;
		float width2 = width / 10;
		if (width2 < 2.f)
			width2 = 2.;
		if (width2 > 3)
			width2 = 3.;

		float entityHeight = Foot.y - Head.y;
		float boxLeft = Foot.x - entityHeight / 3;
		float boxRight = Head.x + entityHeight / 3;
		float barPercentWidth = thickness2;
		float barPixelWidth = barPercentWidth * (boxRight - boxLeft);
		float barHeight = entityHeight * (health / 100.0f);
		Vector2 barTop = Vector2(boxLeft - barPixelWidth, Foot.y - barHeight);
		Vector2 barBottom = Vector2(boxLeft, Foot.y);

		if (DrawHealth)
			DrawProgressBar(canvas, barBottom.x - 3.5f, barBottom.y, width2, barBottom.y - Head.y + (entityHeight * 0.2), health, 100, ImColor(0, 255, 0));
		if (DrawShield)
			DrawProgressBar(canvas, barBottom.x - 7.0f, barBottom.y, width2, barBottom.y - Head.y + (entityHeight * 0.2), shield, maxShield, shieldBarColor);
	}
	if (BarStyle == 1) { // Top
		float height = BarHeight; // 8.0f
		float entityHeight = Foot.y - Head.y;
		float width = BarWidth;   // 80.0f
		Vector2 rectPosition = Vector2(Foot.x - width / 2, Head.y - (entityHeight * 0.2) - 20.0f);
		Vector2 size = Vector2(width, height);

		if (DrawHealth) {
			// HealthBar
			float fill = (float)health / (float)maxHealth;
			drawBorderedFillRectangle(rectPosition, size, Color::lerp(Color(1.0, 0.0, 0.0), Color(0.0, 1.0, 0.0), fill), Color(), thickness, fill);
		}
		if (DrawShield) {
			// ShieldBar
			float fillAP = (float)shield / (float)maxShield;
			if (maxShield == 125) { // Red Shield
				drawBorderedFillRectangle(Vector2(rectPosition.x, rectPosition.y - (height + 3)), size, Color::lerp(Color(1.0, 0.0, 0.0), Color(1.0, 0.0, 0.0), fillAP), Color(), thickness, fillAP);
			}
			if (maxShield == 100) { // Purple Shield
				drawBorderedFillRectangle(Vector2(rectPosition.x, rectPosition.y - (height + 3)), size, Color::lerp(Color(0.501, 0.00, 0.970), Color(0.501, 0.00, 0.970), fillAP), Color(), thickness, fillAP);
			}
			if (maxShield == 75) { // Blue Shield
				drawBorderedFillRectangle(Vector2(rectPosition.x, rectPosition.y - (height + 3)), size, Color::lerp(Color(0.0297, 0.734, 0.990), Color(0.0297, 0.734, 0.990), fillAP), Color(), thickness, fillAP);
			}
			if (maxShield == 50) { // Grey Shield
				drawBorderedFillRectangle(Vector2(rectPosition.x, rectPosition.y - (height + 3)), size, Color::lerp(Color(0.707, 0.702, 0.700), Color(0.707, 0.702, 0.700), fillAP), Color(), thickness, fillAP);
			}
		}
	}
}

inline void DrawSpectatorList(const std::vector<std::string>& spectators)
{
	if (spectators.empty())
		return;

	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	ImFont* font = ImGui::GetFont();
	float fontSize = ImGui::GetFontSize();
	const float paddingX = 6.0f;
	const float paddingY = 3.0f;
	const float rounding = 5.0f;

	// Position: bottom-left corner with some margin
	ImVec2 basePos(15, ImGui::GetIO().DisplaySize.y - 20 - spectators.size() * (fontSize + 4));

	// Calculate widest text width to size the box
	float maxTextWidth = ImGui::CalcTextSize("Spectating You:").x;
	for (auto& name : spectators) {
		float w = ImGui::CalcTextSize(name.c_str()).x;
		if (w > maxTextWidth)
			maxTextWidth = w;
	}

	ImVec2 boxMin(basePos.x - paddingX, basePos.y - paddingY);
	ImVec2 boxMax(basePos.x + maxTextWidth + paddingX * 2, basePos.y + (fontSize + 4) * (spectators.size() + 1) + paddingY);

	// Background with a bit of transparency
	drawList->AddRectFilled(boxMin, boxMax, IM_COL32(30, 30, 35, 220), rounding);

	// Border
	drawList->AddRect(boxMin, boxMax, IM_COL32(255, 255, 255, 80), rounding, 0, 1.0f);

	// Title text (yellow)
	drawList->AddText(ImVec2(basePos.x, basePos.y), IM_COL32(255, 230, 50, 255), "Spectating You:");

	// Draw each spectator name below the title
	for (size_t i = 0; i < spectators.size(); ++i) {
		ImVec2 textPos(basePos.x, basePos.y + (i + 1) * (fontSize + 4));
		drawList->AddText(textPos, IM_COL32(255, 255, 255, 230), spectators[i].c_str());
	}
}


void loop()
{
	static const float halfWidth = Width / 2.0f;
	static const float halfHeight = Height / 2.0f;

	if (settings::showfov)
		ImGui::GetForegroundDrawList()->AddCircle(ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2), settings::fov, ImColor(0, 0, 0), 100, 3.5f);
	ImGui::GetForegroundDrawList()->AddCircle(ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2), settings::fov, ImColor(settings::fov_color[0], settings::fov_color[1], settings::fov_color[2], 1.f), 100, 1.0f);


	if (!Pointers->local_player.valid()) {
		return;
	}

	float closest_distance = FLT_MAX;
	c_player* closest_player = nullptr;
	int closest_index = -1;

	bool aimbotKeyHeld = (imp(GetAsyncKeyState)(settings::key) & 0x8000);
	static bool wasAimbotKeyHeld = false;

	for (size_t i = 0; i < EntityList.size(); ++i) {
		auto& player_entity = EntityList[i];

		if (player_entity.distance > settings::playerDistance ||
			player_entity.health <= 0 ||
			player_entity.player.b_downed()) // ✅ skip knocked
			continue;
		player_entity.visible = player_entity.player.visible(static_cast<int>(i));

		int pos_y_top = 15;
		int pos_y_bottom = 5;

		vector3 head_pos = player_entity.player.bone_position(8);
		vector3 head_pos1 = head_pos + vector3(0, 0, 5);
		vector3 head1 = world_to_screen(head_pos1);

		if (head1.is_zero()) continue;
		if (!is_within_screen_bounds(ImVec2(head1.x, head1.y))) continue;


		vector3 origin_pos = player_entity.player.vec_origin() - vector3(0, 0, 5);
		vector3 head = world_to_screen(head_pos);
		vector3 origin = world_to_screen(origin_pos);
		float box_height = abs(head1.y - origin.y);
		float box_width = box_height * 0.70f;
		float box_x = head1.x - (box_width / 2);
		float box_y = head1.y;
		float y_offset = head1.y - 16;
		if (settings::health_shield && player_entity.distance <= 100)
		{
			y_offset = head1.y - 5;
		}

		float y_offset1 = origin.y + 3;


		if (settings::box)
		{
			ImColor boxColor = player_entity.visible
				? ImColor(settings::box_color_visible[0], settings::box_color_visible[1], settings::box_color_visible[2])
				: ImColor(settings::box_color_not_visible[0], settings::box_color_not_visible[1], settings::box_color_not_visible[2]);
			draw::draw_cornered_box(box_x, box_y, box_width, box_height, boxColor, 1.f);
		}

		if (settings::normal_box)
		{
			ImColor normalBoxColor = player_entity.visible
				? ImColor(settings::normal_box_color_visible[0], settings::normal_box_color_visible[1], settings::normal_box_color_visible[2])
				: ImColor(settings::normal_box_color_not_visible[0], settings::normal_box_color_not_visible[1], settings::normal_box_color_not_visible[2]);
			draw::draw_normal_box(box_x, box_y, box_width, box_height, normalBoxColor, 1.f);
		}

		if (settings::skeleton)
		{
			ImDrawList* draw = ImGui::GetForegroundDrawList();

			ImColor skelColor = player_entity.visible
				? ImColor(settings::skel_color_visible[0], settings::skel_color_visible[1], settings::skel_color_visible[2], 255.f)
				: ImColor(settings::skel_color_not_visible[0], settings::skel_color_not_visible[1], settings::skel_color_not_visible[2], 255.f);

			auto bone_screen = [&](int bone_id) -> vector3 {
				return world_to_screen(player_entity.player.bone_position(bone_id));
				};

			auto draw_line = [&](int a, int b) {
				vector3 pa = bone_screen(a);
				vector3 pb = bone_screen(b);
				if (!pa.is_zero() && !pb.is_zero())
					draw->AddLine(ImVec2(pa.x, pa.y), ImVec2(pb.x, pb.y), skelColor, 1.0f);
				};

			// Spine
			draw_line(8, 5);   // Head -> Upper chest
			draw_line(5, 0);   // Upper chest -> pelvis

			// Left arm
			draw_line(5, 33);  // Chest -> L shoulder
			draw_line(33, 35); // Shoulder -> Elbow
			draw_line(35, 36); // Elbow -> Hand

			// Right arm
			draw_line(5, 10);  // Chest -> R shoulder
			draw_line(10, 12); // Shoulder -> Elbow
			draw_line(12, 13); // Elbow -> Hand

			// Left leg
			draw_line(0, 55);  // Pelvis -> L hip
			draw_line(55, 56); // Hip -> Knee
			draw_line(56, 58); // Knee -> Foot

			// Right leg
			draw_line(0, 60);  // Pelvis -> R hip
			draw_line(60, 62); // Hip -> Knee
			draw_line(62, 63); // Knee -> Foot
		}

		if (settings::triggerbot && (GetAsyncKeyState(VK_XBUTTON1) & 0x8000)) {
			Vector2 screenCenter = { Width / 2.0f, Height / 2.0f };
			if (player_entity.health <= 0 || player_entity.player.b_downed() || (settings::vischeck && !player_entity.visible))
				continue;

			// Bone indices: 8 = head, 3 = chest
			static const std::vector<int> triggerBones = { 0,1,2,3,4,5,6,7,8};

			for (int boneIndex : triggerBones) {

				vector3 bonePos = player_entity.player.bone_position(boneIndex);

				// Axis-aligned box size per bone
				float boxSizeX = (boneIndex == 0) ? 5.0f : 8.0f;
				float boxSizeY = (boneIndex == 0) ? 5.0f : 8.0f;
				float boxSizeZ = (boneIndex == 0) ? 5.0f : 12.0f;

				// Create corner points of a 3D box
				std::vector<vector3> corners = {
					{ bonePos.x + boxSizeX, bonePos.y + boxSizeY, bonePos.z + boxSizeZ },
					{ bonePos.x - boxSizeX, bonePos.y - boxSizeY, bonePos.z - boxSizeZ },
					{ bonePos.x + boxSizeX, bonePos.y - boxSizeY, bonePos.z + boxSizeZ },
					{ bonePos.x - boxSizeX, bonePos.y + boxSizeY, bonePos.z - boxSizeZ },
				};

				float minX = FLT_MAX, minY = FLT_MAX;
				float maxX = -FLT_MAX, maxY = -FLT_MAX;

				for (const auto& corner : corners) {
					vector3 screen = world_to_screen(corner);
					if (screen.is_zero() || !is_within_screen_bounds({ screen.x, screen.y }))
						continue;

					minX = min(minX, screen.x);
					maxX = max(maxX, screen.x);
					minY = min(minY, screen.y);
					maxY = max(maxY, screen.y);
				}

				// Crosshair check inside box
				if (screenCenter.x >= minX && screenCenter.x <= maxX &&
					screenCenter.y >= minY && screenCenter.y <= maxY) {
					triggerShot(); // fire
					return;        // only fire once per tick
				}
			}
		}

		if (settings::health_shield && player_entity.distance <= 100)
		{
			ImDrawList* draww = ImGui::GetForegroundDrawList();
			draw::SeerHealth(draww, head1.x, y_offset, player_entity.player.i_shield(), player_entity.player.i_max_shield(), player_entity.player.i_shield_type(), player_entity.player.i_health());
			y_offset -= 27;
		}

		if (settings::name)
		{
			auto username = player_entity.name;
			ImColor nameColor = player_entity.visible
				? ImColor(settings::name_color_visible[0], settings::name_color_visible[1], settings::name_color_visible[2])
				: ImColor(settings::name_color_not_visible[0], settings::name_color_not_visible[1], settings::name_color_not_visible[2]);
			draw::DrawModernNametag(head1.x, y_offset, username.c_str());
			y_offset -= 18;
		}

		if (settings::distance)
		{
			auto username = "(" + to_string(player_entity.distance) + "m)";
			auto size = ImGui::CalcTextSize(username.c_str());
			const ImVec2 text_pos = ImVec2(head1.x - (size.x / 3), y_offset1);
			ImColor distanceColor = player_entity.visible
				? ImColor(settings::distance_color_visible[0], settings::distance_color_visible[1], settings::distance_color_visible[2])
				: ImColor(settings::distance_color_not_visible[0], settings::distance_color_not_visible[1], settings::distance_color_not_visible[2]);
			draw::DrawTextWithOutline(ImGui::GetBackgroundDrawList(), ImGui::GetFont(), 13, text_pos, distanceColor, ImColor(0, 0, 0), username.c_str(), true);
			y_offset1 += (size.y / 1.5);

		}

		if (settings::weapon)
		{
			auto username = "[" + player_entity.weapon + "]";
			auto size = ImGui::CalcTextSize(username.c_str());
			const ImVec2 text_pos = ImVec2(head1.x - (size.x / 3), y_offset1);
			ImColor weaponColor = player_entity.visible
				? ImColor(settings::weapon_color_visible[0], settings::weapon_color_visible[1], settings::weapon_color_visible[2])
				: ImColor(settings::weapon_color_not_visible[0], settings::weapon_color_not_visible[1], settings::weapon_color_not_visible[2]);
			draw::DrawTextWithOutline(ImGui::GetBackgroundDrawList(), ImGui::GetFont(), 13, text_pos, weaponColor, ImColor(0, 0, 0), username.c_str(), true);
			y_offset1 += (size.y / 1.5);

		}
		if (settings::fov_changer)
		{
			float centerX = Width / 2.0f;
			float centerY = Height / 2.0f;
			float fov_radius = settings::fov;

			for (size_t i = 0; i < EntityList.size(); ++i) {
				auto& player_entity = EntityList[i];
				if (player_entity.distance > settings::playerDistance || player_entity.health <= 0) continue;

				vector3 head_pos = player_entity.player.bone_position(8);
				vector3 head_screen = world_to_screen(head_pos);

				float dx = head_screen.x - centerX;
				float dy = head_screen.y - centerY;
				float dist = sqrtf(dx * dx + dy * dy);

				if (dist > fov_radius) {
					float angle = atan2f(dy, dx);

					float arrow_length = 33.0f;
					float arrow_radius = fov_radius;

					ImVec2 arrow_base = ImVec2(
						centerX + cosf(angle) * arrow_radius,
						centerY + sinf(angle) * arrow_radius
					);
					ImVec2 arrow_tip = ImVec2(
						centerX + cosf(angle) * (arrow_radius + arrow_length),
						centerY + sinf(angle) * (arrow_radius + arrow_length)
					);

					float side_angle = 0.7f;
					ImVec2 left = ImVec2(
						arrow_base.x + cosf(angle + side_angle) * (arrow_length * 0.8f),
						arrow_base.y + sinf(angle + side_angle) * (arrow_length * 0.8f)
					);
					ImVec2 right = ImVec2(
						arrow_base.x + cosf(angle - side_angle) * (arrow_length * 0.8f),
						arrow_base.y + sinf(angle - side_angle) * (arrow_length * 0.8f)
					);

					ImColor arrowColor = player_entity.visible
						? ImColor(settings::box_color_visible[0], settings::box_color_visible[1], settings::box_color_visible[2])
						: ImColor(settings::box_color_not_visible[0], settings::box_color_not_visible[1], settings::box_color_not_visible[2]);

					ImDrawList* draw_list = ImGui::GetForegroundDrawList();
					draw_list->AddTriangleFilled(arrow_tip, left, right, arrowColor);
					draw_list->AddTriangle(arrow_tip, left, right, IM_COL32(0, 0, 0, 255), 2.0f);
				}
			}
		}



		//if (settings::weapon_icon)
		//{
		//	struct BoxDimensions {
		//		int width;
		//		int height;
		//		int yheight;
		//	};

		//	int y_height = 40;

		//	BoxDimensions weaponBox = { 20, 20, y_height };
		//	BoxDimensions ammoBox = { 30, 15, 35 };

		//	int boxX = origin.x - (weaponBox.width / 2);
		//	int boxY = y_offset1 + weaponBox.yheight;
		//	int flippedBoxY = boxY - weaponBox.height;

		//	int AboxX = origin.x - (ammoBox.width / 2);
		//	int AboxY = y_offset1 + ammoBox.yheight;
		//	int AflippedBoxY = AboxY - ammoBox.height;

		//	/*static const std::unordered_map<int, LPDIRECT3DTEXTURE9> weaponTextureMap = {
		//		{117, WeaponIcon::PistolICON::P2020Texture},
		//		{87, WeaponIcon::PistolICON::Re45Texture},
		//		{86, WeaponIcon::PistolICON::AlternatorTexture},
		//		{121, WeaponIcon::PistolICON::WingmenTexture},
		//		{95, WeaponIcon::ShotgunICON::EVA8Texture},
		//		{104, WeaponIcon::ShotgunICON::MastiffTexture},
		//		{114, WeaponIcon::ShotgunICON::peacekeeperTexture},
		//		{106, WeaponIcon::ShotgunICON::MozambiqueTexture},
		//		{125, WeaponIcon::Rifle::CarTexture},
		//		{0, WeaponIcon::Rifle::CarbineTexture},
		//		{97, WeaponIcon::Rifle::FlatlineTexture},
		//		{99, WeaponIcon::Rifle::HemlockTexture},
		//		{111, WeaponIcon::Rifle::ProwlerTexture},
		//		{7, WeaponIcon::Rifle::RampageTexture},
		//		{124, WeaponIcon::Rifle::RepeaterTexture},
		//		{118, WeaponIcon::Rifle::SpitFireTexture},
		//		{3, WeaponIcon::Special::BowTexture},
		//		{90, WeaponIcon::Special::DevotionTexture},
		//		{186, WeaponIcon::Special::KnifeTexture},
		//		{101, WeaponIcon::Special::KraberTexture},
		//		{115, WeaponIcon::Special::R99Texture},
		//		{89, WeaponIcon::Sniper::ChargedRifleTexture},
		//		{92, WeaponIcon::Sniper::DMRTexture},
		//		{2, WeaponIcon::Sniper::SentinelTexture},
		//		{126, WeaponIcon::Energy::NemesisTexture},
		//		{123, WeaponIcon::Energy::VoltTexture},
		//		{120,WeaponIcon::Energy::TripleTexture},
		//		{102, WeaponIcon::Energy::EMGTexture},
		//		{93, WeaponIcon::Energy::HAVOCTexture},
		//	};*/


		//	int weapon_index = player_entity.player.GetWeaponIndex();

		//	auto it = weaponTextureMap.find(weapon_index);
		//	if (it != weaponTextureMap.end()) {
		//		LPDIRECT3DTEXTURE9 texture = it->second;
		//		{

		//			ImColor WeaponICONColor = player_entity.visible
		//				? ImColor(settings::weapon_icon_color_visible[0], settings::weapon_icon_color_visible[1], settings::weapon_icon_color_visible[2], 255.f)
		//				: ImColor(settings::weapon_icon_color_not_visible[0], settings::weapon_icon_color_not_visible[1], settings::weapon_icon_color_not_visible[2], 255.f);

		//			if (weapon_index == 112 || weapon_index == 85 || weapon_index == 84 || weapon_index == 115) { 
		//				//draw::DrawRotatedImageWithBoxColor(texture, AboxX, AflippedBoxY, ammoBox.width, -ammoBox.height, WeaponICONColor);
		//				y_offset1 += (ammoBox.height / 1.5);

		//			}
		//			else {
		//			//	draw::DrawRotatedImageWithBoxColor(texture, boxX, flippedBoxY, weaponBox.width, -weaponBox.height, WeaponICONColor);
		//				y_offset1 += (weaponBox.height / 1.5);

		//			}
		//		}
		//	}

		//}

		if (settings::op)
		{
			auto username = player_entity.op;
			auto size = ImGui::CalcTextSize(username.c_str());
			const ImVec2 text_pos = ImVec2(head1.x - (size.x / 3), y_offset);
			ImColor opColor = player_entity.visible
				? ImColor(settings::operator_color_visible[0], settings::operator_color_visible[1], settings::operator_color_visible[2])
				: ImColor(settings::operator_color_not_visible[0], settings::operator_color_not_visible[1], settings::operator_color_not_visible[2]);
			draw::DrawTextWithOutline(ImGui::GetBackgroundDrawList(), ImGui::GetFont(), 13, text_pos, opColor, ImColor(0, 0, 0), username.c_str(), true);
			y_offset -= (size.y / 1.5);

		}

		float dx = head.x - halfWidth;
		float dy = head.y - halfHeight;
		float dist = sqrtf(dx * dx + dy * dy);

		if (settings::aimbot && settings::vischeck && !player_entity.visible)
			continue;

		if (dist <= settings::fov && dist < closest_distance) {
			closest_distance = dist;
			closest_player = &player_entity.player;
			closest_index = static_cast<int>(i);
		}

		if (!aimbotKeyHeld) {
			lockedAimbotTarget = nullptr;
			lockedAimbotIndex = -1;
		}
		else {
			bool valid = false;
			if (lockedAimbotTarget && lockedAimbotIndex >= 0 && lockedAimbotIndex < (int)EntityList.size()) {
				auto& ent = EntityList[lockedAimbotIndex];
				vector3 screen_pos = world_to_screen(ent.player.bone_position(8));
				float dx = screen_pos.x - halfWidth;
				float dy = screen_pos.y - halfHeight;
				float dist = sqrtf(dx * dx + dy * dy);
				valid = ent.health > 0 && ent.distance <= settings::playerDistance && dist <= settings::fov && is_within_screen_bounds(ImVec2(screen_pos.x, screen_pos.y));
				if (settings::vischeck) valid = valid && ent.visible;
			}
			if (!valid) {
				lockedAimbotTarget = closest_player;
				lockedAimbotIndex = closest_index;
			}
		}
		wasAimbotKeyHeld = aimbotKeyHeld;

		if (settings::aimbot && !settings::menu && (lockedAimbotTarget || closest_player)) {
			c_player* target_player = lockedAimbotTarget ? lockedAimbotTarget : closest_player;

			vector3 target_pos;
			switch (aimpart) {
			case HEAD:   target_pos = world_to_screen(target_player->bone_position(8)); break;
			case NECK:   target_pos = world_to_screen(target_player->bone_position(5)); break;
			case CHEST:  target_pos = world_to_screen(target_player->bone_position(3)); break;
			case PELVIS: target_pos = world_to_screen(target_player->bone_position(0)); break;
			default:     target_pos = world_to_screen(target_player->bone_position(8)); break;
			}

			if (!target_pos.is_zero()) {
				float centerX = Width / 2.0f;
				float centerY = Height / 2.0f;
				Point start{ centerX, centerY };
				Point end{ target_pos.x, target_pos.y };
				last_aimbot_curve = generateControlPoints(start, end, 30);
				draw_aimbot_curve(last_aimbot_curve);

				if (aimbotKeyHeld) {
					aimbot(target_pos.x, target_pos.y);
				}
			}
			else {
				last_aimbot_curve.clear();
			}
		}
		else {
			last_aimbot_curve.clear();
		}
	}
}
auto InputHandler() -> void {
	if (settings::menu == true)
	{
		for (int i = 0; i < 5; i++) {
			ImGui::GetIO().MouseDown[i] = false;
		}

		int Button = -1;
		if (imp(GetAsyncKeyState)(VK_LBUTTON)) {
			Button = 0;
		}

		if (Button != -1) {
			ImGui::GetIO().MouseDown[Button] = true;
		}
	}
}
void render() {
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	InputHandler();
	loop();
	loop_loot();
	{
		static float frames = 0;
		static float lastTime = 0;
		static float fps = 0;
		float currentTime = ImGui::GetTime();
		frames++;
		if (currentTime - lastTime >= 1.0f) {
			fps = frames / (currentTime - lastTime);
			lastTime = currentTime;
			frames = 0;
		}
		char fpsText[32];
		snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", fps);
		ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, 10), IM_COL32(255, 255, 0, 255), fpsText);
	}
	ImGui::GetIO().MouseDrawCursor = settings::menu;
	if (imp(GetAsyncKeyState)(VK_INSERT) & 1) settings::menu = !settings::menu;
	if (settings::menu == true)
	{
		drawmenu();
		SetWindowLong(Overlay.Hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
		UpdateWindow(Overlay.Hwnd);
		SetFocus(Overlay.Hwnd);
	}
	else {
		SetWindowLong(Overlay.Hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
		UpdateWindow(Overlay.Hwnd);
	}
	ImGui::EndFrame();

	extern ComPtr<ID3D11DeviceContext> g_pd3dDeviceContext;
	extern ComPtr<ID3D11RenderTargetView> g_mainRenderTargetView;
	extern ComPtr<IDXGISwapChain> g_pSwapChain;

	float clear_color[4] = { 0, 0, 0, 0 };
	g_pd3dDeviceContext->OMSetRenderTargets(1, g_mainRenderTargetView.GetAddressOf(), nullptr);
	g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView.Get(), clear_color);

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	g_pSwapChain->Present(0, 0);
}


void render_loop()
{
	static RECT OldRect = { 0 };
	constexpr double frames = 1000.0 / 300.0;

	MSG msg = { 0 };

	extern ComPtr<IDXGISwapChain> g_pSwapChain;
	extern ComPtr<ID3D11Device> g_pd3dDevice;
	extern ComPtr<ID3D11DeviceContext> g_pd3dDeviceContext;
	extern ComPtr<ID3D11RenderTargetView> g_mainRenderTargetView;

	while (msg.message != WM_QUIT) {
		auto frameStart = std::chrono::high_resolution_clock::now();

		if (PeekMessage(&msg, Overlay.Hwnd, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		RECT TempRect = { 0 };
		POINT TempPoint = { 0 };
		GetClientRect(Overlay.Hwnd, &TempRect);
		ClientToScreen(Process.Hwnd, &TempPoint);

		TempRect.left = TempPoint.x;
		TempRect.top = TempPoint.y;

		if (memcmp(&TempRect, &OldRect, sizeof(RECT)) != 0) {
			OldRect = TempRect;
			Width = TempRect.right;
			Height = TempRect.bottom;
		}

		render();

		auto frameEnd = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> frameDuration = frameEnd - frameStart;

		if (frameDuration.count() < frames) {
			std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(frames - frameDuration.count()));
		}
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	g_mainRenderTargetView.Reset();
	g_pSwapChain.Reset();
	g_pd3dDeviceContext.Reset();
	g_pd3dDevice.Reset();

	DestroyWindow(Overlay.Hwnd);
	UnregisterClass(Overlay.WindowClass.lpszClassName, Overlay.WindowClass.hInstance);
}

void glow_loop() {
    static bool pp_glow = false;
	std::unordered_map<uintptr_t, int> written_map;
    while (true) {
        if (!Pointers->local_player.valid()) {
            continue;
        }
        std::scoped_lock lock(EntityListMutex);
        for (size_t i = 0; i < EntityList.size(); ++i) {
            auto& player_entity = EntityList[i];
            if (player_entity.distance > settings::playerDistance || player_entity.health <= 0) continue;
            if (settings::glow) {
				player_glow(&player_entity, written_map);
            } else if (pp_glow) {
				stop_glow(player_entity.player.get_address());
            }
        }
		pp_glow = settings::glow;
        if (settings::weapon_glow) {
            weapon::index = settings::weapon_glow_index;
            weapon_glow();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}