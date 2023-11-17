#Don't have a linux machine. SO this can't be tested. TEST THIS


CompileDirectory="compiled"
BinDirectory="../build/bin"

if [ ! -d "$CompileDirectory" ]; then
	mkdir "$CompileDirectory"
fi

/home/user/Code_Libraries/vulkan/bin/glslc shader.vert -o compiled/vert.spv
/home/user/Code_Libraries/vulkan/bin/glslc shader.frag -o compiled/frag.spv



if [ ! -d "$BinDirectory" ]; then
	mkdir -p "$BinDirectory"
fi

cp -r "$CompileDirectory" "$BinDirectory/shaders"
cp -r "textures" "$BinDirectory/textures"