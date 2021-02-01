#include "InputHandler.h"
#include "glfw3.h"

namespace Renderer
{
	

	void InputHandler::Setup(GLFWwindow* window)
	{
		RegisterGLFWCallbacks(window);
	}
	void InputHandler::PollEvents()
	{
		glfwPollEvents();
	}
	
	void InputHandler::RegisterMouseMoveCallBack(std::function<bool(float, float)> callback, InputPriority priority)
	{
		mouseMoveCallbacks.emplace(Callback<std::function<bool(float,float)>>{static_cast<int>(priority), std::move(callback)});
	}
	void InputHandler::RegisterMouseClickCallBack(std::function<bool(int, int)> callback, InputPriority priority)
	{
		mouseClickCallbacks.emplace(Callback<std::function<bool(int,int)>>{static_cast<int>(priority), std::move(callback)});
	}
	void InputHandler::RegisterMouseScrollCallBack(std::function<bool(float)> callback, InputPriority priority)
	{
		mouseScrollCallbacks.emplace(Callback<std::function<bool(float)>>{static_cast<int>(priority), std::move(callback)});
	}
	void InputHandler::RegisterKeyCallBack(std::function<bool(int, int)> callback, InputPriority priority)
	{
		keyCallbacks.emplace(Callback<std::function<bool(int, int)>>{static_cast<int>(priority), std::move(callback)});
	}
	void InputHandler::RegisterCharCallBack(std::function<bool(unsigned int)> callback, InputPriority priority)
	{
		charCallbacks.emplace(Callback<std::function<bool(unsigned int)>>{static_cast<int>(priority), std::move(callback)});
	}
	void InputHandler::RegisterGLFWCallbacks(GLFWwindow* window)
	{
		glfwSetWindowUserPointer(window, this);
		
		glfwSetMouseButtonCallback(window, mouseButtonCallback);
		glfwSetCursorPosCallback(window, mouseMoveCallback);
		glfwSetScrollCallback(window, mouseScrollCallback);
		glfwSetKeyCallback(window, keyCallback);
	}
	void InputHandler::keyCallback(GLFWwindow* window, int key, int, int action, int)
	{
		auto* input = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

		for (const auto& keyCallback : input->keyCallbacks)
		{
			if(keyCallback.callback(key, action)) return;
		}
	}

	void InputHandler::charCallback(struct GLFWwindow* window, unsigned codepoint)
	{
		auto* input = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

		for (const auto& keyCallback : input->charCallbacks)
		{
			if(keyCallback.callback(codepoint)) return;
		}
	}

	void InputHandler::mouseButtonCallback(GLFWwindow* window, int button, int action, int)
	{
		auto* input = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

		for (const auto& mouseClickCallback : input->mouseClickCallbacks)
		{
			if(mouseClickCallback.callback(button, action)) return;
		}
	}
	void InputHandler::mouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto* input = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

		for (const auto& mouseMoveCallback : input->mouseMoveCallbacks)
		{
			if(mouseMoveCallback.callback(static_cast<float>(xpos), static_cast<float>(ypos))) return;
		}
	}
	void InputHandler::mouseScrollCallback(GLFWwindow* window, double xoffset, double)
	{
		auto* input = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

		for (const auto& mouseScrollCallback : input->mouseScrollCallbacks)
		{
			if(mouseScrollCallback.callback(static_cast<float>(xoffset))) return;
		}
	}
}