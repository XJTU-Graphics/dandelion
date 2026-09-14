#pragma once

#include <unordered_map>
#include <memory>

#include <spdlog/spdlog.h>

#include "../utils/rendering.hpp"
#include "../geometry/mesh.hpp"
#include "../scene/scene.h"
#include "../platform/gl.hpp"
#include "../platform/shader.hpp"

/*!
 * \file render/preview_renderer.h
 * \ingroup rendering
 * \~chinese
 * \brief 用于实时预览的渲染器。
 */

/*!
 * \~chinese
 * \brief 用于实时预览的渲染器。
 *
 * `PreviewRenderer` 在形式上与离线渲染器相似，但并不受 `RenderEngine` 的管理。
 */
class PreviewRenderer
{
public:

    /*!
     * \~chinese
     * 构造渲染器。因为 OpenGL shader 需要在 context 初始化之后再编译，
     * 所以 shader 相关的初始化操作需要单独调用 `compile_shaders` 方法。
     */
    PreviewRenderer();
    /*! \~chinese 加载并编译编译 OpenGL shader 。 */
    void compile_shaders();
    /*! \~chinese 删除 OpenGL shader 。 */
    void delete_shaders();
    /*! \~chinese 释放所有使用了 OpenGL VAO/VBO/EBO 的可渲染对象。 */
    void delete_drawable_resources();
    /*!
     * \~chinese
     * \brief 渲染一帧的画面。
     *
     * 该方法会按需更新 GPU 数据、发起所有的 draw call ，但不会交换 back buffer 与
     * front buffer ，因此不会直接引发画面更新。
     * \param scene 渲染的场景
     * \param mode 当前的工作模式，这决定了场景的渲染方式。
     */
    void render(Scene& scene, WorkingMode mode);

private:

    void fill_drawable_mesh(const Mesh& mesh, GL::DrawableMesh& drawable_mesh);
    void fill_drawable_lineset(const LineSet& lineset, GL::DrawableLineSet& drawable_lineset);
    /*! \~chinese 将场景中所有的 `Mesh` 几何数据同步到 `GL::DrawableMesh` 。 */
    void update_drawable_meshes(Scene& scene);
    /*! \~chinese 将场景中所有的 `LineSet` 几何数据同步到 `GL::DrawableLineSet` 。 */
    void update_drawable_linesets(Scene& scene);
    /*!
     * \~chinese
     * 处于布局模式下且有物体被选中时，渲染这个物体的高亮效果。
     */
    void render_selected_object_if_exist(const Scene& scene);

    std::unique_ptr<Shader>                                                  primitive_shader;
    std::unique_ptr<Shader>                                                  phong_shader;
    std::unordered_map<const Mesh*, std::unique_ptr<GL::DrawableMesh>>       drawable_meshes;
    std::unordered_map<const LineSet*, std::unique_ptr<GL::DrawableLineSet>> drawable_linesets;
    std::shared_ptr<spdlog::logger>                                          logger;
};
