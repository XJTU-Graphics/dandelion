#include <filesystem>

#ifdef _WIN32
    #include <Windows.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
// workaround for mingw64 clang, for its __GXX_ABI_VERSION version is 1002
// and it will cause file dialog to use old ui, which does not work correctly
#define PFD_HAS_IFILEDIALOG 1
#include <portable-file-dialogs.h>
#include <stb/stb_image.h>
#include <spdlog/spdlog.h>

#include "menubar.h"
#include "settings.h"
#include "help.inc"
#include "about.inc"

using namespace UI;
using std::string;
using std::vector;
namespace fs = std::filesystem;

const char* usage_title               = "Usage";
const char* about_title               = "About Us";
const char* debug_options_panel_title = "Debug Options";

DebugOptions::DebugOptions() : show_picking_ray(false), show_BVH(false)
{
}

Menubar::Menubar(DebugOptions& debug_options) : debug_options(debug_options)
{
    int            width, height, n_channels;
    unsigned char* icon_data;
    icon_data = stbi_load("./resources/icons/dandelion_64.png", &width, &height, &n_channels, 4);
    glGenTextures(1, &gl_icon_texture);
    glBindTexture(GL_TEXTURE_2D, gl_icon_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, icon_data);
    stbi_image_free(icon_data);
}

Menubar::~Menubar()
{
    glDeleteTextures(1, &gl_icon_texture);
}

void Menubar::render(Scene& scene)
{
    bool open_usage               = false;
    bool open_about               = false;
    bool open_debug_options_panel = false;
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import File as a Group")) {
                pfd::open_file file_dialog = pfd::open_file(
                    "Choose a file", ".", {"3D Scene / Object", "*.obj *.fbx *.dae"}
                );
                vector<string> result = file_dialog.result();
                if (!result.empty()) {
                    scene.import_group(result[0].c_str());
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("New Scene")) {
                auto button = pfd::message(
                                  "New Scene",
                                  "Your current scene will be cleared and forever lost. Continue?",
                                  pfd::choice::yes_no, pfd::icon::question
                )
                                  .result();
                if (button == pfd::button::yes) {
                    scene.clear();
                }
            }
            if (ImGui::MenuItem("Open Scene")) {
                pfd::select_folder file_dialog = pfd::select_folder("Choose scene folder");
                string             result      = file_dialog.result();
                if (!result.empty()) {
                    scene.clear();
                    try {
                        scene.load(result);
                    } catch (std::exception const& e) {
                        spdlog::error("failed to load scene: {}", e.what());
                        scene.clear();
                    }
                    spdlog::info("scene loaded from path {}", result);
                }
            }
            if (ImGui::MenuItem("Save Scene")) {
                pfd::select_folder file_dialog = pfd::select_folder("Choose save folder");
                string             result      = file_dialog.result();
                if (!result.empty()) {
                    if (fs::exists(result) && fs::is_directory(result)) {
                        auto dirIter = fs::directory_iterator(result);
                        bool empty   = true;
                        for (auto& _: dirIter) {
                            empty = false;
                            break;
                        }
                        bool should_save = empty;
                        if (!empty) {
                            auto button =
                                pfd::message(
                                    "Save Confirmation",
                                    "Folder has contents inside, do you want to save anyways?",
                                    pfd::choice::yes_no, pfd::icon::question
                                )
                                    .result();
                            if (button == pfd::button::yes) {
                                should_save = true;
                            }
                        }
                        spdlog::info("should save? {}", should_save);
                        if (should_save) {
                            try {
                                scene.save(result);
                            } catch (std::exception const& e) {
                                spdlog::error("failed to save scene: {}", e.what());
                            }
                            spdlog::info("scene saved to path {}", result);
                        }
                    } else {
                        spdlog::error("selected path is not a folder");
                    }
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            logging_levels_menu();
            if (ImGui::MenuItem("Debug Options")) {
                open_debug_options_panel = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Usage")) {
                open_usage = true;
            }
            if (ImGui::MenuItem("About Us...")) {
                open_about = true;
            }
            ImGui::EndMenu();
        }
        menubar_height = ImGui::GetWindowHeight();
        ImGui::TextColored(
            ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "FPS: %d", (int)ImGui::GetIO().Framerate
        );
        ImGui::EndMainMenuBar();
    }
    if (open_debug_options_panel) {
        ImGui::OpenPopup(debug_options_panel_title);
    }
    debug_options_panel();
    if (open_usage) {
        ImGui::OpenPopup(usage_title);
    }
    usage();
    if (open_about) {
        ImGui::OpenPopup(about_title);
    }
    about();
}

float UI::Menubar::height() const
{
    return menubar_height;
}

void Menubar::logging_levels_menu()
{
    if (ImGui::BeginMenu("Global Logging Level")) {
        if (ImGui::MenuItem("Warn")) {
            spdlog::set_level(spdlog::level::warn);
            spdlog::warn("set global logging level to warn");
        }
        if (ImGui::MenuItem("Info")) {
            spdlog::set_level(spdlog::level::info);
            spdlog::info("set global logging level to info");
        }
        if (ImGui::MenuItem("Debug")) {
            spdlog::set_level(spdlog::level::debug);
            spdlog::info("set global logging level to debug");
        }
        if (ImGui::MenuItem("Trace")) {
            spdlog::set_level(spdlog::level::trace);
            spdlog::info("set global logging level to trace");
        }
        ImGui::EndMenu();
    }
}

void Menubar::usage()
{
    // The Usage window.
    if (ImGui::BeginPopup(usage_title)) {
        ImGui::Text("%s", gui_usage);
        ImGui::EndPopup();
    }
}

/*
 * Note: When passing the parameter `p_open` to BeginPopupModal, a local
 * bool variable should be passed instead of a global one, and the variable
 * must be true. The example in imgui_demo.cpp is right but the explanation
 * in comment is wrong.
 */

void Menubar::about()
{
    bool always_true = true;
    // The About window.
    ImVec2 icon_size((64), (64));
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(about_title, &always_true)) {
        ImGui::SetWindowSize(ImVec2(px(300.0f), px(200.0f)));
        ImGui::Image((void*)(intptr_t)(gl_icon_texture), icon_size);
        ImGui::SameLine();
        ImGui::Text("Dandelion 3D");
        ImGui::TextWrapped("%s", about_message);
        ImGui::EndPopup();
    }
}

void Menubar::debug_options_panel()
{
    bool always_true = true;
    if (ImGui::BeginPopupModal(debug_options_panel_title, &always_true)) {
        ImGui::SetWindowSize(ImVec2(px(300.0f), px(200.0f)));
        ImGui::Checkbox("Show Picking Ray", &debug_options.show_picking_ray);
        ImGui::Checkbox("Show BVH", &debug_options.show_BVH);
        ImGui::EndPopup();
    }
}
