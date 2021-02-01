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

function CreateProject(name)
	project ("" .. name) -- Concatonate name to an empty string
	location ("Projects/" .. name)
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	editAndContinue "off"

	targetdir "Projects/%{prj.name}/bin/"
	objdir "Projects/%{prj.name}/bin-int/"

	files
	{
		("Projects/" .. name .. "/resources/**"),
		("Projects/" .. name .. "/src/**.h"),
		("Projects/" .. name .. "/src/**.cpp"),
		("Projects/" .. name .. "/**.md")
	}

	includedirs
	{
		"src/",
		"external/",
		"externals/glfw/include/GLFW",
		"externals/imgui",
		"externals/implot",
		"externals/glm",
		"externals/stb",
		"externals/SPIRV-Cross",
		"externals/spdlog/include",
		"externals/glslang",
		"externals/tracy",
		(_OPTIONS["vulkanPath"] .. "/Include/vulkan/")
	}

	links
	{
		"Vulkan Renderer",
		"Tracy"
	}

	filter "configurations:Verbose"
		defines { "VERBOSE", "TRACE", "DEBUG", "TRACY_ENABLE" }
		runtime "Debug"
		symbols "on"

	filter "configurations:Trace"
		defines { "TRACE", "DEBUG", "TRACY_ENABLE"}
		runtime "Debug"
		symbols "on"

	filter "configurations:Debug"
		defines { "DEBUG", "TRACY_ENABLE"}
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines  {"NDEBUG", "TRACY_ENABLE"}
		runtime "Release"
		optimize "on"
end

workspace "Renderer"
	architecture "x64"

	configurations
	{
		"Verbose", "Trace", "Debug", "Release", 
	}

	workspace_files
	{
		"premake5.lua", "README.md", "premake.bat", "externals/premake5.lua"
	}

	filter "configurations:Verbose"
		defines { "VERBOSE", "TRACE", "DEBUG", "TRACY_ENABLE" }
		runtime "Debug"
		symbols "on"

	filter "configurations:Trace"
		defines { "TRACE", "DEBUG", "TRACY_ENABLE"}
		runtime "Debug"
		symbols "on"

	filter "configurations:Debug"
		defines { "DEBUG", "TRACY_ENABLE"}
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines  {"NDEBUG", "TRACY_ENABLE"}
		runtime "Release"
		optimize "on"

group "Third Party"
	include "externals/"
group ""

project "Vulkan Renderer"
	location ""
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	editAndContinue "off"

	targetdir "bin/"
	objdir "bin-int/"

	files
	{
		"resources/**",
		"src/**.h",
		"src/**.cpp",
		"externals/stb/stb_image.h",
	}

	includedirs
	{
		"externals/glfw/include/GLFW",
		"externals/imgui",
		"externals/implot",
		"externals/glm",
		"externals/stb",
		"externals/SPIRV-Cross",
		"externals/spdlog/include",
		"externals/glslang",
		"externals/tracy",
		(_OPTIONS["vulkanPath"] .. "/Include/vulkan/")
	}

	links
	{
		(_OPTIONS["vulkanPath"] .. "/Lib/vulkan-1.lib"),
		"GLFW",
		"ImGui",
		"ImPlot",
		"Glslang",
		"SPIRV-Cross"
	}

project "Common"
	location ""
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	editAndContinue "off"

	targetdir "bin/"
	objdir "bin-int/"

	files
	{
		"resources/**",
		"src/**.h",
		"src/**.cpp",
		"externals/stb/stb_image.h",
	}


group "Projects"

CreateProject("Fibonacci Sphere")

group ""




