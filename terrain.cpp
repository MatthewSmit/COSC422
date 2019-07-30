#include <iostream>

#include <GL/glew.h>
#include <GL/freeglut.h>

void initialise() {
}

void update(int) {
    glutTimerFunc(50, update, 0);
    glutPostRedisplay();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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