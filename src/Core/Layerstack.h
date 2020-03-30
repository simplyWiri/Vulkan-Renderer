#pragma once
#include <vector>
#include "glm/glm.hpp"

enum class EventType {
	MouseMove, MousePress, MouseScroll,
	KeyPressed,
	WindowResize, WindowClose
};

class Event {
public:
	Event(EventType t, bool p = false) : persistant(p), type(t) {}; // creates an event of EventType

	void						setHandled(bool h) { if (!persistant) handled = h; }
	bool						isHandled() { return handled; }
	EventType					type;
private:
	bool						persistant;
	bool						handled = false;
};

/*
	Mouse Related Events
	- MouseMove, MousePress, MouseScroll
*/
class EventMouseMove : public Event {
public:
	EventMouseMove(EventType t, float x, float y, bool p = false) : mousePos({ x, y }), Event(t, p) {}

	inline glm::vec2			getMousePos() { return mousePos; }
	inline float				getMouseX() { return mousePos.x; }
	inline float				getMouseY() { return mousePos.y; }

private:
	glm::vec2					mousePos;
};
class EventMousePress : public Event {
public:
	EventMousePress(EventType t, int b, int a, bool p = false) : button(b), action(a), Event(t, p) {}

	inline int					getButton() { return button; }
	inline int					getAction() { return action; }

private:
	int							button; // LEFT, RIGHT (MIDDLE?)
	int							action; // PRESS, RELEASE
};
class EventMouseScroll : public Event {
public:
	EventMouseScroll(EventType t, int x, int y, bool p = false) : mouseOffsets({ x, y }), Event(t, p) {}

	inline glm::vec2			getOffset() { return mouseOffsets; }
	inline float				getOffsetX() { return mouseOffsets.x; }
	inline float				getOffsetY() { return mouseOffsets.y; }

private:
	glm::vec2					mouseOffsets;
};
/*
	Keyboard Related Events
	- KeyInput
*/
class EventKeyPressed : public Event {
public:
	EventKeyPressed(EventType t, int k, int a, int m, bool p = false) : key(k), action(a), mod(m), Event(t, p) {}

	inline int					getKey() { return key; }
	inline int					getAction() { return action; }
	inline int					getMods() { return mod; }

private:
	int							key;
	int							action;
	int							mod;
};
/*
	Window Related Events
	- WindowResize, WindowClose
*/
class EventWindowResize : public Event {
public:
	EventWindowResize(EventType t, float x, float y) : windowSize({ x, y }), Event(t, true) {}

	inline glm::vec2			getWindowSize() { return windowSize; }
	inline float				getWindowSizeX() { return windowSize.x; }
	inline float				getWindowSizeY() { return windowSize.y; }

private:
	glm::vec2					windowSize;
};
class EventWindowClose : public Event {
public:
	EventWindowClose(EventType t) : Event(t, true) {};
};

class Layer {
public:
	virtual void						Attach() = 0; // when attached to layerstack
	virtual void						Detach() = 0; // when detached from layertstack
	virtual void						Add() = 0;
	virtual void						OnEvent(Event& event) = 0; // when an event is passed to it
	virtual void						Clear() = 0;
	virtual void						Flush() = 0; // flushing events to renderer
};

class LayerStack
{
public:

	void									pushLayer(Layer* layer) { layerStack.push_back(layer); };
	void									pushOverlay(Layer* layer) { layerStack.push_back(layer); }

	std::vector<Layer*>::iterator			begin() { return layerStack.begin(); }
	std::vector<Layer*>::iterator			end() { return layerStack.end(); }

	std::vector<Layer*>::reverse_iterator 	rbegin() { return layerStack.rbegin(); }
	std::vector<Layer*>::reverse_iterator 	rend() { return layerStack.rend(); }
private:
	std::vector<Layer*> layerStack;
};
