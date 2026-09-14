#include "material.hpp"

using Eigen::Vector3f;
using std::size_t;

size_t Material::next_available_id = 0;

Material::Material()
{
    id = next_available_id;
    ++next_available_id;
}

PhongMaterial::PhongMaterial(
    const Vector3f& K_ambient, const Vector3f& K_diffuse, const Vector3f& K_specular,
    float shininess
) : ambient(K_ambient), diffuse(K_diffuse), specular(K_specular), shininess(shininess)
{
}

MaterialType PhongMaterial::type() const noexcept
{
    return MaterialType::Phong;
}
