// dear imgui: Renderer Backend for OpenGL2 (legacy OpenGL, fixed pipeline)
// This needs to be used along with a Platform Backend (e.g. GLFW, SDL, Win32, custom..)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID!

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this. 
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in imgui_impl_opengl3.cpp**
// This code is mostly provided as a reference to learn how ImGui integration works, because it is shorter to read.
// If your code is using GL3+ context or any semi modern OpenGL calls, using this is likely to make everything more
// complicated, will require your code to reset every single OpenGL attributes to their initial state, and might
// confuse your GPU driver.
// The GL2 code is unable to reset attributes or even call e.g. "glUseProgram(0)" because they don't exist in that API.

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

IMGUI_API bool ImGui_ImplVitaGL_Init(void);
IMGUI_API void ImGui_ImplVitaGL_Shutdown(void);
IMGUI_API void ImGui_ImplVitaGL_NewFrame(void);
IMGUI_API void ImGui_ImplVitaGL_RenderDrawData(ImDrawData* draw_data);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API bool ImGui_ImplVitaGL_CreateDeviceObjects(void);
IMGUI_API void ImGui_ImplVitaGL_DestroyDeviceObjects(void);
