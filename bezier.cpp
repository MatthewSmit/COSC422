#include <iostream>
#include <memory>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include "model.h"
#include "shader.h"
#include "util.h"

class Floor : public Model {
public:
    Floor() {
        shader = std::make_unique<Shader>("floor.vert", "floor.frag");
    }

    void update() override {
    }

    void render(const Scene& scene) override {

    }
};

std::unique_ptr<Scene> scene;
std::unique_ptr<BezierModel> model;
std::unique_ptr<Floor> floor;

void initialise() {
    scene = std::make_unique<Scene>();
    model = std::make_unique<BezierModel>("PatchVerts_Teapot.txt");
    floor = std::make_unique<Floor>();
}

void update(int) {
    floor->update();
    model->update();

    glutTimerFunc(50, update, 0);
    glutPostRedisplay();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    floor->render(*scene);
    model->render(*scene);

    glutSwapBuffers();
}

int main(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("COSC 422 Assignment 1 - MJS351 - Bezier");
    glutInitContextVersion (4, 2);
    glutInitContextProfile (GLUT_CORE_PROFILE);

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

    initialise();
    glutDisplayFunc(display);
    glutTimerFunc(50, update, 0);
    glutMainLoop();
}