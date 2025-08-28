#include "json_serialize.hpp"

void to_json(json& j, const Object& o)
{
    j = json({
        {"center",   o.center           },
        {"scaling",  o.scaling          },
        {"rotation", o.rotation.coeffs()},
        {"velocity", o.velocity         },
        {"force",    o.force            },
        {"mass",     o.mass             },
    });
}

void from_json(const json& j, Object& o)
{
    j.at("center").get_to(o.center);
    j.at("scaling").get_to(o.scaling);
    o.rotation = Eigen::Quaternionf(j.at("rotation").get<Eigen::Vector4f>());
    j.at("velocity").get_to(o.velocity);
    j.at("force").get_to(o.force);
    j.at("mass").get_to(o.mass);
}

void to_json(json& j, const Camera& c)
{
    j = json({
        {"position",      c.position     },
        {"target",        c.target       },
        {"near_plane",    c.near_plane   },
        {"far_plane",     c.far_plane    },
        {"fov_y_degrees", c.fov_y_degrees},
        {"aspect_ratio",  c.aspect_ratio },
    });
}

void from_json(const json& j, Camera& c)
{
    j.at("position").get_to(c.position);
    j.at("target").get_to(c.target);
    j.at("near_plane").get_to(c.near_plane);
    j.at("far_plane").get_to(c.far_plane);
    j.at("fov_y_degrees").get_to(c.fov_y_degrees);
    j.at("aspect_ratio").get_to(c.aspect_ratio);
}

void to_json(json& j, const Light& l)
{
    j = json({
        {"position",  l.position },
        {"intensity", l.intensity},
    });
}

void from_json(const json& j, Light& l)
{
    j.at("position").get_to(l.position);
    j.at("intensity").get_to(l.intensity);
}
