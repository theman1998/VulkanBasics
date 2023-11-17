@echo off


set "CompileDirectory=compiled"
set "BinDirectory=../build/bin"

if not exist "%CompileDirectory%\" (
    md "%CompileDirectory%"
)

C:\Code_Libraries\vulkan\Bin/glslc.exe shader.vert -o compiled/vert.spv
C:\Code_Libraries\vulkan\Bin/glslc.exe shader.frag -o compiled/frag.spv




if not exist "%BinDirectory%\" (
    md "%BinDirectory%"
)

robocopy "%CompileDirectory%" "%BinDirectory%/shaders" /E
robocopy "textures" "%BinDirectory%/textures" /E