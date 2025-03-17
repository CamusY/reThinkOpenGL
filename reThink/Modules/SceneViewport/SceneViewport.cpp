#include "SceneViewport.h"
#include <glad/glad.h>

SceneViewport::SceneViewport(Model& model) : model(model) {}

void SceneViewport::SetProjection(const glm::mat4& proj) {
    projection = proj;
}

void SceneViewport::Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (model.isLoaded()) {
        // 实际渲染逻辑...
    }
}