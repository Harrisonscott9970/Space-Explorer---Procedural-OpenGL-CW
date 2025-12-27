#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <cmath>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO, VBO, EBO;

    Mesh() : VAO(0), VBO(0), EBO(0) {}

    ~Mesh() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (EBO != 0) glDeleteBuffers(1, &EBO);
    }

    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        // TexCoord
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoord));

        glBindVertexArray(0);
    }

    void Draw() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

inline void generateUVSphere(Mesh& mesh, float radius, int slices, int stacks) {
    mesh.vertices.clear();
    mesh.indices.clear();

    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = glm::pi<float>() / 2.0f - i * glm::pi<float>() / stacks;
        float xy = radius * cos(stackAngle);
        float z = radius * sin(stackAngle);

        for (int j = 0; j <= slices; ++j) {
            float sliceAngle = j * 2.0f * glm::pi<float>() / slices;

            Vertex v;
            v.Position = glm::vec3(xy * cos(sliceAngle), z, xy * sin(sliceAngle));
            v.Normal = glm::normalize(v.Position);
            v.TexCoord = glm::vec2((float)j / slices, (float)i / stacks);

            mesh.vertices.push_back(v);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (slices + 1);
        int k2 = k1 + slices + 1;

        for (int j = 0; j < slices; ++j) {
            if (i != 0) {
                mesh.indices.push_back(k1);
                mesh.indices.push_back(k2);
                mesh.indices.push_back(k1 + 1);
            }

            if (i != (stacks - 1)) {
                mesh.indices.push_back(k1 + 1);
                mesh.indices.push_back(k2);
                mesh.indices.push_back(k2 + 1);
            }

            k1++;
            k2++;
        }
    }

    mesh.setupMesh();
}

inline void generateCube(Mesh& mesh, float size) {
    mesh.vertices.clear();
    mesh.indices.clear();

    float h = size / 2.0f;

    // Front face
    mesh.vertices.push_back({ glm::vec3(-h, -h, h), glm::vec3(0, 0, 1), glm::vec2(0, 0) });
    mesh.vertices.push_back({ glm::vec3(h, -h, h), glm::vec3(0, 0, 1), glm::vec2(1, 0) });
    mesh.vertices.push_back({ glm::vec3(h, h, h), glm::vec3(0, 0, 1), glm::vec2(1, 1) });
    mesh.vertices.push_back({ glm::vec3(-h, h, h), glm::vec3(0, 0, 1), glm::vec2(0, 1) });

    // Back face
    mesh.vertices.push_back({ glm::vec3(-h, -h, -h), glm::vec3(0, 0, -1), glm::vec2(1, 0) });
    mesh.vertices.push_back({ glm::vec3(-h, h, -h), glm::vec3(0, 0, -1), glm::vec2(1, 1) });
    mesh.vertices.push_back({ glm::vec3(h, h, -h), glm::vec3(0, 0, -1), glm::vec2(0, 1) });
    mesh.vertices.push_back({ glm::vec3(h, -h, -h), glm::vec3(0, 0, -1), glm::vec2(0, 0) });

    // Left face
    mesh.vertices.push_back({ glm::vec3(-h, -h, -h), glm::vec3(-1, 0, 0), glm::vec2(0, 0) });
    mesh.vertices.push_back({ glm::vec3(-h, -h, h), glm::vec3(-1, 0, 0), glm::vec2(1, 0) });
    mesh.vertices.push_back({ glm::vec3(-h, h, h), glm::vec3(-1, 0, 0), glm::vec2(1, 1) });
    mesh.vertices.push_back({ glm::vec3(-h, h, -h), glm::vec3(-1, 0, 0), glm::vec2(0, 1) });

    // Right face
    mesh.vertices.push_back({ glm::vec3(h, -h, h), glm::vec3(1, 0, 0), glm::vec2(0, 0) });
    mesh.vertices.push_back({ glm::vec3(h, -h, -h), glm::vec3(1, 0, 0), glm::vec2(1, 0) });
    mesh.vertices.push_back({ glm::vec3(h, h, -h), glm::vec3(1, 0, 0), glm::vec2(1, 1) });
    mesh.vertices.push_back({ glm::vec3(h, h, h), glm::vec3(1, 0, 0), glm::vec2(0, 1) });

    // Top face
    mesh.vertices.push_back({ glm::vec3(-h, h, h), glm::vec3(0, 1, 0), glm::vec2(0, 1) });
    mesh.vertices.push_back({ glm::vec3(h, h, h), glm::vec3(0, 1, 0), glm::vec2(1, 1) });
    mesh.vertices.push_back({ glm::vec3(h, h, -h), glm::vec3(0, 1, 0), glm::vec2(1, 0) });
    mesh.vertices.push_back({ glm::vec3(-h, h, -h), glm::vec3(0, 1, 0), glm::vec2(0, 0) });

    // Bottom face
    mesh.vertices.push_back({ glm::vec3(-h, -h, -h), glm::vec3(0, -1, 0), glm::vec2(0, 1) });
    mesh.vertices.push_back({ glm::vec3(h, -h, -h), glm::vec3(0, -1, 0), glm::vec2(1, 1) });
    mesh.vertices.push_back({ glm::vec3(h, -h, h), glm::vec3(0, -1, 0), glm::vec2(1, 0) });
    mesh.vertices.push_back({ glm::vec3(-h, -h, h), glm::vec3(0, -1, 0), glm::vec2(0, 0) });

    // Indices for all 6 faces
    for (unsigned int i = 0; i < 6; ++i) {
        unsigned int base = i * 4;
        mesh.indices.push_back(base);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
    }

    mesh.setupMesh();
}