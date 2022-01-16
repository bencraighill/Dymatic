# Dymatic [![License](https://img.shields.io/github/license/BenC25/Dymatic.svg)](https://github.com/BenC25/Dymatic/blob/master/LICENSE)

![Dymatic_Logo_Dark](/Resources/Branding/Dymatic_Logo_Dark.png#gh-dark-mode-only)
![Dymatic_Logo_Light](/Resources/Branding/Dymatic_Logo_Light.png#gh-light-mode-only)

Dymatic is a basic, open source, WIP game engine created using C++.
Currently, Microsoft Visual Studio is required to build, and only officially supports Windows.

<h4>Latest Build:</h4>
<i>Dymatic V1.2.2 - Development</i>

## Building Source
#### Downloading the Repository
Clone this GitHub repository using: <pre><code>git clone --recursive https://github.com/BenC25/Dymatic.git</code></pre>
If previously cloned non-recursively, clone the necessary submodules using:
<pre><code>git submodule update --init</code></pre>

#### Dependencies

1. Run the [`Setup.bat`](https://github.com/BenC25/Dymatic/Public/scripts/Setup.bat) file inside the `scripts` directory which will install any necessary submodules.

2. If the Vulkan SDK, is not installed, `VulaknSDK.exe` will be launched via the script and will prompt the user to install the SDK. After installation run [`Setup.bat`](https://github.com/BenC25/Dymatic/Public/scripts/Setup.bat) again, and if the previous step was successful, this will lead to the Vulkan SDK debug libraries being downloaded and un-zipped automatically.

3. Submodules are updated automatically.

4. The [`Win-GenProjects.bat`](https://github.com/BenC25/Dymatic/Public/scripts/Win-GenProjects.bat) script file will get executed automatically, and will use `premake` to generate project files and configure them to the specifications of the included lua file.

5. If any future changes are made, use [`Win-GenProjects.bat`](https://github.com/BenC25/Dymatic/Public/scripts/Win-GenProjects.bat) to regenerate the project.
  
Enjoy using dymatic.