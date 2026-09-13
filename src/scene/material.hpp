#pragma once

#include <cstddef>

#include <Eigen/Core>

/*!
 * \file scene/material.hpp
 * \~chinese
 * 提供所有材质相关类型的定义。
 */

/*!
 * \~chinese
 * \brief 材质类型枚举，每一种类型对应一种材质模型。
 */
enum class MaterialType
{
    Phong
};

/*!
 * \ingroup scene
 * \ingroup rendering
 * \~chinese
 * \brief 所有材质的基类。
 */
struct Material
{
    Material();
    virtual ~Material() = default;
    /*! \~chinese 获取材质类型，所有可用的材质模型参考 `MaterialType` 。 */
    virtual MaterialType type() const noexcept = 0;

    /*! \~chinese 材质的唯一 ID ，不会与其他材质重复。 */
    std::size_t id;

private:

    static std::size_t next_available_id;
};

/*!
 * \~chinese
 * \brief 实现 Phong 材质模型的材质。
 *
 * 该类型实现了一个简单的 Phong 材质模型，包含环境光、漫反射、镜面反射三个颜色向量，
 * 以及一个光滑度参数。
 */
struct PhongMaterial : public Material
{
    /*!
     * \~chinese
     * \brief 构造一个材质对象。
     * \param K_ambient 环境光系数（颜色）
     * \param K_diffuse 漫反射系数（颜色）
     * \param K_specular 镜面反射系数（颜色）
     * \param shininess 光滑度
     */
    PhongMaterial(
        const Eigen::Vector3f& K_ambient  = Eigen::Vector3f(1.0f, 1.0f, 1.0f),
        const Eigen::Vector3f& K_diffuse  = Eigen::Vector3f(0.5f, 0.5f, 0.5f),
        const Eigen::Vector3f& K_specular = Eigen::Vector3f(0.0f, 0.0f, 0.0f),
        float                  shininess  = 5.0f
    );
    virtual ~PhongMaterial() = default;
    /*! \~chinese 返回 `MaterialType::Phong` 。 */
    virtual MaterialType type() const noexcept;

    /*! \~chinese 环境光反射系数（颜色）。 */
    Eigen::Vector3f ambient;
    /*! \~chinese 漫反射光反射系数（颜色）。 */
    Eigen::Vector3f diffuse;
    /*! \~chinese 镜面反射光反射系数（颜色）。 */
    Eigen::Vector3f specular;
    /*! \~chinese Phong 模型计算镜面反射时的指数 */
    float shininess;
};
