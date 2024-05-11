#include <algorithm>

#include "config.h"
#include "gui.h"
#include "imgui.h"
#include "tabs.h"
#include "textures.h"

namespace Tabs {
    static const ImVec2 tex_size = ImVec2(20, 20);
    static const char *sort_by[] = {"Title", "Title ID"};
    static const char *sort_folders[] = {"Both", "Apps only", "Folders only"};

    void Sort(AppEntries &entries, State &state, bool &backupExists) {
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

                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
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
                        
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
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
            if (ImGui::Button("Apply Sort", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0.0f))) {
                state = StateConfirmSort;
            }
            GUI::DisableButtonExit(cfg.sort_mode == SortDefault);

            ImGui::SameLine();
            
            GUI::DisableButtonInit(!backupExists);
            if (ImGui::Button("Restore Backup", ImVec2(ImGui::GetContentRegionAvail().x * 1.0f, 0.0f))) {
                state = StateRestore;
            }
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
                        ImGui::Image(reinterpret_cast<ImTextureID>(icons[Folder].ptr), tex_size);
                        
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
                        ImGui::Image(reinterpret_cast<ImTextureID>(icons[App].ptr), tex_size);
                        
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
}