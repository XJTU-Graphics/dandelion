#ifndef DANDELION_LIGHT_H
#define DANDELION_LIGHT_H

#include <Eigen/Core>

/*!
 * \ingroup rendering
 * \file scene/light.h
 */

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 一个点光源。
 */
struct Light
{
    /*! \~chinese 禁止无参构造。 */
    Light() = delete;
    Light(const Eigen::Vector3f& position, float intensity);

    /*! \~chinese 光源位置。 */
    Eigen::Vector3f position;
    /*! \~chinese 光源强度。 */
    float intensity;
};

#endif // DANDELION_LIGHT_H
