#version 330 core

layout (location = 0) in vec3 position;
out vec3 vertex_color;

uniform mat4 model;
uniform mat4 view_projection;
uniform vec3 color;

void main()
{
    gl_Position = view_projection * model * vec4(position, 1.0);
    vertex_color = color;
}
