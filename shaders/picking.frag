#version 330 core
// Всеки обект се рисува с уникален цвят, който кодира неговия индекс:
// id = R + G·256 + B·65536 (0 = фон).
uniform vec3 uIdColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(uIdColor, 1.0);
}
