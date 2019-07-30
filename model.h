#pragma once

#include <string>

#include <GL/glew.h>

#include "shader.h"

class Model;

class Scene {

};

class Model {
public:
    virtual void update() = 0;
    virtual void render(const Scene& scene) = 0;

protected:
    std::unique_ptr<Shader> shader{};
};

class BezierModel : public Model {
public:
    explicit BezierModel(const std::string& inputFile) {

    }

    void update() override {
    }

    void render(const Scene& scene) override {

    }
};