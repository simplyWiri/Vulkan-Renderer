#include "glfw/include/GLFW/glfw3.h"

#include "InputHandler.h"

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
		
		glfwSetMouseButtonCallback(window, MouseButtonCallback);
		glfwSetCursorPosCallback(window, MouseMoveCallback);
		glfwSetScrollCallback(window, MouseScrollCallback);
		glfwSetKeyCallback(window, KeyCallback);
	}
	void InputHandler::KeyCallback(GLFWwindow* window, int key, int, int action, int)
	{
		auto* input = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

		for (const auto& keyCallback : input->keyCallbacks)
		{
			if(keyCallback.callback(key, action)) return;
		}
	}

	void InputHandler::CharCallback(struct GLFWwindow* window, unsigned codepoint)
	{
		auto* input = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

		for (const auto& keyCallback : input->charCallbacks)
		{
			if(keyCallback.callback(codepoint)) return;
		}
	}

	void InputHandler::MouseButtonCallback(GLFWwindow* window, int button, int action, int)
	{
		auto* input = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

		for (const auto& mouseClickCallback : input->mouseClickCallbacks)
		{
			if(mouseClickCallback.callback(button, action)) return;
		}
	}
	void InputHandler::MouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto* input = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

		for (const auto& mouseMoveCallback : input->mouseMoveCallbacks)
		{
			if(mouseMoveCallback.callback(static_cast<float>(xpos), static_cast<float>(ypos))) return;
		}
	}
	void InputHandler::MouseScrollCallback(GLFWwindow* window, double, double yoffset)
	{
		auto* input = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

		for (const auto& mouseScrollCallback : input->mouseScrollCallbacks)
		{
			if(mouseScrollCallback.callback(static_cast<float>(yoffset))) return;
		}
	}
}