#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
out vec3 vertex_color;

struct Material
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform mat4 model;
uniform mat4 view_projection;
uniform mat4 normal_transform;
uniform bool color_per_vertex;
uniform bool use_global_color;
uniform vec3 global_color;
uniform Material material;
uniform vec3 camera_position;

void main()
{
    gl_Position = view_projection * model * vec4(position, 1.0);

    if (color_per_vertex) {
        vertex_color = color;
        return;
    }
    if (use_global_color) {
        vertex_color = global_color;
        return;
    }
    vec3 world_position = (model * vec4(position, 1.0)).xyz;
    vec3 world_normal = normalize((normal_transform * vec4(normal, 0.0)).xyz);
    vec3 V = normalize(camera_position - world_position);
    vec3 H = V;
    float N_dot_V = max(0.0, dot(world_normal, V));
    float N_dot_H = pow(max(0.0, dot(world_normal, H)), material.shininess);
    vertex_color = 0.1 * material.ambient
                 + material.diffuse * (0.5 * N_dot_V + 0.5)
                 + material.specular * N_dot_H;
}
