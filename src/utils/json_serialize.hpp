#ifndef DANDELION_UTILS_JSON_SERIALIZE_HPP
#define DANDELION_UTILS_JSON_SERIALIZE_HPP

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <Eigen/Core>

#include "../scene/object.h"
#include "../scene/camera.h"
#include "../scene/light.h"

using nlohmann::json;

/*!
 * \file utils/json_serialize.hpp
 * \ingroup utils
 * \~chinese
 * \brief 实现场景保存、读取时，所有场景相关对象的 JSON 序列化和反序列化函数。
 */

// Eigen

namespace Eigen {

using Eigen::Matrix;

// implement json (de)serializer for any Eigen matrix
template<typename T, int Rows, int Cols>
void to_json(json& j, const Matrix<T, Rows, Cols>& m)
{
    std::vector<T> flat_data(m.data(), m.data() + m.size());
    j = flat_data;
}

template<typename T, int Rows, int Cols>
void from_json(const json& j, Matrix<T, Rows, Cols>& m)
{
    if ((size_t)j.size() != (size_t)m.size()) {
        spdlog::error("JSON array size does not match matrix size.");
        return;
    }
    std::vector<T> flat_data = j.get<std::vector<T>>();

    // use Eigen::Map to create a view and copy data to the Matrix
    m = Eigen::Map<const Matrix<T, Rows, Cols>>(flat_data.data());
}

}; // namespace Eigen

// Object
void to_json(json& j, const Object& o);
void from_json(const json& j, Object& o);

// Camera
void to_json(json& j, const Camera& c);
void from_json(const json& j, Camera& c);

// Light
void to_json(json& j, const Light& l);
void from_json(const json& j, Light& l);

#endif // DANDELION_UTILS_JSON_SERIALIZE_HPP
