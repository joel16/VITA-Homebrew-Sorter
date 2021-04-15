// Implemented features:
//  [X] User texture binding. Cast 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

#include <stdio.h>
#include <stdlib.h>
#include <vitaGL.h>
#include <math.h>
#include "imgui.h"
#include "imgui_impl_vitagl.h"
#include "utils.h"

#define lerp(value, from_max, to_max) ((((value*10) * (to_max*10))/(from_max*10))/10)

// Data
static uint64_t g_Time = 0;
static bool g_MousePressed[3] = { false, false, false };
static GLuint g_FontTexture = 0;
static SceCtrlData g_OldPad;
static int hires_x = 0;
static int hires_y = 0;

static float *startVertex = nullptr;
static float *startTexCoord = nullptr;
static uint8_t *startColor = nullptr;
static float *gVertexBuffer = nullptr;
static float *gTexCoordBuffer = nullptr;
static uint8_t *gColorBuffer = nullptr;
static uint16_t *gIndexBuffer = nullptr;
static uint32_t gCounter = 0;

static bool shaders_usage = false;

static void ImGui_ImplVitaGL_SetupRenderState(ImDrawData* draw_data, int fb_width, int fb_height) {
	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers, polygon fill.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	//glDisable(GL_LIGHTING);
	//glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_SCISSOR_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	
	// If you are using this code with non-legacy OpenGL header/contexts (which you should not, prefer using imgui_impl_opengl3.cpp!!),
	// you may need to backup/reset/restore current shader using the lines below. DO NOT MODIFY THIS FILE! Add the code in your calling function:
	//  GLint last_program;
	//  glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
	//  glUseProgram(0);
	//  ImGui_ImplOpenGL2_RenderDrawData(...);
	//  glUseProgram(last_program)
	
	// Setup viewport, orthographic projection matrix
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
	glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(draw_data->DisplayPos.x, draw_data->DisplayPos.x + draw_data->DisplaySize.x, draw_data->DisplayPos.y + draw_data->DisplaySize.y, draw_data->DisplayPos.y, -1.0f, +1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}

// OpenGL2 Render function.
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so. 
void ImGui_ImplVitaGL_RenderDrawData(ImDrawData* draw_data) {
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
	int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
	if (fb_width == 0 || fb_height == 0)
		return;
		
	// Backup GL state
	GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
	GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
	GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
	//GLint last_tex_env_mode; glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &last_tex_env_mode);
	//glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
	
	// Setup desired GL state
	ImGui_ImplVitaGL_SetupRenderState(draw_data, fb_width, fb_height);
	
	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
	
	// Render command lists
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
		const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
		glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos)));
		glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv)));
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col)));
		
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					ImGui_ImplVitaGL_SetupRenderState(draw_data, fb_width, fb_height);
				else
					pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				// Project scissor/clipping rectangles into framebuffer space
				ImVec4 clip_rect;
				clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
				clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
				clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
				clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;
				
				if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
				{
					float *vp = gVertexBuffer;
					float *tp = gTexCoordBuffer;
					uint8_t *cp = gColorBuffer;
					uint8_t *indices = (uint8_t*)idx_buffer;
					
					// Apply scissor/clipping rectangle
					glScissor((int)clip_rect.x, (int)(fb_height - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));
					
					// Bind texture, Draw
					glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
					
					for (int idx=0; idx < pcmd->ElemCount; idx++)
					{
						uint16_t index = *((uint16_t*)(indices + sizeof(ImDrawIdx) * idx));
						float *vertices = (float*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos) + sizeof(ImDrawVert) * index);
						float *texcoords = (float*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv) + sizeof(ImDrawVert) * index);
						uint8_t *colors = (uint8_t*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col) + sizeof(ImDrawVert) * index);
						gVertexBuffer[0] = vertices[0];
						gVertexBuffer[1] = vertices[1];
						gVertexBuffer[2] = 0.0f;
						gTexCoordBuffer[0] = texcoords[0];
						gTexCoordBuffer[1] = texcoords[1];
						gColorBuffer[0] = colors[0];
						gColorBuffer[1] = colors[1];
						gColorBuffer[2] = colors[2];
						gColorBuffer[3] = colors[3];
						gVertexBuffer += 3;
						gTexCoordBuffer += 2;
						gColorBuffer += 4;
					}
					
					if (shaders_usage)
					{
						vglVertexAttribPointerMapped(0, vp);
						vglVertexAttribPointerMapped(1, tp);
						vglVertexAttribPointerMapped(2, cp);
					}
					else
					{
						vglVertexPointerMapped(vp);
						vglTexCoordPointerMapped(tp);
						vglColorPointerMapped(GL_UNSIGNED_BYTE, cp);
					}

					vglDrawObjects(GL_TRIANGLES, pcmd->ElemCount, GL_TRUE);
				}
			}
			idx_buffer += pcmd->ElemCount;
			gCounter += pcmd->ElemCount;
			if (gCounter > 0x99900)
			{
				gVertexBuffer = startVertex;
				gColorBuffer = startColor;
				gTexCoordBuffer = startTexCoord;
				gCounter = 0;
			}
		}
	}
	
	// Restore modified GL state
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	//glPopAttrib();
	glPolygonMode(GL_FRONT, (GLenum)last_polygon_mode[0]); glPolygonMode(GL_BACK, (GLenum)last_polygon_mode[1]);
	glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
	glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, last_tex_env_mode);
}

bool ImGui_ImplVitaGL_CreateDeviceObjects() {
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
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
	glGenTextures(1, &g_FontTexture);
	glBindTexture(GL_TEXTURE_2D, g_FontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	
	// Store our identifier
	io.Fonts->TexID = reinterpret_cast<ImTextureID>(g_FontTexture);
	
	// Restore state
	glBindTexture(GL_TEXTURE_2D, last_texture);
	return true;
}

void ImGui_ImplVitaGL_DestroyFontsTexture() {
	if (g_FontTexture) {
		ImGuiIO& io = ImGui::GetIO();
		glDeleteTextures(1, &g_FontTexture);
		io.Fonts->TexID = 0;
		g_FontTexture = 0;
	}
}

bool ImGui_ImplVitaGL_Init() {
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	
	// Setup back-end capabilities flags
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDrawCursor = false;
	
	// Initializing buffers
	startVertex = (float*)malloc(sizeof(float) * 0x100000 * 3);
	startTexCoord = (float*)malloc(sizeof(float) * 0x100000 * 2);
	startColor = (uint8_t*)malloc(sizeof(uint8_t) * 0x100000 * 4);
	gIndexBuffer = (uint16_t*)malloc(sizeof(uint16_t) * 0x5000);
	
	gVertexBuffer = startVertex;
	gColorBuffer = startColor;
	gTexCoordBuffer = startTexCoord;
	
	for (uint16_t i = 0; i < 0x5000; i++)
	{
		gIndexBuffer[i] = i;
	}
	
	io.ClipboardUserData = NULL;
	return true;
}

void ImGui_ImplVitaGL_Shutdown() {
	// Destroy buffers
	free(gVertexBuffer);
	free(gTexCoordBuffer);
	free(gColorBuffer);
	free(gIndexBuffer);
	
	// Destroy OpenGL objects
	ImGui_ImplVitaGL_DestroyFontsTexture();
}

static void ImGui_ImplVitaGL_UpdateGamepads() {
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

void ImGui_ImplVitaGL_NewFrame() {
	if (!g_FontTexture)
		ImGui_ImplVitaGL_CreateDeviceObjects();
		
	ImGuiIO& io = ImGui::GetIO();
	
	// Setup display size (every frame to accommodate for window resizing)
	int w, h;
	int display_w, display_h;
	GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
	w = display_w = viewport[2];
	h = display_h = viewport[3];
	io.DisplaySize = ImVec2((float)w, (float)h);
	if (w > 0 && h > 0)
		io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);
		
	static uint64_t frequency = 1000000;
	uint64_t current_time = sceKernelGetProcessTimeWide();
	io.DeltaTime = g_Time > 0 ? (float)((double)(current_time - g_Time) / frequency) : (float)(1.0f / 60.0f);
	g_Time = current_time;
	
	ImGui_ImplVitaGL_UpdateGamepads();
	ImGui::NewFrame();
	vglIndexPointerMapped(gIndexBuffer);
}
