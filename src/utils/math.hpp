#ifndef DANDELION_UTILS_MATH_HPP
#define DANDELION_UTILS_MATH_HPP

#include <cmath>
#include <tuple>

#include <Eigen/Core>

/*!
 * \ingroup utils
 * \file utils/math.hpp
 * \~chinese
 * \brief 这个文件提供一些方便使用的数学函数。
 */

const Eigen::Matrix3f I3f = Eigen::Matrix3f::Identity();
const Eigen::Matrix4f I4f = Eigen::Matrix4f::Identity();

/*!
 * \ingroup utils
 * \~chinese
 * \brief 返回 float 或 double 类型的 \f$\pi\f$ 值。
 */
template<typename T>
T pi();

template<>
inline constexpr double pi<double>()
{
    return 3.141592653589793238462643383279;
}

template<>
inline constexpr float pi<float>()
{
    return 3.14159265358f;
}

/*!
 * \ingroup utils
 * \~chinese
 * \brief 将角度转换为弧度。
 */
template<typename T>
inline constexpr T radians(T degrees)
{
    return degrees / static_cast<T>(180.0) * pi<T>();
}

/*!
 * \ingroup utils
 * \~chinese
 * \brief 将弧度转换为角度。
 */
template<typename T>
inline constexpr T degrees(T radians)
{
    return radians / pi<T>() * static_cast<T>(180.0);
}

/*!
 * \ingroup utils
 * \~chinese
 * \brief 求一个数的平方。
 */
template<typename T>
inline constexpr T squ(T x)
{
    return x * x;
}

/*!
 * \ingroup utils
 * \~chinese
 * \brief 将一个数截断在给定的上下界之间
 */
template<typename T>
inline constexpr T clamp(T low, T high, T value)
{
    return std::max(low, std::min(high, value));
}

/*!
 * \ingroup utils
 * \~chinese
 * \brief 将一个\f$1\times 3\f$方向向量转化为\f$1\times 4\f$的方向向量
 */
template<typename T>
inline constexpr Eigen::Vector<T, 4> to_vec4(Eigen::Vector<T, 3> vec3)
{
    return Eigen::Vector<T, 4>(vec3.x(), vec3.y(), vec3.z(), static_cast<T>(0.0));
}

/*!
 * \ingroup utils
 * \~chinese
 * \brief 求向量 \f$\mathbf{I}\f$ 关于向量 \f$\mathbf{N}\f$ 的反射。
 * \param I 表示入射射线的向量（不必是单位向量）
 * \param N 反射平面的法线（必须是单位向量）
 */
inline Eigen::Vector3f reflect(const Eigen::Vector3f& I, const Eigen::Vector3f& N)
{
    return I - 2 * I.dot(N) * N;
}

/*!
 * \ingroup utils
 * \~chinese
 * \brief 符号函数，正数的求值结果为 1，负数为 -1，零的求值结果是 0.
 */
template<typename T>
inline constexpr T sign(T x)
{
    if (x > static_cast<T>(0.0)) {
        return static_cast<T>(1.0);
    }
    if (x < static_cast<T>(0.0)) {
        return static_cast<T>(-1.0);
    }
    return static_cast<T>(0.0);
}

/*!
 * \ingroup utils
 * \~chinese
 * \brief 将旋转的四元数表示形式转换为 ZYX 欧拉角表示形式。
 *
 * 这个函数采用 ZYX 顺规（即依次绕自身 \f$z, y, x\f$ 轴旋转）。在 Dandelion
 * 的坐标系约定下，ZYX 顺规对应 roll-yaw-pitch 旋转顺序（滚转、航向、俯仰）。
 *
 * 由于欧拉角固有的缺陷，这个函数并不能消除航向角（或称方向角 yaw）在 \f$\pm 90^\circ\f$
 * 附近时产生的万向锁和抖动问题。
 *
 * 虽然函数的返回值是 `tuple`，但使用 `tuple` 接受返回值并不方便，建议用结构化绑定
 * ```cpp
 * auto [x_angle, y_angle, z_angle] = quaternion_to_ZYX_euler(w, x, y, z);
 * ```
 * 或 `std::tie` 来接受返回值。
 *
 * \returns 一个元组，它的三个分量依次是绕 x 轴、绕 y 轴和绕 z 轴的旋转角。
 */
template<typename T>
inline std::tuple<T, T, T> quaternion_to_ZYX_euler(T w, T x, T y, T z)
{
    const T test          = x * z + w * y;
    constexpr T threshold = static_cast<T>(0.5 - 1e-6);
    T x_rad, y_rad, z_rad;
    if (std::abs(test) > threshold) {
        x_rad = static_cast<T>(0.0);
        y_rad = sign(test) * pi<T>() / static_cast<T>(2.0);
        z_rad = sign(test) * static_cast<T>(2.0) * std::atan2(x, w);
    } else {
        x_rad =
            std::atan2(static_cast<T>(-2.0) * (y * z - w * x), squ(w) - squ(x) - squ(y) + squ(z));
        y_rad = std::asin(static_cast<T>(2.0) * (x * z + w * y));
        z_rad =
            std::atan2(static_cast<T>(-2.0) * (x * y - w * z), squ(w) + squ(x) - squ(y) - squ(z));
    }
    const T x_angle = degrees(x_rad);
    const T y_angle = degrees(y_rad);
    const T z_angle = degrees(z_rad);
    return std::make_tuple(x_angle, y_angle, z_angle);
}

#endif // DANDELION_UTILS_MATH_HPP
