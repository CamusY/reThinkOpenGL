#version 330 core
uniform vec3 color; // 线条颜色

out vec4 FragColor; // 输出颜色

void main() {
    FragColor = vec4(color, 1.0); // 设置线条颜色
}