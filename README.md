# Dymatic [![License](https://img.shields.io/github/license/BenC25/Dymatic.svg)](https://github.com/BenC25/Dymatic/blob/master/LICENSE)

![Dymatic_Logo_Dark](/Resources/Branding/Dymatic_Logo_Dark.png#gh-dark-mode-only)
![Dymatic_Logo_Light](/Resources/Branding/Dymatic_Logo_Light.png#gh-light-mode-only)

Dymatic is a basic, open source, C++ game engine.
Currently, Microsoft Visual Studio 2022 is required to build, and only supports Windows with OpenGL.

<h4>Latest Build:</h4>
<i>Dymatic Engine version 23.1.0 (Development)</i>

![Dymatic_Editor_Screenshot](/Resources/Branding/Screenshots/EditorScreenshot.png)

## Features
- Real-time 2D and 3D renderer with lighting, volumetrics and animation
- Extensive and customizable scene building toolset
- Project management system
- Entity component system
- 2D and 3D physics engine
- Audio engine
- Asset manager
- C# script engine
- Built in source control
- Visual scripting system
- Python based editor scripting
- Application deployment system

## Building
#### Downloading the Repository
Clone this GitHub repository using: <pre><code>git clone --recursive https://github.com/BenC25/Dymatic.git</code></pre>
If previously cloned non-recursively, clone the necessary submodules using:
<pre><code>git submodule update --init</code></pre>

#### Dependencies

1. Run the [`Setup.bat`](https://github.com/BenC25/Dymatic/Public/scripts/Setup.bat) file inside the `scripts` directory which will install any necessary submodules. Currently, python is required for these scripts to execute.

2. If the Vulkan SDK, is not installed, `VulaknSDK.exe` will be launched via the script and will prompt the user to install the SDK. After installation run [`Setup.bat`](https://github.com/BenC25/Dymatic/Public/scripts/Setup.bat) again, and if the previous step was successful, this will lead to the Vulkan SDK debug libraries being downloaded and un-zipped automatically.

3. Submodules are updated automatically.

4. Registration scripts will run, adding shortcuts, file extensions and ensuring that the Dymatic Editor and Runtime both use a 'High Performance' display setting.

5. The [`Win-GenProjects.bat`](https://github.com/BenC25/Dymatic/Public/scripts/Win-GenProjects.bat) script file will get executed automatically, and will use `premake` to generate project files and configure them to the specifications of the included lua file.

6. If any future changes are made to the project setup, use [`Win-GenProjects.bat`](https://github.com/BenC25/Dymatic/Public/scripts/Win-GenProjects.bat) to regenerate the project.

## [Available Tools](https://www.dymaticengine.com/tools)
### Dymatic Engine
The Dymatic Editor, Dymatic's powerful visual toolset for managing and creating assets, designing realistic and engaging 3D worlds, scripting interactive experiences and more.

### [Dymatic Tools VSIX](https://github.com/BenC25/DymaticTools)
Dymatic's custom Visual Studio extension. Provides a toolset allowing for quick and easy debugging of C# scripts during Dymatic runtime in editor, including breakpoints, callstack, variable inspection and more. Introduces multiple C# templates to reduce time spent on boilerplate code.

## Rationale
Dymatic Engine is a personal project developed by Ben Craighill, serving as as an educational tool, continually evolving as new features are introduced.