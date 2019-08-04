#include <iostream>
#include <memory>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include "model.h"
#include "shader.h"

class Floor : public Model {
public:
    explicit Floor(const Scene& scene) {
        shader = std::make_unique<Shader>("data/floor.vert", "data/floor.frag");

        // Create a grid of lines from -TOTAL_VERTICES to TOTAL_VERTICES in both x and z
        glm::vec2 VERTEX_DATA[TOTAL_VERTICES]{};

        for (auto i = -GRID_SIZE, j = 0; i <= GRID_SIZE; i++, j += 4) {
            VERTEX_DATA[j + 0] = glm::vec2{i, -GRID_SIZE};
            VERTEX_DATA[j + 1] = glm::vec2{i, GRID_SIZE};
            VERTEX_DATA[j + 2] = glm::vec2{-GRID_SIZE, i};
            VERTEX_DATA[j + 3] = glm::vec2{GRID_SIZE, i};
        }

        // Send buffer data to GPU
        glNamedBufferStorage(buffers[VERTEX_BUFFER], sizeof(VERTEX_DATA), VERTEX_DATA, 0);

        // Setup vertex attributes
        glEnableVertexArrayAttrib(vertexArray, 0);
        glVertexArrayAttribFormat(vertexArray, 0, 2, GL_FLOAT, GL_FALSE, 0);

        glVertexArrayVertexBuffer(vertexArray, 0, buffers[VERTEX_BUFFER], 0, sizeof(VERTEX_DATA[0]));
        glVertexArrayAttribBinding(vertexArray, 0, 0);

        // Setup uniform blocks
        GLuint sceneInputDataIndex = glGetUniformBlockIndex(shader->program, "SceneInputData");
        glUniformBlockBinding(shader->program, sceneInputDataIndex, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, scene.getSceneUniformBuffer());
    }

    ~Floor() override = default;

    void update(float) override {
    }

    void render(const Scene& scene) override {
        glBindVertexArray(vertexArray);
        glUseProgram(shader->program);
        glDrawArrays(GL_LINES, 0, TOTAL_VERTICES);
    }
private:
    static constexpr auto GRID_SIZE = 50;
    static constexpr auto TOTAL_VERTICES = (GRID_SIZE * 4 + 2) * 2;
};

class BezierModel : public Model {
public:
    BezierModel(const Scene& scene, const std::string& inputFile) {
        shader = std::make_unique<Shader>("data/bezier.vert",
                "data/bezier.tesc",
                "data/bezier.tese",
                "data/bezier.frag");

        auto vertexData = loadModelData(inputFile, numberVertices);

        // Send buffer data to GPU
        glNamedBufferStorage(buffers[VERTEX_BUFFER], sizeof(glm::vec3) * numberVertices, vertexData.get(), 0);
        glNamedBufferStorage(buffers[UNIFORM_BUFFER], sizeof(modelInputData), &modelInputData, GL_DYNAMIC_STORAGE_BIT);

        // Setup vertex attributes
        glEnableVertexArrayAttrib(vertexArray, 0);
        glVertexArrayAttribFormat(vertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);

        glVertexArrayVertexBuffer(vertexArray, 0, buffers[VERTEX_BUFFER], 0, sizeof(glm::vec3));
        glVertexArrayAttribBinding(vertexArray, 0, 0);

        // Setup uniform blocks
        GLuint sceneInputDataIndex = glGetUniformBlockIndex(shader->program, "SceneInputData");
        glUniformBlockBinding(shader->program, sceneInputDataIndex, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, scene.getSceneUniformBuffer());

        GLuint modelInputDataIndex = glGetUniformBlockIndex(shader->program, "ModelInputData");
        glUniformBlockBinding(shader->program, modelInputDataIndex, 1);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, buffers[UNIFORM_BUFFER]);
    }

    ~BezierModel() override = default;

    void update(float delta) override {
        rotateX += delta / 4;
        rotateY += delta;

        modelInputData.world = glm::translate(glm::identity<glm::mat4>(), glm::vec3{0, 5, 0}) *
                glm::rotate(glm::identity<glm::mat4>(), rotateX, glm::vec3{1, 0, 0}) *
                glm::rotate(glm::identity<glm::mat4>(), rotateY, glm::vec3{0, 1, 0}) *
                glm::scale(glm::identity<glm::mat4>(), glm::vec3{2});
        glNamedBufferSubData(buffers[UNIFORM_BUFFER], 0, sizeof(modelInputData), &modelInputData);
    }

    void render(const Scene& scene) override {
        glPatchParameteri(GL_PATCH_VERTICES, 16);
        glBindVertexArray(vertexArray);
        glUseProgram(shader->program);
        glDrawArrays(GL_PATCHES, 0, numberVertices);
    }

private:
    struct {
        glm::mat4 world{};
    } modelInputData{};

    int numberVertices{};
    float rotateY{};
    float rotateX{};

    static std::unique_ptr<glm::vec3[]> loadModelData(const std::string &inputFile, int& numberVertices) {
        std::ifstream file(inputFile.c_str());
        if(!file.good()) {
            std::cerr << "Error opening patch file: " << inputFile << std::endl;
            throw std::exception{};
        }

        file >> numberVertices;

        auto vertexData = std::unique_ptr<glm::vec3[]>{new glm::vec3[numberVertices]};
        for (auto i = 0; i < numberVertices; i++) {
            float x, y, z;
            file >> x >> y >> z;
            vertexData[i] = glm::vec3{x, y, z};
        }

        file.close();

        return vertexData;
    }
};

std::unique_ptr<Scene> scene;

GLAPIENTRY void debugCallback(GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei,
        const GLchar* message,
        const void*) {
    static const char* SOURCE[] = {
            "API",
            "Window System",
            "Shader Compiler",
            "Thirdy Party",
            "Application",
            "Other"
    };

    static const char* TYPE[] = {
            "Error",
            "Deprecated Behaviour",
            "Undefined Behaviour",
            "Portability",
            "Performance",
            "Other"
    };

    static const char* SEVERITY[] = {
            "High",
            "Medium",
            "Low"
    };

    printf("Source: %s; Type: %s; Id: %d; Severity: %s, Message: %s\n",
            SOURCE[source - GL_DEBUG_SOURCE_API],
            TYPE[type - GL_DEBUG_TYPE_ERROR],
            id,
            severity == GL_DEBUG_SEVERITY_NOTIFICATION ? "Notification" : SEVERITY[severity - GL_DEBUG_SEVERITY_HIGH],
            message);
}

void initialise() {
    scene = std::make_unique<Scene>();
    scene->addModel(std::make_unique<BezierModel>(*scene, "data/PatchVerts_Teapot.txt"));
    scene->addModel(std::make_unique<Floor>(*scene));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);

//    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glClearColor(1, 1, 1, 1);
}

void update(int) {
    scene->update();
    glutTimerFunc(50, update, 0);
    glutPostRedisplay();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    scene->render();
    glutSwapBuffers();
}

int main(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutSetOption(GLUT_MULTISAMPLE, 8);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(800, 600);
    glutInitContextVersion(4, 5);
    glutInitContextProfile(GLUT_CORE_PROFILE);
#ifndef NDEBUG
    glutInitContextFlags(GLUT_DEBUG);
#endif
    glutCreateWindow("COSC 422 Assignment 1 - MJS351 - Bezier");

    if(glewInit() == GLEW_OK)
    {
        std::cout << "GLEW initialization successful! " << std::endl;
        std::cout << " Using GLEW version " << glewGetString(GLEW_VERSION) << std::endl;
    }
    else
    {
        std::cerr << "Unable to initialize GLEW  ...exiting." << std::endl;
        exit(EXIT_FAILURE);
    }

#ifndef NDEBUG
    glDebugMessageCallback(debugCallback, nullptr);
#endif

    initialise();
    glutDisplayFunc(display);
    glutTimerFunc(50, update, 0);
    glutMainLoop();
}