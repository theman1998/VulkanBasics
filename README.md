# VulkanBasics

Just a project that follows along the Vulkan Tutorial, https://vulkan-tutorial.com/.

##  Dependencies

---------------------------------------------------

 * VULKAN - Graphics Agnostic : https://www.vulkan.org/tools#download-these-essential-development-tools
 * STB - Image Loading Library : https://github.com/nothings/stb/tree/master
 * GLM - Matrix Transformation : https://glm.g-truc.net/0.9.9/index.html
 * GLFW - Multi Media Library : https://www.glfw.org/

---------------------------------------------------- 
 * CMake - For Compiling C++: https://cgold.readthedocs.io/en/latest/first-step/installation.html
 * GLSLC - For Compiling Shaders: Packaged in with the Vulkan SDK

The CMakeLists.txt file links all the dependencies within a known library folder (C:\\Code_Libraries\\SFML-2.6.0 for windows). This will need to be updated if a different OS is used.


## Building Project

Using Cmake, run the follow command

~~~
cmake -B build .
~~~


## COMPILE THE SHADERS

shaders are programs that can talk nativly to the GPU. It can be executed with the C++ program to increase performance and reduce complex workloads. There should be a script created to compile the shaders found within the shaders folder. These compile shaders should live within the runtime folder along with the executable.

Windows will use "compile.bat"
Linux will use "compile.sh"