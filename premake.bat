premake5 --vulkanpath=%VULKAN_SDK% vs2019

cmake -S externals/glfw -B externals/glfw
cmake --build externals/glfw 

cmake -S externals/glslang -B externals/glslang/builtversion
cmake --build externals/glslang/builtversion --config Debug --target install --BUILD_SHARED_LIBS=ON

cmake -S externals/SPIRV-Cross -B externals/SPIRV-Cross/builtversion
cmake --build externals/SPIRV-Cross/builtversion --config Debug 

pause 