#ifndef DANDELION_RENDER_TRIANGLE_H
#define DANDELION_RENDER_TRIANGLE_H

#include <Eigen/Core>
#include <Eigen/Geometry>

/*!
 * \file render/triangle.h
 */

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 表示一个三角形，包括三个顶点的世界坐标以及每个顶点的法向向量
 */
class Triangle
{
public:
    /*! \~chinese 三角形三个顶点的世界坐标，v0,v1,v2 逆时针顺序排布 */
    Eigen::Vector4f vertex[3];
    /*! \~chinese 每个顶点的法向向量 */
    Eigen::Vector3f normal[3];

    Triangle();
};

#endif // DANDELION_RENDER_TRIANGLE_H
