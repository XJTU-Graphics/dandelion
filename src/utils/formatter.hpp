#ifndef DANDELION_UTILS_FORMATTER_HPP
#define DANDELION_UTILS_FORMATTER_HPP

#include <Eigen/Core>
#include <fmt/core.h>

/*!
 * \file utils/formatter.hpp
 * \ingroup utils
 * \~chinese
 * \brief 提供对 Eigen 列向量和方阵类型的格式化支持。
 */

/*!
 * \~chinese
 * \brief 提供对 Eigen 列向量类型的格式化支持。
 *
 * 这个格式化器调用 `Scalar` 类型的格式化器来格式化每个分量，
 * 并在各分量之间添加逗号、空格，在开头和结尾添加括号。由于它直接调用 `Scalar`
 * 类型的格式化器格式化分量，任何可用于 `Scalar` 类型的格式符也都可用于格式化列向量。
 * 相应地，`Scalar` 类型必须可以被格式化。
 *
 * 格式符：与分量类型的格式符相同，例如 `Vector3f` 可以使用所有 `float`
 * 类型的格式符，`Vector2i` 可以使用所有 `int` 类型的格式符。
 *
 * \tparam Scalar 向量中分量的类型
 * \tparam n_dim 向量维数
 */
template<typename Scalar, int n_dim>
struct fmt::formatter<Eigen::Matrix<Scalar, n_dim, 1>> : formatter<Scalar>
{
    static_assert(n_dim > 0, "Dimension of a vector must be positive");
    fmt::appender format(const Eigen::Matrix<Scalar, n_dim, 1>& v, format_context& ctx) const
    {
        fmt::appender iter = fmt::format_to(ctx.out(), "(");
        iter               = formatter<Scalar>::format(v(0), ctx);
        for (int i = 1; i < n_dim; ++i) {
            iter = fmt::format_to(iter, ", ");
            iter = formatter<Scalar>::format(v(i), ctx);
        }
        iter = fmt::format_to(iter, ")");
        return iter;
    }
};

/*!
 * \~chinese
 * \brief 提供对 Eigen 方阵类型的格式化支持。
 *
 * 这个格式化器调用 `Scalar` 类型的格式化器来格式化每个元素，
 * 并在同一行的各元素间添加空格，在行间添加换行符。由于它直接调用 `Scalar`
 * 类型的格式化器来格式化元素，任何可用于 `Scalar` 类型的格式符也都可用于格式化方阵。
 * 相应地，`Scalar` 类型必须可以被格式化。
 *
 * 格式符：与元素类型的格式符相同，例如 `Matrix4f` 可以使用所有 `float`
 * 类型的格式符，`Matrix3d` 可以使用所有 `double` 类型的格式符。
 *
 * 请注意，方阵的最后一行不会添加换行符，如果使用 `fmt::print` / `std::printf` /
 * `std::cout` 等工具直接输出时，请自行添加换行。使用 spdlog 的日志输出则不必。
 *
 * 当矩阵中的元素位数不一时（例如 10 和 0），默认的格式符 `{}` 并不会对齐它们。
 * 如果使用者希望将数字右对齐，请直接使用 fmtlib 的右对齐格式，例如：
 * ```cpp
 * Eigen::Matrix4f m = 10.0f * Eigen::Matrix4f::Identity();
 * fmt::print("matrix: {:>5.1f}", m);
 * ```
 * 会输出
 * ```
 * matrix:
 *  10.0   0.0   0.0   0.0
 *   0.0  10.0   0.0   0.0
 *   0.0   0.0  10.0   0.0
 *   0.0   0.0   0.0  10.0
 * ```
 *
 * \tparam Scalar 方阵中的元素类型
 * \tparam n_dim 方阵的维数
 */
template<typename Scalar, int n_dim>
struct fmt::formatter<Eigen::Matrix<Scalar, n_dim, n_dim, 0, n_dim, n_dim>> : formatter<Scalar>
{
    static_assert(n_dim > 0, "Dimension of a square matrix must be positive");
    fmt::appender format(const Eigen::Matrix<Scalar, n_dim, n_dim, 0, n_dim, n_dim>& m, format_context& ctx) const
    {
        fmt::appender iter = fmt::format_to(ctx.out(), "\n");
        for (int row = 0; row < n_dim - 1; ++row) {
            for (int column = 0; column < n_dim; ++column) {
                iter = formatter<Scalar>::format(m(row, column), ctx);
                iter = fmt::format_to(iter, " ");
            }
            iter = fmt::format_to(iter, "\n");
        }
        for (int column = 0; column < n_dim; ++column) {
            iter = formatter<Scalar>::format(m(n_dim - 1, column), ctx);
            iter = fmt::format_to(iter, " ");
        }
        return iter;
    }
};

#endif // DANDELION_UTILS_FORMATTER_HPP
