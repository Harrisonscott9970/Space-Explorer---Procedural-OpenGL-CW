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

// Project headers
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "PlanetGenerator.h"
#include "StarRenderer.h"
#include "HUDRenderer.h"
#include "GameState.h"
#include "Texture.h"

// Assimp model wrapper for the probe models
#include "ProbeModel.h"

float randf(float min, float max) {
    return min + static_cast<float>(rand()) /
        static_cast<float>(RAND_MAX) * (max - min);
}

static float rand01() {
    return (float)rand() / (float)RAND_MAX;
}

// Simple label for biome types (used for HUD text)
static const char* biomeLabel(int t) {
    if (t == 0) return "GREEN";
    if (t == 1) return "ROCKY";
    if (t == 2) return "ICE";
    return "UNKNOWN";
}

// Forward declaration for render() (definition is later)
void render(float deltaTime, const glm::mat4& view, const glm::mat4& projection);

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const char* WINDOW_TITLE = "Space Explorer - Procedural Generation";

// ---------------------------
// Global state
// ---------------------------

// Camera + mouse tracking
std::unique_ptr<Camera> g_camera;
float g_lastX = WINDOW_WIDTH / 2.0f;
float g_lastY = WINDOW_HEIGHT / 2.0f;
bool g_firstMouse = true;

// Shaders
std::unique_ptr<Shader> g_shader;      // main shader for planets/asteroids/probes
std::unique_ptr<Shader> g_starShader;  // special shader for background stars
std::unique_ptr<Shader> g_hudShader;   // 2D HUD shader

// Procedural objects
std::vector<Planet> g_planets;
std::vector<Asteroid> g_asteroids;
std::vector<Star> g_stars;

// Textyre for asteroids/moons
std::unique_ptr<Texture> g_asteroidTexture;
std::unique_ptr<Texture> g_moonTexture;


// Main star in the scene
Sun g_sun{ glm::vec3(0.f), 25.f };

// Basic meshes (generated at runtime)
Mesh* g_sphereMesh = nullptr;
Mesh* g_cubeMesh = nullptr;

// Render helpers
StarRenderer* g_starRenderer = nullptr;
HUDRenderer* g_hudRenderer = nullptr;

// ---------------------------
// Gameplay state (scanning / score / completion)
// ---------------------------
std::unique_ptr<GameState> g_gameState;

// HUD animation values
static float g_radarAngle = 0.0f;
static float g_pulseTime = 0.0f;

// Assimp probe models (normal probe and broken probe)
std::unique_ptr<ProbeModel> g_probeModel;
std::unique_ptr<ProbeModel> g_brokenProbeModel;

// A single “broken probe” placed in space
struct BrokenProbeInstance {
    glm::vec3 pos;
    float scale;
};

// Many broken probes scattered in the world
std::vector<BrokenProbeInstance> g_brokenProbes;

// Probe that orbits a specific planet (used for scan jamming)
struct ProbeEntity {
    int planetIndex = -1;        // which planet this probe belongs to
    float orbitRadius = 0.0f;    // distance from planet centre
    float orbitSpeed = 0.0f;     // radians per second
    float orbitAngle = 0.0f;     // current orbit angle
    float yOffset = 0.0f;        // small vertical offset to avoid all probes being flat
    glm::vec3 pos = glm::vec3(0.0f);
};

std::vector<ProbeEntity> g_probes;

// ---------------------------
// Collision helpers
// ---------------------------

// Basic sphere-sphere collision test
bool checkSphereCollision(
    const glm::vec3& aPos, float aRadius,
    const glm::vec3& bPos, float bRadius)
{
    return glm::length(aPos - bPos) < (aRadius + bRadius);
}

// Convert planet orbit parameters into a world position (in XZ plane)
glm::vec3 getPlanetWorldPosition(const Planet& planet) {
    float x = g_sun.pos.x + cos(planet.angle) * planet.distance;
    float z = g_sun.pos.z + sin(planet.angle) * planet.distance;
    return glm::vec3(x, 0.0f, z);
}

// Finds the closest planet that has not been scanned yet
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

// Checks if the cameara is aiming within aimDegrees of the target
bool isLookingAtTarget(const glm::vec3& targetPos, float aimDegrees) {
    glm::vec3 toTarget = glm::normalize(targetPos - g_camera->Position);
    glm::vec3 forward = glm::normalize(g_camera->Front);

    float cosThreshold = cos(glm::radians(aimDegrees));
    float d = glm::dot(forward, toTarget);
    return d >= cosThreshold;
}

// ---------------------------
// Probe spawning logic
// ---------------------------

// Slightly weighted random choice: most planets get 1 probe if they get any
static int rollProbeCount() {
    float r = rand01();
    if (r < 0.75f) return 1;
    if (r < 0.95f) return 2;
    return 3;
}

// Spawn orbiting probes around some planets (these can jam scanning)
static void spawnProbesForPlanets() {
    g_probes.clear();

    for (int i = 0; i < (int)g_planets.size(); ++i) {
        const Planet& planet = g_planets[i];

        // Only some planets get probes to keep it varied
        if (rand01() > 0.40f) continue;

        int count = rollProbeCount();

        for (int k = 0; k < count; ++k) {
            ProbeEntity p;
            p.planetIndex = i;

            // Orbit a bit outside the planet collision radius so it doesn't clip
            float base = planet.collisionRadius + 6.0f;
            p.orbitRadius = base + randf(2.0f, 12.0f);
            p.orbitSpeed = randf(0.4f, 1.2f);
            p.orbitAngle = randf(0.0f, glm::two_pi<float>());
            p.yOffset = randf(-2.0f, 2.0f);

            // Convert orbit values into an initial position
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

// Scatter “broken probes” in the scene (static decoration)
static void spawnBrokenProbes()
{
    g_brokenProbes.clear();

    // Random amount each run
    int count = 5 + rand() % 12;

    // Keep them in a band around the origin so player can find them
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

// Update orbiting probes each frame (simple circular motion)
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

// Render orbiting probes (reuses the main shader but sets neutral values)
static void renderProbes() {
    if (!g_probeModel || !g_probeModel->loaded()) return;
    if (g_probes.empty()) return;

    g_shader->Use();

    // Treat probes as “plain” mesh with a fixed colour (not planet noise)
    g_shader->SetFloat("isEmissive", 0.0f);
    g_shader->SetVec3("baseColor", glm::vec3(0.75f, 0.78f, 0.85f));

    // Reset planet-related uniforms so the probe doesn't get planet shading logic
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

    // Just to be tidy, keep emissive off afterwards
    g_shader->SetFloat("isEmissive", 0.0f);
}

// Render broken probes (static objects)
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

// ---------------------------
// OpenGL error handling / debug
// ---------------------------

// Polls glGetError and prints any errors found
void glCheckError(const char* file, int line) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL Error at " << file << ":" << line << " - "
            << "0x" << std::hex << err << std::endl;
    }
}

#define GL_CHECK() glCheckError(__FILE__, __LINE__)

// OpenGL debug callback (prints driver messages)
void GLAPIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* userParam) {

    // Ignore noisy/non-important IDs
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::cerr << "GL Debug: " << message << " (type: " << std::hex << type << ")" << std::endl;
}

// ---------------------------
// GLFW input callbacks
// ---------------------------

// Mouse move => update camera look direction
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

// Window resize => update viewport so rendering scales correctly
void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    glViewport(0, 0, w, h);
}

// Basic key handler (escape to quit)
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

// ---------------------------
// Initialization functions
// ---------------------------

// Creates the window + sets up GLFW callbacks
GLFWwindow* initializeWindow() {
    if (!glfwInit()) {
        throw std::runtime_error("GLFW initialization failed");
    }

    // Request OpenGL 4.1 core (mac friendly too)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Ask for debug context so we can get driver messages
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Window creation failed");
    }

    glfwMakeContextCurrent(window);

    // VSYNC on (limits FPS to monitor refresh, helps stability)
    glfwSwapInterval(1);

    // Input setup
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // Lock/hide cursor for FPS-style camera movement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    return window;
}

// Loads OpenGL function pointers with GLEW and enables debug output
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

// Basic OpenGL state setup
void initializeOpenGL() {
    glEnable(GL_DEPTH_TEST);

    // Alpha blending for glow/HUD style effects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Dark space background
    glClearColor(0.0f, 0.0f, 0.02f, 1.0f);

    GL_CHECK();
}

// Load the shaders from disk
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

// Create basic meshes procedurally (sphere for planets, cube for asteroids)
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

// Generates all procedural content and loads models/textures
void initializeScene() {
    try {
        // Seed randomness so each run is different
        srand((unsigned)time(0));

        // Procedural generation (planets, asteroids, stars, clusters)
        PlanetGenerator::generatePlanets(g_planets, 600.0f);
        PlanetGenerator::generateAsteroids(g_asteroids, 120);
        PlanetGenerator::generateStars(g_stars, 2000);
        PlanetGenerator::generateAsteroidClusters(g_asteroids, 4, 25, 55, 300.0f, 1400.0f);

        // Give each planet a random noise offset so surfaces look different
        for (auto& planet : g_planets) {
            planet.noiseOffset = glm::vec3(
                randf(-1000.0f, 1000.0f),
                randf(-1000.0f, 1000.0f),
                randf(-1000.0f, 1000.0f)
            );
        }

        // Star renderer uses positions only (cheap GPU instancing style draw)
        g_starRenderer = new StarRenderer();
        std::vector<glm::vec3> starPositions;
        for (const auto& star : g_stars) {
            starPositions.push_back(star.pos);
        }
        g_starRenderer->loadStars(starPositions);

        // HUD + gameplay state
        g_hudRenderer = new HUDRenderer();
        g_gameState = std::make_unique<GameState>();

        // Shared asteroid texture
        g_asteroidTexture = std::make_unique<Texture>("assets/asteroid.jpg");
        g_moonTexture = std::make_unique<Texture>("assets/moon.png");

        // Initialize scoring counts
        g_gameState->totalPlanets = (int)g_planets.size();
        g_gameState->scannedPlanets = 0;
        g_gameState->score = 0;

        // Load probe models with Assimp
        g_probeModel = std::make_unique<ProbeModel>("assets/models/probe/probe.obj");
        g_brokenProbeModel = std::make_unique<ProbeModel>("assets/models/probe/Brokenprobe.obj");

        // Spawn decorative + gameplay objects
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

// ---------------------------
// Rendering functions
// ---------------------------

// Draw starfield in the background
void renderStars(const glm::mat4& view, const glm::mat4& projection) {
    g_starShader->Use();

    glm::mat4 model = glm::mat4(1.0f);

    // Remove translation so stars feel “infinitely far”
    glm::mat4 starView = glm::mat4(glm::mat3(view));

    g_starShader->SetMat4("model", model);
    g_starShader->SetMat4("view", starView);
    g_starShader->SetMat4("projection", projection);
    g_starShader->SetVec3("baseColor", glm::vec3(1.0f, 1.0f, 1.0f));

    g_starRenderer->render();
}

// Draw sun + simple glow by blending a bigger sphere
void renderSun() {
    if (!g_sphereMesh) return;

    g_shader->Use();
    g_shader->SetFloat("surfaceNoise", 0.0f);

    // Core sphere
    glm::mat4 model = glm::translate(glm::mat4(1.0f), g_sun.pos);
    model = glm::scale(model, glm::vec3(g_sun.radius));

    g_shader->SetMat4("model", model);
    g_shader->SetVec3("baseColor", glm::vec3(1.0f, 0.9f, 0.6f));
    g_shader->SetFloat("isEmissive", 1.0f);

    g_sphereMesh->Draw();

    // Glow pass using additive blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glm::mat4 glowModel = glm::translate(glm::mat4(1.0f), g_sun.pos);
    glowModel = glm::scale(glowModel, glm::vec3(g_sun.radius * 1.6f));

    g_shader->SetMat4("model", glowModel);
    g_shader->SetVec3("baseColor", glm::vec3(1.0f, 0.7f, 0.2f));
    g_shader->SetFloat("isEmissive", 1.0f);

    g_sphereMesh->Draw();

    // Restore default blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Draw planets, update their orbit + rotation, and apply scan highlight if targeted
void renderPlanets(float deltaTime) {
    g_shader->Use();
    g_shader->SetFloat("scanHighlight", 0.0f);

    for (int i = 0; i < (int)g_planets.size(); ++i) {
        Planet& planet = g_planets[i];

        // Orbit around the sun
        planet.angle += planet.speed * deltaTime;
        if (planet.angle > glm::two_pi<float>())
            planet.angle -= glm::two_pi<float>();

        float x = g_sun.pos.x + cos(planet.angle) * planet.distance;
        float z = g_sun.pos.z + sin(planet.angle) * planet.distance;
        glm::vec3 planetPos = glm::vec3(x, 0.0f, z);

        // Spin planet around its axis
        planet.rotationAngle += planet.rotationSpeed * deltaTime;
        if (planet.rotationAngle > 360.0f)
            planet.rotationAngle -= 360.0f;

        // Model transform: translate -> rotate -> scale
        glm::mat4 model = glm::translate(glm::mat4(1.0f), planetPos);
        model = glm::rotate(model, glm::radians(planet.rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(planet.size));

        g_shader->Use();
        g_shader->SetMat4("model", model);

        // Planet surface noise setup
        g_shader->SetVec3("noiseOffset", planet.noiseOffset);
        g_shader->SetFloat("planetSeed", (float)planet.seed);
        g_shader->SetInt("planetType", planet.biomeType);

        int slice = planet.seed % (int)planet.surfaceVariation.size();
        float variation = planet.surfaceVariation[slice];

        g_shader->SetFloat("surfaceNoise", variation);

        glm::vec3 surfaceColor = PlanetGenerator::getPlanetSurfaceColor(planet, variation);
        g_shader->SetVec3("baseColor", surfaceColor);

        g_shader->SetFloat("isEmissive", 0.0f);

        // Scan highlight if this is the current target and player is aiming + in range
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

    // Reset highlight so it doesn't affect later draws
    g_shader->Use();
    g_shader->SetFloat("scanHighlight", 0.0f);
}

// Draw moons using asteroid texture and sphere mesh
void renderMoons(Planet& planet, const glm::vec3& planetPos, float deltaTime) {
    g_shader->Use();

    g_shader->SetInt("diffuseMap", 0);
    g_shader->SetFloat("isAsteroid", 1.0f);
    g_shader->SetFloat("scanHighlight", 0.0f);
    g_shader->SetFloat("surfaceNoise", 0.0f);

    g_moonTexture->Bind(0);

    for (int i = 0; i < (int)planet.moons.size(); ++i) {
        Moon& moon = planet.moons[i];

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

    // Reset so non-asteroid objects aren't treated as asteroids
    g_shader->SetFloat("isAsteroid", 0.0f);
}

// Draw asteroids (some orbit around origin, others orbit around cluster centers)
void renderAsteroids(float currentTime, float deltaTime) {
    g_shader->Use();
    g_shader->SetInt("diffuseMap", 0);
    g_shader->SetFloat("isAsteroid", 1.0f);
    g_shader->SetFloat("scanHighlight", 0.0f);
    g_asteroidTexture->Bind(0);

    for (int i = 0; i < (int)g_asteroids.size(); ++i) {
        Asteroid& asteroid = g_asteroids[i];

        // Clustered asteroids orbit within a local cluster
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
        // Non-clustered asteroids orbit around origin
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

        // Model transform: translate -> rotate -> scale
        glm::mat4 model = glm::translate(glm::mat4(1.0f), asteroid.pos);
        model = glm::rotate(model, glm::radians(asteroid.rot.x + currentTime * 10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(asteroid.rot.y + currentTime * 15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(asteroid.scale));

        g_shader->SetMat4("model", model);
        g_shader->SetVec3("baseColor", glm::vec3(1.0f));
        g_shader->SetFloat("isEmissive", 0.0f);

        // Cube mesh for asteroids (cheap geometry)
        g_cubeMesh->Draw();
    }

    // Reset flag afterwards
    g_shader->Use();
    g_shader->SetFloat("isAsteroid", 0.0f);
}

// Builds the 2D HUD geometry each frame (radar, speedometer, scan info, etc.)
void buildHUD(float deltaTime) {
    // Animate radar sweep + a general pulse timer for blinking effects
    g_radarAngle += 2.0f * 3.1415926f * deltaTime * 0.1f;
    if (g_radarAngle > 2.0f * 3.1415926f) g_radarAngle -= 2.0f * 3.1415926f;
    g_pulseTime += deltaTime;

    // Clear last frame's HUD draw calls
    g_hudRenderer->clear();

    const float PI2 = 2.0f * 3.1415926f;

    // Convert camera yaw to radians (offset is for your coordinate convention)
    float cameraYaw = glm::radians(g_camera->Yaw + 90.0f);

    // ---------------------------
    // Speedometer (bottom right)
    // ---------------------------
    float cx = 1150, cy = 100;
    float radius = 70.f;

    // Ratio of current speed vs boost speed, clamped to [0,1]
    float speedRatio = glm::clamp(glm::length(g_camera->Velocity) / g_camera->BoostSpeed, 0.0f, 1.0f);

    // Outer rings
    g_hudRenderer->addCircle(glm::vec2(cx, cy), radius + 3, glm::vec3(0.0f, 1.0f, 0.8f), 64);
    g_hudRenderer->addCircle(glm::vec2(cx, cy), radius, glm::vec3(0.0f, 0.7f, 1.0f), 64);

    // Draw arc segments depending on speed
    float speedColor = speedRatio < 0.7f ? (1.0f - speedRatio * 0.5f) : 1.0f;
    for (int i = 0; i < int(speedRatio * 32); ++i) {
        float angle1 = PI2 * i / 32;
        float angle2 = PI2 * (i + 1) / 32;
        glm::vec2 p1 = glm::vec2(cx, cy) + glm::vec2(cos(angle1) * radius, sin(angle1) * radius);
        glm::vec2 p2 = glm::vec2(cx, cy) + glm::vec2(cos(angle2) * radius, sin(angle2) * radius);
        g_hudRenderer->addLine(p1, p2, glm::vec3(0.0f, speedColor, 0.3f));
    }

    // Center marker
    g_hudRenderer->addLine(glm::vec2(cx - 8, cy), glm::vec2(cx + 8, cy), glm::vec3(0.0f, 1.0f, 0.8f));
    g_hudRenderer->addLine(glm::vec2(cx, cy - 8), glm::vec2(cx, cy + 8), glm::vec3(0.0f, 1.0f, 0.8f));

    // ---------------------------
    // Radar (bottom left)
    // ---------------------------
    float rx = 80, ry = 100;
    float radarRadius = 70.f;

    // Radar rings
    g_hudRenderer->addCircle(glm::vec2(rx, ry), radarRadius, glm::vec3(0.0f, 0.6f, 1.0f), 64);
    g_hudRenderer->addCircle(glm::vec2(rx, ry), radarRadius * 0.75f, glm::vec3(0.0f, 0.3f, 0.6f), 32);
    g_hudRenderer->addCircle(glm::vec2(rx, ry), radarRadius * 0.5f, glm::vec3(0.0f, 0.3f, 0.6f), 32);
    g_hudRenderer->addCircle(glm::vec2(rx, ry), radarRadius * 0.25f, glm::vec3(0.0f, 0.3f, 0.6f), 32);
    g_hudRenderer->addLine(glm::vec2(rx, ry + radarRadius + 5), glm::vec2(rx, ry + radarRadius + 15), glm::vec3(0.0f, 0.5f, 1.0f));

    // Faded sweep trail
    for (int trail = 3; trail > 0; --trail) {
        float trailAngle = g_radarAngle - (trail * 0.15f);
        glm::vec2 sweepEnd = glm::vec2(rx, ry) + glm::vec2(sin(trailAngle) * radarRadius, cos(trailAngle) * radarRadius);
        glm::vec3 trailColor = glm::vec3(0.2f, 1.0f, 0.4f) * (1.0f - (trail / 3.0f)) * 0.6f;
        g_hudRenderer->addLine(glm::vec2(rx, ry), sweepEnd, trailColor);
    }

    // Main sweep line
    glm::vec2 mainSweepEnd = glm::vec2(rx, ry) + glm::vec2(sin(g_radarAngle) * radarRadius, cos(g_radarAngle) * radarRadius);
    g_hudRenderer->addLine(glm::vec2(rx, ry), mainSweepEnd, glm::vec3(0.0f, 1.0f, 0.5f));

    // Detect asteroids near the player and plot them on radar
    float radarDetectionRange = 150.0f;
    for (const auto& a : g_asteroids) {
        glm::vec3 offset = a.pos - g_camera->Position;
        float distance = glm::length(offset);

        if (distance > radarDetectionRange) continue;

        // Convert world-space direction to a radar angle relative to camera yaw
        float asteroidAngle = atan2f(offset.x, offset.z) - cameraYaw;
        float asteroidDistance = glm::length(glm::vec2(offset.x, offset.z));

        // Map distance to radar radius
        float radarScale = asteroidDistance / 80.0f;
        float radarX = sin(asteroidAngle) * radarScale * radarRadius;
        float radarY = cos(asteroidAngle) * radarScale * radarRadius;

        // Clamp to radar edge
        radarX = glm::clamp(radarX, -radarRadius, radarRadius);
        radarY = glm::clamp(radarY, -radarRadius, radarRadius);

        // Highlight if it lines up with the radar sweep
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

    // ---------------------------
    // Crosshair (screen centre)
    // ---------------------------
    float centerX = 640.0f, centerY = 360.0f;
    g_hudRenderer->addLine(glm::vec2(centerX - 10, centerY), glm::vec2(centerX + 10, centerY), glm::vec3(0.0f, 1.0f, 1.0f));
    g_hudRenderer->addLine(glm::vec2(centerX, centerY - 10), glm::vec2(centerX, centerY + 10), glm::vec3(0.0f, 1.0f, 1.0f));

    // ---------------------------
    // Target info (name/class) if aimed / scanning
    // ---------------------------
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
                    g_hudRenderer->addText(glm::vec2(namePos.x, namePos.y - 44.0f), 14.0f,
                        glm::vec3(1.0f, 0.2f, 0.2f), "JAMMED");
                }
            }
        }
    }

    // Big warning ring around crosshair when jammed
    if (g_gameState && g_gameState->scanJammed) {
        float pulse = 0.35f + 0.65f * (0.5f + 0.5f * sinf(g_pulseTime * 8.0f));
        g_hudRenderer->addCircle(glm::vec2(centerX, centerY), 28.0f,
            glm::vec3(1.0f, 0.1f, 0.1f) * pulse, 48);
    }

    // ---------------------------
    // Scanned planets indicator (top left)
    // ---------------------------
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

    // ---------------------------
    // Scan progress bar (top middle)
    // ---------------------------
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

            // Glowing scan line (pulses while scanning)
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

            // Little cap at the end of the progress
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

    // ---------------------------
    // End screen (survey complete)
    // ---------------------------
    if (g_gameState && g_gameState->surveyComplete) {
        float cx = WINDOW_WIDTH * 0.5f;
        float cy = WINDOW_HEIGHT * 0.55f;

        float pulse = 0.6f + 0.4f * (0.5f + 0.5f * sinf(g_pulseTime * 2.0f));
        glm::vec3 frameCol = glm::vec3(0.0f, 0.9f, 0.7f) * pulse;
        glm::vec3 dimCol = glm::vec3(0.08f, 0.16f, 0.22f);

        // Decorative rings
        g_hudRenderer->addCircle(glm::vec2(cx, cy), 140.0f, frameCol, 96);
        g_hudRenderer->addCircle(glm::vec2(cx, cy), 110.0f, dimCol, 96);
        g_hudRenderer->addCircle(glm::vec2(cx, cy), 90.0f, frameCol * 0.8f, 96);

        // Cross lines
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

        // Prompts
        float blink = (sinf(g_pulseTime * 4.0f) > 0.0f) ? 1.0f : 0.35f;
        glm::vec3 promptCol = glm::vec3(0.0f, 1.0f, 0.85f) * blink;
        g_hudRenderer->addText(glm::vec2(cx - 150.0f, cy - 70.0f), 14.0f, promptCol, "PRESS R TO RESTART");
        g_hudRenderer->addText(glm::vec2(cx - 120.0f, cy - 92.0f), 12.0f, dimCol * 1.3f, "ESC TO QUIT");
    }

    // Upload HUD geometry to GPU buffers
    g_hudRenderer->finalize();
}

// Draw the HUD (2D overlay)
void renderHUD() {
    glDisable(GL_DEPTH_TEST);

    // Simple orthographic projection for screen-space drawing
    glm::mat4 projection = glm::ortho(
        0.0f, (float)WINDOW_WIDTH,
        0.0f, (float)WINDOW_HEIGHT,
        -1.0f, 1.0f
    );

    g_hudShader->Use();
    g_hudShader->SetMat4("projection", projection);

    g_hudRenderer->render();

    glEnable(GL_DEPTH_TEST);
}

// Master render function called once per frame
void render(float deltaTime, const glm::mat4& view, const glm::mat4& projection) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Background first
    renderStars(view, projection);

    // Setup main shader camera/light uniforms once
    g_shader->Use();
    g_shader->SetMat4("view", view);
    g_shader->SetMat4("projection", projection);
    g_shader->SetVec3("lightPos", g_sun.pos);
    g_shader->SetVec3("viewPos", g_camera->Position);

    // World objects
    renderSun();
    renderPlanets(deltaTime);

    for (auto& planet : g_planets) {
        glm::vec3 planetPos = getPlanetWorldPosition(planet);
        renderMoons(planet, planetPos, deltaTime);
    }

    renderAsteroids((float)glfwGetTime(), deltaTime);

    // Probes are drawn after planets/asteroids so they stand out slightly
    renderProbes();
    renderBrokenProbes();

    // UI last
    buildHUD(deltaTime);
    renderHUD();

    GL_CHECK();
}

// Separate moon update (note: you also update moons inside renderMoons)
void updateMoons(float deltaTime) {
    for (auto& planet : g_planets) {
        for (auto& moon : planet.moons) {
            moon.angle += moon.speed * deltaTime;
        }
    }
}

// ---------------------------
// Main program
// ---------------------------
int main() {
    try {
        std::cout << "=== Initializing Space Explorer ===" << std::endl;

        GLFWwindow* window = initializeWindow();
        std::cout << "Window created" << std::endl;

        initializeGLEW();
        std::cout << "GLEW initialized" << std::endl;

        initializeOpenGL();
        std::cout << "OpenGL context ready" << std::endl;

        // Start camera a bit above the plane looking into the scene
        g_camera = std::make_unique<Camera>(glm::vec3(0.0f, 30.0f, 100.0f));

        initializeShaders();
        initializeGeometry();
        initializeScene();

        std::cout << "=== Initialization complete. Starting main loop ===" << std::endl;

        float lastTime = (float)glfwGetTime();
        int lastTarget = -1;

        while (!glfwWindowShouldClose(window))
        {
            // Delta time for frame-rate independent movement
            float currentTime = (float)glfwGetTime();
            float deltaTime = currentTime - lastTime;
            lastTime = currentTime;

            // Save old position in case we need to undo movement due to collision
            glm::vec3 oldPos = g_camera->Position;

            // Input-driven movement (WASD etc. handled inside Camera)
            g_camera->ProcessKeyboard(window, deltaTime);

            // Update orbiting probes around planets
            updateProbes(deltaTime);

            // ---------------------------
            // Scanning logic
            // ---------------------------

            // Always target the nearest unscanned planet
            g_gameState->currentTarget = findNearestUnscannedPlanet(g_camera->Position);

            // If the target changes, reset scan progress
            if (g_gameState->currentTarget != lastTarget) {
                g_gameState->resetScan();
                lastTarget = g_gameState->currentTarget;
            }

            // Default: not jammed, then we check probes below
            g_gameState->scanJammed = false;

            if (g_gameState->currentTarget != -1) {
                Planet& target = g_planets[g_gameState->currentTarget];
                glm::vec3 planetPos = getPlanetWorldPosition(target);

                float distance = glm::distance(g_camera->Position, planetPos);
                float scanRange = target.collisionRadius + 12.0f;

                bool aimed = isLookingAtTarget(planetPos, 6.0f);
                bool inRange = distance < scanRange;

                // Jamming: if any probe is close to the target planet, scanning is blocked
                bool jammed = false;
                for (const auto& pr : g_probes) {
                    float d = glm::distance(pr.pos, planetPos);
                    if (d < 18.0f) {
                        jammed = true;
                        break;
                    }
                }
                g_gameState->scanJammed = jammed;

                // Hold E to scan (only works if not jammed, aimed, and in range)
                if (!jammed && aimed && inRange && glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                    g_gameState->isScanning = true;
                }
                else {
                    g_gameState->isScanning = false;
                }

                // Update scan progress over time
                g_gameState->updateScan(deltaTime);

                // When scan finishes, mark planet scanned and award points
                if (g_gameState->scanProgress >= 1.0f && !target.scanned) {
                    target.scanned = true;
                    g_gameState->scannedPlanets++;
                    g_gameState->score += 100;
                    g_gameState->resetScan();
                }
            }

            // If all planets are scanned, show completion UI
            if (g_gameState->scannedPlanets == g_gameState->totalPlanets) {
                g_gameState->surveyComplete = true;
            }

            // Debug shortcut: press K to instantly complete the game
            if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
                for (auto& p : g_planets) p.scanned = true;
                g_gameState->scannedPlanets = g_gameState->totalPlanets;
                g_gameState->surveyComplete = true;
                g_gameState->resetScan();
            }

            // Restart game on completion (press R)
            if (g_gameState && g_gameState->surveyComplete && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                g_gameState->surveyComplete = false;
                g_gameState->score = 0;
                g_gameState->scannedPlanets = 0;
                g_gameState->resetScan();

                for (auto& p : g_planets) p.scanned = false;

                // Re-roll probes so the new run feels different
                spawnProbesForPlanets();
            }

            // ---------------------------
            // Collision checks
            // ---------------------------

            float playerRadius = 2.0f;

            // Sun collision
            if (checkSphereCollision(g_camera->Position, playerRadius, g_sun.pos, g_sun.radius)) {
                g_camera->Position = oldPos;
            }

            // Planet collision
            for (const auto& planet : g_planets) {
                glm::vec3 planetPos = getPlanetWorldPosition(planet);
                if (checkSphereCollision(g_camera->Position, playerRadius, planetPos, planet.collisionRadius)) {
                    g_camera->Position = oldPos;
                    break;
                }
            }

            // Asteroid collision
            for (const auto& asteroid : g_asteroids) {
                if (checkSphereCollision(g_camera->Position, playerRadius, asteroid.pos, asteroid.collisionRadius)) {
                    g_camera->Position = oldPos;
                    break;
                }
            }

            // Update moons (note: this is duplicated with renderMoons updates)
            updateMoons(deltaTime);

            // ---------------------------
            // Camera matrices + render
            // ---------------------------

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

        // Clean up heap allocations (could be converted to unique_ptr for safety)
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
