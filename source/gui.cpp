#include <algorithm>
#include <cstdio>
#include <imgui_vita.h>
#include <vitaGL.h>

#include "applist.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "textures.h"

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
    enum State {
        StateNone,
        StateConfirm,
        StateDone
    };

    static void SetupPopup(const char *id) {
        ImGui::OpenPopup(id);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
        ImGui::SetNextWindowPos(ImVec2(480.0f, 272.0f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    };
    
    static void ExitPopup(void) {
        ImGui::EndPopup();
        ImGui::PopStyleVar();
    };

    static void SetupWindow(void) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(960.0f, 544.0f), ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    };
    
    static void ExitWindow(void) {
        ImGui::End();
        ImGui::PopStyleVar();
    };

    void RenderSplash(void) {
        bool done = false;
        SceRtcTick start, now;

        sceRtcGetCurrentTick(&start);

        while(!done) {
            sceRtcGetCurrentTick(&now);

            Renderer::Start();
            GUI::SetupWindow();
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

            if (ImGui::Begin("splash", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
                ImGui::SetCursorPos((ImGui::GetContentRegionAvail() - ImVec2((splash.width), (splash.height))) * 0.5f);
                ImGui::Image(reinterpret_cast<ImTextureID>(splash.id), ImVec2(splash.width, splash.height));
            }
            
            GUI::ExitWindow();
            ImGui::PopStyleVar();
            Renderer::End(ImVec4(0.05f, 0.07f, 0.13f, 1.00f));

            if ((now.tick - start.tick) >= 5000000)
                done = true;
        }
    }

    static void ConfirmPopup(State *state, std::vector<AppInfoIcon> &entries) {
        ImGui::OpenPopup("Confirmation");
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::SetNextWindowPos(ImVec2(480.0f, 272.0f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal("Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to apply this sorting method?");
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            ImGui::Text("A restart is required to view the changes.");
            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            if (ImGui::Button("Ok", ImVec2(120, 0))) {
                AppList::Backup();
                AppList::Save(entries);
                ImGui::CloseCurrentPopup();
                *state = StateDone;
            }

            ImGui::SameLine(0.0f, 15.0f);

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                *state = StateNone;
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();
    }

    static void RestartPopup(State *state) {
        ImGui::OpenPopup("Restart");
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::SetNextWindowPos(ImVec2(480.0f, 272.0f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal("Restart", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Do you wish to restart your device?");
            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            if (ImGui::Button("Ok", ImVec2(120, 0)))
                scePowerRequestColdReset();

            ImGui::SameLine(0.0f, 15.0f);

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                *state = StateNone;
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();
    }

    int RenderLoop(void) {
        bool done = false;

        std::vector<AppInfoIcon> entries;
        std::vector<AppInfoPage> pages;
        std::vector<AppInfoFolder> folders;
        int ret = AppList::Get(entries, pages, folders);

        enum SortMode {
            SortDefault,
            SortAsc,
            SortDesc
        };

        enum IconType {
            App,
            Folder,
        };
        
        static SortMode sort = SortDefault;
        static State state = StateNone;

        ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
        
        while (!done) {
            Renderer::Start();
            GUI::SetupWindow();

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
                
                if (sort == SortDefault) {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }

                if (ImGui::Button("Apply Sort"))
                    state = StateConfirm;

                if (sort == SortDefault) {
                    ImGui::PopItemFlag();
                    ImGui::PopStyleVar();
                }
                
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

                if (ImGui::BeginTable("AppEntries", 5, tableFlags)) {
                    ImGui::TableSetupColumn("");
                    ImGui::TableSetupColumn("Title");
                    ImGui::TableSetupColumn("Page ID");
                    ImGui::TableSetupColumn("Page Number");
                    ImGui::TableSetupColumn("Position");
                    ImGui::TableHeadersRow();

                    for (int i = 0; i < entries.size(); i++) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::Image(reinterpret_cast<ImTextureID>(entries[i].folder? icons[Folder].id : icons[App].id), ImVec2(20, 20));
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

            GUI::ExitWindow();

            switch(state) {
                case StateNone:
                    break;

                case StateConfirm:
                    GUI::ConfirmPopup(&state, entries);
                    break;

                case StateDone:
                    GUI::RestartPopup(&state);
                    break;

            }

            Renderer::End(ImVec4(0.45f, 0.55f, 0.60f, 1.00f));
        }

        return 0;
    }
}
