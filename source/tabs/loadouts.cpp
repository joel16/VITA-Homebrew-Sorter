#include "fs.h"
#include "imgui.h"
#include "loadouts.h"
#include "tabs.h"
#include "textures.h"
#include "utils.h"

namespace Tabs {
    static const ImVec2 tex_size = ImVec2(20, 20);

    void Loadouts(std::vector<SceIoDirent> &loadouts, State &state, int &date_format, std::string &loadout_name) {
        ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTabItem("Loadouts")) {
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button("Backup current loadout", ImVec2(ImGui::GetContentRegionAvail().x * 1.0f, 0.0f))) {
                if (R_SUCCEEDED(Loadouts::Backup())) {
                    FS::GetDirList("ux0:data/VITAHomebrewSorter/loadouts", loadouts);
                }
            }
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::BeginTable("LoadoutList", 4, tableFlags)) {
                if (loadouts.empty()) {
                    ImGui::Text("No loadouts found");
                }
                else {
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Title");
                    ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableHeadersRow();
                    
                    for (unsigned int i = 0; i < loadouts.size(); i++) {
                        ImGui::TableNextRow();
                        
                        ImGui::TableNextColumn();
                        ImGui::Image(reinterpret_cast<ImTextureID>(icons[DB].ptr), tex_size);
                        
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
                        if (ImGui::ImageButton("", reinterpret_cast<ImTextureID>(icons[Trash].ptr), tex_size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f))) {
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
}
