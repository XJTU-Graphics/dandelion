#include "kinetic_state.h"

using Eigen::Vector3f;

float time_step = 1.0f / 30.0f;

KineticState::KineticState(const Vector3f& position, const Vector3f& velocity,
                           const Vector3f& acceleration)
    : position(position), velocity(velocity), acceleration(acceleration)
{
}
