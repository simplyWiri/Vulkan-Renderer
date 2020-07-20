#pragma once
#include "Event.h"

class Layer
{
public:
	virtual void Attach() = 0; // when attached to layerstack
	virtual void Detach() = 0; // when detached from layertstack
	virtual void Add() = 0;
	virtual void OnEvent(Event& event) = 0; // when an event is passed to it
	virtual void Clear() = 0;
	virtual void Flush() = 0; // flushing events to renderer
};
