# LowEngine – Custom C++ Game Engine

![image](https://github.com/user-attachments/assets/80542124-09cd-4024-adce-e02085e3c49c)

Welcome to LowEngine, a custom-built game engine developed entirely in C++ with a Vulkan-based renderer. Designed with flexibility and performance in mind, this engine leverages a data-driven ECS architecture, multithreading, and a robust editor to support a wide range of game development needs.

## Key Features
### Rendering
* Vulkan-based deferred renderer with configurable post-processing.
* Frame graph-driven rendering pipeline.
* Skeletal animation support.
* Real-time shader hot-reloading.

### Entity Component System (ECS)
* Data-driven design with Structure of Arrays (SoA) memory layout.
* Optimized for efficient entity management.

### Scripting & Tools
* Code scripting and visual scripting support.
* Custom-built editor with undo/redo functionality.
* Prefabs and runtime module loading.

### Physics & Collision
* Integrated NVIDIA PhysX for rigid body physics.
* Raycasting and collision detection with box colliders.
* Synced physics and visual worlds.

### World Streaming & Asset Management
* Dynamic world streaming based on camera proximity.
* Reference-counted asset management for efficient memory use.

### Navigation & AI
* NavMesh-based pathfinding using Recast & Detour.

## Getting Started
At this stage, LowEngine is not yet packaged for easy project setup. While the engine source can be cloned and built, it will not run out-of-the-box — it requires a properly configured game project to load at startup. Currently, setting up a project involves multiple manual steps, and a streamlined workflow is still in development.
If you're interested in experimenting with the engine, feel free to explore the codebase, but keep in mind that additional setup is required to create a functional project. More details on the setup process will be added in the future as the engine evolves.

## Current Status
While some features, such as a dedicated sound system, are still in development, the engine is already being used by a small team to create a role-playing game with card-based combat. The project leverages both the code scripting and visual scripting systems, allowing designers to define card behaviors without modifying engine code.

## Showcase

![image](https://github.com/user-attachments/assets/3ae8b975-8b78-4e19-898b-f3a6fae7c868)
_Visual scripting_

![image](https://github.com/user-attachments/assets/497bd144-9b42-42ff-b532-1aa73259572f) ![image](https://github.com/user-attachments/assets/b999ea3a-d3ac-4139-b46e-29bc5f5ab23a)
_Box collider debug visualization and physics settings_

![image](https://github.com/user-attachments/assets/bf748b7d-58eb-490c-8434-f53cf7696baf)
_NavMesh debug visualization_

![region_streaming](https://github.com/user-attachments/assets/74f8f433-507d-4a56-94a0-1bbdd58527bb)
_Proximity based region and asset streaming_

