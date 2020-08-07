#pragma once
#include "../../Resources/Shader.h"
#include "vulkan.h"
#include <vector>
#include <tuple>

#include "../../Resources/ShaderManager.h"
#include "../Wrappers/Pipeline.h"

namespace Renderer
{
	struct Tether
	{
		std::string passId;

		void AddColourOutput(const std::string& resName) { }

		void RegisterDependancy(const std::string resName) { }
		void RegisterStorageIn(const std::string resName) { }

		std::vector<std::string> dependencies;
	};
}
