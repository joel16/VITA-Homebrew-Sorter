// Implemented features:
//  [X] User texture binding. Cast 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

IMGUI_API bool ImGui_ImplVitaGL_Init(void);
IMGUI_API void ImGui_ImplVitaGL_Shutdown(void);
IMGUI_API void ImGui_ImplVitaGL_NewFrame(void);
IMGUI_API void ImGui_ImplVitaGL_RenderDrawData(ImDrawData* draw_data);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API bool ImGui_ImplVitaGL_CreateDeviceObjects(void);
IMGUI_API void ImGui_ImplVitaGL_DestroyDeviceObjects(void);
