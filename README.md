# Space Explorer – Procedural OpenGL Prototype
**COMP3016 – Immersive Game Technologies**  
**Author:** Harrison Scott  

---

## Project Overview
Space Explorer is a real-time interactive 3D prototype developed in C++ using OpenGL 4.x.  

The project focuses on procedural content generation (PCG), real-time rendering, and user-controlled navigation within a space environment.

The application was built without using any game engine, following strictly with the module requirements and approved libraries.

---

## Features
- OpenGL 4.x core profile rendering
- Custom vertex and fragment shaders
- Procedurally generated planets
- Height variation
- Multiple biome types per planet
- Real-time keyboard and mouse input
- Free-flight camera system
- Scene animation (planet rotation / movement)
- Assimp-loaded 3D models
- Dynamic Lighting (Sun)
- Texture loading via stb_image
- Starfield rendering system
- HUD rendering via separate shader
- 10-minute video

---

## Gameplay Description
The demo game allows the player to freely explore a procedurally generated space scene.  

Each time you load up it produces a different configuration of planets with varying heights and biome types.

The experience focuses on exploration rather than win/lose conditions, aligning with a more sandbox-style design.

---

| Controls           | Action                      |
| ------------------ | --------------------------- |
| **W**              | forward                     |
| **S**              | backward                    |
| **A**              | left                        |
| **D**              | right                       |
| **Space**          | up                          |
| **Left Ctrl**      | down                        |
| **Left Shift**     | Boost                       |
| **E**              | Scan                        |
| **Mouse Movement** | Rotate camera / look around |
| **Esc**            | Exit application            |

---

## Original Contributions
The following original features were implemented and are covered in lecture material:

1. **Procedural Content Generation**
   - Randomised planet count per run
   - Height variation applied per planet
   - Multiple biome types
   - Non-repeating layouts

2. **Real-Time Interaction System**
   - Keyboard-based movement
   - Mouse-controlled camera rotation
   - Frame-rate independent updates

---

## Technologies & Dependencies
All libraries used are permitted by the module.

| Library | Purpose |
|------|-------|
| OpenGL 4.x | Rendering |
| GLFW | Window & input handling |
| GLEW | OpenGL function loading |
| GLM | Mathematics |
| ASSIMP | Model loading |
| stb_image | Texture loading |

---

# Use of AI

ChatGPT was used for various aspects of this project, aiding in both the development and debugging phases.

Concept Clarification
ChatGPT helped refine the procedural generation logic for planets, height maps, and biomes to ensure that they behaved as expected. This included offering suggestions on how to structure and implement different procedural elements.

Debugging Assistance
It provided solutions to several OpenGL initialization and shader issues, as well as fixes for procedural generation errors that were encountered during development. 

Code Templates
ChatGPT generated small code snippets, such as the `randf` function for random number generation, which helped me with the procedural logic for generating features of the planets. This was key in speeding up the development of the procedural generation system.

HUD Rendering Logic
Assistance was also provided in designing the system for rendering text on the HUD. This involved:

- Mapping characters (digits and letters) to 2D line segments for text rendering.
- Suggesting approaches for dynamically rendering each character by adding corresponding line segments to the HUD.

### Example (Glyph Mapping for Text Rendering):
```cpp

static const std::vector<Seg> A = {
    {0.20f, 0.15f, 0.20f, 0.85f},  // Left vertical line
    {0.80f, 0.15f, 0.80f, 0.85f},  // Right vertical line
    {0.20f, 0.85f, 0.80f, 0.85f},  // Top horizontal line
    {0.20f, 0.50f, 0.80f, 0.50f},  // Middle horizontal line
};

static const std::vector<Seg> B = {
    {0.20f, 0.15f, 0.20f, 0.85f},  // Left vertical line
    {0.20f, 0.85f, 0.70f, 0.85f},  // Top curve
    {0.70f, 0.85f, 0.75f, 0.75f},  // Top curve curve right
    {0.75f, 0.75f, 0.70f, 0.65f},  // Right curve
    {0.70f, 0.65f, 0.20f, 0.65f},  // Bottom curve
    {0.20f, 0.65f, 0.70f, 0.65f},  // Right curve bottom
    // Additional segments for bottom and middle parts...
};

switch (c) {
    case 'A': return A;
    case 'B': return B;
    // Other characters handled here...
    default: return EMPTY;  // Default case for unknown characters
}
```
---

## Software Design
- Object-Oriented Programming (OOP)
- Separation of concerns:
  - Rendering
  - Input
  - Game state
  - Procedural generation
- Modular class-based architecture
- Time-based movement and updates

---

## How to Run
1. Open the `OpenGl SpaceExplorer\x64\Debug` folder
2. Run `OpenGl SpaceExplorer.exe`
3. Ensure the following folders are present:
   - `assets/`
   - `shaders/`

---

## Error Handling & Testing
- OpenGL initialisation checks
- Runtime checks for asset loading
- Defensive programming to avoid null access
- Stable execution across multiple runs

---

## Audio Implementation

In an attempt to integrate audio into the project, I reached out to Ambiera for a student license for the IrrKlang SDK, as the download was restricted to licensed users. Unfortunately, I was informed that the SDK is only available for existing license holders, so I was unable to use it for the project.

---

## Screenshots

<img width="1281" height="720" alt="image" src="https://github.com/user-attachments/assets/294042b8-6578-414d-9f94-ce460ecbbc5c" />

Procedurally generated space scene featuring a star and surrounding planetary debris. The HUD displays real-time navigation and system status, providing essential feedback for interaction within the environment.

<img width="1280" height="720" alt="image" src="https://github.com/user-attachments/assets/2dc42149-c7d3-4b94-94c5-ea47ea04bc00" />
Procedurally generated planet Xelonix-1, classified as 'Class Green,' with varied biomes including green landmasses and metallic terrain, demonstrating dynamic surface generation.

---

## Video Demonstration
**YouTube: (https://youtu.be/_xX4xQ8fphU)**

---

## Evaluation
This project successfully demonstrates:
- A working OpenGL 4.x rendering pipeline
- Procedural generation techniques
- Interactive 3D navigation
- Clean C++ architecture without reliance on an engine

Future improvements would include:
- Additional model formats
- Audio integration
- More realistic space behaviour 
- Expanded gameplay mechanics
