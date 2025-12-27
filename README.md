# Space Explorer – Procedural OpenGL Prototype
**COMP3016 – Immersive Game Technologies**  
**Author:** Harrison Scott  

---

## Project Overview
Space Explorer is a real-time interactive 3D prototype developed in C++ using OpenGL 4.x.  
The project focuses on procedural content generation (PCG), real-time rendering, and user-controlled navigation within a space environment.

The application was built without using any game engine, adhering strictly to the module requirements and approved libraries.

---

## Features
- OpenGL 4.x core profile rendering
- Custom vertex and fragment shaders
- Procedurally generated planets
- Height variation (non-flat terrain)
- Multiple biome types per planet
- Real-time keyboard and mouse input
- Free-flight camera system
- Scene animation (planet rotation / movement)
- Assimp-loaded 3D model(s)
- Texture loading via stb_image
- Starfield rendering system
- HUD rendering via separate shader

---

## Gameplay Description
The prototype allows the player to freely explore a procedurally generated space scene.  
Each execution produces a different configuration of planets with varying heights and biome types.

The experience focuses on exploration rather than win/lose conditions, aligning with a sandbox-style design.

---

## Original Contributions (Key Requirement B)
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

## Use of AI
ChatGPT was used for:
- Concept clarification
- Debugging assistance
- Small code templates

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

## Screenshots
*(Add screenshots here)*

---

## Video Demonstration
**YouTube (Unlisted):** *(link to be added)*  
Includes:
- Live walkthrough
- Feature explanation
- Real-time demo
- Face camera for authentication

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
- Advanced lighting techniques
- Expanded gameplay mechanics
