#include "ProbeModel.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

static glm::vec3 safeNormal(const aiVector3D& n) {
    glm::vec3 nn(n.x, n.y, n.z);
    float len = glm::length(nn);
    if (len < 0.00001f) return glm::vec3(0, 1, 0);
    return nn / len;
}

ProbeModel::ProbeModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || !scene->HasMeshes()) {
        throw std::runtime_error(std::string("Assimp load failed: ") + importer.GetErrorString());
    }

    aiMesh* mesh = scene->mMeshes[0];
    if (!mesh || mesh->mNumVertices == 0 || mesh->mNumFaces == 0) {
        throw std::runtime_error("Assimp mesh invalid/empty: " + path);
    }

    std::vector<Vertex> verts;
    verts.reserve(mesh->mNumFaces * 3);

    for (unsigned i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& f = mesh->mFaces[i];
        if (f.mNumIndices != 3) continue;

        for (unsigned j = 0; j < 3; ++j) {
            unsigned idx = f.mIndices[j];
            aiVector3D p = mesh->mVertices[idx];
            aiVector3D n = mesh->HasNormals() ? mesh->mNormals[idx] : aiVector3D(0, 1, 0);

            Vertex v;
            v.pos = glm::vec3(p.x, p.y, p.z);
            v.normal = safeNormal(n);
            verts.push_back(v);
        }
    }

    if (verts.empty()) {
        throw std::runtime_error("Assimp produced no triangles: " + path);
    }

    vertexCount = (GLsizei)verts.size();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glBindVertexArray(0);
}

ProbeModel::~ProbeModel() {
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
}

void ProbeModel::draw() const {
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}
