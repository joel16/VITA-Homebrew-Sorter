#include <algorithm>
#include <cstdio>
#include <imgui_vita.h>
#include <vitaGL.h>

#include "applist.h"

namespace Renderer {
    static void Start(void) {
        ImGui_ImplVitaGL_NewFrame();
    }
    
    static void End(ImVec4 clear_color) {
        glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
        vglSwapBuffers(GL_FALSE);
    }
}

namespace GUI {
    int Confirmpopup(std::vector<AppInfoIcon> &entries) {
        return 0;
    }

    int RenderLoop(void) {
        bool done = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        std::vector<AppInfoIcon> entries;
        std::vector<AppInfoPage> pages;
        std::vector<AppInfoFolder> folders;
        int ret = AppList::Get(entries, pages, folders);

        enum SortMode {
            SortDefault,
            SortAsc,
            SortDesc
        };
        
        static int sort = 0;
        ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
        
        while (!done) {
            Renderer::Start();

            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(960.0f, 544.0f), ImGuiCond_Once);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

            if (ImGui::Begin("VITA Homebrew Sorter", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
                if (ImGui::RadioButton("Default", sort == SortDefault)) {
                    sort = SortDefault;
                    ret = AppList::Get(entries, pages, folders);
                }

                ImGui::SameLine();

                if (ImGui::RadioButton("Asc", sort == SortAsc)) {
                    sort = SortAsc;
                    std::sort(entries.begin(), entries.end(), AppList::SortAlphabeticalAsc);
                    AppList::Sort(entries, pages, folders);
                }
                
                ImGui::SameLine();
                
                if (ImGui::RadioButton("Desc", sort == SortDesc)) {
                    sort = SortDesc;
                    std::sort(entries.begin(), entries.end(), AppList::SortAlphabeticalDesc);
                    AppList::Sort(entries, pages, folders);
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Apply Sort")) {
                    AppList::Backup();
                    AppList::Save(entries);
                }
                
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

                if (ImGui::BeginTable("AppEntries", 4, tableFlags)) {
                    ImGui::TableSetupColumn("Title");
                    ImGui::TableSetupColumn("Page ID");
                    ImGui::TableSetupColumn("Page Number");
                    ImGui::TableSetupColumn("Position");
                    ImGui::TableHeadersRow();

                    for (int i = 0; i < entries.size(); i++) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        
                        ImGui::Selectable(entries[i].title.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", entries[i].pageId);
                        
                        ImGui::TableNextColumn();
                        if (entries[i].pageNo < 0)
                            ImGui::Text("Inside folder");
                        else
                            ImGui::Text("%d", entries[i].pageNo);
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", entries[i].pos);
                    }
                    
                    ImGui::EndTable();
                }
            }
            
            ImGui::End();
            ImGui::PopStyleVar();

            Renderer::End(clear_color);
        }

        return 0;
    }
}
