// Utils/MathUtils.h
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <limits>

/**
 * @brief OpenGL数学工具类
 */
class MathUtils {
public:
    static constexpr float Epsilon = std::numeric_limits<float>::epsilon() * 100;

    /**
     * @brief 安全向量归一化
     * @param vec 输入向量
     * @return 归一化后的向量，输入为零向量时返回零向量
     */
    static inline glm::vec3 SafeNormalize(const glm::vec3& vec) noexcept {
        const float length = glm::length(vec);
        return (length > Epsilon) ? (vec / length) : glm::vec3(0);
    }

    /**
     * @brief 从变换矩阵提取位移分量
     * @param transform 变换矩阵
     * @return 位移向量
     */
    static inline glm::vec3 ExtractTranslation(const glm::mat4& transform) noexcept {
        return glm::vec3(transform[3]);
    }

    /**
     * @brief 分解变换矩阵为平移、旋转、缩放分量
     * @param transform 输入矩阵
     * @param translation [out] 平移分量
     * @param rotation [out] 欧拉角（弧度制）
     * @param scale [out] 缩放分量
     * @return 是否成功分解
     */
    static bool DecomposeTransform(const glm::mat4& transform,
                                  glm::vec3& translation,
                                  glm::vec3& rotation,
                                  glm::vec3& scale) noexcept {
        using namespace glm;
        using T = float;

        mat4 localMatrix(transform);

        // 检查矩阵是否可逆
        if (determinant(localMatrix) < Epsilon)
            return false;

        // 分离平移分量
        translation = ExtractTranslation(localMatrix);
        localMatrix[3] = vec4(0, 0, 0, localMatrix[3].w);

        // 提取缩放分量
        scale.x = length(vec3(localMatrix[0]));
        scale.y = length(vec3(localMatrix[1]));
        scale.z = length(vec3(localMatrix[2]));

        // 标准化矩阵
        if (scale.x > Epsilon) localMatrix[0] /= scale.x;
        if (scale.y > Epsilon) localMatrix[1] /= scale.y;
        if (scale.z > Epsilon) localMatrix[2] /= scale.z;

        //存储剪切变换信息
        vec3 skew;
        
        //存储透视变换信息
        vec4 perspective;

        // 提取旋转分量
        quat orientation;
        if (!decompose(localMatrix, scale, orientation, translation, skew, perspective))
            return false;

        rotation = eulerAngles(orientation);
        return true;
    }

    /**
     * @brief 将自定义矩阵转换为GLM矩阵
     * @tparam Matrix4x4 自定义矩阵类型
     * @param matrix 输入矩阵
     * @return glm::mat4格式矩阵
     */
    template<typename Matrix4x4>
    static inline glm::mat4 ConvertToGLM(const Matrix4x4& matrix) noexcept {
        glm::mat4 result;
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                result[col][row] = matrix.m[col][row];
            }
        }
        return result;
    }
};