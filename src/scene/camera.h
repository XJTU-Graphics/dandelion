#ifndef DANDELION_SCENE_CAMERA_H
#define DANDELION_SCENE_CAMERA_H

#ifdef _WIN32
#undef near
#undef far
#endif

#include <Eigen/Core>

/*!
 * \ingroup rendering
 * \file scene/camera.h
 */

/*!
 * \ingroup rendering
 * \~chinese
 * \brief 表示观察点的相机，既可以用于预览视角，也可以用于渲染视角。
 *
 * 这个类封装了从观察点、按一定视角去观察目标点的功能，可以完成相应的几何计算，
 * 包括观察矩阵和投影矩阵 (view & projection matrix)。
 * 预览场景中相机的显示不由 Camera 类自己负责，它的功能只是存储参数并构造变换矩阵。
 * Dandelion 工作于渲染模式下时，Scene 对象将主动渲染一个相机视锥。
 */
struct Camera
{
public:
    Camera() = delete;
    /*!
     * \~chinese
     * \param position 相机位置（观察点）
     * \param target 相机指向的目标点
     * \param near_plane 可视范围近平面到观察坐标系原点的距离（正值）
     * \param far_plane 可视范围远平面到观察坐标系原点的距离（正值）
     * \param fov_y_degrees 高度方向（Y 方向）的视角大小，单位是角度
     * \param aspect_ratio 宽高比，即视角的宽度 / 高度
     */
    Camera(const Eigen::Vector3f& position, const Eigen::Vector3f& target, float near_plane = 0.1f,
           float far_plane = 10.0f, float fov_y_degrees = 45.0f, float aspect_ratio = 1.33f);
    ~Camera() = default;
    /*! \~chinese 获取该相机的 View 矩阵，对应观察变换。 */
    Eigen::Matrix4f view();
    /*! \~chinese 获取该相机的 Projection 矩阵，对应投影变换。 */
    Eigen::Matrix4f projection();
    /*! \~chinese 相机的位置（视点）。 */
    Eigen::Vector3f position;
    /*! \~chinese 相机看向的位置（目标点）。 */
    Eigen::Vector3f target;
    /*! \~chinese 可视范围近平面到观察坐标系原点的距离（正值）。 */
    float near;
    /*! \~chinese 可视范围远平面到观察坐标系原点的距离（正值）。 */
    float far;
    /*! \~chinese Y 方向（上下）的视角大小。 */
    float fov_y_degrees;
    /*! \~chinese 可视范围的宽度与高度之比。 */
    float aspect_ratio;
    /*! \~chinese 世界坐标系中的“上”方向向量，用于求相机坐标系的正交基。 */
    Eigen::Vector3f world_up;
};

#endif // DANDELION_SCENE_CAMERA_H
