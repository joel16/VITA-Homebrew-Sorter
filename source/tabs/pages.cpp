#include "gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "tabs.h"
#include "textures.h"

namespace Tabs {
    static int old_page_id = -1;
    static const ImVec2 tex_size = ImVec2(20, 20);

    void Pages(AppEntries &entries, State &state, bool &backupExists) {
        ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter;
        
        if (ImGui::BeginTabItem("Pages")) {
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button("Reset", ImVec2(ImGui::GetContentRegionAvail().x * 0.33f, 0.0f))) {
                AppList::Get(entries);
                old_page_id = -1;
            }

            ImGui::SameLine();

            if (ImGui::Button("Apply Changes", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0.0f))) {
                state = StateConfirmSwap;
            }

            ImGui::SameLine();

            GUI::DisableButtonInit(!backupExists);
            if (ImGui::Button("Restore Backup", ImVec2(ImGui::GetContentRegionAvail().x * 1.0f, 0.0f))) {
                state = StateRestore;
            }
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
                    ImGui::Image(reinterpret_cast<ImTextureID>(icons[Page].ptr), tex_size);
                    
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", entries.pages[i].pageId);

                    ImGui::TableNextColumn();
                    std::string pageNo = std::to_string(entries.pages[i].pageNo);
                    const bool is_selected = (old_page_id == static_cast<int>(i));
                    if (ImGui::Selectable(pageNo.c_str(), is_selected)) {
                        if (old_page_id == -1) {
                            old_page_id = i;
                        }
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
}
