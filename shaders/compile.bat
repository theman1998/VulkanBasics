@echo off


set "CompileDirectory=compiled"
set "BinDirectory=../build/bin"

if not exist "%CompileDirectory%\" (
    md "%CompileDirectory%"
)

C:\Libs\VulkanSDK\Bin/glslc.exe shader.vert -o compiled/vert.spv
C:\Libs\VulkanSDK\Bin/glslc.exe shader.frag -o compiled/frag.spv




if not exist "%BinDirectory%\" (
    md "%BinDirectory%"
)

robocopy "%CompileDirectory%" "%BinDirectory%/shaders" /E
robocopy "textures" "%BinDirectory%/textures" /E
robocopy "models" "%BinDirectory%/models" /E