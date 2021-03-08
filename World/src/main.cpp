#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION

#include "Renderer/Core.h"
#include "imgui/imgui.h"
#include "glm/glm/gtc/matrix_transform.hpp"
#include "GUI.h"
#include "Utils/Camera.h"
#include "World/Generation/PlanetGenerator.h"
#include "World/PlanetRenderer.h"
#include "tracy/Tracy.hpp"
#include <Utils\AnchoredCamera.h>
#include "volk/volk.h"


using namespace Renderer;
using namespace World::Generation;

void* operator new(std :: size_t count)
{
	auto ptr = malloc(count);
	TracyAllocS(ptr , count, 12);
	return ptr;
}
void operator delete(void* ptr) noexcept
{
	TracyFreeS(ptr, 12);
	free(ptr);
}

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

int main()
{
	Settings s = {};
	s.width = 1280;
	s.height = 720;
	s.vsync = false;

	auto renderer = std::make_unique<Core>(s);
	GUI gui;

	renderer->Initialise();
	
	
	gui.initialise(renderer.get());

	auto program = renderer->GetShaderManager()->getProgram({ renderer->GetShaderManager()->DefaultVertex(), renderer->GetShaderManager()->DefaultFragment() });
	program->InitialiseResources();

	DescriptorSetKey descriptorSetKey = { program };
	auto vert = Vertex::defaultVertex();

	float fov = 45.0f;
	
	auto anchoredCam = AnchoredCamera( {0, 0, 0}, {0, 1, 0}, { 0, 0, 3});

	anchoredCam.SetModel(rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
	anchoredCam.SetProj(glm::perspective(glm::radians(fov), s.width / static_cast<float>(s.height), 0.1f, 10.0f));

	auto* gen = new PlanetGenerator(500, false);
	auto planetRenderer = new World::PlanetRenderer(gen, renderer->GetAllocator());
	UniformBufferObject ubo = {};


	float lastX = s.width/2, lastY = s.height/2;
	bool active = false;
	float camSpeed = 2.5f;

	auto mouseMoveCallback = [&](float xpos, float ypos)
	{
	    auto xOffset = lastX - xpos;
	    auto yOffset = ypos - lastY; // y flipped in vulkan
		
	    lastX = xpos;
	    lastY = ypos;

		if(!active) return false;

	    xOffset *= camSpeed;
	    yOffset *= camSpeed;

		auto zoomMultiplier = glm::clamp(fov / 45.0f, 0.25f, 1.0f);
		
		anchoredCam.Rotate({ xOffset * zoomMultiplier, yOffset * zoomMultiplier});

		return false;	
	};

	auto mouseClickCallback = [&active](int button, int action)
	{
		if(button != 0) return false;

		if(action == GLFW_RELEASE) active = false;
		else active = true;

		return false;
	};

	auto scrollCallback = [&](float yOffset)
	{
		fov -= yOffset;

		fov = glm::clamp(fov, 1.0f, 45.0f);

		anchoredCam.SetProj(glm::perspective(glm::radians(fov), static_cast<float>(s.width) / static_cast<float>(s.height), 0.1f, 10.0f));

		return false;
	};

	renderer->GetInputHandler()->RegisterMouseClickCallBack(mouseClickCallback, InputPriority::LOW_PRIORITY);
	renderer->GetInputHandler()->RegisterMouseMoveCallBack(mouseMoveCallback, InputPriority::LOW_PRIORITY);
	renderer->GetInputHandler()->RegisterMouseScrollCallBack(scrollCallback, InputPriority::LOW_PRIORITY);

	bool pause = false;
	bool showBeachline = true;
	bool showSweepline = true;
	bool showSites = true;
	bool showVoronoiEdges = true;
	bool showDelanuayEdges = false;
	bool showFaces = false;
	int points = 500;
	auto twoPi = 2 * 3.14159265359f;
	float step = twoPi / 16; // 16s to finish
	bool random = false;

	RenderGraph::GraphBuilder rgBuilder{};

	gen->Step(2 * 3.14159265359f);

	gui.AddToGraph(renderer.get(), &rgBuilder);

	
	rgBuilder.AddPass("Drawing Sphere", RenderGraph::QueueType::Graphics)
		.WriteImage("depth", 0, VK_ATTACHMENT_LOAD_OP_CLEAR, 0, RenderGraph::ImageInfo { .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, .format = VK_FORMAT_D32_SFLOAT, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL })
		.WriteToBackbuffer(VK_ATTACHMENT_LOAD_OP_CLEAR)
		.SetInitialisationFunc([&](auto* graph)	
		{
			renderer->GetDescriptorSetCache()->WriteBuffer(descriptorSetKey, "ubo");
		})
		.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, RenderGraph::GraphContext& context)
		{
			{
				static auto startTime = std::chrono::high_resolution_clock::now();

				auto currentTime = std::chrono::high_resolution_clock::now();
				float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
				
				camSpeed = 1.25f * frameInfo.delta;

			    if (glfwGetKey(context.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(context.window, true);

				ubo.model = anchoredCam.GetModel();
				ubo.view = anchoredCam.GetView(); 
				ubo.proj = anchoredCam.GetProj();

				ImGui::Text("Generation Options");
				{
					ImGui::SliderInt("Points", &points, 100, 8000);
					ImGui::Checkbox("Random Point Distribution", &random);
					
					if(ImGui::Button("New"))
					{
						delete gen;
						gen = new PlanetGenerator(points, random);

						planetRenderer->Reset(gen);
					}

					ImGui::SameLine();

					if(ImGui::Button(pause ? "Unpause" : "Pause")) pause = !pause;

					ImGui::SameLine();

					if(ImGui::Button("Finish")) gen->Step(2 * 3.14159265359f);
					
					if(!gen->Finished()) 
					{
						ImGui::Text("Site Event Left %d\nCircle Events Left %d\nSweepline Coverage %.2f", gen->siteEventQueue.size(), gen->circleEventQueue.size(), gen->sweepline / (2.0f * 3.14159265359f));
						ImGui::SliderFloat("Step", &step, twoPi / 40.0f, twoPi / 4.0f);
					}
				}

				ImGui::NewLine();

				ImGui::Text("Rendering Options");
				{
					if(!gen->Finished())
					{
						ImGui::Checkbox("Draw Beachline", &showBeachline);
						ImGui::Checkbox("Draw Sweepline", &showSweepline);
					}
					ImGui::Checkbox("Draw Sites", &showSites);
					ImGui::Checkbox("Draw Voronoi Edges", &showVoronoiEdges);
					ImGui::Checkbox("Draw Delanauy", &showDelanuayEdges);
					
					if(gen->Finished()) ImGui::Checkbox("Draw Voronoi Faces", &showFaces);
				}
				
				renderer->GetDescriptorSetCache()->SetResource(descriptorSetKey, "ubo", &ubo, sizeof(UniformBufferObject));
			}

			renderer->GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);

			planetRenderer->SetFrameState(renderer.get(), buffer, &context, &vert, &descriptorSetKey);
			
			if(showFaces)
			{
				planetRenderer->DrawVoronoiFaces();
			}
			
			if(!gen->Finished())
			{
				if(showBeachline)
				{
					planetRenderer->DrawBeachline();
				}
				if(showSweepline)
				{
					planetRenderer->DrawSweepline();
				}
			}
			
			if(showSites)
			{
				planetRenderer->DrawSites();
			}
			if(showVoronoiEdges)
			{
				planetRenderer->DrawVoronoiEdges();
			}

			if(showDelanuayEdges)
			{
				planetRenderer->DrawDelanuayEdges();
			}

			
			if(!pause && !gen->Finished())
				gen->Step(( gen->sweepline > 3.14159265359f ? step * 4 : step ) * static_cast<float>(frameInfo.delta));
		});

	auto* rg = renderer->CreateRenderGraph(rgBuilder);

	while (renderer->Run())
	{
		rg->Execute();
	}

	vkDeviceWaitIdle(renderer->GetDevice()->GetDevice());

	delete gen;
	delete planetRenderer;
	delete program;
	delete gui.key.program;
}
