# Dymatic [![License](https://img.shields.io/github/license/bencraighill/Dymatic.svg)](https://github.com/bencraighill/Dymatic/blob/master/LICENSE)

![Dymatic_Logo_Dark](/Resources/Branding/Dymatic_Logo_Dark.png#gh-dark-mode-only)
![Dymatic_Logo_Light](/Resources/Branding/Dymatic_Logo_Light.png#gh-light-mode-only)

Dymatic is a basic open source C++ game engine.

<h4>Latest Build:</h4>
<i>Dymatic Engine version 25.1.0 (Development)</i>

## Features
- Real-time 2D and deferred 3D renderer
    - Static and Animated Meshes (with custom or generated LODs)
    - Various Shadowed lighting types
    - Volumetric Fog
    - Global Illumination (VXGI)
    - GPU Particle System (with GPU and screen space collision)
    - GPU frustum culling
    - Decals
    - Primitive Software Path Tracing
    - Post Processing Effects
- Extensive and customizable scene building toolset
- Project management system
- Entity component system
- C# script engine
- Navigation Query System (using recast/detour)
- 2D and 3D Physics Engine (built on Jolt Physics)
    - Rigid and Soft Bodies (highly controllable from scripts)
    - Ragdolls
    - Constraints
    - Force and buoyancy fields
    - Characters
    - Vehicles
    - Various query/tracing types on layers
- Audio engine
    - Audio Graph System coming Soon
- Asset manager 
    - Automatically tracked file changes
    - Content browser with thumbnail generation and file operations
- Built in source control with Git
- Visual Scripting System compiling to Native C#
- Material Graph
    - Supports surfaces, particles and custom post processing effects
    - GLSL shader compiler (can insert custom code if desired in editor)
- Animation Graphs
    - Various Animation Blending and Layering Operations
    - Inverse Kinematics (Two Bone and FABRIK)
    - State Machines and Blendspaces
    - Support for blend shapes (or morph targets)
    - Direct Bone Manipulation
    - Runtime parameters and logic
    - Compiler for optimized runtime representation
- Python based editor scripting
    - Custom UI, Shortcuts and Toolbar Elements
    - Perform editor operations automatically
- Application build and deployment system
    - Optimized Runtime Executable
    - Packaged Assets and Binaries in custom engine runtime format
    - Obfuscated and stripped shaders

## Preview
![Global_Illumination_Screenshot](/Resources/Branding/Screenshots/GlobalIllumination.png)
![Dymatic_Editor_Screenshot](/Resources/Branding/Screenshots/EditorScreenshot.png)
![Path_Tracing_Screenshot](/Resources/Branding/Screenshots/PathTracing.png)

## Build Requirements
- Microsoft Visual Studio 2022
- Windows with OpenGL and the `ARB_bindless_texture` driver extension (supported on most modern Nvidia and AMD graphics cards)

## Building
#### Downloading the Repository
Clone this GitHub repository using: <pre><code>git clone --recursive https://github.com/bencraighill/Dymatic.git</code></pre>
If previously cloned non-recursively, clone the necessary submodules using:
<pre><code>git submodule update --init</code></pre>

#### Dependencies

1. Run the [`Setup.bat`](https://github.com/bencraighill/Dymatic/Public/scripts/Setup.bat) file inside the `scripts` directory which will install any necessary submodules. Currently, python is required for these scripts to execute.

2. If the Vulkan SDK, is not installed, `VulaknSDK.exe` will be launched via the script and will prompt the user to install the SDK. After installation run [`Setup.bat`](https://github.com/bencraighill/Dymatic/Public/scripts/Setup.bat) again, and if the previous step was successful, this will lead to the Vulkan SDK debug libraries being downloaded and un-zipped automatically.

3. Submodules are updated automatically.

4. Registration scripts will run, adding shortcuts, file extensions and ensuring that the Dymatic Editor and Runtime both use a 'High Performance' display setting.

5. The [`Win-GenProjects.bat`](https://github.com/bencraighill/Dymatic/Public/scripts/Win-GenProjects.bat) script file will get executed automatically, and will use `premake` to generate project files and configure them to the specifications of the included lua file.

6. If any future changes are made to the project setup, use [`Win-GenProjects.bat`](https://github.com/bencraighill/Dymatic/Public/scripts/Win-GenProjects.bat) to regenerate the project.

## [Available Tools](https://www.dymaticengine.com/tools)
### Dymatic Engine
The Dymatic Editor, Dymatic's powerful visual toolset for managing and creating assets, designing realistic and engaging 3D worlds, scripting interactive experiences and more.

### [Dymatic Tools VSIX](https://github.com/bencraighill/DymaticTools)
Dymatic's custom Visual Studio extension. Provides a toolset allowing for quick and easy debugging of C# scripts during Dymatic runtime in editor, including breakpoints, callstack, variable inspection and more. Introduces multiple C# templates to reduce time spent on boilerplate code.

### [Dymatic Live Link](https://github.com/bencraighill/DymaticLiveLink)
Example Swift application that connects to the Editor Live Link Server allowing users to see their realtime camera movements on a mobile device reflected in editor.

## Rationale
Dymatic Engine is a personal project developed by Ben Craighill, serving as as an educational tool, continually evolving as new features are introduced.

<br><br><i>© 2025 Dymatic Technologies, All rights reserved</i>