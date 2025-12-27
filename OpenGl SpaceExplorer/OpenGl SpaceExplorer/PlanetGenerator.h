#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <cstdlib>
#include <cmath>

struct Star {
    glm::vec3 pos;
    float brightness;
};

struct Moon {
    float distance;
    float size;
    float speed;
    float angle;
};

struct Ring {
    float innerRadius;
    float outerRadius;
    glm::vec3 color;
};

struct Planet {
    glm::vec3 color;
    glm::vec3 secondaryColor;
    float distance;
    float size;
    float speed;
    float angle;
    float collisionRadius;
    float rotationSpeed;
    float rotationAngle;
    float height;

    bool scanned = false;

    int biomeType;
    unsigned int seed;
    std::vector<float> surfaceVariation;
    std::vector<Moon> moons;

    glm::vec3 noiseOffset;
    std::string name;
};

struct Sun {
    glm::vec3 pos;
    float radius;
};

struct Asteroid {
    glm::vec3 pos;
    glm::vec3 rot;
    float scale;
    float collisionRadius;
    float orbitRadius;
    float orbitSpeed;
    float orbitAngle;
    float orbitHeight;

    bool clustered = false;
    glm::vec3 clusterCenter = glm::vec3(0.0f);
    float localRadius = 0.0f;
    float localAngle = 0.0f;
    float localSpeed = 0.0f;
};

inline float fract(float x) {
    return x - std::floor(x);
}

inline float hash(float n) {
    float sinVal = std::sin(n);
    float scaled = sinVal * 43758.5453123f;
    return scaled - std::floor(scaled);
}

inline float noise1D(int x, unsigned int seed) {
    x += seed * 131;
    x = (x << 13) ^ x;
    return 1.0f - ((x * (x * x * 15731 + 789221)
        + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

inline float randFloat(float minV, float maxV) {
    return minV + (float(rand()) / float(RAND_MAX)) * (maxV - minV);
}

inline unsigned int xorshift32(unsigned int& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

inline int randRange(unsigned int& state, int minV, int maxV) {
    unsigned int r = xorshift32(state);
    int span = (maxV - minV + 1);
    return minV + (int)(r % (unsigned int)span);
}

// PLANET GENERATOR

class PlanetGenerator {
public:
    // Procedural name based on seed
    static std::string generatePlanetName(unsigned int seed, int index) {
 
        static const char* syllA[] = {
            "AR","ZA","XE","OR","VE","KA","LI","NO","RA","TU","SA","MI","EL","UN","DO","CY","LO","NE","VI","QU"
        };
        static const char* syllB[] = {
            "LON","RIN","THA","VEX","MOR","TAR","NEX","SEN","KAL","DOR","VAN","SOL","ZEN","KIR","NAR","VEL","RAX","TOR","LUX","PYR"
        };
        static const char* syllC[] = {
            "IA","ON","US","A","IS","OS","UM","E","IX","AR","ER","OR"
        };

        unsigned int st = seed ^ (unsigned int)(index * 0x9E3779B9u);

        int a = randRange(st, 0, (int)(sizeof(syllA) / sizeof(syllA[0])) - 1);
        int b = randRange(st, 0, (int)(sizeof(syllB) / sizeof(syllB[0])) - 1);
        int c = randRange(st, 0, (int)(sizeof(syllC) / sizeof(syllC[0])) - 1);

        bool addExtra = (randRange(st, 0, 99) < 35);
        int extra = randRange(st, 0, (int)(sizeof(syllA) / sizeof(syllA[0])) - 1);

        std::string name = std::string(syllA[a]) + std::string(syllB[b]) + std::string(syllC[c]);
        if (addExtra) name += std::string(syllA[extra]);

        bool addNum = (randRange(st, 0, 99) < 40);
        if (addNum) {
            int n = (index + 1);
            name += "-" + std::to_string(n);
        }
        return name;
    }

    static void generatePlanets(std::vector<Planet>& planets, int minCount = 4, int maxCount = 9, float minSunDistance = 1500.0f) {
        int biomeTypes[] = { 0, 1, 2 };
        float currentDistance = minSunDistance;

        // Randomly determine how many planets to generate between 4 and 9
        int planetCount = 4 + (rand() % 6);

        for (int i = 0; i < planetCount; ++i) {
            Planet p;
            p.biomeType = biomeTypes[i % 3];

            // BIOME-SPECIFIC SIZE
            switch (p.biomeType) {
            case 0: p.size = 15.0f + (rand() % 6); break;  // Green
            case 1: p.size = 10.0f + (rand() % 6); break;  // Rocky
            case 2: p.size = 12.0f + (rand() % 6); break;  // Ice
            }

            p.distance = currentDistance;

            // change the planets height on the Y position
            p.height = randFloat(-120.0f, 120.0f);

            // PROCEDURAL SURFACE VARIATION
            int resolution = 64;
            p.surfaceVariation.resize(resolution);

            p.seed = rand();

            for (int k = 0; k < resolution; ++k) {
                float latitude = float(k) / resolution; 
                float baseNoise = noise1D(k, p.seed) * 0.5f + 0.5f;

                // Biome shaping
                if (p.biomeType == 0) { // Green
                    baseNoise = glm::smoothstep(0.2f, 0.8f, baseNoise);
                }
                else if (p.biomeType == 1) { // Rocky
                    baseNoise = std::pow(baseNoise, 0.6f);
                }
                else if (p.biomeType == 2) { // Ice
                    baseNoise = glm::mix(baseNoise, 1.0f, latitude * 0.6f);
                }

                p.surfaceVariation[k] = baseNoise;
            }

            // CALCULATE NEXT DISTANCE
            float spacing = 80.0f + (rand() % 60);
            currentDistance = p.distance + p.size + spacing;

            // RANDOM ORBIT SPEED
            p.speed = 0.01f + static_cast<float>(rand() % 50) / 1000.0f;

            // RANDOM ANGLE
            p.angle = (rand() % 360) * 3.14159265f / 180.0f;
            p.collisionRadius = p.size * 1.5f;

            // ROTATION
            p.rotationAngle = 0.0f;
            p.rotationSpeed = 20.0f + rand() % 40;

            // BIOME COLORS
            switch (p.biomeType) {
            case 0: // Green
                p.color = glm::vec3(0.0f, 0.6f + (rand() % 20) / 100.0f, 0.0f);
                p.secondaryColor = glm::vec3(0.0f, 0.3f, 0.4f); // water
                break;
            case 1: // Rocky
                p.color = glm::vec3(0.5f, 0.4f, 0.3f);
                p.secondaryColor = glm::vec3(0.3f, 0.3f, 0.3f);
                break;
            case 2: // Ice
                p.color = glm::vec3(0.8f, 0.9f, 1.0f);
                p.secondaryColor = glm::vec3(0.6f, 0.7f, 0.9f);
                break;
            }

            // NAME (procedural)
            p.name = generatePlanetName(p.seed, i);

            // MOONS
            int moonCount = 1 + (i % 2);
            for (int m = 0; m < moonCount; ++m) {
                Moon moon;
                moon.distance = p.size + 2.5f + (m * 1.8f);
                moon.size = 0.2f + (rand() % 20) / 100.0f;
                moon.speed = 0.03f + (rand() % 10) / 10.0f;
                moon.angle = (rand() % 360) * 3.14159265f / 180.0f;
                p.moons.push_back(moon);
            }
            planets.push_back(p);
        }
    }

    static void generateAsteroids(std::vector<Asteroid>& asteroids, int count) {
        for (int i = 0; i < count; ++i) {
            float distance = 80.0f + (rand() % 400) / 10.0f;
            float height = (rand() % 40 - 20) * 0.15f;
            float speed = 0.03f + (rand() % 15) / 1000.0f;

            Asteroid a;
            a.scale = 0.3f + (rand() % 80) / 100.0f;
            a.collisionRadius = a.scale * 0.8f;
            a.orbitRadius = distance;
            a.orbitHeight = height;
            a.orbitSpeed = speed;
            a.orbitAngle = (rand() % 360) * 3.14159265f / 180.0f;
            a.rot = glm::vec3(rand() % 360, rand() % 360, rand() % 360);
            a.pos = glm::vec3(cos(a.orbitAngle) * a.orbitRadius, a.orbitHeight, sin(a.orbitAngle) * a.orbitRadius);
            asteroids.push_back(a);
        }
    }

    static void generateAsteroidClusters(
        std::vector<Asteroid>& asteroids,
        int clusterCount,
        int minPerCluster,
        int maxPerCluster,
        float minClusterDist,
        float maxClusterDist
    ) {
        for (int c = 0; c < clusterCount; ++c) {

            // pick a random cluster center around the sun
            float angle = (rand() % 360) * 3.14159265f / 180.0f;
            float dist = minClusterDist + (rand() % (int)(maxClusterDist - minClusterDist + 1));
            float height = ((rand() % 600) - 300) * 0.05f;

            glm::vec3 center = glm::vec3(
                cos(angle) * dist,
                height,
                sin(angle) * dist
            );

            int count = minPerCluster + (rand() % (maxPerCluster - minPerCluster + 1));

            for (int i = 0; i < count; ++i) {
                Asteroid a;

                a.clustered = true;
                a.clusterCenter = center;

                // asteroid size
                a.scale = 0.25f + (rand() % 90) / 100.0f;
                a.collisionRadius = a.scale * 0.8f;

                a.localRadius = 6.0f + (rand() % 220) / 10.0f;
                a.localAngle = (rand() % 360) * 3.14159265f / 180.0f;
                a.localSpeed = 0.2f + (rand() % 120) / 100.0f;

                // random Y offset
                float yOff = ((rand() % 800) - 400) * 0.02f;
                a.orbitHeight = yOff;


                a.pos = a.clusterCenter + glm::vec3(
                    cos(a.localAngle) * a.localRadius,
                    a.orbitHeight,
                    sin(a.localAngle) * a.localRadius
                );
                a.orbitRadius = dist;
                a.orbitSpeed = 0.0f;
                a.orbitAngle = 0.0f;

                asteroids.push_back(a);
            }
        }
    }

    static void generateStars(std::vector<Star>& stars, int count) {
        for (int i = 0; i < count; ++i) {
            Star s;
            float theta = (rand() % 360) * 3.14159265f / 180.0f;
            float phi = (rand() % 180) * 3.14159265f / 180.0f;
            float r = 3000.0f + (rand() % 4000) / 10.0f;

            s.pos = glm::vec3(
                r * sin(phi) * cos(theta),
                r * sin(phi) * sin(theta),
                r * cos(phi)
            );
            s.brightness = 0.3f + (rand() % 70) / 100.0f;
            stars.push_back(s);
        }
    }

    static glm::vec3 getPlanetSurfaceColor(const Planet& p, float variation) {
        switch (p.biomeType) {
        case 0: {  // Green - lakes & grass
            return glm::mix(p.secondaryColor, p.color, variation);
        }
        case 1: {  // Rocky - rocks & hills
            float rockyMask = std::pow(variation, 0.8f);
            return glm::mix(p.secondaryColor, p.color, rockyMask);
        }
        case 2: {  // Ice
            return glm::mix(p.secondaryColor, p.color, variation);
        }
        default:
            return p.color;
        }
    }
};
