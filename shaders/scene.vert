#version 460 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;

out vec2 fragTexCoord;
uniform mat4 matrix;

void main() {
   fragTexCoord = texCoord;
   gl_Position = matrix * position;
}