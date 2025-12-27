#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cfloat>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "PlanetGenerator.h"
#include "StarRenderer.h"
#include "HUDRenderer.h"
#include "GameState.h"
#include "Texture.h"

#include "ProbeModel.h"

float randf(float min, float max) {
    return min + static_cast<float>(rand()) /
        static_cast<float>(RAND_MAX) * (max - min);
}

static float rand01() {
    return (float)rand() / (float)RAND_MAX;
}

static const char* biomeLabel(int t) {
    if (t == 0) return "GREEN";
    if (t == 1) return "ROCKY";
    if (t == 2) return "ICE";
    return "UNKNOWN";
}

void render(float deltaTime, const glm::mat4& view, const glm::mat4& projection);

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const char* WINDOW_TITLE = "Space Explorer - Procedural Generation";

// GLOBALS
std::unique_ptr<Camera> g_camera;
float g_lastX = WINDOW_WIDTH / 2.0f;
float g_lastY = WINDOW_HEIGHT / 2.0f;
bool g_firstMouse = true;

std::unique_ptr<Shader> g_shader;
std::unique_ptr<Shader> g_starShader;
std::unique_ptr<Shader> g_hudShader;

std::vector<Planet> g_planets;
std::vector<Asteroid> g_asteroids;
std::vector<Star> g_stars;

std::unique_ptr<Texture> g_asteroidTexture;

Sun g_sun{ glm::vec3(0.f), 25.f };
Mesh* g_sphereMesh = nullptr;
Mesh* g_cubeMesh = nullptr;
StarRenderer* g_starRenderer = nullptr;
HUDRenderer* g_hudRenderer = nullptr;

// GAMEPLAY
std::unique_ptr<GameState> g_gameState;

static float g_radarAngle = 0.0f;
static float g_pulseTime = 0.0f;

std::unique_ptr<ProbeModel> g_probeModel;
std::unique_ptr<ProbeModel> g_brokenProbeModel;

struct BrokenProbeInstance {
    glm::vec3 pos;
    float scale;
};

std::vector<BrokenProbeInstance> g_brokenProbes;


struct ProbeEntity {
    int planetIndex = -1;
    float orbitRadius = 0.0f;
    float orbitSpeed = 0.0f;
    float orbitAngle = 0.0f;
    float yOffset = 0.0f;
    glm::vec3 pos = glm::vec3(0.0f);
};

std::vector<ProbeEntity> g_probes;

// COLLISION
bool checkSphereCollision(
    const glm::vec3& aPos, float aRadius,
    const glm::vec3& bPos, float bRadius)
{
    return glm::length(aPos - bPos) < (aRadius + bRadius);
}

glm::vec3 getPlanetWorldPosition(const Planet& planet) {
    float x = g_sun.pos.x + cos(planet.angle) * planet.distance;
    float z = g_sun.pos.z + sin(planet.angle) * planet.distance;
    return glm::vec3(x, 0.0f, z);
}

int findNearestUnscannedPlanet(const glm::vec3& playerPos) {
    float bestDist = FLT_MAX;
    int bestIndex = -1;

    for (int i = 0; i < (int)g_planets.size(); ++i) {
        if (g_planets[i].scanned) continue;

        glm::vec3 pos = getPlanetWorldPosition(g_planets[i]);
        float d = glm::distance(playerPos, pos);

        if (d < bestDist) {
            bestDist = d;
            bestIndex = i;
        }
    }
    return bestIndex;
}

bool isLookingAtTarget(const glm::vec3& targetPos, float aimDegrees) {
    glm::vec3 toTarget = glm::normalize(targetPos - g_camera->Position);
    glm::vec3 forward = glm::normalize(g_camera->Front);

    float cosThreshold = cos(glm::radians(aimDegrees));
    float d = glm::dot(forward, toTarget);
    return d >= cosThreshold;
}

// PROBE SPAWN
static int rollProbeCount() {
    float r = rand01();
    if (r < 0.75f) return 1;
    if (r < 0.95f) return 2;
    return 3;
}

static void spawnProbesForPlanets() {
    g_probes.clear();

    for (int i = 0; i < (int)g_planets.size(); ++i) {
        const Planet& planet = g_planets[i];

        if (rand01() > 0.40f) continue;

        int count = rollProbeCount();

        for (int k = 0; k < count; ++k) {
            ProbeEntity p;
            p.planetIndex = i;

            float base = planet.collisionRadius + 6.0f;
            p.orbitRadius = base + randf(2.0f, 12.0f);
            p.orbitSpeed = randf(0.4f, 1.2f);
            p.orbitAngle = randf(0.0f, glm::two_pi<float>());
            p.yOffset = randf(-2.0f, 2.0f);

            glm::vec3 center = getPlanetWorldPosition(planet);
            p.pos = center + glm::vec3(
                cos(p.orbitAngle) * p.orbitRadius,
                p.yOffset,
                sin(p.orbitAngle) * p.orbitRadius
            );

            g_probes.push_back(p);
        }
    }

    std::cout << "Spawned probes: " << g_probes.size() << std::endl;
}

static void spawnBrokenProbes()
{
    g_brokenProbes.clear();

    int count = 5 + rand() % 12;
    float minDist = 80.0f;
    float maxDist = 400.0f;

    for (int i = 0; i < count; ++i) {
        float angle = randf(0.0f, glm::two_pi<float>());
        float dist = randf(minDist, maxDist);
        float height = randf(-15.0f, 15.0f);

        BrokenProbeInstance bp;
        bp.pos = glm::vec3(
            cos(angle) * dist,
            height,
            sin(angle) * dist
        );
        bp.scale = randf(1.5f, 3.5f);

        g_brokenProbes.push_back(bp);
    }

    std::cout << "Spawned broken probes: " << g_brokenProbes.size() << std::endl;
}


static void updateProbes(float dt) {
    for (auto& p : g_probes) {
        if (p.planetIndex < 0 || p.planetIndex >= (int)g_planets.size()) continue;

        p.orbitAngle += p.orbitSpeed * dt;
        if (p.orbitAngle > glm::two_pi<float>())
            p.orbitAngle -= glm::two_pi<float>();

        glm::vec3 center = getPlanetWorldPosition(g_planets[p.planetIndex]);

        p.pos = center + glm::vec3(
            cos(p.orbitAngle) * p.orbitRadius,
            p.yOffset,
            sin(p.orbitAngle) * p.orbitRadius
        );
    }
}

static void renderProbes() {
    if (!g_probeModel || !g_probeModel->loaded()) return;
    if (g_probes.empty()) return;

    g_shader->Use();

    g_shader->SetFloat("isEmissive", 0.0f);
    g_shader->SetVec3("baseColor", glm::vec3(0.75f, 0.78f, 0.85f));


    g_shader->SetFloat("scanHighlight", 0.0f);
    g_shader->SetFloat("isAsteroid", 0.0f);
    g_shader->SetVec3("noiseOffset", glm::vec3(0));
    g_shader->SetFloat("planetSeed", 0.0f);
    g_shader->SetInt("planetType", 0);
    g_shader->SetFloat("surfaceNoise", 0.0f);

    for (const auto& p : g_probes) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), p.pos);
        model = glm::scale(model, glm::vec3(2.0f));

        g_shader->SetMat4("model", model);
        g_probeModel->draw();
    }

    g_shader->SetFloat("isEmissive", 0.0f);
}

static void renderBrokenProbes()
{
    if (!g_brokenProbeModel || !g_brokenProbeModel->loaded()) return;
    if (g_brokenProbes.empty()) return;

    g_shader->Use();

    g_shader->SetVec3("baseColor", glm::vec3(0.6f, 0.6f, 0.65f));
    g_shader->SetFloat("isEmissive", 0.0f);

    for (const auto& bp : g_brokenProbes) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), bp.pos);
        model = glm::scale(model, glm::vec3(bp.scale));

        g_shader->SetMat4("model", model);
        g_brokenProbeModel->draw();
    }
}

// ERROR HANDLING
void glCheckError(const char* file, int line) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL Error at " << file << ":" << line << " - "
            << "0x" << std::hex << err << std::endl;
    }
}

#define GL_CHECK() glCheckError(__FILE__, __LINE__)

void GLAPIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* userParam) {
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    std::cerr << "GL Debug: " << message << " (type: " << std::hex << type << ")" << std::endl;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    try {
        if (g_firstMouse) {
            g_lastX = (float)xpos;
            g_lastY = (float)ypos;
            g_firstMouse = false;
        }

        float xOffset = (float)xpos - g_lastX;
        float yOffset = g_lastY - (float)ypos;

        g_lastX = (float)xpos;
        g_lastY = (float)ypos;

        g_camera->ProcessMouse(xOffset, yOffset);
    }
    catch (const std::exception& e) {
        std::cerr << "Mouse callback error: " << e.what() << std::endl;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    glViewport(0, 0, w, h);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

// INITIALIZATION
GLFWwindow* initializeWindow() {
    if (!glfwInit()) {
        throw std::runtime_error("GLFW initialization failed");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Window creation failed");
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    return window;
}

void initializeGLEW() {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        throw std::runtime_error(std::string("GLEW init failed: ") +
            (const char*)glewGetErrorString(err));
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glDebugOutput, nullptr);

    GL_CHECK();
}

void initializeOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.02f, 1.0f);

    GL_CHECK();
}

void initializeShaders() {
    try {
        g_shader = std::make_unique<Shader>("vertex.glsl", "fragment.glsl");
        g_starShader = std::make_unique<Shader>("star_vertex.glsl", "star_fragment.glsl");
        g_hudShader = std::make_unique<Shader>("hud_vertex.glsl", "hud_fragment.glsl");
        std::cout << "Shaders loaded successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Shader error: " << e.what() << std::endl;
        throw;
    }
}

void initializeGeometry() {
    try {
        g_sphereMesh = new Mesh();
        generateUVSphere(*g_sphereMesh, 1.0f, 32, 16);

        g_cubeMesh = new Mesh();
        generateCube(*g_cubeMesh, 1.0f);

        std::cout << "Geometry initialized" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Geometry init error: " << e.what() << std::endl;
        throw;
    }
}

void initializeScene() {
    try {
        srand((unsigned)time(0));

        PlanetGenerator::generatePlanets(g_planets, 600.0f);
        PlanetGenerator::generateAsteroids(g_asteroids, 120);
        PlanetGenerator::generateStars(g_stars, 2000);
        PlanetGenerator::generateAsteroidClusters(g_asteroids, 4, 25, 55, 300.0f, 1400.0f);

        for (auto& planet : g_planets) {
            planet.noiseOffset = glm::vec3(
                randf(-1000.0f, 1000.0f),
                randf(-1000.0f, 1000.0f),
                randf(-1000.0f, 1000.0f)
            );
        }

        g_starRenderer = new StarRenderer();
        std::vector<glm::vec3> starPositions;
        for (const auto& star : g_stars) {
            starPositions.push_back(star.pos);
        }
        g_starRenderer->loadStars(starPositions);

        g_hudRenderer = new HUDRenderer();
        g_gameState = std::make_unique<GameState>();
        g_asteroidTexture = std::make_unique<Texture>("assets/asteroid.jpg");

        g_gameState->totalPlanets = (int)g_planets.size();
        g_gameState->scannedPlanets = 0;
        g_gameState->score = 0;

        // Load probe model with Assimp 
        g_probeModel = std::make_unique<ProbeModel>("assets/models/probe/probe.obj");
        g_brokenProbeModel = std::make_unique<ProbeModel>(
            "assets/models/probe/Brokenprobe.obj"
        );

        spawnBrokenProbes();
        spawnProbesForPlanets();

        std::cout << "Scene generated: "
            << g_planets.size() << " planets, "
            << g_asteroids.size() << " asteroids, "
            << g_stars.size() << " stars\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Scene init error: " << e.what() << std::endl;
        throw;
    }
}

// RENDERING
void renderStars(const glm::mat4& view, const glm::mat4& projection) {
    g_starShader->Use();

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 starView = glm::mat4(glm::mat3(view));

    g_starShader->SetMat4("model", model);
    g_starShader->SetMat4("view", starView);
    g_starShader->SetMat4("projection", projection);
    g_starShader->SetVec3("baseColor", glm::vec3(1.0f, 1.0f, 1.0f));

    g_starRenderer->render();
}

void renderSun() {
    if (!g_sphereMesh) return;

    g_shader->Use();
    g_shader->SetFloat("surfaceNoise", 0.0f);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), g_sun.pos);
    model = glm::scale(model, glm::vec3(g_sun.radius));

    g_shader->SetMat4("model", model);
    g_shader->SetVec3("baseColor", glm::vec3(1.0f, 0.9f, 0.6f));
    g_shader->SetFloat("isEmissive", 1.0f);

    g_sphereMesh->Draw();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glm::mat4 glowModel = glm::translate(glm::mat4(1.0f), g_sun.pos);
    glowModel = glm::scale(glowModel, glm::vec3(g_sun.radius * 1.6f));

    g_shader->SetMat4("model", glowModel);
    g_shader->SetVec3("baseColor", glm::vec3(1.0f, 0.7f, 0.2f));
    g_shader->SetFloat("isEmissive", 1.0f);

    g_sphereMesh->Draw();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void renderPlanets(float deltaTime) {
    g_shader->Use();
    g_shader->SetFloat("scanHighlight", 0.0f);

    for (int i = 0; i < (int)g_planets.size(); ++i) {
        Planet& planet = g_planets[i];

        planet.angle += planet.speed * deltaTime;
        if (planet.angle > glm::two_pi<float>())
            planet.angle -= glm::two_pi<float>();

        float x = g_sun.pos.x + cos(planet.angle) * planet.distance;
        float z = g_sun.pos.z + sin(planet.angle) * planet.distance;
        glm::vec3 planetPos = glm::vec3(x, 0.0f, z);

        planet.rotationAngle += planet.rotationSpeed * deltaTime;
        if (planet.rotationAngle > 360.0f)
            planet.rotationAngle -= 360.0f;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), planetPos);
        model = glm::rotate(model, glm::radians(planet.rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(planet.size));

        g_shader->Use();
        g_shader->SetMat4("model", model);

        g_shader->SetVec3("noiseOffset", planet.noiseOffset);
        g_shader->SetFloat("planetSeed", (float)planet.seed);
        g_shader->SetInt("planetType", planet.biomeType);

        int slice = planet.seed % (int)planet.surfaceVariation.size();
        float variation = planet.surfaceVariation[slice];

        g_shader->SetFloat("surfaceNoise", variation);

        glm::vec3 surfaceColor = PlanetGenerator::getPlanetSurfaceColor(planet, variation);
        g_shader->SetVec3("baseColor", surfaceColor);

        g_shader->SetFloat("isEmissive", 0.0f);

        float highlight = 0.0f;

        if (g_gameState && g_gameState->currentTarget == i && !planet.scanned) {
            float distance = glm::distance(g_camera->Position, planetPos);
            float scanRange = planet.collisionRadius + 12.0f;
            bool aimed = isLookingAtTarget(planetPos, 6.0f);

            if (aimed && distance < scanRange) {
                highlight = g_gameState->isScanning ? 0.25f : 0.15f;
            }
        }

        g_shader->SetFloat("scanHighlight", highlight);

        g_sphereMesh->Draw();
    }

    g_shader->Use();
    g_shader->SetFloat("scanHighlight", 0.0f);
}

void renderMoons(Planet& planet, const glm::vec3& planetPos, float deltaTime) {
    g_shader->Use();

    g_shader->SetInt("diffuseMap", 0);
    g_shader->SetFloat("isAsteroid", 1.0f);
    g_shader->SetFloat("scanHighlight", 0.0f);
    g_shader->SetFloat("surfaceNoise", 0.0f);

    g_asteroidTexture->Bind(0);

    for (int i = 0; i < (int)planet.moons.size(); ++i) {
        Moon& moon = planet.moons[i];
        moon.angle += moon.speed * deltaTime;

        float mx = cos(moon.angle) * moon.distance;
        float mz = sin(moon.angle) * moon.distance;

        glm::vec3 moonWorldPos = planetPos + glm::vec3(mx, 0.0f, mz);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), moonWorldPos);
        model = glm::scale(model, glm::vec3(moon.size));

        g_shader->SetMat4("model", model);
        g_shader->SetVec3("baseColor", glm::vec3(1.0f));
        g_shader->SetFloat("isEmissive", 0.0f);

        g_sphereMesh->Draw();
    }
    g_shader->SetFloat("isAsteroid", 0.0f);
}

void renderAsteroids(float currentTime, float deltaTime) {
    g_shader->Use();
    g_shader->SetInt("diffuseMap", 0);
    g_shader->SetFloat("isAsteroid", 1.0f);
    g_shader->SetFloat("scanHighlight", 0.0f);
    g_asteroidTexture->Bind(0);

    for (int i = 0; i < (int)g_asteroids.size(); ++i) {
        Asteroid& asteroid = g_asteroids[i];

        if (asteroid.clustered) {
            asteroid.localAngle += asteroid.localSpeed * deltaTime;
            if (asteroid.localAngle > glm::two_pi<float>())
                asteroid.localAngle -= glm::two_pi<float>();

            asteroid.pos = asteroid.clusterCenter + glm::vec3(
                cos(asteroid.localAngle) * asteroid.localRadius,
                asteroid.orbitHeight,
                sin(asteroid.localAngle) * asteroid.localRadius
            );
        }
        else {
            asteroid.orbitAngle += asteroid.orbitSpeed * deltaTime;
            if (asteroid.orbitAngle > glm::two_pi<float>()) {
                asteroid.orbitAngle -= glm::two_pi<float>();
            }
            float x = cos(asteroid.orbitAngle) * asteroid.orbitRadius;
            float z = sin(asteroid.orbitAngle) * asteroid.orbitRadius;
            float y = asteroid.orbitHeight;

            asteroid.pos = glm::vec3(x, y, z);
        }


        glm::mat4 model = glm::translate(glm::mat4(1.0f), asteroid.pos);
        model = glm::rotate(model, glm::radians(asteroid.rot.x + currentTime * 10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(asteroid.rot.y + currentTime * 15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(asteroid.scale));

        g_shader->SetMat4("model", model);
        g_shader->SetVec3("baseColor", glm::vec3(1.0f));
        g_shader->SetFloat("isEmissive", 0.0f);

        g_cubeMesh->Draw();
    }

    g_shader->Use();
    g_shader->SetFloat("isAsteroid", 0.0f);
}

void buildHUD(float deltaTime) {
    g_radarAngle += 2.0f * 3.1415926f * deltaTime * 0.1f;
    if (g_radarAngle > 2.0f * 3.1415926f) g_radarAngle -= 2.0f * 3.1415926f;
    g_pulseTime += deltaTime;

    g_hudRenderer->clear();

    const float PI2 = 2.0f * 3.1415926f;
    float cameraYaw = glm::radians(g_camera->Yaw + 90.0f);

    // Speedometer Bottom Right UI
    float cx = 1150, cy = 100;
    float radius = 70.f;
    float speedRatio = glm::clamp(glm::length(g_camera->Velocity) / g_camera->BoostSpeed, 0.0f, 1.0f);

    g_hudRenderer->addCircle(glm::vec2(cx, cy), radius + 3, glm::vec3(0.0f, 1.0f, 0.8f), 64);
    g_hudRenderer->addCircle(glm::vec2(cx, cy), radius, glm::vec3(0.0f, 0.7f, 1.0f), 64);

    float speedColor = speedRatio < 0.7f ? (1.0f - speedRatio * 0.5f) : 1.0f;
    for (int i = 0; i < int(speedRatio * 32); ++i) {
        float angle1 = PI2 * i / 32;
        float angle2 = PI2 * (i + 1) / 32;
        glm::vec2 p1 = glm::vec2(cx, cy) + glm::vec2(cos(angle1) * radius, sin(angle1) * radius);
        glm::vec2 p2 = glm::vec2(cx, cy) + glm::vec2(cos(angle2) * radius, sin(angle2) * radius);
        g_hudRenderer->addLine(p1, p2, glm::vec3(0.0f, speedColor, 0.3f));
    }

    g_hudRenderer->addLine(glm::vec2(cx - 8, cy), glm::vec2(cx + 8, cy), glm::vec3(0.0f, 1.0f, 0.8f));
    g_hudRenderer->addLine(glm::vec2(cx, cy - 8), glm::vec2(cx, cy + 8), glm::vec3(0.0f, 1.0f, 0.8f));

    // Radar Bottom Left UI
    float rx = 80, ry = 100;
    float radarRadius = 70.f;

    g_hudRenderer->addCircle(glm::vec2(rx, ry), radarRadius, glm::vec3(0.0f, 0.6f, 1.0f), 64);
    g_hudRenderer->addCircle(glm::vec2(rx, ry), radarRadius * 0.75f, glm::vec3(0.0f, 0.3f, 0.6f), 32);
    g_hudRenderer->addCircle(glm::vec2(rx, ry), radarRadius * 0.5f, glm::vec3(0.0f, 0.3f, 0.6f), 32);
    g_hudRenderer->addCircle(glm::vec2(rx, ry), radarRadius * 0.25f, glm::vec3(0.0f, 0.3f, 0.6f), 32);
    g_hudRenderer->addLine(glm::vec2(rx, ry + radarRadius + 5), glm::vec2(rx, ry + radarRadius + 15), glm::vec3(0.0f, 0.5f, 1.0f));

    for (int trail = 3; trail > 0; --trail) {
        float trailAngle = g_radarAngle - (trail * 0.15f);
        glm::vec2 sweepEnd = glm::vec2(rx, ry) + glm::vec2(sin(trailAngle) * radarRadius, cos(trailAngle) * radarRadius);
        glm::vec3 trailColor = glm::vec3(0.2f, 1.0f, 0.4f) * (1.0f - (trail / 3.0f)) * 0.6f;
        g_hudRenderer->addLine(glm::vec2(rx, ry), sweepEnd, trailColor);
    }

    glm::vec2 mainSweepEnd = glm::vec2(rx, ry) + glm::vec2(sin(g_radarAngle) * radarRadius, cos(g_radarAngle) * radarRadius);
    g_hudRenderer->addLine(glm::vec2(rx, ry), mainSweepEnd, glm::vec3(0.0f, 1.0f, 0.5f));

    float radarDetectionRange = 150.0f;
    for (const auto& a : g_asteroids) {
        glm::vec3 offset = a.pos - g_camera->Position;
        float distance = glm::length(offset);

        if (distance > radarDetectionRange) continue;

        float asteroidAngle = atan2f(offset.x, offset.z) - cameraYaw;
        float asteroidDistance = glm::length(glm::vec2(offset.x, offset.z));
        float radarScale = asteroidDistance / 80.0f;
        float radarX = sin(asteroidAngle) * radarScale * radarRadius;
        float radarY = cos(asteroidAngle) * radarScale * radarRadius;

        radarX = glm::clamp(radarX, -radarRadius, radarRadius);
        radarY = glm::clamp(radarY, -radarRadius, radarRadius);

        float angleToAst = atan2f(radarX, radarY);
        float angleDiff = fabs(angleToAst - g_radarAngle);
        if (angleDiff > 3.1415926f) angleDiff = 2.0f * 3.1415926f - angleDiff;

        bool isHighlighted = angleDiff < 0.15f;
        float blinkAlpha = isHighlighted ? (0.6f + 0.4f * sinf(g_pulseTime * 10.0f)) : 0.8f;
        float asteroidRadius = 3.0f;
        glm::vec2 asteroidPos = glm::vec2(rx + radarX, ry + radarY);
        glm::vec3 asteroidColor = glm::vec3(1.0f, isHighlighted ? 1.0f : 0.6f, 0.0f) * blinkAlpha;
        g_hudRenderer->addCircle(asteroidPos, asteroidRadius, asteroidColor, 16);
    }

    // Center Crosshair
    float centerX = 640.0f, centerY = 360.0f;
    g_hudRenderer->addLine(glm::vec2(centerX - 10, centerY), glm::vec2(centerX + 10, centerY), glm::vec3(0.0f, 1.0f, 1.0f));
    g_hudRenderer->addLine(glm::vec2(centerX, centerY - 10), glm::vec2(centerX, centerY + 10), glm::vec3(0.0f, 1.0f, 1.0f));


    // Show planets name and class only when highlighted or scanning
    if (g_gameState && g_gameState->currentTarget != -1) {
        Planet& target = g_planets[g_gameState->currentTarget];

        if (!target.scanned) {
            glm::vec3 targetPos = getPlanetWorldPosition(target);
            float distance = glm::distance(g_camera->Position, targetPos);
            float scanRange = target.collisionRadius + 50.0f;

            bool aimed = isLookingAtTarget(targetPos, 6.0f);
            bool inRange = distance < scanRange;
            bool showInfo = (aimed && inRange) || g_gameState->isScanning;

            if (showInfo) {
                glm::vec2 namePos(
                    WINDOW_WIDTH * 0.5f - 140.0f,
                    WINDOW_HEIGHT - 60.0f
                );
                glm::vec3 textCol(0.8f, 0.95f, 1.0f);
                g_hudRenderer->addText(namePos, 16.0f, textCol, target.name);
                std::string cls = std::string("CLASS: ") + biomeLabel(target.biomeType);
                g_hudRenderer->addText(glm::vec2(namePos.x, namePos.y - 22.0f), 14.0f, textCol, cls);

                if (g_gameState->scanJammed) {
                    g_hudRenderer->addText(glm::vec2(namePos.x, namePos.y - 44.0f), 14.0f, glm::vec3(1.0f, 0.2f, 0.2f), "JAMMED");
                }
            }
        }
    }

    if (g_gameState && g_gameState->scanJammed) {
        float pulse = 0.35f + 0.65f * (0.5f + 0.5f * sinf(g_pulseTime * 8.0f));
        g_hudRenderer->addCircle(glm::vec2(centerX, centerY), 28.0f, glm::vec3(1.0f, 0.1f, 0.1f) * pulse, 48);
    }

    // SCANNED PLANETS Top Left UI
    int total = g_gameState ? g_gameState->totalPlanets : (int)g_planets.size();
    int scanned = g_gameState ? g_gameState->scannedPlanets : 0;

    float dotsX = 40.0f;
    float dotsY = 680.0f;
    float dotR = 6.0f;
    float gap = 18.0f;

    for (int i = 0; i < total; ++i) {
        glm::vec3 c = (i < scanned) ? glm::vec3(0.0f, 1.0f, 0.6f) : glm::vec3(0.15f, 0.25f, 0.35f);
        g_hudRenderer->addCircle(glm::vec2(dotsX + i * gap, dotsY), dotR, c, 20);
    }

// SCAN PROGRESS BAR
    if (g_gameState && g_gameState->currentTarget != -1 &&
        !g_planets[g_gameState->currentTarget].scanned)
    {
        Planet& target = g_planets[g_gameState->currentTarget];
        glm::vec3 targetPos = getPlanetWorldPosition(target);

        float distance = glm::distance(g_camera->Position, targetPos);
        float scanRange = target.collisionRadius + 30.0f;
        bool aimed = isLookingAtTarget(targetPos, 6.0f);

        if ((aimed && distance < scanRange) || g_gameState->isScanning)
        {
            float barW = 360.0f;
            float barH = 10.0f;
            float barX = (WINDOW_WIDTH * 0.5f) - (barW * 0.5f);
            float barY = 55.0f;

            float progress = g_gameState->scanProgress;

            // Background track
            g_hudRenderer->addLine(
                glm::vec2(barX, barY),
                glm::vec2(barX + barW, barY),
                glm::vec3(0.08f, 0.15f, 0.2f)
            );

            // Active scanning glow
            float pulse = g_gameState->isScanning
                ? (0.7f + 0.3f * sinf(g_pulseTime * 8.0f))
                : 0.6f;

            glm::vec3 scanColor = g_gameState->isScanning
                ? glm::vec3(0.0f, 1.0f, 0.85f) * pulse
                : glm::vec3(0.0f, 0.6f, 0.9f);

            g_hudRenderer->addLine(
                glm::vec2(barX, barY),
                glm::vec2(barX + barW * progress, barY),
                scanColor
            );

            if (progress > 0.01f) {
                float capX = barX + barW * progress;
                g_hudRenderer->addLine(
                    glm::vec2(capX, barY - 3),
                    glm::vec2(capX, barY + 3),
                    scanColor
                );
            }
        }
    }


    // GAMEE COMPLETE END SCREEN
    if (g_gameState && g_gameState->surveyComplete) {
        float cx = WINDOW_WIDTH * 0.5f;
        float cy = WINDOW_HEIGHT * 0.55f;

        float pulse = 0.6f + 0.4f * (0.5f + 0.5f * sinf(g_pulseTime * 2.0f));
        glm::vec3 frameCol = glm::vec3(0.0f, 0.9f, 0.7f) * pulse;
        glm::vec3 dimCol = glm::vec3(0.08f, 0.16f, 0.22f);

        // outer + inner rings
        g_hudRenderer->addCircle(glm::vec2(cx, cy), 140.0f, frameCol, 96);
        g_hudRenderer->addCircle(glm::vec2(cx, cy), 110.0f, dimCol, 96);
        g_hudRenderer->addCircle(glm::vec2(cx, cy), 90.0f, frameCol * 0.8f, 96);

        // cross lines
        g_hudRenderer->addLine(glm::vec2(cx - 160, cy), glm::vec2(cx + 160, cy), dimCol);
        g_hudRenderer->addLine(glm::vec2(cx, cy - 120), glm::vec2(cx, cy + 120), dimCol);

        // Title
        glm::vec3 titleCol(0.85f, 1.0f, 1.0f);
        g_hudRenderer->addText(glm::vec2(cx - 170.0f, cy + 50.0f), 20.0f, titleCol, "SURVEY COMPLETE");

        // Stats
        std::string scoreLine = "SCORE: " + std::to_string(g_gameState->score);
        std::string planetsLine = "PLANETS: " + std::to_string(g_gameState->scannedPlanets) +
            "/" + std::to_string(g_gameState->totalPlanets);

        g_hudRenderer->addText(glm::vec2(cx - 120.0f, cy + 10.0f), 16.0f, titleCol, scoreLine);
        g_hudRenderer->addText(glm::vec2(cx - 120.0f, cy - 15.0f), 16.0f, titleCol, planetsLine);

        float blink = (sinf(g_pulseTime * 4.0f) > 0.0f) ? 1.0f : 0.35f;
        glm::vec3 promptCol = glm::vec3(0.0f, 1.0f, 0.85f) * blink;
        g_hudRenderer->addText(glm::vec2(cx - 150.0f, cy - 70.0f), 14.0f, promptCol, "PRESS R TO RESTART");
        g_hudRenderer->addText(glm::vec2(cx - 120.0f, cy - 92.0f), 12.0f, dimCol * 1.3f, "ESC TO QUIT");
    }
    g_hudRenderer->finalize();
    }


void renderHUD() {
    glDisable(GL_DEPTH_TEST);

    glm::mat4 projection = glm::ortho(0.0f, (float)WINDOW_WIDTH, 0.0f, (float)WINDOW_HEIGHT, -1.0f, 1.0f);

    g_hudShader->Use();
    g_hudShader->SetMat4("projection", projection);
    g_hudRenderer->render();

    glEnable(GL_DEPTH_TEST);
}

void render(float deltaTime, const glm::mat4& view, const glm::mat4& projection) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderStars(view, projection);

    g_shader->Use();
    g_shader->SetMat4("view", view);
    g_shader->SetMat4("projection", projection);
    g_shader->SetVec3("lightPos", g_sun.pos);
    g_shader->SetVec3("viewPos", g_camera->Position);

    renderSun();
    renderPlanets(deltaTime);

    for (auto& planet : g_planets) {
        glm::vec3 planetPos = getPlanetWorldPosition(planet);
        renderMoons(planet, planetPos, deltaTime);
    }

    renderAsteroids((float)glfwGetTime(), deltaTime);

    // Draw probes after everything else
    renderProbes();
    renderBrokenProbes();

    buildHUD(deltaTime);
    renderHUD();

    GL_CHECK();
}

void updateMoons(float deltaTime) {
    for (auto& planet : g_planets) {
        for (auto& moon : planet.moons) {
            moon.angle += moon.speed * deltaTime;
        }
    }
}

// Main Loop
int main() {
    try {
        std::cout << "=== Initializing Space Explorer ===" << std::endl;

        GLFWwindow* window = initializeWindow();
        std::cout << "Window created" << std::endl;

        initializeGLEW();
        std::cout << "GLEW initialized" << std::endl;

        initializeOpenGL();
        std::cout << "OpenGL context ready" << std::endl;

        g_camera = std::make_unique<Camera>(glm::vec3(0.0f, 30.0f, 100.0f));

        initializeShaders();
        initializeGeometry();
        initializeScene();

        std::cout << "=== Initialization complete. Starting main loop ===" << std::endl;

        float lastTime = (float)glfwGetTime();
        int lastTarget = -1;

        while (!glfwWindowShouldClose(window))
        {
            float currentTime = (float)glfwGetTime();
            float deltaTime = currentTime - lastTime;
            lastTime = currentTime;

            glm::vec3 oldPos = g_camera->Position;
            g_camera->ProcessKeyboard(window, deltaTime);

            // Update probes orbiting planets
            updateProbes(deltaTime);

            // Scan
            g_gameState->currentTarget = findNearestUnscannedPlanet(g_camera->Position);

            if (g_gameState->currentTarget != lastTarget) {
                g_gameState->resetScan();
                lastTarget = g_gameState->currentTarget;
            }

            g_gameState->scanJammed = false;

            if (g_gameState->currentTarget != -1) {
                Planet& target = g_planets[g_gameState->currentTarget];
                glm::vec3 planetPos = getPlanetWorldPosition(target);

                float distance = glm::distance(g_camera->Position, planetPos);

                float scanRange = target.collisionRadius + 12.0f;

                bool aimed = isLookingAtTarget(planetPos, 6.0f);
                bool inRange = distance < scanRange;

                // Jamming if any probe is near the target planet scnaing is blocked
                bool jammed = false;
                for (const auto& pr : g_probes) {
                    float d = glm::distance(pr.pos, planetPos);
                    if (d < 18.0f) {
                        jammed = true;
                        break;
                    }
                }
                g_gameState->scanJammed = jammed;

                // Hold E to scan
                if (!jammed && aimed && inRange && glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                    g_gameState->isScanning = true;
                }
                else {
                    g_gameState->isScanning = false;
                }

                g_gameState->updateScan(deltaTime);

                if (g_gameState->scanProgress >= 1.0f && !target.scanned) {
                    target.scanned = true;
                    g_gameState->scannedPlanets++;
                    g_gameState->score += 100;
                    g_gameState->resetScan();
                }
            }

            if (g_gameState->scannedPlanets == g_gameState->totalPlanets) {
                g_gameState->surveyComplete = true;
            }

            // Admin Debug: instantly complete game (press K)
            if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
                for (auto& p : g_planets) p.scanned = true;
                g_gameState->scannedPlanets = g_gameState->totalPlanets;
                g_gameState->surveyComplete = true;
                g_gameState->resetScan();
            }

            // Restart (press R when complete)
            if (g_gameState && g_gameState->surveyComplete && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                g_gameState->surveyComplete = false;
                g_gameState->score = 0;
                g_gameState->scannedPlanets = 0;
                g_gameState->resetScan();

                for (auto& p : g_planets) p.scanned = false;

                spawnProbesForPlanets();
            }

            float playerRadius = 2.0f;
            if (checkSphereCollision(g_camera->Position, playerRadius, g_sun.pos, g_sun.radius)) {
                g_camera->Position = oldPos;
            }

            for (const auto& planet : g_planets) {
                glm::vec3 planetPos = getPlanetWorldPosition(planet);
                if (checkSphereCollision(g_camera->Position, playerRadius, planetPos, planet.collisionRadius)) {
                    g_camera->Position = oldPos;
                    break;
                }
            }

            for (const auto& asteroid : g_asteroids) {
                if (checkSphereCollision(g_camera->Position, playerRadius, asteroid.pos, asteroid.collisionRadius)) {
                    g_camera->Position = oldPos;
                    break;
                }
            }

            updateMoons(deltaTime);

            glm::mat4 projection = glm::perspective(
                glm::radians(60.0f),
                (float)WINDOW_WIDTH / WINDOW_HEIGHT,
                0.1f,
                50000.0f
            );

            glm::mat4 view = g_camera->GetViewMatrix();

            render(deltaTime, view, projection);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        delete g_sphereMesh;
        delete g_cubeMesh;
        delete g_starRenderer;
        delete g_hudRenderer;

        g_gameState.reset();

        glfwDestroyWindow(window);
        glfwTerminate();

        std::cout << "=== Space Explorer closed successfully ===" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return -1;
    }
}
