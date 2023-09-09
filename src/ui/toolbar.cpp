#include "toolbar.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <string>
#include <variant>
#include <optional>

#include <imgui/imgui.h>
#include <glad/glad.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <fmt/format.h>
#include "../utils/formatter.hpp"
#include <spdlog/spdlog.h>

#include "settings.h"
#include "../scene/camera.h"
#include "../scene/object.h"
#include "../utils/math.hpp"
#include "../utils/rendering.hpp"
#include "../utils/kinetic_state.h"
#include "../render/render_engine.h"
#include "../simulation/solver.h"

using namespace UI;
using Eigen::AngleAxisf;
using Eigen::Matrix4f;
using Eigen::Quaternionf;
using Eigen::Vector3f;
using fmt::format;
using std::get_if;
using std::holds_alternative;
using std::optional;
using std::size_t;
using std::string;

constexpr float FLOAT_INF     = std::numeric_limits<float>::max();
constexpr float POSITION_UNIT = 0.02f;
constexpr float ANGLE_UNIT    = 0.2f;
constexpr float SCALING_UNIT  = 0.1f;
constexpr float PHYSICS_UNIT  = 0.01f;

Toolbar::Toolbar(WorkingMode& mode, const SelectableType& selected_element)
    : mode(mode), selected_element(selected_element)
{
    mode = WorkingMode::LAYOUT;
    glGenTextures(1, &gl_rendered_texture);
    glBindTexture(GL_TEXTURE_2D, gl_rendered_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Toolbar::~Toolbar()
{
    glDeleteTextures(1, &(gl_rendered_texture));
}

void Toolbar::render(Scene& scene)
{
    ImGui::SetNextWindowSize(ImVec2(px(300.0f), px(500.0f)), ImGuiCond_Once);
    ImGui::Begin("Tools");
    if (ImGui::BeginTabBar("Mode")) {
        layout_mode(scene);
        model_mode(scene);
        render_mode(scene);
        simulate_mode(scene);
        ImGui::EndTabBar(); // Mode
    }
    ImGui::End(); // End Tools
}

void Toolbar::scene_hierarchies(Scene& scene)
{
    ImGui::SeparatorText("Scene");
    Object* clicked_object = nullptr;
    ImGuiTreeNodeFlags group_node_flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    ImGuiTreeNodeFlags object_node_flags = ImGuiTreeNodeFlags_SpanAvailWidth |
                                           ImGuiTreeNodeFlags_Leaf |
                                           ImGuiTreeNodeFlags_NoTreePushOnOpen;
    for (size_t i = 0; i < scene.groups.size(); ++i) {
        const auto& group        = scene.groups[i];
        const string group_label = fmt::format("{} (ID: {})", group->name, group->id);
        if (ImGui::TreeNodeEx(group_label.c_str(), group_node_flags)) {
            for (const auto& object : group->objects) {
                ImGuiTreeNodeFlags flags = object_node_flags;
                if (object.get() == scene.selected_object) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }
                ImGui::TreeNodeEx(object->name.c_str(), flags);
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    clicked_object = object.get();
                }
            }
            ImGui::TreePop();
        }
    }
    // Only call on_object_selected with clicked_object when there is an
    // object is selected, because the tree list is not designed to support
    // un-selection.
    if (clicked_object != nullptr) {
        on_element_selected(clicked_object);
    }
}

void Toolbar::xyz_drag(float* x, float* y, float* z, float v_speed, const char* format)
{
    float min_val = -FLOAT_INF;
    float max_val = FLOAT_INF;
    if (v_speed == SCALING_UNIT) {
        min_val = 0.1f;
        max_val = 1000.0f;
    }
    ImGui::PushItemWidth(0.33f * ImGui::CalcItemWidth());
    ImGui::DragFloat("x", x, v_speed, min_val, max_val, format);
    ImGui::SameLine();
    ImGui::DragFloat("y", y, v_speed, min_val, max_val, format);
    ImGui::SameLine();
    ImGui::DragFloat("z", z, v_speed, min_val, max_val, format);
    ImGui::PopItemWidth();
}

void Toolbar::material_editor(GL::Material& material)
{
    static constexpr ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs;
    ImGui::SeparatorText("Material");
    ImGui::ColorEdit3("Ambient", material.ambient.data(), flags);
    ImGui::SameLine();
    ImGui::ColorEdit3("Diffuse", material.diffuse.data(), flags);
    ImGui::SameLine();
    ImGui::ColorEdit3("Specular", material.specular.data(), flags);
    ImGui::SliderFloat("Shininess", &material.shininess, 0.0f, 1e6f, "%.1f",
                       ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Objects whose shininess > %.1f will be treated as mirrors by the "
                          "Whitted-Style Ray-Tracer",
                          WhittedRenderer::mirror_threshold);
    }
}

void Toolbar::layout_mode(Scene& scene)
{
    if (ImGui::BeginTabItem("Layout")) {
        if (mode != WorkingMode::LAYOUT) {
            on_selection_canceled();
            mode = WorkingMode::LAYOUT;
        }
        scene_hierarchies(scene);

        Object* selected_object = scene.selected_object;
        if (selected_object != nullptr) {
            material_editor(selected_object->mesh.material);
            ImGui::SeparatorText("Transform");
            ImGui::Text("Translation");
            ImGui::PushID("Translation##");
            Vector3f& center = selected_object->center;
            xyz_drag(&center.x(), &center.y(), &center.z(), POSITION_UNIT);
            ImGui::PopID();

            ImGui::Text("Scaling");
            ImGui::PushID("Scaling##");
            Vector3f& scaling = selected_object->scaling;
            xyz_drag(&scaling.x(), &scaling.y(), &scaling.z(), SCALING_UNIT);
            ImGui::PopID();

            const Quaternionf& r             = selected_object->rotation;
            auto [x_angle, y_angle, z_angle] = quaternion_to_ZYX_euler(r.w(), r.x(), r.y(), r.z());
            ImGui::Text("Rotation (ZYX Euler)");
            ImGui::PushID("Rotation##");
            ImGui::PushItemWidth(0.3f * ImGui::CalcItemWidth());
            ImGui::DragFloat("pitch", &x_angle, ANGLE_UNIT, -180.0f, 180.0f, "%.1f deg",
                             ImGuiSliderFlags_AlwaysClamp);
            ImGui::SameLine();
            ImGui::DragFloat("yaw", &y_angle, ANGLE_UNIT, -90.0f, 90.0f, "%.1f deg",
                             ImGuiSliderFlags_AlwaysClamp);
            ImGui::SameLine();
            ImGui::DragFloat("roll", &z_angle, ANGLE_UNIT, -180.0f, 180.0f, "%.1f deg",
                             ImGuiSliderFlags_AlwaysClamp);
            ImGui::PopItemWidth();
            ImGui::PopID();
            selected_object->rotation = AngleAxisf(radians(x_angle), Vector3f::UnitX()) *
                                        AngleAxisf(radians(y_angle), Vector3f::UnitY()) *
                                        AngleAxisf(radians(z_angle), Vector3f::UnitZ());
        }
        ImGui::EndTabItem();
    }
}

void Toolbar::model_mode(Scene& scene)
{
    if (ImGui::BeginTabItem("Model")) {
        mode = WorkingMode::MODEL;

        bool no_halfedge_mesh = !scene.halfedge_mesh;
        bool halfedge_mesh_failed =
            scene.halfedge_mesh && scene.halfedge_mesh->error_info.has_value();
        if (no_halfedge_mesh || halfedge_mesh_failed) {
            if (no_halfedge_mesh) {
                ImGui::TextWrapped("No halfedge mesh has been built yet");
            }
            if (halfedge_mesh_failed) {
                ImGui::TextWrapped(
                    "Failed to build a halfedge mesh for the current"
                    "object, or the halfedge mesh was broken by some invalid operation");
            }
            ImGui::Text("No operation available");
            ImGui::EndTabItem();
            return;
        }

        ImGui::SeparatorText("Local Operations");
        if (holds_alternative<const Halfedge*>(selected_element)) {
            const Halfedge* h = std::get<const Halfedge*>(selected_element);
            ImGui::Text("Halfedge (ID: %zu)", h->id);
            if (ImGui::Button("Inverse")) {
                const Halfedge* t = h->inv;
                if (!(t->is_boundary())) {
                    on_element_selected(t);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Next")) {
                on_element_selected(h->next);
            }
            ImGui::SameLine();
            if (ImGui::Button("Previous")) {
                on_element_selected(h->prev);
            }
            if (ImGui::Button("From")) {
                on_element_selected(h->from);
            }
            ImGui::SameLine();
            if (ImGui::Button("Edge")) {
                on_element_selected(h->edge);
            }
            ImGui::SameLine();
            if (ImGui::Button("Face")) {
                on_element_selected(h->face);
            }
        } else if (holds_alternative<Vertex*>(selected_element)) {
            Vertex* v = std::get<Vertex*>(selected_element);
            ImGui::Text("Vertex (ID: %zu)", v->id);
            Vector3f& position = v->pos;
            if (ImGui::Button("Halfedge")) {
                on_element_selected(v->halfedge);
            }
            ImGui::Text("Position");
            ImGui::PushID("Selected Vertex##");
            xyz_drag(&position.x(), &position.y(), &position.z(), POSITION_UNIT);
            ImGui::PopID();
        } else if (holds_alternative<Edge*>(selected_element)) {
            Edge* e = std::get<Edge*>(selected_element);
            ImGui::Text("Edge (ID: %zu)", e->id);
            Vector3f center = e->center();
            if (ImGui::Button("Halfedge")) {
                on_element_selected(e->halfedge);
            }
            if (ImGui::Button("Flip")) {
                optional<Edge*> result = scene.halfedge_mesh->flip_edge(e);
                if (result.has_value()) {
                    scene.halfedge_mesh->global_inconsistent = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Split")) {
                on_selection_canceled();
                optional<Vertex*> result = scene.halfedge_mesh->split_edge(e);
                if (result.has_value()) {
                    scene.halfedge_mesh->global_inconsistent = true;
                    on_element_selected(result.value());
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Collapse")) {
                on_selection_canceled();
                optional<Vertex*> result = scene.halfedge_mesh->collapse_edge(e);
                if (result.has_value()) {
                    scene.halfedge_mesh->global_inconsistent = true;
                    on_element_selected(result.value());
                }
            }
            ImGui::Text("Position");
            ImGui::PushID("Selected Edge##");
            xyz_drag(&center.x(), &center.y(), &center.z(), POSITION_UNIT);
            ImGui::PopID();
            // Only sync the positions of endpoints if no local operation has been performed.
            // Because an local operation makes the halfedge mesh and the mesh inconsistent,
            // and no modification should take place at the inconsistent state.
            if (!scene.halfedge_mesh->global_inconsistent) {
                Vector3f delta = center - e->center();
                Vertex* v1     = e->halfedge->from;
                Vertex* v2     = e->halfedge->inv->from;
                v1->pos += delta;
                v2->pos += delta;
            }
        } else if (holds_alternative<Face*>(selected_element)) {
            Face* f = std::get<Face*>(selected_element);
            ImGui::Text("Face (ID: %zu)", f->id);
            Vector3f center = f->center();
            if (ImGui::Button("Halfedge")) {
                on_element_selected(f->halfedge);
            }
            ImGui::Text("Position");
            ImGui::PushID("Selected Face##");
            xyz_drag(&center.x(), &center.y(), &center.z(), POSITION_UNIT);
            ImGui::PopID();
            Vector3f delta = center - f->center();
            Halfedge* h    = f->halfedge;
            do {
                h->from->pos += delta;
                h = h->next;
            } while (h != f->halfedge);
        }

        ImGui::SeparatorText("Global Operations");
        if (ImGui::Button("Loop Subdivide")) {
            scene.halfedge_mesh->loop_subdivide();
        }
        ImGui::SameLine();
        if (ImGui::Button("Simplify")) {
            scene.halfedge_mesh->simplify();
        }
        ImGui::SameLine();
        if (ImGui::Button("Isotropic Remesh")) {
            scene.halfedge_mesh->isotropic_remesh();
        }

        ImGui::EndTabItem();
    }
}

const char* renderer_names[] = {"Rasterizer Renderer", "Rasterizer Renderer (MT)",
                                "Whitted-Style Ray-Tracer"};

void Toolbar::render_mode(Scene& scene)
{
    if (ImGui::BeginTabItem("Render")) {
        // un-select the selected object (if there is a selected object)
        if (mode != WorkingMode::RENDER) {
            on_selection_canceled();
            mode = WorkingMode::RENDER;
        }
        bool open_rendered_image = false;
        bool always_true         = true;

        static RenderEngine render_engine;
        static bool rendering_ready          = false;
        static int renderer_index            = 0;
        static RendererType current_renderer = RendererType::RASTERIZER;

        ImGui::Combo("Renderer", &renderer_index, renderer_names, 3);
        switch (renderer_index) {
        case 0: current_renderer = RendererType::RASTERIZER; break;
        case 1: current_renderer = RendererType::RASTERIZER_MT; break;
        case 2: current_renderer = RendererType::WHITTED_STYLE; break;
        default: break;
        }
        if (current_renderer == RendererType::RASTERIZER_MT) {
            ImGui::SetNextItemWidth(0.5f * ImGui::CalcItemWidth());
            ImGui::InputInt("Number of Threads", &render_engine.n_threads);
        }
        if (current_renderer == RendererType::WHITTED_STYLE) {
            ImGui::Checkbox("Use BVH for Acceleration", &render_engine.whitted_render->use_bvh);
        }
        ImGui::ColorEdit3("Background Color", RenderEngine::background_color.data(),
                          ImGuiColorEditFlags_NoInputs);
        ImGui::SameLine();
        if (ImGui::Button("Render to Image")) {
            open_rendered_image = true;
            rendering_ready     = false;
        }

        ImGui::SeparatorText("Lights");
        Light* const* result  = get_if<Light*>(&selected_element);
        Light* selected_light = result != nullptr ? *result : nullptr;
        size_t index          = 1;
        for (auto& light : scene.lights) {
            const string light_name = format("Light {}", index);
            bool selected           = &light == selected_light;
            if (ImGui::Selectable(light_name.c_str(), selected)) {
                selected_light = &light;
                on_element_selected(selected_light);
            }
            ++index;
        }
        if (ImGui::Button("Add a Light")) {
            scene.lights.emplace_back(Vector3f(0.0f, 5.0f, 0.0f), 10.0f);
        }
        if (selected_light != nullptr) {
            ImGui::PushID("Selected Light##");
            Vector3f& position = selected_light->position;
            xyz_drag(&position.x(), &position.y(), &position.z(), POSITION_UNIT);
            ImGui::DragFloat("intensity", &(selected_light->intensity), 0.2f, 1.0f, FLOAT_INF,
                             "%.1f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::PopID();
        }

        ImGui::SeparatorText("Camera");
        ImGui::Text("Position");
        ImGui::PushID("Position##");
        Vector3f& position = scene.camera.position;
        xyz_drag(&position.x(), &position.y(), &position.z(), POSITION_UNIT);
        ImGui::PopID();

        ImGui::Text("Look At");
        ImGui::PushID("Look At##");
        Vector3f& target = scene.camera.target;
        xyz_drag(&target.x(), &target.y(), &target.z(), POSITION_UNIT);
        ImGui::PopID();

        ImGui::PushItemWidth(0.5f * ImGui::CalcItemWidth());
        ImGui::AlignTextToFramePadding();
        ImGui::BeginGroup();
        ImGui::Text("Aspect Ratio (W/H)");
        ImGui::SliderFloat("ratio", &(scene.camera.aspect_ratio), 1.0f, 3.0f, "%.1f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("FOV Y");
        ImGui::SliderFloat("fov", &(scene.camera.fov_y_degrees), 30.0f, 60.0f, "%.1f deg",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::EndGroup();
        ImGui::AlignTextToFramePadding();
        ImGui::BeginGroup();
        ImGui::Text("Near Plane");
        ImGui::SliderFloat("near", &(scene.camera.near), 0.0001f, scene.camera.far, "%.4f",
                           ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("Far Plane");
        ImGui::SliderFloat("far", &(scene.camera.far), scene.camera.near, 1000.0f, "%.1f",
                           ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
        ImGui::EndGroup();
        ImGui::PopItemWidth();

        if (open_rendered_image) {
            ImGui::OpenPopup("Rendered Image");
        }
        if (ImGui::BeginPopupModal("Rendered Image", &always_true)) {
            ImVec2 image_size(px(480.0f), px(480.0f / scene.camera.aspect_ratio));
            render_engine.width  = image_size.x;
            render_engine.height = image_size.y;
            if (!rendering_ready) {
                render_engine.render(scene, current_renderer);
                glBindTexture(GL_TEXTURE_2D, gl_rendered_texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GLsizei(image_size.x), GLsizei(image_size.y),
                             0, GL_RGB, GL_UNSIGNED_BYTE, render_engine.rendering_res.data());
                glBindTexture(GL_TEXTURE_2D, 0);
                rendering_ready = true;
            }
            ImGui::SetWindowSize(ImVec2(px(500.0f), px(400.0f)));
            ImGui::Image((void*)(intptr_t)gl_rendered_texture, image_size);
            ImGui::EndPopup();
        }

        ImGui::EndTabItem();
    }
}

const char* solver_names[] = {"Forward Euler", "4-th Runge-Kutta", "Backward Euler",
                              "Symplectic Euler"};

void Toolbar::simulate_mode(Scene& scene)
{
    if (ImGui::BeginTabItem("Simulate")) {
        if (mode != WorkingMode::SIMULATE) {
            on_selection_canceled();
            mode = WorkingMode::SIMULATE;
        }

        static int current_solver_index = 0;
        ImGui::Combo("Kinetic Solver", &current_solver_index, solver_names, 4);
        switch (current_solver_index) {
        case 0: Object::step = forward_euler_step; break;
        case 1: Object::step = runge_kutta_step; break;
        case 2: Object::step = backward_euler_step; break;
        case 3: Object::step = symplectic_euler_step; break;
        default: Object::step = forward_euler_step; break;
        }
        float fps = 1.0f / time_step;
        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() * 0.8f);
        ImGui::SliderFloat("Simulation FPS", &fps, 5.0f, 60.0f, "%.1f",
                           ImGuiSliderFlags_AlwaysClamp);
        time_step = 1.0f / fps;
        ImGui::Checkbox("Use BVH to accererate collision", &Object::BVH_for_collision);
        if (ImGui::Button("Start")) {
            scene.start_simulation();
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            scene.stop_simulation();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            scene.reset_simulation();
        }

        scene_hierarchies(scene);
        Object* selected_object = scene.selected_object;
        if (!scene.check_during_simulation() && selected_object != nullptr) {
            ImGui::SeparatorText("Physical Properties");
            ImGui::Text("Mass");
            ImGui::DragFloat("mass", &(selected_object->mass), PHYSICS_UNIT, -FLOAT_INF, FLOAT_INF,
                             "%.2f kg");

            ImGui::Text("Velocity");
            ImGui::PushID("Velocity##");
            Vector3f& velocity = selected_object->velocity;
            xyz_drag(&velocity.x(), &velocity.y(), &velocity.z(), PHYSICS_UNIT, "%.2f m/s");
            ImGui::PopID();

            ImGui::Text("Force");
            ImGui::PushID("Force##");
            Vector3f& force = selected_object->force;
            xyz_drag(&force.x(), &force.y(), &force.z(), PHYSICS_UNIT, "%.2f N");
            ImGui::PopID();
        }
        ImGui::EndTabItem();
    }
}
