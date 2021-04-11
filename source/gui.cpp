#include <algorithm>
#include <cstdio>
#include <imgui_vita.h>
#include <vitaGL.h>

#include "applist.h"
#include "fs.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "loadouts.h"
#include "textures.h"
#include "utils.h"

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
        StateRestore,
        StateLoadoutRestore,
        StateDone
    };

    static bool backupExists = false;

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

    static void DisableButtonInit(bool disable) {
        if (disable) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
    }

    static void DisableButtonExit(bool disable) {
        if (disable) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
    }

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

            if ((now.tick - start.tick) >= 3000000)
                done = true;
        }
    }

    static void ConfirmRestorePopup(State *state, std::vector<AppInfoIcon> &entries, const char *name) {
        ImGui::OpenPopup(*state == StateConfirm? "Confirmation" : "Restoration");
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::SetNextWindowPos(ImVec2(480.0f, 272.0f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal(*state == StateConfirm? "Confirmation" : "Restoration", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            switch (*state) {
                case StateConfirm:
                    ImGui::Text("Are you sure you want to apply this sorting method?");
                    break;

                case StateRestore:
                    ImGui::Text("Are you sure you want to restore this backup?");
                    break;

                case StateLoadoutRestore:
                    ImGui::Text("Are you sure you want to apply this loadout?");
                    break;

                default:
                    break;
            }

            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            ImGui::Text("You must reboot your device for the changes to take effect.");
            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            if (ImGui::Button("Ok", ImVec2(120, 0))) {
                if (*state == StateConfirm) {
                    AppList::Backup();

                    if (!backupExists)
                        backupExists = true;
                    
                    AppList::Save(entries);
                }
                else if (*state == StateRestore)
                    AppList::Restore();
                else if (*state == StateLoadoutRestore)
                    Loadouts::Restore(name);
                
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
        backupExists = FS::FileExists("ux0:/data/VITAHomebrewSorter/backup/app.db");
        std::vector<AppInfoIcon> apps;
        std::vector<AppInfoPage> pages;
        std::vector<AppInfoFolder> folders;
        std::vector<SceIoDirent> loadouts;
        int ret = AppList::Get(apps, pages, folders);
        ret = FS::GetDirList("ux0:data/VITAHomebrewSorter/loadouts", loadouts);
        int date_format = Utils::GetDateFormat();
        std::string loadout_name;

        enum SortMode {
            SortDefault,
            SortAsc,
            SortDesc
        };

        enum IconType {
            App,
            DB,
            Folder
        };
        
        static SortMode sort = SortDefault;
        static State state = StateNone;

        ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
        
        while (!done) {
            Renderer::Start();
            GUI::SetupWindow();

            if (ImGui::Begin("VITA Homebrew Sorter", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
                if (ImGui::BeginTabBar("VITA Homebrew Sorter tabs", ImGuiTabBarFlags_None)) {
                    if (ImGui::BeginTabItem("Sort/Backup")) {
                        ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

                        if (ImGui::RadioButton("Default", sort == SortDefault)) {
                            sort = SortDefault;
                            ret = AppList::Get(apps, pages, folders);
                        }
                        
                        ImGui::SameLine();
                        
                        if (ImGui::RadioButton("Asc", sort == SortAsc)) {
                            sort = SortAsc;
                            ret = AppList::Get(apps, pages, folders);
                            std::sort(apps.begin(), apps.end(), AppList::SortAlphabeticalAsc);
                            AppList::Sort(apps, pages, folders);
                        }
                        
                        ImGui::SameLine();
                        
                        if (ImGui::RadioButton("Desc", sort == SortDesc)) {
                            sort = SortDesc;
                            ret = AppList::Get(apps, pages, folders);
                            std::sort(apps.begin(), apps.end(), AppList::SortAlphabeticalDesc);
                            AppList::Sort(apps, pages, folders);
                        }
                        
                        ImGui::SameLine();
                        
                        DisableButtonInit(sort == SortDefault);
                        if (ImGui::Button("Apply Sort"))
                            state = StateConfirm;
                        DisableButtonExit(sort == SortDefault);
                        
                        if (backupExists) {
                            ImGui::SameLine();
                            
                            if (ImGui::Button("Restore Backup"))
                                state = StateRestore;
                        }
                        
                        ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                        
                        if (ImGui::BeginTable("AppList", 5, tableFlags)) {
                            ImGui::TableSetupColumn("");
                            ImGui::TableSetupColumn("Title");
                            ImGui::TableSetupColumn("Page ID");
                            ImGui::TableSetupColumn("Page Number");
                            ImGui::TableSetupColumn("Position");
                            ImGui::TableHeadersRow();
                            
                            for (int i = 0; i < apps.size(); i++) {
                                ImGui::TableNextRow();
                                
                                ImGui::TableNextColumn();
                                ImGui::Image(reinterpret_cast<ImTextureID>(apps[i].folder? icons[Folder].id : icons[App].id), ImVec2(20, 20));
                                
                                ImGui::TableNextColumn();
                                ImGui::Selectable(apps[i].title.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
                                
                                ImGui::TableNextColumn();
                                ImGui::Text("%d", apps[i].pageId);
                                
                                ImGui::TableNextColumn();
                                if (apps[i].pageNo < 0)
                                    ImGui::Text("Inside folder");
                                else
                                    ImGui::Text("%d", apps[i].pageNo);
                                    
                                ImGui::TableNextColumn();
                                ImGui::Text("%d", apps[i].pos);
                            }
                            
                            ImGui::EndTable();
                        }

                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Loadouts")) {
                        ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

                        if (ImGui::Button("Backup current loadout")) {
                            if (R_SUCCEEDED(Loadouts::Backup()))
                                FS::GetDirList("ux0:data/VITAHomebrewSorter/loadouts", loadouts);
                        }

                        ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                        
                        if (ImGui::BeginTable("LoadoutList", 3, tableFlags)) {
                            ImGui::TableSetupColumn("");
                            ImGui::TableSetupColumn("Title");
                            ImGui::TableSetupColumn("Date");
                            ImGui::TableHeadersRow();

                            if (loadouts.empty()) {
                                ImGui::Text("No loadouts found");
                            }
                            else {
                                for (int i = 0; i < loadouts.size(); i++) {
                                    ImGui::TableNextRow();
                                    
                                    ImGui::TableNextColumn();
                                    ImGui::Image(reinterpret_cast<ImTextureID>(icons[DB].id), ImVec2(20, 20));
                                    
                                    ImGui::TableNextColumn();
                                    if (ImGui::Selectable(loadouts[i].d_name, false, ImGuiSelectableFlags_SpanAllColumns)) {
                                        loadout_name = loadouts[i].d_name;
                                        state = StateLoadoutRestore;
                                    }

                                    ImGui::TableNextColumn();
                                    char date[16];
                                    Utils::GetDateString(date, static_cast<SceSystemParamDateFormat>(date_format), &loadouts[i].d_stat.st_mtime);
                                    ImGui::Text(date);
                                }
                            }

                            ImGui::EndTable();
                        }
                        
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }
            }

            GUI::ExitWindow();

            switch(state) {
                case StateNone:
                    break;

                case StateConfirm:
                    GUI::ConfirmRestorePopup(&state, apps, loadout_name.c_str());
                    break;
                
                case StateRestore:
                    GUI::ConfirmRestorePopup(&state, apps, loadout_name.c_str());
                    break;

                case StateLoadoutRestore:
                    GUI::ConfirmRestorePopup(&state, apps, loadout_name.c_str());
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
