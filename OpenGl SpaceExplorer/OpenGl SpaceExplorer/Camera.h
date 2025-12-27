#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <algorithm>

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;
    float Speed;
    float BoostSpeed;
    float BoostAccel;
    float Sensitivity;

    glm::vec3 Velocity;

    Camera(glm::vec3 startPos)
        : Position(startPos), Front(0.0f, 0.0f, -1.0f), WorldUp(0.0f, 1.0f, 0.0f),
        Yaw(-90.0f), Pitch(0.0f), Speed(50.0f), BoostSpeed(120.0f),
        BoostAccel(40.0f), Sensitivity(0.1f), Velocity(0.0f) {
        updateCameraVectors();
    }

    void ProcessKeyboard(GLFWwindow* window, float dt) {
        glm::vec3 dir(0.0f);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dir += Front;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dir -= Front;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dir -= Right;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dir += Right;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) dir += WorldUp;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) dir -= WorldUp;

        float targetSpeed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            ? BoostSpeed : Speed;

        glm::vec3 targetVel(0.0f);
        if (glm::length(dir) > 0.0f) {
            targetVel = glm::normalize(dir) * targetSpeed;
        }

        glm::vec3 diff = targetVel - Velocity;
        float maxAccel = BoostAccel * dt;

        if (glm::length(diff) > maxAccel) {
            diff = glm::normalize(diff) * maxAccel;
        }

        Velocity += diff;
        Position += Velocity * dt;
    }

    void ProcessMouse(float xOffset, float yOffset) {
        xOffset *= Sensitivity;
        yOffset *= Sensitivity;

        Yaw += xOffset;
        Pitch += yOffset;
        Pitch = glm::clamp(Pitch, -89.0f, 89.0f);

        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() const {
        return glm::lookAt(Position, Position + Front, Up);
    }

private:

    void updateCameraVectors() {
        glm::vec3 f;
        f.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        f.y = sin(glm::radians(Pitch));
        f.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(f);

        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};