#ifndef DANDELION_SCENE_GROUP_H
#define DANDELION_SCENE_GROUP_H

#include <cstddef>
#include <string>

#include <spdlog/spdlog.h>

#include "object.h"

/*!
 * \ingroup rendering
 * \file scene/group.h
 */

/*!
 * \ingroup rendering
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
    /*! \~chinese 创建一个组只需要指定组名，加载模型数据要显式调用 `load` 方法。 */
    Group(const std::string& group_name);
    ///@{
    /*! \~chinese 禁止复制组。 */
    Group(Group& other)       = delete;
    Group(const Group& other) = delete;
    ///@}
    ~Group() = default;
    /*! \~chinese 被 `Scene::load` 调用，真正加载模型数据的函数。 */
    bool load(const std::string& file_path);
    /*! \~chinese 组中所有的物体。 */
    std::vector<std::unique_ptr<Object>> objects;
    /*! \~chinese 组的唯一 ID 。 */
    std::size_t id;
    /*! \~chinese 组名，来自加载时的文件名。 */
    std::string name;

private:
    /*! \~chinese 下一个可用的组 ID 。 */
    static std::size_t next_available_id;
    std::shared_ptr<spdlog::logger> logger;
};

#endif // DANDELION_SCENE_GROUP_H
