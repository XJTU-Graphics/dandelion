#include "preview_renderer.h"

#include <cstring>
#include <array>
#include <memory>
#include <vector>
#include <unordered_set>

#include <Eigen/Core>

#include "../utils/logger.h"
#include "../utils/math.hpp"

using Eigen::Matrix4f;
using Eigen::Vector3f;
using std::array;
using std::make_unique;
using std::memcpy;
using std::size_t;
using std::unique_ptr;
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
    drawable_meshes.clear();
    drawable_linesets.clear();
}

void PreviewRenderer::render(Scene& scene, WorkingMode mode)
{
    update_drawable_meshes(scene);
    update_drawable_linesets(scene);

    const Matrix4f view_projection = scene.main_camera.projection() * scene.main_camera.view();
    primitive_shader->use();
    primitive_shader->set_uniform("view_projection", view_projection);
    primitive_shader->set_uniform("model", I4f);
    for (const auto& [lineset, drawable_lineset]: drawable_linesets) {
        primitive_shader->set_uniform("color", lineset->color);
        drawable_lineset->VAO.draw(GL_LINES, 0, drawable_lineset->positions.count());
    }

    const bool selected_object_highlight =
        mode == WorkingMode::LAYOUT || mode == WorkingMode::SIMULATE;
    if (selected_object_highlight) {
        render_selected_object_if_exist(scene);
    }

    phong_shader->use();
    phong_shader->set_uniform("view_projection", view_projection);
    phong_shader->set_uniform("camera_position", scene.main_camera.position);
    for (const unique_ptr<Group>& group: scene.groups) {
        for (const unique_ptr<Object>& object: group->objects) {
            if (object->material->type() != MaterialType::Phong)
                continue;
            const Mesh*             mesh     = &object->mesh;
            const PhongMaterial&    material = dynamic_cast<PhongMaterial&>(*(object->material));
            const Matrix4f          model    = object->model();
            const Matrix4f          normal_transform = model.inverse().transpose();
            const GL::DrawableMesh& drawable_mesh    = *drawable_meshes[mesh];
            phong_shader->set_uniform("model", model);
            phong_shader->set_uniform("normal_transform", normal_transform);
            phong_shader->set_uniform("material.ambient", material.ambient);
            phong_shader->set_uniform("material.diffuse", material.diffuse);
            phong_shader->set_uniform("material.specular", material.specular);
            phong_shader->set_uniform("material.shininess", material.shininess);
            drawable_mesh.VAO.bind();
            drawable_mesh.triangles.bind();
            glDrawElements(
                GL_TRIANGLES, static_cast<GLsizei>(drawable_mesh.triangles.data.size()),
                GL_UNSIGNED_INT, (void*)0
            );
            drawable_mesh.triangles.release();
            drawable_mesh.VAO.release();
        }
    }
}

void PreviewRenderer::fill_drawable_mesh(const Mesh& mesh, GL::DrawableMesh& drawable_mesh)
{
    logger->info("sync mesh \"{}\" to GPU", mesh.name);

    drawable_mesh.positions.data.resize(mesh.positions.size() * 3);
    memcpy(
        drawable_mesh.positions.data.data(), mesh.positions.data(),
        mesh.positions.size() * sizeof(Vector3f)
    );
    logger->debug(
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
    logger->debug(
        "{} unsigned int numbers copied as edge indices", drawable_mesh.edges.data.size()
    );
    drawable_mesh.triangles.data.resize(mesh.faces.size() * 3);
    memcpy(
        drawable_mesh.triangles.data.data(), mesh.faces.data(),
        mesh.faces.size() * sizeof(array<unsigned int, 3>)
    );
    logger->debug(
        "{} unsigned int numbers copied as triangle indices", drawable_mesh.triangles.data.size()
    );
    drawable_mesh.to_gpu();
}

void PreviewRenderer::fill_drawable_lineset(
    const LineSet& lineset, GL::DrawableLineSet& drawable_lineset
)
{
    logger->info("sync line set \"{}\" to GPU", lineset.name);

    drawable_lineset.positions.data.resize(lineset.positions.size() * 3);
    memcpy(
        drawable_lineset.positions.data.data(), lineset.positions.data(),
        lineset.positions.size() * sizeof(Vector3f)
    );
    logger->debug(
        "{} float numbers copied as vertex positions", drawable_lineset.positions.data.size()
    );
    drawable_lineset.lines.data.resize(lineset.lines.size() * 2);
    memcpy(
        drawable_lineset.lines.data.data(), lineset.lines.data(),
        lineset.lines.size() * sizeof(array<unsigned int, 2>)
    );
    logger->debug(
        "{} unsigned int numbers copied as line indices", drawable_lineset.lines.data.size()
    );
    drawable_lineset.to_gpu();
}

void PreviewRenderer::update_drawable_meshes(Scene& scene)
{
    unordered_set<const Mesh*> exist_meshes;
    for (unique_ptr<Group>& group: scene.groups) {
        for (unique_ptr<Object>& object: group->objects) {
            Mesh* mesh = &object->mesh;
            exist_meshes.insert(mesh);
            if (!mesh->modified)
                continue;
            if (!drawable_meshes.contains(mesh)) {
                drawable_meshes[mesh] = make_unique<GL::DrawableMesh>();
            }
            fill_drawable_mesh(*mesh, *drawable_meshes[mesh]);
            mesh->modified = false;
        }
    }
    vector<const Mesh*> deleted_meshes;
    for (auto& [mesh, drawable_mesh]: drawable_meshes) {
        if (!exist_meshes.contains(mesh))
            deleted_meshes.push_back(mesh);
    }
    for (const Mesh* mesh: deleted_meshes) {
        drawable_meshes.erase(mesh);
        logger->info("drawable mesh corresponding to mesh \"{}\" is removed", mesh->name);
    }
}

void PreviewRenderer::update_drawable_linesets(Scene& scene)
{
    unordered_set<const LineSet*> exist_linesets;
    vector<LineSet*>              linesets = {
        &scene.x_axis, &scene.y_axis, &scene.z_axis, &scene.arrows, &scene.ground_grid
    };
    for (LineSet* lineset: linesets) {
        exist_linesets.insert(lineset);
        if (!lineset->modified)
            continue;
        if (!drawable_linesets.contains(lineset)) {
            drawable_linesets[lineset] = make_unique<GL::DrawableLineSet>();
        }
        fill_drawable_lineset(*lineset, *drawable_linesets[lineset]);
        lineset->modified = false;
    }
    vector<const LineSet*> deleted_linesets;
    for (auto& [lineset, drawable_lineset]: drawable_linesets) {
        if (!exist_linesets.contains(lineset))
            deleted_linesets.push_back(lineset);
    }
    for (const LineSet* lineset: deleted_linesets) {
        drawable_linesets.erase(lineset);
        logger->info(
            "drawable line set corresponding to line set \"{}\" is removed", lineset->name
        );
    }
}

void PreviewRenderer::render_selected_object_if_exist(const Scene& scene)
{
    if (!scene.selected_object) {
        return;
    }

    const Mesh*             mesh          = &scene.selected_object->mesh;
    const GL::DrawableMesh& drawable_mesh = *drawable_meshes[mesh];
    primitive_shader->set_uniform("color", default_wireframe_color);
    drawable_mesh.VAO.bind();
    drawable_mesh.edges.bind();
    glDrawElements(
        GL_LINES, static_cast<GLsizei>(drawable_mesh.edges.data.size()), GL_UNSIGNED_INT, (void*)0
    );
    drawable_mesh.edges.release();
    drawable_mesh.VAO.release();
}
