#version 460 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 color;

out vec4 fragColor;
uniform mat4 matrix;

void main() {
   fragColor = vec4(color, 1.0f);
   gl_Position = matrix * position;
}