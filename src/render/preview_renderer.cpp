#include "preview_renderer.h"

#include <cstring>
#include <array>
#include <memory>
#include <vector>
#include <unordered_set>
#include <variant>

#include <Eigen/Core>

#include "../utils/logger.h"
#include "../utils/math.hpp"

using Eigen::Matrix4f;
using Eigen::Vector3f;
using std::array;
using std::holds_alternative;
using std::make_unique;
using std::memcpy;
using std::size_t;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

PreviewRenderer::PreviewRenderer()
{
    logger = get_logger("PreviewRenderer");
}

void PreviewRenderer::compile_shaders()
{
    primitive_shader = make_unique<Shader>(logger);
    primitive_shader->load_vertex_shader("resources/shaders/primitive.vert");
    primitive_shader->load_fragment_shader("resources/shaders/copy-color.frag");
    if (!primitive_shader->compile()) {
        logger->critical("Failed to compile primitive shader");
    }
    phong_shader = make_unique<Shader>(logger);
    phong_shader->load_vertex_shader("resources/shaders/phong.vert");
    phong_shader->load_fragment_shader("resources/shaders/copy-color.frag");
    if (!phong_shader->compile()) {
        logger->critical("Failed to compile Phong shader");
    }
}

void PreviewRenderer::delete_shaders()
{
    primitive_shader.reset();
    phong_shader.reset();
}

void PreviewRenderer::delete_drawable_resources()
{
    general_meshes.clear();
    general_linesets.clear();
    overlay_meshes.clear();
    overlay_linesets.clear();
}

void PreviewRenderer::render(Scene& scene, WorkingMode mode, const DebugOptions& debug_options)
{
    if (mode == WorkingMode::MODEL && scene.halfedge_mesh) {
        scene.halfedge_mesh->sync();
    }
    update_drawable_meshes(scene);
    update_drawable_linesets(scene);
    logger->trace("all render data has been transferred to GPU");

    const Matrix4f view_projection = scene.main_camera.projection() * scene.main_camera.view();

    // The Phong shader is used to render triangle meshes.
    logger->trace("rendering meshes with Phong material");
    phong_shader->use();
    phong_shader->set_uniform("view_projection", view_projection);
    phong_shader->set_uniform("camera_position", scene.main_camera.position);
    scene.for_each_object([this, &scene, mode](const Object& object) -> void {
        if (object.material->type() != MaterialType::Phong)
            return;
        const Mesh*             mesh             = &object.mesh;
        const PhongMaterial&    material         = dynamic_cast<PhongMaterial&>(*(object.material));
        const Matrix4f          model            = object.model();
        const GL::DrawableMesh* drawable_mesh    = general_meshes[mesh].get();
        const Matrix4f          normal_transform = model.inverse().transpose();
        if (mode == WorkingMode::MODEL && scene.selected_object == &object)
            phong_shader->set_uniform("model", I4f);
        else
            phong_shader->set_uniform("model", model);
        phong_shader->set_uniform("normal_transform", normal_transform);
        phong_shader->set_uniform("material.ambient", material.ambient);
        phong_shader->set_uniform("material.diffuse", material.diffuse);
        phong_shader->set_uniform("material.specular", material.specular);
        phong_shader->set_uniform("material.shininess", material.shininess);
        drawable_mesh->VAO.bind();
        drawable_mesh->triangles.bind();
        glDrawElements(
            GL_TRIANGLES, static_cast<GLsizei>(drawable_mesh->triangles.data.size()),
            GL_UNSIGNED_INT, (void*)0
        );
        drawable_mesh->triangles.release();
        drawable_mesh->VAO.release();
    });
    // Phong shader off

    // The primitive shader is used first to render primitives with a uniform color.
    primitive_shader->use();
    primitive_shader->set_uniform("view_projection", view_projection);
    primitive_shader->set_uniform("color", highlight_wireframe_color);
    unordered_set<const LineSet*> filtered_linesets = {&scene.camera_wireframe};
    logger->trace("rendering indicators requiring depth test");
    scene.for_each_object([this, &debug_options, &filtered_linesets](const Object& object) -> void {
        const LineSet* boxes = &object.BVH_boxes;
        filtered_linesets.insert(boxes);
        if (debug_options.show_BVH) {
            const GL::DrawableLineSet* drawable_lineset = general_linesets[boxes].get();
            primitive_shader->set_uniform("model", object.model());
            drawable_lineset->VAO.bind();
            glDrawElements(
                GL_LINES, static_cast<GLsizei>(drawable_lineset->lines.data.size()),
                GL_UNSIGNED_INT, (void*)0
            );
        }
    });
    primitive_shader->set_uniform("color", scene.camera_wireframe.color);
    primitive_shader->set_uniform("model", Matrix4f(scene.camera.view().inverse()));
    const LineSet* camera = &scene.camera_wireframe;
    general_linesets[&scene.camera_wireframe]->VAO.bind();
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(camera->positions.size()));
    glDrawElements(
        GL_LINES, static_cast<GLsizei>(camera->n_lines() * 2), GL_UNSIGNED_INT, (void*)0
    );
    logger->trace("rendering general line sets");
    primitive_shader->set_uniform("model", I4f);
    for (const auto& [lineset, drawable_lineset]: general_linesets) {
        if (filtered_linesets.contains(lineset))
            continue;
        primitive_shader->set_uniform("color", lineset->color);
        drawable_lineset->VAO.bind();
        glDrawElements(
            GL_LINES, static_cast<GLsizei>(drawable_lineset->lines.data.size()), GL_UNSIGNED_INT,
            (void*)0
        );
    }

    const bool highlight_selected_object =
        mode == WorkingMode::LAYOUT || mode == WorkingMode::SIMULATE || mode == WorkingMode::MODEL;
    if (highlight_selected_object) {
        render_selected_object(scene, mode);
    }
    // Disable depth test to render indicators.
    logger->trace("rendering overlay indicators");
    glDisable(GL_DEPTH_TEST);
    if (mode == WorkingMode::RENDER) {
        const Mesh* light_indicator = &scene.light_indicator;
        primitive_shader->set_uniform("color", default_wireframe_color);
        for (const Light& light: scene.lights) {
            Matrix4f model          = Matrix4f::Identity();
            model.block<3, 1>(0, 3) = light.position;
            primitive_shader->set_uniform("model", model);
            overlay_meshes[light_indicator]->VAO.bind();
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(light_indicator->positions.size()));
        }
    }
    primitive_shader->set_uniform("model", I4f);
    primitive_shader->set_uniform("color", highlight_face_color);
    const Mesh*             element      = &scene.highlighted_element;
    const GL::DrawableMesh* overlay_mesh = overlay_meshes[element].get();
    if (holds_alternative<const Halfedge*>(scene.selected_element)) {
        const LineSet* halfedge = &scene.highlighted_halfedge;
        overlay_linesets[halfedge]->VAO.bind();
        glDrawElements(
            GL_LINES, static_cast<GLsizei>(halfedge->n_lines() * 2), GL_UNSIGNED_INT, (void*)0
        );
    } else if (holds_alternative<Vertex*>(scene.selected_element)) {
        overlay_mesh->VAO.bind();
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(overlay_mesh->positions.count()));
    } else if (holds_alternative<Light*>(scene.selected_element)) {
        overlay_mesh->VAO.bind();
        const Light* light      = std::get<Light*>(scene.selected_element);
        Matrix4f     model      = Matrix4f::Identity();
        model.block<3, 1>(0, 3) = light->position;
        primitive_shader->set_uniform("model", model);
        overlay_mesh->VAO.bind();
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(overlay_mesh->positions.count()));
    } else if (holds_alternative<Edge*>(scene.selected_element)) {
        overlay_mesh->VAO.bind();
        overlay_mesh->edges.bind();
        glDrawElements(
            GL_LINES, static_cast<GLsizei>(overlay_mesh->edges.data.size()), GL_UNSIGNED_INT,
            (void*)0
        );
    } else if (holds_alternative<Face*>(scene.selected_element)) {
        overlay_mesh->VAO.bind();
        glDrawElements(
            GL_TRIANGLES, static_cast<GLsizei>(overlay_mesh->triangles.data.size()),
            GL_UNSIGNED_INT, (void*)0
        );
    }
    const LineSet* arrows = &scene.arrows;
    overlay_linesets[arrows]->VAO.bind();
    glDrawElements(
        GL_LINES, static_cast<GLsizei>(arrows->n_lines() * 2), GL_UNSIGNED_INT, (void*)0
    );
    glEnable(GL_DEPTH_TEST);
    // Primitive shader off
}

void PreviewRenderer::fill_drawable_mesh(const Mesh& mesh, GL::DrawableMesh& drawable_mesh)
{
    logger->trace("sync mesh \"{}\" to GPU", mesh.name);

    drawable_mesh.positions.data.resize(mesh.positions.size() * 3);
    memcpy(
        drawable_mesh.positions.data.data(), mesh.positions.data(),
        mesh.positions.size() * sizeof(Vector3f)
    );
    logger->trace(
        "{} float numbers copied as vertex positions", drawable_mesh.positions.data.size()
    );
    drawable_mesh.normals.data.resize(mesh.normals.size() * 3);
    memcpy(
        drawable_mesh.normals.data.data(), mesh.normals.data(),
        mesh.normals.size() * sizeof(Vector3f)
    );
    drawable_mesh.edges.data.resize(mesh.edges.size() * 2);
    memcpy(
        drawable_mesh.edges.data.data(), mesh.edges.data(),
        mesh.edges.size() * sizeof(array<unsigned int, 2>)
    );
    logger->trace(
        "{} unsigned int numbers copied as edge indices", drawable_mesh.edges.data.size()
    );
    drawable_mesh.triangles.data.resize(mesh.faces.size() * 3);
    memcpy(
        drawable_mesh.triangles.data.data(), mesh.faces.data(),
        mesh.faces.size() * sizeof(array<unsigned int, 3>)
    );
    logger->trace(
        "{} unsigned int numbers copied as triangle indices", drawable_mesh.triangles.data.size()
    );
    drawable_mesh.to_gpu();
}

void PreviewRenderer::fill_drawable_lineset(
    const LineSet& lineset, GL::DrawableLineSet& drawable_lineset
)
{
    logger->trace("sync line set \"{}\" to GPU", lineset.name);

    drawable_lineset.positions.data.resize(lineset.positions.size() * 3);
    memcpy(
        drawable_lineset.positions.data.data(), lineset.positions.data(),
        lineset.positions.size() * sizeof(Vector3f)
    );
    logger->trace(
        "{} float numbers copied as vertex positions", drawable_lineset.positions.data.size()
    );
    drawable_lineset.lines.data.resize(lineset.lines.size() * 2);
    memcpy(
        drawable_lineset.lines.data.data(), lineset.lines.data(),
        lineset.lines.size() * sizeof(array<unsigned int, 2>)
    );
    logger->trace(
        "{} unsigned int numbers copied as line indices", drawable_lineset.lines.data.size()
    );
    drawable_lineset.to_gpu();
}

void PreviewRenderer::update_drawable_meshes(Scene& scene)
{
    unordered_set<const Mesh*> updated_general_meshes;
    // Collect meshes from all objects in the scene.
    scene.for_each_object([this, &updated_general_meshes](Object& object) -> void {
        Mesh* mesh = &object.mesh;
        updated_general_meshes.insert(mesh);
        if (!mesh->modified)
            return;
        if (!general_meshes.contains(mesh)) [[unlikely]] {
            logger->info("create a drawable mesh for general mesh \"{}\"", mesh->name);
            general_meshes[mesh] = make_unique<GL::DrawableMesh>();
        }
        fill_drawable_mesh(*mesh, *general_meshes[mesh]);
        mesh->modified = false;
    });
    // Update/create drawable meshes for indicators(e.g. picking ray or speed vector).
    const vector<Mesh*> updated_overlay_meshes = {
        &scene.highlighted_element, &scene.light_indicator
    };
    for (Mesh* mesh: updated_overlay_meshes) {
        if (mesh->modified) {
            if (!overlay_meshes.contains(mesh)) [[unlikely]] {
                logger->info("create a drawable mesh for overlay mesh \"{}\"", mesh->name);
                overlay_meshes[mesh] = make_unique<GL::DrawableMesh>();
            }
            fill_drawable_mesh(*mesh, *overlay_meshes[mesh]);
            mesh->modified = false;
        }
    }

    vector<const Mesh*> general_meshes_to_delete;
    for (auto& [mesh, drawable_mesh]: general_meshes) {
        if (!updated_general_meshes.contains(mesh))
            general_meshes_to_delete.push_back(mesh);
    }
    for (const Mesh* mesh: general_meshes_to_delete) {
        general_meshes.erase(mesh);
        logger->info("drawable mesh corresponding to mesh \"{}\" is removed", mesh->name);
    }
}

void PreviewRenderer::update_drawable_linesets(Scene& scene)
{
    unordered_set<const LineSet*> updated_general_linesets;
    // Pre-defined line sets
    vector<LineSet*> linesets = {
        // Axes and the ground grid.
        &scene.x_axis, &scene.y_axis, &scene.z_axis, &scene.ground_grid,
        // Global indicators (depth test enabled).
        &scene.picking_ray, &scene.camera_wireframe
    };
    // Global indicators (depth test disabled).
    const vector<LineSet*> overlay_indicators = {&scene.arrows, &scene.highlighted_halfedge};
    if (scene.halfedge_mesh) {
        linesets.push_back(&scene.halfedge_mesh->halfedge_arrows);
    }
    // Collect BVH visualization result (if exist) for debugging.
    auto update_lineset =
        [this](
            LineSet*                                                        lineset,
            unordered_map<const LineSet*, unique_ptr<GL::DrawableLineSet>>& lineset_map
        ) -> void {
        if (!lineset->modified)
            return;
        if (!lineset_map.contains(lineset)) [[unlikely]] {
            logger->info("create a drawable line set for line set \"{}\"", lineset->name);
            lineset_map[lineset] = make_unique<GL::DrawableLineSet>();
        }
        fill_drawable_lineset(*lineset, *lineset_map[lineset]);
        lineset->modified = false;
    };
    for (LineSet* lineset: overlay_indicators) update_lineset(lineset, overlay_linesets);
    scene.for_each_object(
        [this, &updated_general_linesets, &update_lineset](Object& object) -> void {
            LineSet* boxes = &object.BVH_boxes;
            updated_general_linesets.insert(boxes);
            update_lineset(boxes, general_linesets);
        }
    );
    for (LineSet* lineset: linesets) {
        updated_general_linesets.insert(lineset);
        update_lineset(lineset, general_linesets);
    }
    vector<const LineSet*> deleted_linesets;
    for (auto& [lineset, drawable_lineset]: general_linesets) {
        if (!updated_general_linesets.contains(lineset))
            deleted_linesets.push_back(lineset);
    }
    for (const LineSet* lineset: deleted_linesets) {
        general_linesets.erase(lineset);
        logger->info(
            "drawable line set corresponding to line set \"{}\" is removed", lineset->name
        );
    }
}

void PreviewRenderer::render_selected_object(const Scene& scene, WorkingMode mode)
{
    if (!scene.selected_object)
        return;

    const Mesh*             mesh          = &scene.selected_object->mesh;
    const GL::DrawableMesh& drawable_mesh = *general_meshes[mesh];
    primitive_shader->set_uniform("color", default_wireframe_color);
    if (mode == WorkingMode::MODEL)
        primitive_shader->set_uniform("model", I4f);
    else
        primitive_shader->set_uniform("model", scene.selected_object->model());
    drawable_mesh.VAO.bind();
    drawable_mesh.edges.bind();
    glDrawElements(
        GL_LINES, static_cast<GLsizei>(drawable_mesh.edges.data.size()), GL_UNSIGNED_INT, (void*)0
    );
    if (mode == WorkingMode::MODEL) {
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(drawable_mesh.positions.count()));
    }
}
