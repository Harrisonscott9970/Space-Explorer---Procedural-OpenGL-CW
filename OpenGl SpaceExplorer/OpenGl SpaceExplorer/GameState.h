#pragma once
#include <glm/glm.hpp>
#include <string>

class GameState {
public:
    int totalPlanets = 0;
    int scannedPlanets = 0;
    int score = 0;

    int currentTarget = -1; 
    float scanProgress = 0.0f;
    bool isScanning = false;
    bool scanJammed = false;

    bool surveyComplete = false;

    void resetScan() {
        scanProgress = 0.0f;
        isScanning = false;
        scanJammed = false;
    }

    void updateScan(float deltaTime) {
        if (!isScanning || scanJammed) return;

        scanProgress += deltaTime * 0.4f;

        if (scanProgress >= 1.0f) {
            scanProgress = 1.0f;
        }
    }
};
