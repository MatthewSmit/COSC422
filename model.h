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
};

class Scene;

class Camera {
public:
    Camera() {
        projection = glm::perspectiveFov(60.0f * DEGREE_TO_RADIAN, 800.0f, 600.0f, 1.0f, 1000.0f);
        view = glm::lookAt(glm::vec3(0.0, 12.0, 20.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
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

private:
    mutable bool dirty{true};
    glm::mat4 projection{};
    glm::mat4 view{};
    mutable glm::mat4 projectionView{};

    void recreate() const {
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

    void update() {
        int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
        int deltaTime = timeSinceStart - oldTimeSinceStart;
        oldTimeSinceStart = timeSinceStart;

        for (const auto& model : models) {
            model->update(deltaTime * 0.001f);
        }
    }

    void render() {
        if (camera->isDirty()) {
            sceneUniformData.projectionView = camera->getProjectionView();
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
    int oldTimeSinceStart{};
};