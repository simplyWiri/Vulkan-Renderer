#pragma once
#include <vector>
#include "Layer.h"

class LayerStack
{
public:

	void pushLayer(Layer* layer) { layerStack.push_back(layer); };
	void pushOverlay(Layer* layer) { layerStack.push_back(layer); }

	std::vector<Layer*>::iterator begin() { return layerStack.begin(); }
	std::vector<Layer*>::iterator end() { return layerStack.end(); }

	std::vector<Layer*>::reverse_iterator rbegin() { return layerStack.rbegin(); }
	std::vector<Layer*>::reverse_iterator rend() { return layerStack.rend(); }
private:
	std::vector<Layer*> layerStack;
};
