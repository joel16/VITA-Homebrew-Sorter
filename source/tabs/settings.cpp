#include <SDL.h>
#include <string>

#include "config.h"
#include "imgui.h"
#include "sqlite3.h"

namespace Tabs {
    static SDL_version sdlVersion;

    void Settings(void) {
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

            SDL_GetVersion(&sdlVersion);
            ImGui::Text("SDL2 version: %u.%u.%u", sdlVersion.major, sdlVersion.minor, sdlVersion.patch);
            
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
}
