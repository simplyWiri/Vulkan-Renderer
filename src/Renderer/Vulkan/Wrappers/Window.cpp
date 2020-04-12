#pragma once
#include "Window.h"
#include "Context.h"

namespace Renderer {
	namespace Wrappers {

		bool buildWindow(Window* window)
		{
			glfwInit();
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

			window->window = glfwCreateWindow(window->getWidth(), window->getHeight(), window->getTitle(), nullptr, nullptr);

			return window->isInitialised();
		}

		bool buildSurface(Window* window, Context* context)
		{
			if (glfwCreateWindowSurface(context->instance, window->window, nullptr, &window->surface) != VK_SUCCESS)
				return false;

			// Used for cleaning up the window surface
			window->setInstance(&context->instance);

			return true;
		}
	}
}