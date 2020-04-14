#pragma once
#include "vulkan.h"
#include "glfw3.h"

/*
	Window Struct
		- Contains the GLFW implementation of a window
		- Contains a Vulkan Surface
		- Contains cleanup calls for both
*/
namespace Renderer {
	struct Context;

	struct Window {
	public:
		GLFWwindow* window;
		VkSurfaceKHR surface;

	private:
		int width = -1, height = -1;
		bool outofDate = false;
		const char* title = "Default";
		VkInstance* instance;

	public:
		inline void initialiseWindow(int w, int h, const char* t) { width = w; height = h; title = t; }
		inline bool isInitialised() const { return width > 0 && height > 0 && window && surface; }

		inline int getWidth() const { return width; }
		inline int getHeight() const { return height; }
		inline const char* getTitle() const { return title; }
		inline void setWidth(int w) { width = w; }
		inline void setheight(int h) { height = h; }
		inline void setTitle(const char* t) { title = t; }
		inline void setInstance(VkInstance* i) { instance = i; }

		inline float getAspectRatio() const { return width / (float)height; }
		inline GLFWwindow* getWindow() const { return window; }
		inline VkSurfaceKHR getSurface() const { return surface; }
		inline bool needsResize() const { return outofDate; }

		inline void cleanup() { vkDestroySurfaceKHR(*instance, surface, nullptr); glfwDestroyWindow(window); glfwTerminate(); }
	};

	namespace Wrappers {
		// Populate a GLFWwindow instance within a Window object
		bool buildWindow(Window* window);
		// Populate a Surface instance within a Window object
		bool buildSurface(Window* window, Context* context);
	}
}