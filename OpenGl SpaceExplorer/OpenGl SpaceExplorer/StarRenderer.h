#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

struct StarVertex {
    glm::vec3 Position;
    float Brightness;
};

class StarRenderer {
private:
    GLuint VAO, VBO;
    unsigned int vertexCount;

public:
    StarRenderer() : VAO(0), VBO(0), vertexCount(0) {}

    ~StarRenderer() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
    }

    void loadStars(const std::vector<glm::vec3>& starPositions) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        std::vector<StarVertex> vertices;
        for (const auto& pos : starPositions) {
            float brightness = 0.3f + (rand() / (float)RAND_MAX) * 0.7f;
            vertices.push_back({ pos, brightness });
        }

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(StarVertex), &vertices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(StarVertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(StarVertex), (void*)offsetof(StarVertex, Brightness));

        glBindVertexArray(0);
        vertexCount = vertices.size();
    }

    void render() {
        glBindVertexArray(VAO);
        glPointSize(2.0f);
        glDrawArrays(GL_POINTS, 0, vertexCount);
        glPointSize(1.0f);
    }
};