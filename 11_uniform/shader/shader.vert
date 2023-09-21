#version 450

layout (location = 0) in vec2 Positions;

void main() {
    gl_Position = vec4(Positions, 0.0, 1.0);
}
