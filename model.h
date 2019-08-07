#pragma once

#include <string>
#include <vector>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.h"
#include "util.h"

struct SceneInputData {
    glm::mat4 projectionView;
    glm::vec3 cameraPosition;
    float _padding;
    glm::vec3 directionLight;
    float ambientLight;
};

class Scene;

class Camera {
public:
    Camera() {
        projection = glm::perspectiveFov(60.0f * DEGREE_TO_RADIAN, 800.0f, 600.0f, 1.0f, 1000.0f);
        cameraPosition = glm::vec3(0.0, 15.0, 20.0);
        view = glm::lookAt(cameraPosition, target, glm::vec3(0.0, 1.0, 0.0));
    }

    void translate(const glm::vec3& translation) {
        cameraPosition += translation;
        dirty = true;
    }

    void lookAt(const glm::vec3& newTarget) {
        target = newTarget;
        dirty = true;
    }

    bool isDirty() const {
        return dirty;
    }

    const glm::mat4& getProjectionView() const {
        if (dirty) {
            recreate();
        }

        return projectionView;
    }

    const glm::vec3& getCameraPosition() const {
        return cameraPosition;
    }

private:
    glm::vec3 cameraPosition{};
    glm::vec3 target{};

    mutable bool dirty{true};
    mutable glm::mat4 projection{};
    mutable glm::mat4 view{};
    mutable glm::mat4 projectionView{};

    void recreate() const {
        view = glm::lookAt(cameraPosition, target, glm::vec3(0.0, 1.0, 0.0));
        projectionView = projection * view;
        dirty = false;
    }
};

class Model {
public:
    Model() {
        glCreateVertexArrays(1, &vertexArray);
        glCreateBuffers(3, buffers);
    }

    virtual ~Model() {
        glDeleteBuffers(3, buffers);
        glDeleteVertexArrays(1, &vertexArray);
    }

    virtual void update(float delta) = 0;
    virtual void render(const Scene& scene) = 0;

protected:
    static const int INDEX_BUFFER = 0;
    static const int VERTEX_BUFFER = 1;
    static const int UNIFORM_BUFFER = 2;

    std::unique_ptr<Shader> shader{};
    GLuint vertexArray{};
    GLuint buffers[3]{};
};

class Scene {
public:
    Scene() {
        camera = std::make_unique<Camera>();
        sceneUniformData.directionLight = glm::normalize(glm::vec3{-10, 100, 0});
        sceneUniformData.ambientLight = 0.2f;
        glCreateBuffers(1, &sceneUniformBuffer);
        glNamedBufferStorage(sceneUniformBuffer, sizeof(sceneUniformData), nullptr, GL_DYNAMIC_STORAGE_BIT);
    }

    ~Scene() {
        glDeleteBuffers(1, &sceneUniformBuffer);
    }

    void addModel(std::unique_ptr<Model>&& model) {
        models.push_back(std::move(model));
    }

    Camera& getCamera() {
        return *camera;
    }

    const Camera& getCamera() const {
        return *camera;
    }

    GLuint getSceneUniformBuffer() const {
        return sceneUniformBuffer;
    }

    void update(float delta) {
        for (const auto& model : models) {
            model->update(delta);
        }
    }

    void render() {
        if (camera->isDirty()) {
            sceneUniformData.projectionView = camera->getProjectionView();
            sceneUniformData.cameraPosition = camera->getCameraPosition();
            glNamedBufferSubData(sceneUniformBuffer, 0, sizeof(sceneUniformData), &sceneUniformData);
        }

        for (const auto& model : models) {
            model->render(*this);
        }
    }

private:
    std::vector<std::unique_ptr<Model>> models{};
    std::unique_ptr<Camera> camera;
    SceneInputData sceneUniformData{};
    GLuint sceneUniformBuffer{};
};