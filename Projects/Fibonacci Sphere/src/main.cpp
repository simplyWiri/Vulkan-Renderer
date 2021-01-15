#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS


#include "Renderer/Core.h"
#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include "GUI.h"
#include "Sphere.h"
#include "Utils/Camera.h"

using namespace Renderer;

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

int main()
{
	Settings s = {};
	s.width = 1200;
	s.height = 840;

	auto renderer = std::make_unique<Core>(s);
	GUI gui;

	renderer->Initialise();
	gui.initialise(renderer.get());

	auto program = renderer->GetShaderManager()->getProgram({ renderer->GetShaderManager()->defaultVertex(), renderer->GetShaderManager()->defaultFragment() });
	program->InitialiseResources(renderer->GetDevice()->GetDevice());

	DescriptorSetKey descriptorSetKey = { program };
	const auto vert = Vertex::defaultVertex();

	float yaw = -90.0f;
	float pitch = 0.0f;

	auto cam = Camera{ 0, 1200, 0, 800 };

	cam.SetPosition( {0, 0, 3});
	cam.SetFront({0,0,-1});

	cam.SetModel(rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
	cam.SetProj(glm::perspective(glm::radians(45.0f), 1200 / 800.0f, 0.1f, 10.0f));

	auto* sp = new Sphere(renderer->GetAllocator(), 50000);
	UniformBufferObject ubo = {};

	int points = 0;
	bool fibo = false;
	bool lock = false;

	float lastX = s.width/2, lastY = s.height/2;
	bool active = false;

	auto mouseMoveCallback = [&](float xpos, float ypos)
	{
	    float xoffset = xpos - lastX;
	    float yoffset = ypos - lastY; 
	    lastX = xpos;
	    lastY = ypos;

		if(!active) return false;

	    const float sensitivity = 0.1f;
	    xoffset *= sensitivity;
	    yoffset *= sensitivity;

	    yaw += xoffset;
	    pitch += yoffset;

	    if(pitch > 89.0f) pitch = 89.0f;
	    if(pitch < -89.0f) pitch = -89.0f;

	    glm::vec3 direction;
	    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	    direction.y = sin(glm::radians(pitch));
	    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		cam.SetFront(normalize(direction));

		return false;
	};

	auto mouseClickCallback = [&active](int button, int action)
	{
		if(button != 0) return false;

		if(action == GLFW_RELEASE) active = false;
		else active = true;

		return false;
	};

	renderer->GetInputHandler()->RegisterMouseClickCallBack(mouseClickCallback, InputPriority::LOW_PRIORITY);
	renderer->GetInputHandler()->RegisterMouseMoveCallBack(mouseMoveCallback, InputPriority::LOW_PRIORITY);

	renderer->GetRendergraph()->AddPass(PassDesc()
		.SetName("GPU Drawing Triangle")
		.SetInitialisationFunc([&descriptorSetKey](Tether& tether) { tether.GetDescriptorCache()->WriteBuffer(descriptorSetKey, "ubo"); })
		.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
		{
			{
				static auto startTime = std::chrono::high_resolution_clock::now();

				auto currentTime = std::chrono::high_resolution_clock::now();
				float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
				
				float camSpeed = 2.5f * frameInfo.delta;

				if (glfwGetKey(context.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
					glfwSetWindowShouldClose(context.window, true);
				
			    if (glfwGetKey(context.window, GLFW_KEY_W) == GLFW_PRESS) 
			        cam.UpdatePosition(camSpeed * cam.GetFront());
			    if (glfwGetKey(context.window, GLFW_KEY_S) == GLFW_PRESS)
					cam.UpdatePosition(camSpeed * cam.GetFront() * glm::vec3(-1));
			    if (glfwGetKey(context.window, GLFW_KEY_A) == GLFW_PRESS)
					cam.UpdatePosition(normalize(cross(cam.GetFront(), cam.up)) * camSpeed * glm::vec3(-1));
			    if (glfwGetKey(context.window, GLFW_KEY_D) == GLFW_PRESS)
					cam.UpdatePosition(normalize(cross(cam.GetFront(), cam.up)) * camSpeed);

				ubo.model = cam.GetModel();
				ubo.view = cam.GetView(); 
				ubo.proj = cam.GetProj();

				context.GetDescriptorSetCache()->SetResource(descriptorSetKey, "ubo", &ubo, sizeof(UniformBufferObject));
			}

			{
				if (!lock && points < 50000) { sp->ReCalculate(points = (points + 1 > 50000) ? 50000 : points + 1); }
				if (ImGui::SliderInt("Points", &points, 1000, 50000)) sp->ReCalculate(points);
				if (ImGui::Checkbox("Use Fibonacci", &fibo)) sp->SetFibo(fibo);
				ImGui::Checkbox("Lock Recalcs", &lock);
			}

			context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
				descriptorSetKey.program);

			context.GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);

			sp->Draw(buffer);

			context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
				descriptorSetKey.program);

			context.GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);

			sp->DrawSweepline(buffer);
		}));


	gui.AddToGraph(renderer->GetRendergraph());

	renderer->GetRendergraph()->Initialise();

	while (renderer->Run()) { renderer->GetRendergraph()->Execute(); }

	vkDeviceWaitIdle(*renderer->GetDevice());

	sp->Cleanup();
	delete sp;
	delete program;
	delete gui.key.program;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{

}  