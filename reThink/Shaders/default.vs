#version 330 core
layout (location = 0) in vec3 aPos;      // 顶点位置
layout (location = 1) in vec3 aNormal;   // 顶点法线

uniform mat4 model;       // 模型矩阵
uniform mat4 view;        // 视图矩阵
uniform mat4 projection;  // 投影矩阵

out vec3 FragPos;  // 片段位置（世界空间）
out vec3 Normal;   // 法线（世界空间）

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));           // 计算世界空间中的片段位置
    Normal = mat3(transpose(inverse(model))) * aNormal; // 计算世界空间中的法线（考虑模型变换）
    gl_Position = projection * view * vec4(FragPos, 1.0); // 计算裁剪空间位置
}