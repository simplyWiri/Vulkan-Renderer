#pragma once
#include "glm/glm/glm.hpp"
#include "glm/glm/ext/matrix_transform.hpp"

class AnchoredCamera
{
private:
	const glm::mat4 identity = glm::mat4(1);


public:
	AnchoredCamera(glm::vec3 target, glm::vec3 up = { 0.0f, 1.0f, 0.0 }, glm::vec3 position = { 0.0f, 0.0f, 1.0f }) : target(target), up(up), position(position)
	{
		UpdateMVPMatrix();
	}

	void Rotate(glm::vec2 delta)
	{
		auto los = target - position;
		auto losLength = glm::length(los);
		auto right = glm::normalize(glm::cross(los, up));
		auto nextUp = glm::cross(right, los);

		auto fixRight = glm::normalize(glm::cross(los, up));
		auto upDotX = glm::dot(fixRight, nextUp);
		nextUp = glm::normalize(nextUp - upDotX * fixRight);
		right = glm::normalize(glm::cross(los, nextUp));
		los = glm::cross(up, right);

		// horizontal axis rotation
		auto rightHorizontal = glm::rotate(identity, delta.y, right);
		nextUp = glm::vec3(rightHorizontal * glm::vec4(nextUp, 0));
		los = glm::vec3(rightHorizontal * glm::vec4(los, 0));

		// vertical axis rotation
		auto rightUp = glm::rotate(identity, delta.x, nextUp);
		right = glm::vec3(rightUp * glm::vec4(right, 0));
		los = glm::vec3(rightUp * glm::vec4(los, 0));

		this->position = target - los * losLength;
		this->up = nextUp;

		this->view = lookAt(position, target, up);
	}

	void UpdateMVPMatrix()
	{
		view = lookAt(position, target, up);

	}

	const glm::mat4& GetModel() { return model; }
	const glm::mat4& GetView() { return view; }
	const glm::mat4& GetProj() { return proj; }

	void SetModel(glm::mat4 model) { this->model = model; } 
	void SetView(glm::mat4 view) { this->view = view; } 
	void SetProj(glm::mat4 proj) { this->proj = proj; } 

private:

	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;


	const glm::vec3 target; 
	glm::vec3 up;
	glm::vec3 position;

};