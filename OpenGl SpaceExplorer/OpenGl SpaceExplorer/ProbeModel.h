#pragma once
#include <string>
#include <vector>
#include <stdexcept>

#include <GL/glew.h>
#include <glm/glm.hpp>

class ProbeModel {
public:
    ProbeModel() = default;
    explicit ProbeModel(const std::string& path);
    ~ProbeModel();

    void draw() const;
    bool loaded() const { return vao != 0; }

private:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    GLsizei vertexCount = 0;
};
