#pragma once
#include <GL/glew.h>
#include <string>

class Texture {
public:
    unsigned int ID = 0;

    explicit Texture(const std::string& path);
    void Bind(unsigned int unit = 0) const;
};
