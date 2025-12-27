#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <cmath>
#include <cctype>

struct HUDVertex {
    glm::vec2 Position;
    glm::vec3 Color;
};

// Renders 2D HUD elements for the radar speedometer crosshair
class HUDRenderer {
private:
    GLuint VAO, VBO, EBO;
    std::vector<HUDVertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int indexCount;

    void addLineInternal(const glm::vec2& p1, const glm::vec2& p2, const glm::vec3& color) {
        unsigned int start = (unsigned int)vertices.size();
        vertices.push_back({ p1, color });
        vertices.push_back({ p2, color });
        indices.push_back(start);
        indices.push_back(start + 1);
    }

    struct Seg { float x1, y1, x2, y2; };

    static const std::vector<Seg>& glyph(char c) {
        static const std::vector<Seg> EMPTY = {};
        static const std::vector<Seg> SPACE = {};

        static const std::vector<Seg> COLON = {
            {0.50f, 0.70f, 0.50f, 0.70f},
            {0.50f, 0.30f, 0.50f, 0.30f},
        };

        static const std::vector<Seg> SLASH = {
            {0.15f, 0.10f, 0.85f, 0.90f},
        };

        static const std::vector<Seg> DASH = {
            {0.20f, 0.50f, 0.80f, 0.50f},
        };

        static const std::vector<Seg> DOT = {
            {0.50f, 0.10f, 0.50f, 0.10f},
        };

        // Digits
        static const std::vector<Seg> D0 = {
            {0.25f,0.15f,0.75f,0.15f},
            {0.75f,0.15f,0.75f,0.85f},
            {0.75f,0.85f,0.25f,0.85f},
            {0.25f,0.85f,0.25f,0.15f},
        };
        static const std::vector<Seg> D1 = {
            {0.55f,0.15f,0.55f,0.85f},
            {0.45f,0.75f,0.55f,0.85f},
        };
        static const std::vector<Seg> D2 = {
            {0.25f,0.85f,0.75f,0.85f},
            {0.75f,0.85f,0.75f,0.55f},
            {0.75f,0.55f,0.25f,0.15f},
            {0.25f,0.15f,0.75f,0.15f},
        };
        static const std::vector<Seg> D3 = {
            {0.25f,0.85f,0.75f,0.85f},
            {0.75f,0.85f,0.75f,0.15f},
            {0.25f,0.50f,0.75f,0.50f},
            {0.25f,0.15f,0.75f,0.15f},
        };
        static const std::vector<Seg> D4 = {
            {0.25f,0.85f,0.25f,0.50f},
            {0.25f,0.50f,0.75f,0.50f},
            {0.75f,0.85f,0.75f,0.15f},
        };
        static const std::vector<Seg> D5 = {
            {0.75f,0.85f,0.25f,0.85f},
            {0.25f,0.85f,0.25f,0.50f},
            {0.25f,0.50f,0.75f,0.50f},
            {0.75f,0.50f,0.75f,0.15f},
            {0.75f,0.15f,0.25f,0.15f},
        };
        static const std::vector<Seg> D6 = {
            {0.75f,0.85f,0.25f,0.85f},
            {0.25f,0.85f,0.25f,0.15f},
            {0.25f,0.15f,0.75f,0.15f},
            {0.75f,0.15f,0.75f,0.50f},
            {0.75f,0.50f,0.25f,0.50f},
        };
        static const std::vector<Seg> D7 = {
            {0.25f,0.85f,0.75f,0.85f},
            {0.75f,0.85f,0.35f,0.15f},
        };
        static const std::vector<Seg> D8 = {
            {0.25f,0.15f,0.75f,0.15f},
            {0.75f,0.15f,0.75f,0.85f},
            {0.75f,0.85f,0.25f,0.85f},
            {0.25f,0.85f,0.25f,0.15f},
            {0.25f,0.50f,0.75f,0.50f},
        };
        static const std::vector<Seg> D9 = {
            {0.25f,0.15f,0.75f,0.15f},
            {0.75f,0.15f,0.75f,0.85f},
            {0.75f,0.85f,0.25f,0.85f},
            {0.25f,0.85f,0.25f,0.50f},
            {0.25f,0.50f,0.75f,0.50f},
        };

        // Letters
        static const std::vector<Seg> A = {
            {0.20f,0.15f,0.20f,0.85f},
            {0.80f,0.15f,0.80f,0.85f},
            {0.20f,0.85f,0.80f,0.85f},
            {0.20f,0.50f,0.80f,0.50f},
        };
        static const std::vector<Seg> B = {
            {0.20f,0.15f,0.20f,0.85f},
            {0.20f,0.85f,0.70f,0.85f},
            {0.70f,0.85f,0.75f,0.75f},
            {0.75f,0.75f,0.70f,0.65f},
            {0.70f,0.65f,0.20f,0.65f},
            {0.20f,0.65f,0.70f,0.65f},
            {0.70f,0.65f,0.75f,0.55f},
            {0.75f,0.55f,0.70f,0.45f},
            {0.70f,0.45f,0.20f,0.45f},
            {0.20f,0.15f,0.70f,0.15f},
            {0.70f,0.15f,0.75f,0.25f},
            {0.75f,0.25f,0.70f,0.35f},
        };
        static const std::vector<Seg> C = {
            {0.80f,0.80f,0.60f,0.85f},
            {0.60f,0.85f,0.30f,0.85f},
            {0.30f,0.85f,0.20f,0.70f},
            {0.20f,0.70f,0.20f,0.30f},
            {0.20f,0.30f,0.30f,0.15f},
            {0.30f,0.15f,0.60f,0.15f},
            {0.60f,0.15f,0.80f,0.20f},
        };
        static const std::vector<Seg> D = {
            {0.20f,0.15f,0.20f,0.85f},
            {0.20f,0.85f,0.60f,0.85f},
            {0.60f,0.85f,0.80f,0.65f},
            {0.80f,0.65f,0.80f,0.35f},
            {0.80f,0.35f,0.60f,0.15f},
            {0.60f,0.15f,0.20f,0.15f},
        };
        static const std::vector<Seg> E = {
            {0.80f,0.85f,0.20f,0.85f},
            {0.20f,0.85f,0.20f,0.15f},
            {0.20f,0.50f,0.65f,0.50f},
            {0.20f,0.15f,0.80f,0.15f},
        };
        static const std::vector<Seg> F = {
            {0.20f,0.85f,0.20f,0.15f},
            {0.20f,0.85f,0.80f,0.85f},
            {0.20f,0.50f,0.65f,0.50f},
        };
        static const std::vector<Seg> G = {
            {0.80f,0.80f,0.60f,0.85f},
            {0.60f,0.85f,0.30f,0.85f},
            {0.30f,0.85f,0.20f,0.70f},
            {0.20f,0.70f,0.20f,0.30f},
            {0.20f,0.30f,0.30f,0.15f},
            {0.30f,0.15f,0.60f,0.15f},
            {0.60f,0.15f,0.80f,0.25f},
            {0.80f,0.25f,0.80f,0.45f},
            {0.80f,0.45f,0.55f,0.45f},
        };
        static const std::vector<Seg> H = {
            {0.20f,0.85f,0.20f,0.15f},
            {0.80f,0.85f,0.80f,0.15f},
            {0.20f,0.50f,0.80f,0.50f},
        };
        static const std::vector<Seg> I = {
            {0.20f,0.85f,0.80f,0.85f},
            {0.50f,0.85f,0.50f,0.15f},
            {0.20f,0.15f,0.80f,0.15f},
        };
        static const std::vector<Seg> J = {
            {0.20f,0.85f,0.80f,0.85f},
            {0.50f,0.85f,0.50f,0.20f},
            {0.50f,0.20f,0.40f,0.15f},
            {0.40f,0.15f,0.25f,0.20f},
        };
        static const std::vector<Seg> K = {
            {0.20f,0.85f,0.20f,0.15f},
            {0.80f,0.85f,0.25f,0.50f},
            {0.80f,0.15f,0.25f,0.50f},
        };
        static const std::vector<Seg> L = {
            {0.20f,0.85f,0.20f,0.15f},
            {0.20f,0.15f,0.80f,0.15f},
        };
        static const std::vector<Seg> M = {
            {0.20f,0.15f,0.20f,0.85f},
            {0.20f,0.85f,0.50f,0.55f},
            {0.50f,0.55f,0.80f,0.85f},
            {0.80f,0.85f,0.80f,0.15f},
        };
        static const std::vector<Seg> N = {
            {0.20f,0.15f,0.20f,0.85f},
            {0.20f,0.85f,0.80f,0.15f},
            {0.80f,0.15f,0.80f,0.85f},
        };
        static const std::vector<Seg> O = D0;
        static const std::vector<Seg> P = {
            {0.20f,0.15f,0.20f,0.85f},
            {0.20f,0.85f,0.75f,0.85f},
            {0.75f,0.85f,0.75f,0.55f},
            {0.75f,0.55f,0.20f,0.55f},
        };
        static const std::vector<Seg> Q = {
            {0.25f,0.15f,0.75f,0.15f},
            {0.75f,0.15f,0.75f,0.85f},
            {0.75f,0.85f,0.25f,0.85f},
            {0.25f,0.85f,0.25f,0.15f},
            {0.55f,0.35f,0.85f,0.10f},
        };
        static const std::vector<Seg> R = {
            {0.20f,0.15f,0.20f,0.85f},
            {0.20f,0.85f,0.75f,0.85f},
            {0.75f,0.85f,0.75f,0.55f},
            {0.75f,0.55f,0.20f,0.55f},
            {0.20f,0.55f,0.80f,0.15f},
        };
        static const std::vector<Seg> S = {
            {0.75f,0.85f,0.25f,0.85f},
            {0.25f,0.85f,0.25f,0.55f},
            {0.25f,0.55f,0.75f,0.55f},
            {0.75f,0.55f,0.75f,0.15f},
            {0.75f,0.15f,0.25f,0.15f},
        };
        static const std::vector<Seg> T = {
            {0.20f,0.85f,0.80f,0.85f},
            {0.50f,0.85f,0.50f,0.15f},
        };
        static const std::vector<Seg> U = {
            {0.20f,0.85f,0.20f,0.25f},
            {0.20f,0.25f,0.30f,0.15f},
            {0.30f,0.15f,0.70f,0.15f},
            {0.70f,0.15f,0.80f,0.25f},
            {0.80f,0.25f,0.80f,0.85f},
        };
        static const std::vector<Seg> V = {
            {0.20f,0.85f,0.50f,0.15f},
            {0.80f,0.85f,0.50f,0.15f},
        };
        static const std::vector<Seg> W = {
            {0.20f,0.85f,0.30f,0.15f},
            {0.30f,0.15f,0.50f,0.45f},
            {0.50f,0.45f,0.70f,0.15f},
            {0.70f,0.15f,0.80f,0.85f},
        };
        static const std::vector<Seg> X = {
            {0.20f,0.85f,0.80f,0.15f},
            {0.80f,0.85f,0.20f,0.15f},
        };
        static const std::vector<Seg> Y = {
            {0.20f,0.85f,0.50f,0.55f},
            {0.80f,0.85f,0.50f,0.55f},
            {0.50f,0.55f,0.50f,0.15f},
        };
        static const std::vector<Seg> Z = {
            {0.20f,0.85f,0.80f,0.85f},
            {0.80f,0.85f,0.20f,0.15f},
            {0.20f,0.15f,0.80f,0.15f},
        };

        switch (c) {
        case ' ': return SPACE;
        case ':': return COLON;
        case '/': return SLASH;
        case '-': return DASH;
        case '.': return DOT;

        case '0': return D0;
        case '1': return D1;
        case '2': return D2;
        case '3': return D3;
        case '4': return D4;
        case '5': return D5;
        case '6': return D6;
        case '7': return D7;
        case '8': return D8;
        case '9': return D9;

        case 'A': return A; case 'B': return B; case 'C': return C; case 'D': return D; case 'E': return E;
        case 'F': return F; case 'G': return G; case 'H': return H; case 'I': return I; case 'J': return J;
        case 'K': return K; case 'L': return L; case 'M': return M; case 'N': return N; case 'O': return O;
        case 'P': return P; case 'Q': return Q; case 'R': return R; case 'S': return S; case 'T': return T;
        case 'U': return U; case 'V': return V; case 'W': return W; case 'X': return X; case 'Y': return Y;
        case 'Z': return Z;
        default: return EMPTY;
        }
    }

public:
    HUDRenderer() : VAO(0), VBO(0), EBO(0), indexCount(0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }

    ~HUDRenderer() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (EBO != 0) glDeleteBuffers(1, &EBO);
    }

    void addLine(glm::vec2 p1, glm::vec2 p2, glm::vec3 color) {
        addLineInternal(p1, p2, color);
    }

    void addCircle(glm::vec2 center, float radius, glm::vec3 color, int segments = 32) {
        unsigned int start = (unsigned int)vertices.size();
        const float PI2 = 2.0f * 3.1415926f;

        for (int i = 0; i < segments; ++i) {
            float angle1 = PI2 * i / segments;
            float angle2 = PI2 * (i + 1) / segments;

            glm::vec2 p1 = center + glm::vec2(cos(angle1) * radius, sin(angle1) * radius);
            glm::vec2 p2 = center + glm::vec2(cos(angle2) * radius, sin(angle2) * radius);

            vertices.push_back({ p1, color });
            vertices.push_back({ p2, color });

            indices.push_back(start + i * 2);
            indices.push_back(start + i * 2 + 1);
        }
    }

    void addText(glm::vec2 pos, float scale, glm::vec3 color, const std::string& text) {
        const float charW = 1.0f;
        const float charH = 1.0f;
        const float spacing = 0.25f;

        glm::vec2 pen = pos;

        for (char raw : text) {
            char c = (char)std::toupper((unsigned char)raw);

            const auto& segs = glyph(c);

            for (const auto& s : segs) {
                glm::vec2 a = pen + glm::vec2(s.x1 * charW, s.y1 * charH) * scale;
                glm::vec2 b = pen + glm::vec2(s.x2 * charW, s.y2 * charH) * scale;
                addLineInternal(a, b, color);
            }
            pen.x += (charW + spacing) * scale;
        }
    }
    void finalize() {
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        if (!vertices.empty()) {
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(HUDVertex), &vertices[0], GL_STATIC_DRAW);
        }
        else {
            glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        if (!indices.empty()) {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        }
        else {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
        }

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(HUDVertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(HUDVertex), (void*)offsetof(HUDVertex, Color));

        glBindVertexArray(0);
        indexCount = (unsigned int)indices.size();
    }

    // Render the HUD
    void render() {
        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // Clear all HUD elements
    void clear() {
        vertices.clear();
        indices.clear();
        indexCount = 0;
    }
};
