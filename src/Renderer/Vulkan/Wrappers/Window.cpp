#pragma once
#include "Window.h"
#include "Context.h"

namespace Renderer
{
	bool Window::buildWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(getWidth(), getHeight(), getTitle(), nullptr, nullptr);

		return isInitialised();
	}
	bool Window::buildSurface(Context* context)
	{
		if (glfwCreateWindowSurface(context->instance, window, nullptr, &surface) != VK_SUCCESS)
			return false;

		// Used for cleaning up the window surface
		setInstance(&context->instance);

		return true;
	}
	void Window::cleanup()
	{
		vkDestroySurfaceKHR(*instance, surface, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}
}