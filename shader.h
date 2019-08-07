#pragma once

#include <fstream>
#include <string>
#include <sstream>

#include <GL/glew.h>

GLuint loadShader(int shaderType, const std::string& shaderFile) {
    std::ifstream file(shaderFile.c_str());
    if(!file.good()) {
        std::cerr << "Error opening shader file: " << shaderFile << std::endl;
        throw std::exception{};
    }

    std::stringstream shaderData{};
    shaderData << file.rdbuf();
    file.close();

    std::string shaderStr = shaderData.str();
    const char* shaderTxt = shaderStr.c_str();

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderTxt, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    GLint infoLogLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (!status || infoLogLength > 0) {
        auto infoLog = std::unique_ptr<GLchar[]>{new GLchar[infoLogLength + 1]};
        glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog.get());
        fprintf(stderr, "Shader compile failure for %s: %s\n", shaderFile.c_str(), infoLog.get());
    }

    if (!status) {
        throw std::exception{};
    }

    return shader;
}

void verifyProgram(GLuint program) {
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    GLint infoLogLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (!status || infoLogLength > 0) {
        auto infoLog = std::unique_ptr<GLchar[]>{new GLchar[infoLogLength + 1]};
        glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog.get());
        fprintf(stderr, "Linker warning: %s\n", infoLog.get());
    }

    if (!status) {
        throw std::exception{};
    }
}

class Shader {
public:
    Shader(const std::string& vertexShaderFile, const std::string& fragmentShaderFile) {
        GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderFile);
        GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderFile);

        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        verifyProgram(program);
    }

    Shader(const std::string& vertexShaderFile,
           const std::string& tesselationControlShaderFile,
           const std::string& tesselationEvaluationShaderFile,
           const std::string& fragmentShaderFile) {
        GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderFile);
        GLuint tesselationControlShader = loadShader(GL_TESS_CONTROL_SHADER, tesselationControlShaderFile);
        GLuint tesselationEvaluationShader = loadShader(GL_TESS_EVALUATION_SHADER, tesselationEvaluationShaderFile);
        GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderFile);

        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, tesselationControlShader);
        glAttachShader(program, tesselationEvaluationShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        verifyProgram(program);
    }

    Shader(const std::string& vertexShaderFile,
           const std::string& tesselationControlShaderFile,
           const std::string& tesselationEvaluationShaderFile,
           const std::string& geometryShaderFile,
           const std::string& fragmentShaderFile) {
        GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderFile);
        GLuint tesselationControlShader = loadShader(GL_TESS_CONTROL_SHADER, tesselationControlShaderFile);
        GLuint tesselationEvaluationShader = loadShader(GL_TESS_EVALUATION_SHADER, tesselationEvaluationShaderFile);
        GLuint geometryShader = loadShader(GL_GEOMETRY_SHADER, geometryShaderFile);
        GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderFile);

        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, tesselationControlShader);
        glAttachShader(program, tesselationEvaluationShader);
        glAttachShader(program, geometryShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        verifyProgram(program);
    }

    ~Shader() {
        glDeleteProgram(program);
    }

    GLuint program;
};