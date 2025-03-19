// Utils/JSONSerializer.h
#pragma once
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <exception>
#include <glm/glm.hpp>

namespace fs = std::filesystem;

/**
 * @brief JSON序列化异常基类
 */
class JsonSerializationException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * @brief 文件未找到异常
 */
class FileNotFoundException : public JsonSerializationException {
public:
    explicit FileNotFoundException(const std::string& path)
        : JsonSerializationException("文件未找到: " + path) {}
};

/**
 * @brief JSON解析异常
 */
class JsonParseException : public JsonSerializationException {
public:
    explicit JsonParseException(const std::string& msg)
        : JsonSerializationException("JSON解析错误: " + msg) {}
};

/**
 * @brief JSON序列化工具类
 */
class JSONSerializer {
public:
    /**
     * @brief 将对象序列化到文件
     * @tparam T 对象类型
     * @param obj 要序列化的对象
     * @param path 文件路径
     * @throws FileNotFoundException 路径无效时抛出
     * @throws JsonParseException 序列化失败时抛出
     */
    template<typename T>
    static void SerializeToFile(const T& obj, const fs::path& path) {
        validateFilePath(path, true);
        
        try {
            nlohmann::json j;
            to_json_adl(j, obj); // ADL查找正确的to_json函数
            
            std::ofstream ofs(path);
            ofs << j.dump(4);
        } catch (const std::exception& e) {
            throw JsonParseException(e.what());
        }
    }

    /**
     * @brief 从文件反序列化对象
     * @tparam T 目标类型
     * @param path 文件路径
     * @return 反序列化后的对象
     * @throws FileNotFoundException 路径无效时抛出
     * @throws JsonParseException 反序列化失败时抛出
     */
    template<typename T>
    static T DeserializeFromFile(const fs::path& path) {
        validateFilePath(path);
        
        try {
            std::ifstream ifs(path);
            nlohmann::json j;
            ifs >> j;
            
            T obj;
            from_json_adl(j, obj); // ADL查找正确的from_json函数
            return obj;
        } catch (const std::exception& e) {
            throw JsonParseException(e.what());
        }
    }

private:
    // 启用ADL的序列化包装器
    template<typename T>
    static auto to_json_adl(nlohmann::json& j, const T& obj) {
        nlohmann::adl_serializer<T>::to_json(j, obj);
    }

    // 启用ADL的反序列化包装器
    template<typename T>
    static auto from_json_adl(const nlohmann::json& j, T& obj) {
        nlohmann::adl_serializer<T>::from_json(j, obj);
    }
    // 验证文件路径有效性
    static void validateFilePath(const fs::path& path, bool checkWritable = false) {
        if (path.empty()) {
            throw FileNotFoundException("空路径");
        }

        if (checkWritable) {
            if (!fs::exists(path.parent_path())) {
                throw FileNotFoundException("目录不存在: " + path.parent_path().string());
            }
        } else {
            if (!fs::exists(path)) {
                throw FileNotFoundException(path.string());
            }
        }
    }
};

// 基础类型序列化特化
namespace nlohmann {
    template<>
    struct adl_serializer<glm::vec3> {
        static void to_json(json& j, const glm::vec3& vec) {
            j = json::array({vec.x, vec.y, vec.z});
        }

        static void from_json(const json& j, glm::vec3& vec) {
            if (j.size() != 3) throw std::invalid_argument("无效的vec3数据");
            vec.x = j[0];
            vec.y = j[1];
            vec.z = j[2];
        }
    };
    template<>
    struct adl_serializer<glm::mat4> {
        static void to_json(json& j, const glm::mat4& mat) {
            // 按列主序序列化为16元素数组
            std::array<float, 16> data;
            for (int col = 0; col < 4; ++col) {
                for (int row = 0; row < 4; ++row) {
                    data[col * 4 + row] = mat[col][row];
                }
            }
            j = data;
        }

        static void from_json(const json& j, glm::mat4& mat) {
            if (!j.is_array() || j.size() != 16) {
                throw std::invalid_argument("无效的mat4数据格式");
            }
            
            // 按列主序解析
            for (int col = 0; col < 4; ++col) {
                for (int row = 0; row < 4; ++row) {
                    mat[col][row] = j[col * 4 + row].get<float>();
                }
            }
        }
    };
}