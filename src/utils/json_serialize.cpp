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
