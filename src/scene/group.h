#ifndef DANDELION_SCENE_GROUP_H
#define DANDELION_SCENE_GROUP_H

#include <cstddef>
#include <string>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "object.h"

using nlohmann::json;

/*!
 * \ingroup rendering
 * \ingroup simulation
 * \file scene/group.h
 * \~chinese
 * \brief 包含物体组的类。
 */

/*!
 * \ingroup rendering
 * \ingroup simulation
 * \~chinese
 * \brief 表示物体组的类。
 *
 * 一个模型文件（例如一个 obj 文件）中可以包含多个物体，
 * 因此加载一个模型文件的结果是一个组而不是一个物体。加载完成后，组名即文件名、
 * 各物体名即文件中各 mesh 的名称。在删除组中所有的物体后该组为空但不会被删除，
 * 未来将实现组间转移物体和向组中添加物体的功能。
 */
class Group
{
public:

    /*!
     * \~chinese
     * 创建一个组只需要指定组名，加载模型数据要显式调用 `load` 方法。
     * \param position 要创建组的组名
     */
    Group(const std::string& group_name);
    ///@{
    /*! \~chinese 禁止复制组。 */
    Group(Group& other)       = delete;
    Group(const Group& other) = delete;
    ///@}
    ~Group() = default;
    /*!
     * \~chinese
     * 被 `Scene::load` 调用，真正加载模型数据的函数。
     * 这个函数只加载模型 mesh 数据，不包括变换、物理属性等 `Object` 层面的信息
     * \param file_path 要加载模型的文件路径
     */
    bool load(const std::string& file_path);
    /*!
     * \~chinese
     * 将 `Group` 保存为单个 obj 文件，包括 mesh、材质等信息
     * \param file_path 保存的文件路径
     */
    bool save(const std::string& file_path);
    /*!
     * \~chinese
     * 加载以 JSON 格式保存的除 mesh 和材质以外的数据，如变换、物理属性
     * \param extra_info JSON 对象
     */
    void load_extra_info(const json& extra_info);
    /*!
     * \~chinese
     * 生成 JSON 对象，用来保存 `save` 方法以外的数据，用 `load_extra_json` 可以读取
     * \returns 表示该组中所有物体信息的 JSON 对象
     */
    json dump_extra_info();
    /*! \~chinese 组中所有的物体。 */
    std::vector<std::unique_ptr<Object>> objects;
    /*! \~chinese 组的唯一 ID。 */
    std::size_t id;
    /*! \~chinese 组名，来自加载时的文件名。 */
    std::string name;

private:

    /*! \~chinese 下一个可用的组 ID。 */
    static std::size_t              next_available_id;
    std::shared_ptr<spdlog::logger> logger;
};

#endif // DANDELION_SCENE_GROUP_H
