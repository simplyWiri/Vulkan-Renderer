#pragma once
#include "Shader.h"
#include <unordered_map>

namespace std
{
	template<> struct hash<std::pair<Renderer::ShaderType, std::string>>
	{
		std::size_t operator()(const std::pair<Renderer::ShaderType, std::string>& s) const noexcept
		{
			return std::hash<string>{}(s.second) ^ (std::hash<size_t>{}(static_cast<std::size_t>(s.first)) << 1);
		}
	};
}

namespace Renderer
{
	class ShaderManager
	{
	private:
		const std::pair<ShaderType, std::string> defVert = { ShaderType::Vertex, "resources/VertexShader.vert" };
		const std::pair<ShaderType, std::string> defFrag = { ShaderType::Fragment,  "resources/FragmentShader.frag" };
	public:
		ShaderManager()
		{
			shaders.emplace(defVert, new Shader(defVert.first, defVert.second));
			shaders.emplace(defFrag, new Shader(defFrag.first, defFrag.second));
		}

		~ShaderManager()
		{
			auto it = shaders.begin();

			while (it != shaders.end())
			{
				delete it->second;
				++it;
			}
		}

		Shader* defaultVertex() { return shaders[defVert]; }
		Shader* defaultFragment() { return shaders[defFrag]; }


		Shader* get(const ShaderType& type, const std::string& path)
		{
			auto& value = shaders[{type, path}];
			if (!value)
			{
				value = new Shader(type, path);
			}

			return value;
		}

		std::unordered_map<std::pair<ShaderType, std::string>, Shader*> shaders;
	};

}