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

project "Renderer"
	location "Core/Renderer"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	editAndContinue "off"

	targetdir "Core/Renderer/bin/"
	objdir "Core/Renderer/bin-int/"

	files
	{
		"Core/Renderer/resources/**",
		"Core/Renderer/src/**.h",
		"Core/Renderer/src/**.cpp",
		"Core/Renderer/externals/stb/stb_image.h",
	}

	includedirs
	{
		"Core/Renderer/src",
		"externals",
		"externals/spdlog/include/",
		(_OPTIONS["vulkanPath"] .. "/Include")
	}

	links
	{
		"Volk",
		"GLFW",
		"ImGui",
		"ImPlot",
		"Glslang",
		"SPIRV-Cross",
		-- "Common"
	}

project "Common"
	location "Core/Common"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	editAndContinue "off"

	targetdir "Core/Common/bin/"
	objdir "Core/Common/bin-int/"

	files
	{
		"Core/Common/src/**.h",
		"Core/Common/src/**.cpp"
	}


project ("World Generator") 
	location ("World/")
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	editAndContinue "off"

	targetdir "World/bin/"
	objdir "World/bin-int/"

	files
	{
		("World/resources/**"),
		("World/src/**.h"),
		("World/src/**.cpp"),
		("World/**.md")
	}

	includedirs
	{
		"Core/Renderer/src",
		"externals",
		(_OPTIONS["vulkanPath"] .. "/Include/vulkan/"),
		"Core/Common/src"
	}

	links
	{
		-- "Common",
		"Renderer",
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



project ("Prototyping") 
		location ("Prototyping/")
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"
		editAndContinue "off"
	
		targetdir "Prototyping/bin/"
		objdir "Prototyping/bin-int/"
	
		files
		{
			("Prototyping/resources/**"),
			("Prototyping/src/**.h"),
			("Prototyping/src/**.cpp"),
			("Prototyping/**.md")
		}
	
		includedirs
		{
			"Core/Renderer/src",
			"externals",
			(_OPTIONS["vulkanPath"] .. "/Include"),
			"externals/spdlog/include/",
			"Core/Common/src"
		}
	
		links
		{
			"Renderer",
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
	
	

