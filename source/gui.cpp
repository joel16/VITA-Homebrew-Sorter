#include <algorithm>
#include <cstdio>
#include <psp2/power.h>
#include <psp2/kernel/clib.h>
#include <SDL.h>

#include "applist.h"
#include "config.h"
#include "fs.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_internal.h"
#include "log.h"
#include "loadouts.h"
#include "tabs.h"
#include "utils.h"

namespace GUI {
    static bool backupExists = false;
    static const ImVec2 tex_size = ImVec2(20, 20);
    
    static SDL_Window *window;
    static SDL_Renderer *renderer;

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

    static void Prompt(State &state, AppEntries &entries, std::vector<SceIoDirent> &loadouts, const std::string &db_name) {
        if (state == StateNone) {
            return;
        }
        
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
                        if ((AppList::Save(entries.icons)) == 0) {
                            Config::Save(cfg);
                            state = StateDone;
                        }
                        else {
                            state = StateError;
                        }
                        break;

                    case StateConfirmSwap:
                        AppList::Backup();
                        backupExists = true;
                        if ((AppList::SavePages(entries.pages)) == 0) {
                            state = StateDone;
                        }
                        else {
                            state = StateError;
                        }
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

    static void Begin(void) {
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
    }
    
    static void End(ImGuiIO &io, ImVec4 clear_color, SDL_Renderer *renderer) {
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }
    
    SDL_Renderer *GetRenderer(void) {
        return renderer;
    }

    SDL_Window *GetWindow(void) {
        return window;
    }

    int Init(void) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
            Log::Error("SDL_Init failed: %s\n", SDL_GetError());
            return -1;
        }
        
        // Create window with SDL_Renderer graphics context
        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        window = SDL_CreateWindow("VITA Customizer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 544, window_flags);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        if (renderer == nullptr) {
            Log::Error("SDL_CreateRenderer failed: %s\n", SDL_GetError());
            return 0;
        }
        
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
        io.IniFilename = nullptr;

        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);

        // Build font atlas
        unsigned char *pixels = nullptr;
        int width = 0, height = 0, bytes_per_pixel = 0;
        
        ImFontConfig font_config;
        font_config.OversampleH = 1;
        font_config.OversampleV = 1;
        font_config.PixelSnapH = 1;
        
        io.Fonts->AddFontFromFileTTF("sa0:/data/font/pvf/jpn0.pvf", 20.0f, std::addressof(font_config), io.Fonts->GetGlyphRangesJapanese());
        io.Fonts->GetTexDataAsAlpha8(std::addressof(pixels), std::addressof(width), std::addressof(height), std::addressof(bytes_per_pixel));
        io.Fonts->Build();
        
        // Set theme/style
        ImGui::GetStyle().FrameRounding = 4.0f;
        ImGui::GetStyle().GrabRounding = 4.0f;
        
        ImVec4 *colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.07f, 0.13f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
        colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
        colors[ImGuiCol_TabActive] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.65f, 0.16f, 0.31f, 1.0f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

        return 0;
    }

    void Exit(void) {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void DisableButtonInit(bool disable) {
        if (disable) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
    }

    void DisableButtonExit(bool disable) {
        if (disable) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
    }

    int RenderLoop(void) {
        bool done = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        backupExists = (FS::FileExists("ux0:/data/VITAHomebrewSorter/backup/app.db") || FS::FileExists("ux0:/data/VITAHomebrewSorter/backup/app.db.bkp"));
        
        AppEntries entries;
        std::vector<SceIoDirent> loadouts;
        
        // Initial sort based on cfg.sort_mode
        AppList::Get(entries);
        std::sort(entries.icons.begin(), entries.icons.end(), cfg.sort_mode == SortAsc? AppList::SortAppDesc : AppList::SortAppDesc);
        std::sort(entries.child_apps.begin(), entries.child_apps.end(), cfg.sort_mode == SortAsc? AppList::SortChildAppDesc : AppList::SortChildAppDesc);
        
        FS::GetDirList("ux0:data/VITAHomebrewSorter/loadouts", loadouts);
        
        int date_format = Utils::GetDateFormat();
        
        std::string loadout_name;
        State state = StateNone;

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        
        while (!done) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event);

                switch (event.type) {
                    case SDL_QUIT:
                        done = true;
                        break;

                    case SDL_WINDOWEVENT:
                        if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
                            done = true;
                        }
                        break;

                    case SDL_CONTROLLERBUTTONDOWN:
                        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
                            done = true;
                        }
                        break;
                }
            }

            GUI::Begin();
            GUI::SetupWindow();

            if (ImGui::Begin("VITA Homebrew Sorter", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
                if (ImGui::BeginTabBar("VITA Homebrew Sorter tabs")) {
                    Tabs::Sort(entries, state, backupExists);
                    Tabs::Pages(entries, state, backupExists);
                    GUI::DisableButtonInit(!cfg.beta_features);
                    Tabs::Loadouts(loadouts, state, date_format, loadout_name);
                    GUI::DisableButtonExit(!cfg.beta_features);
                    Tabs::Settings();
                    ImGui::EndTabBar();
                }
            }

            GUI::ExitWindow();
            GUI::Prompt(state, entries, loadouts, loadout_name.c_str());
            GUI::End(io, clear_color, renderer);
        }

        return 0;
    }
}
