#pragma once
#include "glm/glm.hpp"
#include "vulkan.h"
#include <array>

namespace Renderer
{
	struct VertexAttributes
	{
		enum Type
		{
			singlefloat,
			vec2,
			vec3,
			vec4,
			colour32 // r8g8b8a8
		};

		static VkFormat typeToFormat(Type type)
		{
			switch (type)
			{
			case Type::singlefloat: return VK_FORMAT_R32_SFLOAT;
			case Type::vec2: return VK_FORMAT_R32G32_SFLOAT;
			case Type::vec3: return VK_FORMAT_R32G32B32_SFLOAT;
			case Type::vec4: return VK_FORMAT_R32G32B32A32_SFLOAT;
			case Type::colour32: return VK_FORMAT_B8G8R8_UNORM;
			}

			Assert(false, "Failed to recognise given type");

			return static_cast<VkFormat>(0);
		}

		struct Binding
		{
			uint32_t stride = 0;
			int binding = -1;

			bool operator ==(const Binding& other) const { return std::tie(stride, binding) == std::tie(other.stride, other.binding); }

		};

		struct Attribute
		{
			int binding = -1;
			uint32_t location = 0;
			Type type = Type::vec4;
			uint32_t offset = 0;

			bool operator ==(const Attribute& other) const { return std::tie(binding, location, type, offset) == std::tie(other.binding, other.location, other.type, other.offset); }

		};

		VertexAttributes() { }

		VertexAttributes(std::vector<Binding> bindings, std::vector<Attribute> attributes) : bindings(bindings), attributes(attributes)
		{
			for (auto& binding : bindings)
			{
				if (binding.binding != -1)
				{
					VkVertexInputBindingDescription bindingDesc = {};
					bindingDesc.binding = binding.binding;
					bindingDesc.stride = binding.stride;
					bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
					bindingDescs.push_back(bindingDesc);
				}
			}
			for (auto& attribute : attributes)
			{
				if (attribute.binding != -1)
				{
					VkVertexInputAttributeDescription attributeDesc = {};
					attributeDesc.binding = attribute.binding;
					attributeDesc.location = attribute.location;
					attributeDesc.format = typeToFormat(attribute.type);
					attributeDesc.offset = attribute.offset;

					attributeDescs.push_back(attributeDesc);
				}
			}
		}

		std::vector<VkVertexInputBindingDescription> getBindings() { return bindingDescs; };
		std::vector<VkVertexInputAttributeDescription> getAttributes() { return attributeDescs; };

		bool operator ==(const VertexAttributes& other) const { return std::tie(bindings, attributes) == std::tie(other.bindings, other.attributes); }


	public:
		std::vector<Binding> bindings;
		std::vector<Attribute> attributes;
		std::vector<VkVertexInputBindingDescription> bindingDescs;
		std::vector<VkVertexInputAttributeDescription> attributeDescs;
	};

	struct Vertex
	{
		glm::vec3 pos; // x y
		glm::vec3 colour; // r g b

		static VertexAttributes defaultVertex() { return VertexAttributes({ {sizeof(Vertex), 0} }, { {0, 0, VertexAttributes::Type::vec3, offsetof(Vertex, pos)}, {0, 1, VertexAttributes::Type::vec3, offsetof(Vertex, colour)} }); }
	};
}

namespace std
{
	template<> struct hash<Renderer::VertexAttributes>
	{
		size_t operator()(const Renderer::VertexAttributes& s) const noexcept
		{
			std::size_t bindingsSeed = s.bindings.size();
			for (auto& i : s.bindings) {
				bindingsSeed ^= (hash<uint32_t>{}(i.stride) ^ (hash<int>{}(i.binding) << 1)) + 0x9e3779b9 + (bindingsSeed << 6) + (bindingsSeed >> 2);
			}

			std::size_t attributesSeed = s.attributes.size();
			for (auto& i : s.attributes) {
				size_t h1 = hash<uint32_t>{}(i.offset) ^ (hash<int>{}(i.binding) << 1);
				size_t h2 = hash<uint32_t>{}(i.location) ^ (hash<underlying_type<Renderer::VertexAttributes::Type>::type>{}(i.type) << 1);

				attributesSeed ^= (h1 ^ (h2 << 1)) + 0x9e3779b9 + (attributesSeed << 6) + (attributesSeed >> 2);
			}

			return (attributesSeed ^ (bindingsSeed << 1));
		}
	};
}
