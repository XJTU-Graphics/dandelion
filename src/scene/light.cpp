#include "light.h"

using Eigen::Vector3f;

Light::Light(const Vector3f& position, float intensity) : position(position), intensity(intensity)
{
}
