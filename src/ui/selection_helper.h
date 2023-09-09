#ifndef DANDELION_UI_SELECTION_HELPER_H
#define DANDELION_UI_SELECTION_HELPER_H

#include <variant>

/*!
 * \file ui/selection_helper.h
 * \ingroup ui
 */

template<class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

class Object;
struct Halfedge;
struct Vertex;
struct Edge;
struct Face;
struct Light;

/*!
 * \ingroup ui
 * \~chinese
 * \brief 场景中可被选择的元素类型。
 *
 * 此类型包含所有选中元素的指针类型以及额外的 `std::monostate` 类型。
 * 这些类型可分为三类：
 *
 * - `std::monostate` 表示选中元素为空（没有选中任何元素）
 * - 指向 `const` 对象的指针表示该类型可选中但不可通过 GUI 修改
 * - 一般指针表示该类型可选中且可通过 GUI 修改
 */
using SelectableType =
    std::variant<std::monostate, Object*, const Halfedge*, Vertex*, Edge*, Face*, Light*>;

#endif // DANDELION_UI_SELECTION_HELPER_H
