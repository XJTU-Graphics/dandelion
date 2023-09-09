#ifndef DANDELION_SIMULATION_SOLVER_H
#define DANDELION_SIMULATION_SOLVER_H

#include "../utils/kinetic_state.h"

/*!
 * \file simulation/solver.h
 * \ingroup simulation
 * \~chinese
 * \brief 求解运动方程的各种求解器。
 *
 * 恒定外力下物体的运动方程微分形式是
 * \f{aligned}{
 *     \frac{\mathrm{d}\mathbf{x}}{\mathrm{d}t}&=\mathbf{v}(t) \\
 *     \frac{\mathrm{d}\mathbf{v}}{\mathrm{d}t}&=\mathbf{F}
 * \f}
 * 要求解物体的运动轨迹，就要对这个方程组进行数值积分，这个文件中声明了所有的积分求解器。
 */

/*!
 * \~chinese
 * \brief 前向欧拉法求解器。
 */
KineticState forward_euler_step(const KineticState& previous, const KineticState& current);

/*!
 * \~chinese
 * \brief 四阶龙格-库塔法求解器。
 */
KineticState runge_kutta_step(const KineticState& previous, const KineticState& current);

/*!
 * \~chinese
 * \brief 后向（隐式）欧拉法求解器。
 */
KineticState backward_euler_step(const KineticState& previous, const KineticState& current);

/*!
 * \~chinese
 * \brief 对称（半隐式）欧拉法求解器。
 */
KineticState symplectic_euler_step(const KineticState& previous, const KineticState& current);

#endif // DANDELION_SIMULATION_SOLVER_H
