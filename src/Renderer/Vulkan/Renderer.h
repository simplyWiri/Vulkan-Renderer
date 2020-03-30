#pragma once
#include "Wrappers/Window.h"
#include "Wrappers/Context.h"

class Renderer
{
public:
	bool initialiseRenderer();

	~Renderer();
private:
	Window		window;
	Context		context;
};
