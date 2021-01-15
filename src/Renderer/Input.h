#pragma once
#include <functional>
#include <set>

struct GLFWwindow;

namespace Renderer
{
	
enum class InputPriority
{
	LOW_PRIORITY = 0, GUI = 1, GUI_INPUT = 2, GUI_FOCUS = 3, HIGH_PRIORITY = 4, DEBUGGING = 5 
};

template <class T>
struct Callback
{
	int priority;
	T callback;

	bool operator<(const Callback<T> other) const { return priority < other.priority; }
};

class Input
{
public:

	void Setup(GLFWwindow* window);
	
	// Calls glfwPollEvents();
	void PollEvents();

	// Returned value is whether this action should absorb input
	// x, y
	void RegisterMouseMoveCallBack(std::function<bool(float, float)> callback, InputPriority priority);
	// button, action
	void RegisterMouseClickCallBack(std::function<bool(int, int)> callback, InputPriority priority);
	// scroll delta
	void RegisterMouseScrollCallBack(std::function<bool(float)> callback, InputPriority priority);
	// key, action
	void RegisterKeyCallBack(std::function<bool(int, int)> callback, InputPriority priority);
	// char
	void RegisterCharCallBack(std::function<bool(unsigned int)> callback, InputPriority priority);


private:
	std::multiset<Callback<std::function<bool(float, float)>>> mouseMoveCallbacks;
	std::multiset<Callback<std::function<bool(int, int)>>> mouseClickCallbacks;
	std::multiset<Callback<std::function<bool(float)>>> mouseScrollCallbacks;
	std::multiset<Callback<std::function<bool(int, int)>>> keyCallbacks;
	std::multiset<Callback<std::function<bool(unsigned int)>>> charCallbacks;

	void RegisterGLFWCallbacks(GLFWwindow* window);

	static void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/);
	static void charCallback(GLFWwindow* window, unsigned int codepoint);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/);
	static void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
	static void mouseScrollCallback(GLFWwindow* window, double xoffset, double /*yoffset*/);
};

}
