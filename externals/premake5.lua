project "ImGui"
	kind "StaticLib"
    language "C++"

	targetdir ("bin/%{prj.name}")
	objdir ("bin-int/%{prj.name}")

	files
	{
		"imgui/imgui.cpp",
		"imgui/imgui_draw.cpp",
		"imgui/imgui_widgets.cpp"
	}
	
project "GLFW"
	kind "StaticLib"
	language "C"

	targetdir ("bin/%{prj.name}")
	objdir ("bin-int/%{prj.name}")

	files
	{
		"glfw/include/GLFW/glfw3.h",
		"glfw/include/GLFW/glfw3native.h",
		"glfw/src/glfw_config.h",
		"glfw/src/context.c",
		"glfw/src/init.c",
		"glfw/src/input.c",
		"glfw/src/monitor.c",
		"glfw/src/vulkan.c",
		"glfw/src/window.c"
	}

	filter "system:windows"
		systemversion "latest"
		staticruntime "On"

		files
		{
			"glfw/src/win32_init.c",
			"glfw/src/win32_joystick.c",
			"glfw/src/win32_monitor.c",
			"glfw/src/win32_time.c",
			"glfw/src/win32_thread.c",
			"glfw/src/win32_window.c",
			"glfw/src/wgl_context.c",
			"glfw/src/egl_context.c",
			"glfw/src/osmesa_context.c"
		}

		defines 
		{ 
			"_GLFW_WIN32",
			"_CRT_SECURE_NO_WARNINGS"
		}

project "SPIRV-Cross"
		kind "StaticLib"
		language "C++"
	
		targetdir ("bin/%{prj.name}")
		objdir ("bin-int/%{prj.name}")
	
		files
		{
			"SPIRV-Cross/GLSL.std.450.h",
			"SPIRV-Cross/spirv_common.hpp",
			"SPIRV-Cross/spirv_cross_containers.hpp",
			"SPIRV-Cross/spirv_cross_error_handling.hpp",
			"SPIRV-Cross/spirv.hpp",
			"SPIRV-Cross/spirv_cross.hpp",
			"SPIRV-Cross/spirv_cross.cpp",
			"SPIRV-Cross/spirv_parser.hpp",
			"SPIRV-Cross/spirv_parser.cpp",
			"SPIRV-Cross/spirv_cross_parsed_ir.hpp",
			"SPIRV-Cross/spirv_cross_parsed_ir.cpp",
			"SPIRV-Cross/spirv_cfg.hpp",
			"SPIRV-Cross/spirv_cfg.cpp",
	
			-- Util
			"SPIRV-Cross/spirv_cross_util.cpp",
			"SPIRV-Cross/spirv_cross_util.hpp",
			-- Cpp
			"SPIRV-Cross/spirv_cpp.cpp",
			"SPIRV-Cross/spirv_cpp.hpp",
			-- Reflect
			"SPIRV-Cross/spirv_reflect.cpp",
			"SPIRV-Cross/spirv_reflect.hpp",
			-- Glsl
			"SPIRV-Cross/spirv_glsl.cpp",
			"SPIRV-Cross/spirv_glsl.hpp"
		}
	
project "Glslang"
	kind "StaticLib"
    language "C++"

	targetdir ("bin/%{prj.name}")
	objdir ("bin-int/%{prj.name}")

    defines
    {
        "ENABLE_OPT=0"
    }

	files
	{
        -- Machine Indepdent
        "glslang/glslang/MachineIndependent/glslang.m4",
        "glslang/glslang/MachineIndependent/glslang.y",
        "glslang/glslang/MachineIndependent/glslang_tab.cpp",
        "glslang/glslang/MachineIndependent/attribute.cpp",
        "glslang/glslang/MachineIndependent/Constant.cpp",
        "glslang/glslang/MachineIndependent/iomapper.cpp",
        "glslang/glslang/MachineIndependent/InfoSink.cpp",
        "glslang/glslang/MachineIndependent/Initialize.cpp",
        "glslang/glslang/MachineIndependent/IntermTraverse.cpp",
        "glslang/glslang/MachineIndependent/Intermediate.cpp",
        "glslang/glslang/MachineIndependent/ParseContextBase.cpp",
        "glslang/glslang/MachineIndependent/ParseHelper.cpp",
        "glslang/glslang/MachineIndependent/PoolAlloc.cpp",
        "glslang/glslang/MachineIndependent/RemoveTree.cpp",
        "glslang/glslang/MachineIndependent/Scan.cpp",
        "glslang/glslang/MachineIndependent/ShaderLang.cpp",
        "glslang/glslang/MachineIndependent/SymbolTable.cpp",
        "glslang/glslang/MachineIndependent/Versions.cpp",
        "glslang/glslang/MachineIndependent/intermOut.cpp",
        "glslang/glslang/MachineIndependent/limits.cpp",
        "glslang/glslang/MachineIndependent/linkValidate.cpp",
        "glslang/glslang/MachineIndependent/parseConst.cpp",
        "glslang/glslang/MachineIndependent/reflection.cpp",
        "glslang/glslang/MachineIndependent/preprocessor/Pp.cpp",
        "glslang/glslang/MachineIndependent/preprocessor/PpAtom.cpp",
        "glslang/glslang/MachineIndependent/preprocessor/PpContext.cpp",
        "glslang/glslang/MachineIndependent/preprocessor/PpScanner.cpp",
        "glslang/glslang/MachineIndependent/preprocessor/PpTokens.cpp",
        "glslang/glslang/MachineIndependent/propagateNoContraction.cpp",
        "glslang/glslang/GenericCodeGen/CodeGen.cpp",
        "glslang/glslang/GenericCodeGen/Link.cpp",
        "glslang/glslang/CInterface/glslang_c_interface.cpp",

        "glslang/glslang/Public/ShaderLang.h",
        "glslang/glslang/Include/arrays.h",
        "glslang/glslang/Include/BaseTypes.h",
        "glslang/glslang/Include/Common.h",
        "glslang/glslang/Include/ConstantUnion.h",
        "glslang/glslang/Include/glslang_c_interface.h",
        "glslang/glslang/Include/glslang_c_shader_types.h",
        "glslang/glslang/Include/InfoSink.h",
        "glslang/glslang/Include/InitializeGlobals.h",
        "glslang/glslang/Include/intermediate.h",
        "glslang/glslang/Include/PoolAlloc.h",
        "glslang/glslang/Include/ResourceLimits.h",
        "glslang/glslang/Include/revision.h",
        "glslang/glslang/Include/ShHandle.h",
        "glslang/glslang/Include/Types.h",
        "glslang/glslang/MachineIndependent/attribute.h",
        "glslang/glslang/MachineIndependent/glslang_tab.cpp.h",
        "glslang/glslang/MachineIndependent/gl_types.h",
        "glslang/glslang/MachineIndependent/Initialize.h",
        "glslang/glslang/MachineIndependent/iomapper.h",
        "glslang/glslang/MachineIndependent/LiveTraverser.h",
        "glslang/glslang/MachineIndependent/localintermediate.h",
        "glslang/glslang/MachineIndependent/ParseHelper.h",
        "glslang/glslang/MachineIndependent/reflection.h",
        "glslang/glslang/MachineIndependent/RemoveTree.h",
        "glslang/glslang/MachineIndependent/Scan.h",
        "glslang/glslang/MachineIndependent/ScanContext.h",
        "glslang/glslang/MachineIndependent/SymbolTable.h",
        "glslang/glslang/MachineIndependent/Versions.h",
        "glslang/glslang/MachineIndependent/parseVersions.h",
        "glslang/glslang/MachineIndependent/propagateNoContraction.h",
        "glslang/glslang/MachineIndependent/preprocessor/PpContext.h",
        "glslang/glslang/MachineIndependent/preprocessor/PpTokens.h",

        "glslang/StandAlone/StandAlone.cpp",
        "glslang/StandAlone/DirStackFileIncluder.h",

        -- OGLCompiler
        "glslang/OGLCompilersDLL/InitializeDll.cpp",
        "glslang/OGLCompilersDLL/InitializeDll.h",

        -- SPIRV

        "glslang/SPIRV/GlslangToSpv.cpp",
        "glslang/SPIRV/InReadableOrder.cpp",
        "glslang/SPIRV/Logger.cpp",
        "glslang/SPIRV/SpvBuilder.cpp",
        "glslang/SPIRV/SpvPostProcess.cpp",
        "glslang/SPIRV/doc.cpp",
        "glslang/SPIRV/SpvTools.cpp",
        "glslang/SPIRV/disassemble.cpp",
        "glslang/SPIRV/bitutils.h",
        "glslang/SPIRV/spirv.hpp",
        "glslang/SPIRV/GLSL.std.450.h",
        "glslang/SPIRV/GLSL.ext.EXT.h",
        "glslang/SPIRV/GLSL.ext.KHR.h",
        "glslang/SPIRV/GlslangToSpv.h",
        "glslang/SPIRV/hex_float.h",
        "glslang/SPIRV/Logger.h",
        "glslang/SPIRV/SpvBuilder.h",
        "glslang/SPIRV/spvIR.h",
        "glslang/SPIRV/doc.h",
        "glslang/SPIRV/SpvTools.h",
        "glslang/SPIRV/disassemble.h",
        "glslang/SPIRV/GLSL.ext.AMD.h",
        "glslang/SPIRV/ GLSL.ext.NV.h",
        "glslang/SPIRV/NonSemanticDebugPrintf.h"
    }

    includedirs
    {
        "glslang/"
    }

    filter "system:linux"
		pic "On"

		systemversion "latest"
		-- staticruntime "On"

		files
		{
            "glslang/glslang/OSDependent/osinclude.h",
            "glslang/glslang/OSDependent/Unix/ossource.cpp"
		}
	filter "system:windows"
		systemversion "latest"
		-- staticruntime "On"

		files
		{
            "glslang/glslang/OSDependent/osinclude.h",
            "glslang/glslang/OSDependent/Windows/ossource.cpp"
        }
