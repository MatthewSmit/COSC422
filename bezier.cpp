#include <iostream>
#include <memory>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include "model.h"
#include "shader.h"

bool keyState[256] = {};
bool specialKeyState[GLUT_KEY_INSERT + 1] = {};
int oldTimeSinceStart{};

std::unique_ptr<Scene> scene;
bool wireframeMode{false};
bool exploding{false};

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

    void setScale(float newScale) {
        scale = newScale;
    }

    void update(float delta) override {
        if (!exploding) {
            rotateY += delta / 4;
            modelInputData.time = 0;
        } else {
            modelInputData.time += delta;
        }

        modelInputData.world = glm::translate(glm::mat4(1), glm::vec3{0, 2, 0}) *
                glm::rotate(glm::mat4(1), rotateX, glm::vec3{1, 0, 0}) *
                glm::rotate(glm::mat4(1), rotateY, glm::vec3{0, 1, 0}) *
                glm::scale(glm::mat4(1), glm::vec3{scale});
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
        float time{};
    } modelInputData{};

    int numberVertices{};
    float rotateY{};
    float rotateX{};
    float scale{1};

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

void GLAPIENTRY debugCallback(GLenum source,
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

void keyboardCallback(unsigned char key, int, int) {
    if (key == 'w') {
        wireframeMode = !wireframeMode;
    }

    if (key == ' ') {
        exploding = !exploding;
    }

    keyState[key] = true;
}

void keyboardUpCallback(unsigned char key, int, int) {
    keyState[key] = false;
}

void specialCallback(int key, int, int) {
    if (key >= 0 && key <= GLUT_KEY_INSERT) {
        specialKeyState[key] = true;
    }
}

void specialUpCallback(int key, int, int) {
    if (key >= 0 && key <= GLUT_KEY_INSERT) {
        specialKeyState[key] = false;
    }
}

void initialise() {
    scene = std::make_unique<Scene>();
//    auto model = std::make_unique<BezierModel>(*scene, "data/PatchVerts_Teapot.txt");
//    model->setScale(2);
    auto model = std::make_unique<BezierModel>(*scene, "data/PatchVerts_Gumbo.txt");
    model->setScale(0.5);
    scene->addModel(std::move(model));
    scene->addModel(std::make_unique<Floor>(*scene));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    glClearColor(1, 1, 1, 1);
}

void update(int) {
    int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
    int deltaTime = timeSinceStart - oldTimeSinceStart;
    oldTimeSinceStart = timeSinceStart;

    float delta = deltaTime * 0.001f;

    if (specialKeyState[GLUT_KEY_UP]) {
        scene->getCamera().translate(glm::vec3{0, 0, -10 * delta});
    }
    if (specialKeyState[GLUT_KEY_DOWN]) {
        scene->getCamera().translate(glm::vec3{0, 0, 10 * delta});
    }

    scene->update(delta);
    glutTimerFunc(50, update, 0);
    glutPostRedisplay();
}

void display() {
    glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);

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
    glutSetKeyRepeat(false);
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
    glutKeyboardFunc(keyboardCallback);
    glutKeyboardUpFunc(keyboardUpCallback);
    glutSpecialFunc(specialCallback);
    glutSpecialUpFunc(specialUpCallback);
    glutTimerFunc(50, update, 0);
    glutMainLoop();
}