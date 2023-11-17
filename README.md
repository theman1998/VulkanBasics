# VulkanBasics

Just a project that follows along the Vulkan Tutorial, https://vulkan-tutorial.com/.

##  Dependencies
 * VULKAN - Graphics Agnostic : https://www.vulkan.org/tools#download-these-essential-development-tools
 * STB - Image Loading Library : https://github.com/nothings/stb/tree/master
 * GLM - Matrix Transofmration : https://glm.g-truc.net/0.9.9/index.html
 * GLFW - Multi Media Library : https://www.glfw.org/


 * CMake - For compiling : https://cgold.readthedocs.io/en/latest/first-step/installation.html


The CMakeLists.txt file links all the dependencies within a known library folder (C:\\Code_Libraries\\SFML-2.6.0 for windows). This will need to be updated if a different OS is used.


## Building Project

Using Cmake, run the follow command

~~~
cmake -B build .
~~~