#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    GLuint Program;

    Shader(const char* vertexPath, const char* fragmentPath) {
        try {
            std::string vertexCode = readFile(vertexPath);
            std::string fragmentCode = readFile(fragmentPath);

            const char* vCode = vertexCode.c_str();
            const char* fCode = fragmentCode.c_str();

            // Compile vertex
            GLuint vertex = compileShader(vCode, GL_VERTEX_SHADER, "VERTEX");
            GLuint fragment = compileShader(fCode, GL_FRAGMENT_SHADER, "FRAGMENT");

            Program = glCreateProgram();
            glAttachShader(Program, vertex);
            glAttachShader(Program, fragment);
            glLinkProgram(Program);

            int success;
            char infoLog[1024];
            glGetProgramiv(Program, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(Program, 1024, NULL, infoLog);
                throw std::runtime_error(std::string("Program linking failed: ") + infoLog);
            }

            glDeleteShader(vertex);
            glDeleteShader(fragment);

            std::cout << "Shader program compiled successfully" << std::endl;
        }
        catch (const std::exception& e) {
            throw std::runtime_error(std::string("Shader initialization failed: ") + e.what());
        }
    }

    ~Shader() {
        glDeleteProgram(Program);
    }

    void Use() const {
        glUseProgram(Program);
    }

    void SetMat4(const std::string& name, const glm::mat4& mat) const {
        GLint loc = glGetUniformLocation(Program, name.c_str());
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
    }

    void SetVec3(const std::string& name, const glm::vec3& value) const {
        GLint loc = glGetUniformLocation(Program, name.c_str());
        glUniform3fv(loc, 1, glm::value_ptr(value));
    }

    void SetFloat(const std::string& name, float value) const {
        GLint loc = glGetUniformLocation(Program, name.c_str());
        glUniform1f(loc, value);
    }

    void SetInt(const std::string& name, int value) const {
        GLint loc = glGetUniformLocation(Program, name.c_str());
        glUniform1i(loc, value);
    }

private:
    GLuint compileShader(const char* code, GLenum type, const char* typeName) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &code, NULL);
        glCompileShader(shader);

        int success;
        char infoLog[1024];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            throw std::runtime_error(std::string("Shader compilation failed (") +
                typeName + "): " + infoLog);
        }
        return shader;
    }

    std::string readFile(const char* filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error(std::string("Cannot open shader file: ") + filePath);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};