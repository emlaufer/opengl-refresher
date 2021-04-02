#version 330 core

layout (location = 0) in vec3 pos;
layout(location = 1) in vec3 color;

out vec4 vertColor;

uniform mat4 model;

void main() {
    gl_Position = model * vec4(pos.x, pos.y, pos.z, 1.0);
    vertColor = vec4(color, 1.0);
}
