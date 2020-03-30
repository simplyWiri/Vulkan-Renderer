premake5 --vulkanpath=%VULKAN_SDK% vs2019

cmake -S externals/glfw -B externals/glfw
cmake --build externals/glfw 
pause 