#version 460 core

uniform sampler2D textureSampler;
noperspective in vec2 fragTexCoord;

void main() {
    gl_FragColor = texture(textureSampler, fragTexCoord);
}