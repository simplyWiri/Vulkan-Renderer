-- Variables and Util Functions
newoption {
   trigger     = "vulkanPath",
   value       = "path",
   description = "Vulkan SDK location"
}
-- To make a 'work space (solution) file'
-- https://github.com/premake/premake-core/issues/1061#issuecomment-441417853
require('vstudio')
premake.api.register {
  name = "workspace_files",
  scope = "workspace",
  kind = "list:string",
}

premake.override(premake.vstudio.sln2005, "projects", function(base, wks)
  if wks.workspace_files and #wks.workspace_files > 0 then
    premake.push('Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "Solution Items", "Solution Items", "{' .. os.uuid("Solution Items:"..wks.name) .. '}"')
    premake.push("ProjectSection(SolutionItems) = preProject")
    for _, file in ipairs(wks.workspace_files) do
      file = path.rebase(file, ".", wks.location)
      premake.w(file.." = "..file)
    end
    premake.pop("EndProjectSection")
    premake.pop("EndProject")
  end
  base(wks)
end)

workspace "Vulkan Renderer"
	architecture "x64"
	startproject "Vulkan Renderer"

	configurations
	{
		"Debug", "Release"
	}

	workspace_files
	{
		"premake5.lua", "README.md", "premake.bat"
	}

project "Vulkan Renderer"
	location ""
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"

	targetdir "/bin/"
	objdir "/bin-int/"

	files
	{
		"resources/**",
		"src/**.h",
		"src/**.cpp",
		"externals/stb/stb_image.h",
		"externals/glm/glm/glm.hpp",
		"externals/imgui/imgui.h",
		"externals/imgui/imgui.cpp",
		"externals/imgui/imgui_draw.cpp",
		"externals/imgui/imgui_widgets.cpp",
	}

	includedirs
	{
		"externals/glfw/include/GLFW",
		"externals/VulkanMemoryAllocator/src",
		"externals/imgui",
		"externals/glm",
		"externals/stb",
		"externals/SPIRV-Cross",
		"externals/glslang",
		(_OPTIONS["vulkanPath"] .. "/Include/vulkan/")
	}

	links
	{
		(_OPTIONS["vulkanPath"] .. "/Lib/vulkan-1.lib"),
		("externals/glfw/src/Debug/glfw3.lib")
	}

	filter "configurations:Debug"
		defines "DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "NDEBUG"
		runtime "Release"
		optimize "on"

