#include <algorithm>
#include <cstdio>
#include <psp2/power.h>
#include <psp2/kernel/clib.h>
#include <vitaGL.h>

#include "applist.h"
#include "config.h"
#include "fs.h"
#include "imgui_impl_vitagl.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "loadouts.h"
#include "sqlite3.h"
#include "textures.h"
#include "utils.h"

namespace Renderer {
    static void End(bool clear, ImVec4 clear_color) {
        glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));

        if (clear) {
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        ImGui::Render();
        ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
        vglSwapBuffers(GL_TRUE);
    }
}

namespace GUI {
    enum State {
        StateNone,
        StateConfirmSort,
        StateConfirmSwap,
        StateRestore,
        StateLoadoutRestore,
        StateWarning,
        StateDone,
        StateDelete,
        StateError
    };

    enum SortMode {
        SortDefault,
        SortAsc,
        SortDesc
    };
    
    enum IconType {
        App,
        DB,
        Folder,
        Page,
        Trash
    };

    static bool backupExists = false;
    static const ImVec2 tex_size = ImVec2(20, 20);
    static const char *sort_by[] = {"Title", "Title ID"};
    static const char *sort_folders[] = {"Both", "Apps only", "Folders only"};
    static int old_page_id = -1;

    static void SetupPopup(const char *id) {
        ImGui::OpenPopup(id);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, tex_size);
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

    static void Prompt(State &state, AppEntries &entries, std::vector<SceIoDirent> &loadouts, const std::string &db_name) {
        if (state == StateNone)
            return;
        
        std::string title, prompt;

        switch (state) {
            case StateConfirmSort:
                title = "Confirm";
                prompt = "Are you sure you want to apply this sorting method? This may take a minute.";
                break;

            case StateConfirmSwap:
                title = "Confirm";
                prompt = "Are you sure you want to apply this change?";
                break;

            case StateRestore:
                title = "Restore Backup";
                prompt = "Are you sure you want to restore this backup?";
                break;

            case StateLoadoutRestore:
                title = "Restore Loadout";
                prompt = "Are you sure you want to apply this loadout?";
                break;

            case StateWarning:
                title = "Warning";
                prompt = "This loadout is outdated and may not include some newly installed apps.";
                break;

            case StateDone:
                title = "Restart";
                prompt = "Do you wish to restart your device?";
                break;

            case StateDelete:
                title = "Delete";
                prompt = "Are you sure you want to delete this loadout?";
                break;

            case StateError:
                title = "Error";
                prompt = "An error occured and has been logged. The app will restore your app.db backup.";
                break;

            default:
                break;
        }

        GUI::SetupPopup(title.c_str());
        
        if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text(prompt.c_str());

            if ((state == StateConfirmSort) || (state == StateRestore) || (state == StateLoadoutRestore)) {
                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                ImGui::Text("You must reboot your device for the changes to take effect.");
            }
            else if (state == StateWarning) {
                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                ImGui::Text("Do you still wish to continue restoring this loadout?");
            }

            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            if (ImGui::Button("Ok", ImVec2(120, 0))) {
                switch (state) {
                    case StateConfirmSort:
                        AppList::Backup();
                        backupExists = true;
                        if ((AppList::Save(entries.icons)) == 0)
                            state = StateDone;
                        else
                            state = StateError;
                        break;

                    case StateConfirmSwap:
                        AppList::Backup();
                        backupExists = true;
                        if ((AppList::SavePages(entries.pages)) == 0)
                            state = StateDone;
                        else
                            state = StateError;
                        break;

                    case StateRestore:
                        AppList::Restore();
                        state = StateDone;
                        break;

                    case StateLoadoutRestore:
                        if (AppList::Compare(db_name)) {
                            state = StateWarning;
                        }
                        else {
                            Loadouts::Restore(db_name);
                            state = StateDone;
                        }
                        break;

                    case StateWarning:
                        Loadouts::Restore(db_name);
                        state = StateDone;
                        break;

                    case StateDone:
                        scePowerRequestColdReset();
                        break;

                    case StateDelete:
                        Loadouts::Delete(db_name);
                        FS::GetDirList("ux0:data/VITAHomebrewSorter/loadouts", loadouts);
                        state = StateNone;
                        break;

                    case StateError:
                        state = StateNone;
                        break;

                    default:
                        break;
                }

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine(0.0f, 15.0f);

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                state = StateNone;
            }
        }

        GUI::ExitPopup();
    }

    static void SortTab(AppEntries &entries, State &state) {
        ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
        
        if (ImGui::BeginTabItem("Sort/Backup")) {
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            ImGui::PushID("sort_by");
            ImGui::PushItemWidth(100.f);
            if (ImGui::BeginCombo("", sort_by[cfg.sort_by])) {
                for (int i = 0; i < IM_ARRAYSIZE(sort_by); i++) {
                    const bool is_selected = (cfg.sort_by == i);
                    
                    if (ImGui::Selectable(sort_by[i], is_selected)) {
                        cfg.sort_by = i;
                        Config::Save(cfg);
                    }

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            ImGui::PopID();

            ImGui::SameLine();
            
            ImGui::PushID("sort_folders");
            ImGui::PushItemWidth(150.f);
            if (ImGui::BeginCombo("", sort_folders[cfg.sort_folders])) {
                for (int i = 0; i < IM_ARRAYSIZE(sort_folders); i++) {
                    const bool is_selected = (cfg.sort_folders == i);
                    
                    if (ImGui::Selectable(sort_folders[i], is_selected)) {
                        cfg.sort_folders = i;
                        Config::Save(cfg);
                    }
                        
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            ImGui::PopID();
            
            ImGui::SameLine();
            
            if (ImGui::RadioButton("Default", cfg.sort_mode == SortDefault)) {
                cfg.sort_mode = SortDefault;
                AppList::Get(entries);
            }
            
            ImGui::SameLine();
            
            if (ImGui::RadioButton("Asc", cfg.sort_mode == SortAsc)) {
                cfg.sort_mode = SortAsc;
                AppList::Get(entries);
                std::sort(entries.icons.begin(), entries.icons.end(), AppList::SortAppAsc);
                std::sort(entries.child_apps.begin(), entries.child_apps.end(), AppList::SortChildAppAsc);
                AppList::Sort(entries);
            }
            
            ImGui::SameLine();
            
            if (ImGui::RadioButton("Desc", cfg.sort_mode == SortDesc)) {
                cfg.sort_mode = SortDesc;
                AppList::Get(entries);
                std::sort(entries.icons.begin(), entries.icons.end(), AppList::SortAppDesc);
                std::sort(entries.child_apps.begin(), entries.child_apps.end(), AppList::SortChildAppDesc);
                AppList::Sort(entries);
            }
            
            ImGui::SameLine();
            
            GUI::DisableButtonInit(cfg.sort_mode == SortDefault);
            if (ImGui::Button("Apply Sort", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0.0f)))
                state = StateConfirmSort;
            GUI::DisableButtonExit(cfg.sort_mode == SortDefault);

            ImGui::SameLine();
            
            GUI::DisableButtonInit(!backupExists);
            if (ImGui::Button("Restore Backup", ImVec2(ImGui::GetContentRegionAvail().x * 1.0f, 0.0f)))
                state = StateRestore;
            GUI::DisableButtonExit(!backupExists);
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::BeginTable("AppList", 5, tableFlags)) {
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Title");
                ImGui::TableSetupColumn("Page ID", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Page No", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Pos", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();
                
                for (unsigned int i = 0, counter = 0; i < entries.icons.size(); i++) {
                    if (entries.icons[i].icon0Type == 7) {
                        ImGui::TableNextRow();
                        
                        ImGui::TableNextColumn();
                        ImGui::Image(reinterpret_cast<ImTextureID>(icons[Folder].id), tex_size);
                        
                        ImGui::TableNextColumn();
                        std::string title = std::to_string(counter) + ") ";
                        title.append(entries.icons[i].title);
                        bool open = ImGui::TreeNodeEx(title.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", entries.icons[i].pageId);
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", entries.icons[i].pageNo);
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", entries.icons[i].pos);
                        
                        if (open) {
                            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth;
                            int reserved01 = std::stoi(std::string(entries.icons[i].reserved01));
                            
                            for (unsigned int j = 0; j < entries.child_apps.size(); j++) {
                                if (entries.child_apps[j].pageNo == reserved01) {
                                    ImGui::TableNextRow();
                                    ImGui::TableNextColumn();
                                    
                                    ImGui::TableNextColumn();
                                    ImGui::TreeNodeEx(entries.child_apps[j].title, flags);
                                    
                                    ImGui::TableNextColumn();
                                    ImGui::Text("%d", entries.child_apps[j].pageId);
                                    
                                    ImGui::TableNextColumn();
                                    ImGui::Text("-");
                                    
                                    ImGui::TableNextColumn();
                                    ImGui::Text("%d", entries.child_apps[j].pos);
                                }
                            }
                            
                            ImGui::TreePop();
                        }

                        counter++;
                    }
                    else if (entries.icons[i].pageNo >= 0) {
                        ImGui::TableNextRow();
                        
                        ImGui::TableNextColumn();
                        ImGui::Image(reinterpret_cast<ImTextureID>(icons[App].id), tex_size);
                        
                        ImGui::TableNextColumn();
                        std::string title = std::to_string(counter) + ") ";
                        title.append(entries.icons[i].title);
                        ImGui::Selectable(title.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", entries.icons[i].pageId);
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", entries.icons[i].pageNo);
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", entries.icons[i].pos);
                        
                        counter++;
                    }
                }
                
                ImGui::EndTable();
            }
            
            ImGui::EndTabItem();
        }
    }

    static void PagesTab(AppEntries &entries, State &state) {
        ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter;
        
        if (ImGui::BeginTabItem("Pages")) {
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button("Reset", ImVec2(ImGui::GetContentRegionAvail().x * 0.33f, 0.0f))) {
                AppList::Get(entries);
                old_page_id = -1;
            }

            ImGui::SameLine();

            if (ImGui::Button("Apply Changes", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0.0f)))
                state = StateConfirmSwap;

            ImGui::SameLine();

            GUI::DisableButtonInit(!backupExists);
            if (ImGui::Button("Restore Backup", ImVec2(ImGui::GetContentRegionAvail().x * 1.0f, 0.0f)))
                state = StateRestore;
            GUI::DisableButtonExit(!backupExists);
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::BeginTable("PagesList", 3, tableFlags)) {
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("pageId");
                ImGui::TableSetupColumn("pageNo");
                ImGui::TableHeadersRow();
                
                for (unsigned int i = 0; i < entries.pages.size(); i++) {
                    ImGui::TableNextRow();
                    
                    ImGui::TableNextColumn();
                    ImGui::Image(reinterpret_cast<ImTextureID>(icons[Page].id), tex_size);
                    
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", entries.pages[i].pageId);

                    ImGui::TableNextColumn();
                    std::string pageNo = std::to_string(entries.pages[i].pageNo);
                    const bool is_selected = (old_page_id == static_cast<int>(i));
                    if (ImGui::Selectable(pageNo.c_str(), is_selected)) {
                        if (old_page_id == -1)
                            old_page_id = i;
                        else {
                            int temp = entries.pages[i].pageNo;
                            entries.pages[i].pageNo = entries.pages[old_page_id].pageNo;
                            entries.pages[old_page_id].pageNo = temp;
                            old_page_id = -1;
                            ImGui::ClearActiveID();
                        }
                    }
                }

                ImGui::EndTable();
            }

            ImGui::EndTabItem();
        }
    }

    static void LoadoutsTab(std::vector<SceIoDirent> &loadouts, State &state, int &date_format, std::string &loadout_name) {
        ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTabItem("Loadouts")) {
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button("Backup current loadout", ImVec2(ImGui::GetContentRegionAvail().x * 1.0f, 0.0f))) {
                if (R_SUCCEEDED(Loadouts::Backup()))
                    FS::GetDirList("ux0:data/VITAHomebrewSorter/loadouts", loadouts);
            }
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::BeginTable("LoadoutList", 4, tableFlags)) {
                if (loadouts.empty())
                    ImGui::Text("No loadouts found");
                else {
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Title");
                    ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableHeadersRow();
                    
                    for (unsigned int i = 0; i < loadouts.size(); i++) {
                        ImGui::TableNextRow();
                        
                        ImGui::TableNextColumn();
                        ImGui::Image(reinterpret_cast<ImTextureID>(icons[DB].id), tex_size);
                        
                        ImGui::TableNextColumn();
                        if (ImGui::Selectable(loadouts[i].d_name, false)) {
                            loadout_name = loadouts[i].d_name;
                            state = StateLoadoutRestore;
                        }
                        
                        ImGui::TableNextColumn();
                        char date[16];
                        Utils::GetDateString(date, static_cast<SceSystemParamDateFormat>(date_format), loadouts[i].d_stat.st_mtime);
                        ImGui::Text(date);

                        ImGui::TableNextColumn();
                        ImGui::PushID(i);
                        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(icons[Trash].id), tex_size, ImVec2(0, 0), ImVec2(1, 1), 0)) {
                            loadout_name = loadouts[i].d_name;
                            state = StateDelete;
                        }
                        ImGui::PopID();
                    }
                }
                
                ImGui::EndTable();
            }
            
            ImGui::EndTabItem();
        }
    }

    static void SettingsTab(void) {
        if (ImGui::BeginTabItem("Settings")) {
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            ImGui::Indent(5.f);
            ImGui::TextColored(ImVec4(0.70f, 0.16f, 0.31f, 1.0f), "Beta features:");
            ImGui::Indent(15.f);

            if (ImGui::RadioButton("Enabled", cfg.beta_features == true)) {
                cfg.beta_features = true;
                Config::Save(cfg);
            }
            
            ImGui::SameLine();
            
            if (ImGui::RadioButton("Disabled", cfg.beta_features == false)) {
                cfg.beta_features = false;
                Config::Save(cfg);
            }

            ImGui::Dummy(ImVec2(0.0f, 10.0f)); // Spacing
            ImGui::Unindent();
            
            ImGui::Indent(5.f);
            ImGui::TextColored(ImVec4(0.70f, 0.16f, 0.31f, 1.0f), "App Info:");
            ImGui::Indent(15.f);
            std::string version = APP_VERSION;
            version.erase(0, std::min(version.find_first_not_of('0'), version.size() - 1));
            ImGui::Text("App version: %s", version.c_str());
            ImGui::Text("Author: Joel16");
            ImGui::Text("Assets: PreetiSketch");
            ImGui::Text("Dear imGui version: %s", ImGui::GetVersion());
            ImGui::Text("SQLite 3 version: %s", sqlite3_libversion());
            
            ImGui::Dummy(ImVec2(0.0f, 10.0f)); // Spacing
            ImGui::Unindent();
            
            ImGui::Indent(5.f);
            ImGui::TextColored(ImVec4(0.70f, 0.16f, 0.31f, 1.0f), "Usage:");
            ImGui::Indent(15.f);
            std::string usage = std::string("VITA Homebrew Sorter is a basic PS VITA homebrew application that sorts the application database in your LiveArea.")
                + " The application sorts apps and games that are inside folders as well. This applications also allows you to backup your current 'loadout' that "
                + " you can switch into as you wish. A backup will be made before any changes are applied to the application database."
                + " This backup is overwritten each time you use the sort option. You can find the backup in ux0:/data/VITAHomebrewSorter/backups/app.db."
                + " \n\nIt is always recommended to restart your vita so that it can refresh your livearea/app.db for any changes (deleted icons, new folders, etc.)"
                + " before you run this application.";
            ImGui::TextWrapped(usage.c_str());
            ImGui::Unindent();
            
            ImGui::EndTabItem();
        }
    }

    int RenderLoop(void) {
        bool done = false;
        backupExists = (FS::FileExists("ux0:/data/VITAHomebrewSorter/backup/app.db") || FS::FileExists("ux0:/data/VITAHomebrewSorter/backup/app.db.bkp"));
        
        AppEntries entries;
        std::vector<SceIoDirent> loadouts;
        
        AppList::Get(entries);
        FS::GetDirList("ux0:data/VITAHomebrewSorter/loadouts", loadouts);
        
        int date_format = Utils::GetDateFormat();
        
        std::string loadout_name;
        State state = StateNone;
        SceCtrlData pad = { 0 };
        
        while (!done) {
            ImGui_ImplVitaGL_NewFrame();
            ImGui::NewFrame();
            GUI::SetupWindow();

            if (ImGui::Begin("VITA Homebrew Sorter", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
                if (ImGui::BeginTabBar("VITA Homebrew Sorter tabs")) {
                    GUI::SortTab(entries, state);
                    GUI::PagesTab(entries, state);
                    GUI::DisableButtonInit(!cfg.beta_features);
                    GUI::LoadoutsTab(loadouts, state, date_format, loadout_name);
                    GUI::DisableButtonExit(!cfg.beta_features);
                    GUI::SettingsTab();
                    ImGui::EndTabBar();
                }
            }

            GUI::ExitWindow();
            GUI::Prompt(state, entries, loadouts, loadout_name.c_str());
            Renderer::End(true, ImVec4(0.45f, 0.55f, 0.60f, 1.00f));

            pad = Utils::ReadControls();

            if (pressed & SCE_CTRL_START)
                done = true;
        }

        return 0;
    }
}
