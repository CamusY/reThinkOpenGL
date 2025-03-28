#version 330 core
in vec3 FragPos;    // 片段位置（世界空间）
in vec3 Normal;     // 法线（世界空间）

uniform vec3 lightDir;    // 光源方向
uniform vec3 lightColor;  // 光源颜色
uniform vec3 viewPos;     // 相机位置
uniform vec3 diffuseColor; // 材质漫反射颜色
uniform vec3 specularColor; // 材质镜面反射颜色
uniform float shininess;  // 材质光泽度

out vec4 FragColor;  // 输出颜色

void main() {
    // 环境光
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // 漫反射
    vec3 norm = normalize(Normal);
    vec3 lightDirNorm = normalize(-lightDir); // 注意：lightDir 是方向光的方向，指向光源相反
    float diff = max(dot(norm, lightDirNorm), 0.0);
    vec3 diffuse = diff * lightColor * diffuseColor;

    // 镜面反射（Blinn-Phong 模型）
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDirNorm + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = spec * lightColor * specularColor;

    // 组合结果
    vec3 result = (ambient + diffuse + specular);
    FragColor = vec4(result, 1.0);
}