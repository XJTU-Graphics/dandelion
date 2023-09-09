#ifndef DANDELION_UTILS_KINETIC_STATE_H
#define DANDELION_UTILS_KINETIC_STATE_H

#include <Eigen/Core>

/*!
 * \file utils/kinetic_state.h
 * \ingroup utils
 */

/*!
 * \ingroup simulation
 * \~chinese
 * \brief 物理模拟过程使用的固定时间步。
 *
 * 这个时间步独立于渲染的帧时长，物理模拟总是以每一步经过 `time_step`
 * 秒的方式进行。时间步长的默认值为 \f$1/30\f$ 秒。
 */
extern float time_step;

/*!
 * \ingroup simulation
 * \~chinese
 * \brief 表示物体的质点运动学状态。
 *
 * 在物理模拟模式下选择重置场景时，会将场景中所有物体恢复到动画开始前的状态，
 * 此结构体可以用于备份这一状态，在重置时重新赋值给物体。另外，
 * 它还可以用于给运动求解器传递参数。
 */
struct KineticState
{
    KineticState() = default;
    KineticState(const Eigen::Vector3f& position, const Eigen::Vector3f& velocity,
                 const Eigen::Vector3f& acceleration);
    Eigen::Vector3f position;
    Eigen::Vector3f velocity;
    Eigen::Vector3f acceleration;
};

#endif // DANDELION_UTILS_KINETIC_STATE_H
