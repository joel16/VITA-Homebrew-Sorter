// Implemented features:
//  [X] User texture binding. Cast 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

#include <math.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/processmgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <vitaGL.h>

#include "imgui.h"
#include "imgui_impl_vitagl.h"
#include "utils.h"

#define GL_COLOR_MATERIAL 0x0B57

#define lerp(value, from_max, to_max) ((((value * 10) * (to_max * 10)) / (from_max * 10)) / 10)

struct ImGui_ImplVitaGL_Data {
	GLuint FontTexture = 0;
	uint64_t g_Time = 0;
	ImGui_ImplVitaGL_Data(void) {
		sceClibMemset(this, 0, sizeof(*this));
	}
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplVitaGL_Data *ImGui_ImplVitaGL_GetBackendData(void) {
	return ImGui::GetCurrentContext() ? static_cast<ImGui_ImplVitaGL_Data *>(ImGui::GetIO().BackendRendererUserData) : nullptr;
}

bool ImGui_ImplVitaGL_Init(void) {
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");
	
	// Setup backend capabilities flags
	ImGui_ImplVitaGL_Data *bd = IM_NEW(ImGui_ImplVitaGL_Data)();
	io.BackendRendererUserData = (void *)bd;
	io.BackendRendererName = "imgui_impl_vitagl";
	
	return true;
}

void ImGui_ImplVitaGL_Shutdown(void) {
	ImGui_ImplVitaGL_Data *bd = ImGui_ImplVitaGL_GetBackendData();
	IM_ASSERT(bd != NULL && "No renderer backend to shutdown, or already shutdown?");
	ImGuiIO& io = ImGui::GetIO();
	
	ImGui_ImplVitaGL_DestroyDeviceObjects();
	io.BackendRendererName = nullptr;
	io.BackendRendererUserData = nullptr;
	IM_DELETE(bd);
}

static void ImGui_ImplVitaGL_UpdateGamepads(void) {
	ImGuiIO& io = ImGui::GetIO();
	sceClibMemset(io.NavInputs, 0, sizeof(io.NavInputs));
	if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
		return;
		
	SceCtrlData pad;
	sceCtrlPeekBufferPositive(0, &pad, 1);
	
	int rstick_x = (pad.rx - 127) * 256;
	int rstick_y = (pad.ry - 127) * 256;
	
	// Update gamepad inputs
	#define MAP_BUTTON(NAV_NO, BUTTON_NO)       { io.NavInputs[NAV_NO] = pad.buttons & BUTTON_NO? 1.0f : 0.0f; }
	#define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float vn = (float)(AXIS_NO - V0) / (float)(V1 - V0); if (vn > 1.0f) vn = 1.0f; if (vn > 0.0f && io.NavInputs[NAV_NO] < vn) io.NavInputs[NAV_NO] = vn; }
	const int thumb_dead_zone = 8000;           // SDL_gamecontroller.h suggests using this value.
	MAP_BUTTON(ImGuiNavInput_Activate,      SCE_CTRL_ENTER);       // Cross / A
	MAP_BUTTON(ImGuiNavInput_Cancel,        SCE_CTRL_CANCEL);      // Circle / B
	MAP_BUTTON(ImGuiNavInput_Menu,          SCE_CTRL_SQUARE);      // Square / X
	MAP_BUTTON(ImGuiNavInput_Input,         SCE_CTRL_TRIANGLE);    // Triangle / Y
	MAP_BUTTON(ImGuiNavInput_DpadLeft,      SCE_CTRL_LEFT);        // D-Pad Left
	MAP_BUTTON(ImGuiNavInput_DpadRight,     SCE_CTRL_RIGHT);       // D-Pad Right
	MAP_BUTTON(ImGuiNavInput_DpadUp,        SCE_CTRL_UP);          // D-Pad Up
	MAP_BUTTON(ImGuiNavInput_DpadDown,      SCE_CTRL_DOWN);        // D-Pad Down
	MAP_BUTTON(ImGuiNavInput_FocusPrev,     SCE_CTRL_LTRIGGER);    // L1 / LB
	MAP_BUTTON(ImGuiNavInput_FocusNext,     SCE_CTRL_RTRIGGER);    // R1 / RB
	MAP_BUTTON(ImGuiNavInput_TweakSlow,     SCE_CTRL_LTRIGGER);    // L1 / LB
	MAP_BUTTON(ImGuiNavInput_TweakFast,     SCE_CTRL_RTRIGGER);    // R1 / RB
	MAP_ANALOG(ImGuiNavInput_LStickLeft,    rstick_x, -thumb_dead_zone, -32768);
	MAP_ANALOG(ImGuiNavInput_LStickRight,   rstick_x, +thumb_dead_zone, +32767);
	MAP_ANALOG(ImGuiNavInput_LStickUp,      rstick_y, -thumb_dead_zone, -32767);
	MAP_ANALOG(ImGuiNavInput_LStickDown,    rstick_y, +thumb_dead_zone, +32767);
	
	io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
	#undef MAP_BUTTON
	#undef MAP_ANALOG
}

void ImGui_ImplVitaGL_NewFrame(void) {
	ImGui_ImplVitaGL_Data *bd = ImGui_ImplVitaGL_GetBackendData();
	IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplVitaGL_Init()?");
	
	if (!bd->FontTexture)
		ImGui_ImplVitaGL_CreateDeviceObjects();

	ImGuiIO& io = ImGui::GetIO();
	
	// Setup display size (every frame to accommodate for window resizing)
	int w = 0, h = 0;
	int display_w = 0, display_h = 0;
	
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	w = display_w = viewport[2];
	h = display_h = viewport[3];
	
	io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
	
	if (w > 0 && h > 0)
		io.DisplayFramebufferScale = ImVec2(static_cast<float>(display_w / w),static_cast<float>(display_h) / h);
		
	static uint64_t frequency = 1000000;
	uint64_t current_time = sceKernelGetProcessTimeWide();
	io.DeltaTime = bd->g_Time > 0 ? static_cast<float>(static_cast<double>((current_time - bd->g_Time)) / frequency) : static_cast<float>(1.0f / 60.0f);
	bd->g_Time = current_time;
	
	ImGui_ImplVitaGL_UpdateGamepads();
}

static void ImGui_ImplVitaGL_SetupRenderState(ImDrawData *draw_data, int fb_width, int fb_height) {
	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers, polygon fill.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // In order to composite our output buffer we need to preserve alpha
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_SCISSOR_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel(GL_SMOOTH);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	
	// If you are using this code with non-legacy OpenGL header/contexts (which you should not, prefer using imgui_impl_opengl3.cpp!!),
	// you may need to backup/reset/restore other state, e.g. for current shader using the commented lines below.
	// (DO NOT MODIFY THIS FILE! Add the code in your calling function)
	//   GLint last_program;
	//   glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
	//   glUseProgram(0);
	//   ImGui_ImplVitaGL_RenderDrawData(...);
	//   glUseProgram(last_program)
	// There are potentially many more states you could need to clear/setup that we can't access from default headers.
	// e.g. glBindBuffer(GL_ARRAY_BUFFER, 0), glDisable(GL_TEXTURE_CUBE_MAP).
	
	// Setup viewport, orthographic projection matrix
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
	glViewport(0, 0, static_cast<GLsizei>(fb_width), static_cast<GLsizei>(fb_height));
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(draw_data->DisplayPos.x, draw_data->DisplayPos.x + draw_data->DisplaySize.x, draw_data->DisplayPos.y + draw_data->DisplaySize.y, draw_data->DisplayPos.y, -1.0f, +1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}

// OpenGL2 Render function.
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly.
// This is in order to be able to run within an OpenGL engine that doesn't do so.
void ImGui_ImplVitaGL_RenderDrawData(ImDrawData *draw_data) {
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	int fb_width = static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
	int fb_height = static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
	if (fb_width == 0 || fb_height == 0)
		return;
		
	// Backup GL state
	GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
	GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
	GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
	//GLint last_shade_model; glGetIntegerv(GL_SHADE_MODEL, &last_shade_model);
	//GLint last_tex_env_mode; glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &last_tex_env_mode);
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
	
	// Setup desired GL state
	ImGui_ImplVitaGL_SetupRenderState(draw_data, fb_width, fb_height);
	
	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
	
	// Render command lists
	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		const ImDrawList *cmd_list = draw_data->CmdLists[n];
		const ImDrawVert *vtx_buffer = cmd_list->VtxBuffer.Data;
		const ImDrawIdx *idx_buffer = cmd_list->IdxBuffer.Data;
		glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), reinterpret_cast<const GLvoid *>(reinterpret_cast<const char *>(vtx_buffer) + IM_OFFSETOF(ImDrawVert, pos)));
		glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), reinterpret_cast<const GLvoid *>(reinterpret_cast<const char *>(vtx_buffer) + IM_OFFSETOF(ImDrawVert, uv)));
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), reinterpret_cast<const GLvoid *>(reinterpret_cast<const char *>(vtx_buffer) + IM_OFFSETOF(ImDrawVert, col)));
		
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
			const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback) {
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					ImGui_ImplVitaGL_SetupRenderState(draw_data, fb_width, fb_height);
				else
					pcmd->UserCallback(cmd_list, pcmd);
			}
			else {
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
				ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
				if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
					continue;
					
				// Apply scissor/clipping rectangle (Y is inverted in OpenGL)
				glScissor(static_cast<int>(clip_min.x), static_cast<int>(fb_height - clip_max.y), static_cast<int>(clip_max.x - clip_min.x), static_cast<int>(clip_max.y - clip_min.y));
				
				// Bind texture, Draw
				glBindTexture(GL_TEXTURE_2D, reinterpret_cast<GLuint>(pcmd->GetTexID()));
				glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(pcmd->ElemCount), sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer + pcmd->IdxOffset);
			}
		}
	}
	
	// Restore modified GL state
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(last_texture));
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
	glPolygonMode(GL_FRONT, static_cast<GLenum>(last_polygon_mode[0])); glPolygonMode(GL_BACK, static_cast<GLenum>(last_polygon_mode[1]));
	glViewport(last_viewport[0], last_viewport[1], static_cast<GLsizei>(last_viewport[2]), static_cast<GLsizei>(last_viewport[3]));
	glScissor(last_scissor_box[0], last_scissor_box[1], static_cast<GLsizei>(last_scissor_box[2]), static_cast<GLsizei>(last_scissor_box[3]));
	//glShadeModel(last_shade_model);
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, last_tex_env_mode);
}

bool ImGui_ImplVitaGL_CreateFontsTexture(void) {
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplVitaGL_Data *bd = ImGui_ImplVitaGL_GetBackendData();
	unsigned char *pixels = nullptr;
	int width = 0, height = 0;

	ImFontConfig font_config;
	font_config.OversampleH = 1;
	font_config.OversampleV = 1;
	font_config.PixelSnapH = 1;
	
	io.Fonts->AddFontFromFileTTF("sa0:/data/font/pvf/jpn0.pvf", 20.0f, &font_config, io.Fonts->GetGlyphRangesJapanese());
	io.Fonts->GetTexDataAsRGBA32(static_cast<unsigned char **>(&pixels), &width, &height);

	// Upload texture to graphics system
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGenTextures(1, &bd->FontTexture);
	glBindTexture(GL_TEXTURE_2D, bd->FontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	
	// Store our identifier
	io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(bd->FontTexture));
	
	// Restore state
	glBindTexture(GL_TEXTURE_2D, last_texture);
	return true;
}

void ImGui_ImplVitaGL_DestroyFontsTexture(void) {
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplVitaGL_Data *bd = ImGui_ImplVitaGL_GetBackendData();
	
	if (bd->FontTexture) {
		glDeleteTextures(1, &bd->FontTexture);
		io.Fonts->SetTexID(0);
		bd->FontTexture = 0;
    }
}

bool ImGui_ImplVitaGL_CreateDeviceObjects(void) {
	return ImGui_ImplVitaGL_CreateFontsTexture();
}

void ImGui_ImplVitaGL_DestroyDeviceObjects(void) {
	ImGui_ImplVitaGL_DestroyFontsTexture();
}
