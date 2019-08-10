#include <iostream>
#include <memory>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include "model.h"
#include "shader.h"
#include "texture.h"

bool keyState[256] = {};
bool specialKeyState[GLUT_KEY_INSERT + 1] = {};
int oldTimeSinceStart{};

std::unique_ptr<Scene> scene;
bool wireframeMode{false};

class Terrain : public Model {
public:
    explicit Terrain(const Scene& scene) {
        shader = std::make_unique<Shader>("data/terrain.vert",
                "data/terrain.tesc",
                "data/terrain.tese",
                "data/terrain.geom",
                "data/terrain.frag");

        heightMap1 = std::make_unique<Texture>("data/HeightMap1.tga");
        heightMap2 = std::make_unique<Texture>("data/HeightMap2.png");

        waterTexture = std::make_unique<Texture>("data/Water.png");
        grassTexture = std::make_unique<Texture>("data/Grass.jpg");
        snowTexture = std::make_unique<Texture>("data/Snow.jpg");

        // Create a grid of vertices from -SIZE to SIZE in both x and z
        glm::vec4 VERTEX_DATA[TOTAL_VERTICES]{};
        uint32_t INDEX_DATA[TOTAL_INDICES]{};

        for (auto y = 0; y < GRID_SIZE + 1; y++) {
            for (auto x = 0; x < GRID_SIZE + 1; x++) {
                VERTEX_DATA[y * (GRID_SIZE + 1) + x] = glm::vec4{
                        ((x * 2.0f) / GRID_SIZE - 1.0f) * SIZE,
                        ((y * 2.0f) / GRID_SIZE - 1.0f) * SIZE,
                        (float)x / GRID_SIZE,
                        1 - (float)y / GRID_SIZE
                };
            }
        }

        for (auto y = 0; y < GRID_SIZE; y++) {
            for (auto x = 0; x < GRID_SIZE; x++) {
                INDEX_DATA[(y * GRID_SIZE + x) * 4 + 0] = y * (GRID_SIZE + 1) + x;
                INDEX_DATA[(y * GRID_SIZE + x) * 4 + 1] = (y + 1) * (GRID_SIZE + 1) + x;
                INDEX_DATA[(y * GRID_SIZE + x) * 4 + 2] = (y + 1) * (GRID_SIZE + 1) + (x + 1);
                INDEX_DATA[(y * GRID_SIZE + x) * 4 + 3] = y * (GRID_SIZE + 1) + (x + 1);
            }
        }

        // Send buffer data to GPU
        glNamedBufferStorage(buffers[VERTEX_BUFFER], sizeof(VERTEX_DATA), VERTEX_DATA, 0);
        glNamedBufferStorage(buffers[INDEX_BUFFER], sizeof(INDEX_DATA), INDEX_DATA, 0);

        // Setup vertex attributes
        glEnableVertexArrayAttrib(vertexArray, 0);
        glVertexArrayAttribFormat(vertexArray, 0, 4, GL_FLOAT, GL_FALSE, 0);

        glVertexArrayVertexBuffer(vertexArray, 0, buffers[VERTEX_BUFFER], 0, sizeof(glm::vec4));
        glVertexArrayAttribBinding(vertexArray, 0, 0);

        glVertexArrayElementBuffer(vertexArray, buffers[INDEX_BUFFER]);

        // Setup uniform blocks
        GLuint sceneInputDataIndex = glGetUniformBlockIndex(shader->program, "SceneInputData");
        glUniformBlockBinding(shader->program, sceneInputDataIndex, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, scene.getSceneUniformBuffer());
    }

    ~Terrain() override = default;

    void updateKeyboard(unsigned char key) {
        if (key == '1') {
            heightMap = 0;
        }
        else if (key == '2') {
            heightMap = 1;
        }

        if (key == 'v') {
            waterHeight -= 0.1;
            if (waterHeight < 0) {
                waterHeight = 0;
            }
        }
        if (key == 'b') {
            waterHeight += 0.1;
            if (waterHeight > 7) {
                waterHeight = 7;
            }
        }

        if (key == 'n') {
            snowHeight -= 0.1;
            if (snowHeight < 5) {
                snowHeight = 5;
            }
        }
        if (key == 'm') {
            snowHeight += 0.1;
            if (snowHeight > 10) {
                snowHeight = 10;
            }
        }
    }

    void update(float) override {
    }

    void render(const Scene& scene) override {
        glPatchParameteri(GL_PATCH_VERTICES, 4);
        glBindVertexArray(vertexArray);
        glUseProgram(shader->program);
        if (heightMap == 0) {
            heightMap1->bind(0);
        } else {
            heightMap2->bind(0);
        }
        grassTexture->bind(1);
        snowTexture->bind(2);
        waterTexture->bind(3);
        glUniform1f(glGetUniformLocation(shader->program, "waterHeight"), waterHeight);
        glUniform1f(glGetUniformLocation(shader->program, "snowHeight"), snowHeight);
        glDrawElements(GL_PATCHES, TOTAL_INDICES, GL_UNSIGNED_INT, nullptr);
    }

private:
    static constexpr auto GRID_SIZE = 9;
    static constexpr auto SIZE = 50.0f;
    static constexpr auto TOTAL_INDICES = GRID_SIZE * GRID_SIZE * 4;
    static constexpr auto TOTAL_VERTICES = (GRID_SIZE + 1) * (GRID_SIZE + 1);

    std::unique_ptr<Texture> heightMap1{};
    std::unique_ptr<Texture> heightMap2{};
    std::unique_ptr<Texture> waterTexture{};
    std::unique_ptr<Texture> grassTexture{};
    std::unique_ptr<Texture> snowTexture{};
    float waterHeight{2};
    float snowHeight{7};
    int heightMap{0};
};

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

    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        __asm("int $3");
    }

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

    ((Terrain*)scene->getModel(0))->updateKeyboard(key);

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
    scene->addModel(std::make_unique<Terrain>(*scene));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    glClearColor(1, 1, 1, 1);
}

void update(int) {
    int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
    int deltaTime = timeSinceStart - oldTimeSinceStart;
    oldTimeSinceStart = timeSinceStart;

    float delta = deltaTime * 0.001f;

    auto& camera = scene->getCamera();

    if (specialKeyState[GLUT_KEY_UP]) {
        camera.translate(glm::vec3{0, 0, -10 * delta});
    }
    if (specialKeyState[GLUT_KEY_DOWN]) {
        camera.translate(glm::vec3{0, 0, 10 * delta});
    }

    if (specialKeyState[GLUT_KEY_LEFT]) {
        camera.translate(glm::vec3{-10 * delta, 0, 0});
    }
    if (specialKeyState[GLUT_KEY_RIGHT]) {
        camera.translate(glm::vec3{10 * delta, 0, 0});
    }

    if (keyState['+'] || keyState['=']) {
        camera.translate(glm::vec3{0, 10 * delta, 0});
    }
    if (keyState['-']) {
        camera.translate(glm::vec3{0, -10 * delta, 0});
    }

    camera.lookAt(camera.getCameraPosition() - glm::vec3(0.0, 15.0, 20.0));

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
    ilInit();
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
    glutCreateWindow("COSC 422 Assignment 1 - MJS351 - Terrain");

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