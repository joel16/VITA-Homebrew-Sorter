#include <psp2/sysmodule.h>
#include <vitaGL.h>

#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui_impl_vitagl.h"
#include "log.h"
#include "power.h"
#include "textures.h"
#include "utils.h"

namespace Services {
	void SetDefaultTheme(void) {
		ImGui::GetStyle().FrameRounding = 4.0f;
		ImGui::GetStyle().GrabRounding = 4.0f;
		
		ImVec4 *colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.07f, 0.13f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
		colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
		colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
		colors[ImGuiCol_TabActive] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.IniFilename = nullptr;
	}

	void Init(void) {
		// Initalize vitaGL and imGui contexts
		vglInitExtended(0, 960, 544, 0x800000, SCE_GXM_MULTISAMPLE_4X);
		vglUseVram(GL_TRUE);
		ImGui::CreateContext();
		ImGui_ImplVitaGL_Init();

		// Setup style
		ImGui::StyleColorsDark();

		Utils::InitAppUtil();
		SCE_CTRL_ENTER = Utils::GetEnterButton();
		SCE_CTRL_CANCEL = Utils::GetCancelButton();
		
		sceSysmoduleLoadModule(SCE_SYSMODULE_JSON);

		if (!FS::DirExists("ux0:data/VITAHomebrewSorter/backup"))
			FS::MakeDir("ux0:data/VITAHomebrewSorter/backup");

		if (!FS::DirExists("ux0:data/VITAHomebrewSorter/loadouts"))
			FS::MakeDir("ux0:data/VITAHomebrewSorter/loadouts");
		
		Log::Init();
		Textures::Init();
		Power::InitThread();
		Config::Load();
	}

	void Exit(void) {
		// Clean up
		Textures::Exit();
		Log::Exit();
		sceSysmoduleUnloadModule(SCE_SYSMODULE_JSON);
		Utils::EndAppUtil();
		ImGui_ImplVitaGL_Shutdown();
		ImGui::DestroyContext();
		vglEnd();
	}
}

int main(int argc, char *argv[]) {
	Services::Init();
	Services::SetDefaultTheme();
	GUI::RenderLoop();
	Services::Exit();
	return 0;
}
