#ifndef DANDELION_LIGHT_H
#define DANDELION_LIGHT_H

#include <Eigen/Core>

/*!
 * \ingroup rendering
 * \file scene/light.h
 * \~chinese
 * \brief 包含光源的类，目前只有一个点光源。
 */

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 表示一个点光源的类。
 */
struct Light
{
    /*! \~chinese 禁止无参构造。 */
    Light() = delete;
    /*!
     * \~chinese
     * 在指定位置创建一个指定强度的点光源。
     * \param position 光源位置
     * \param position 光源强度
     */
    Light(const Eigen::Vector3f& position, float intensity);

    /*! \~chinese 光源位置。 */
    Eigen::Vector3f position;
    /*! \~chinese 光源强度。 */
    float intensity;
};

#endif // DANDELION_LIGHT_H
