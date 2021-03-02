#pragma once
#include "glm/glm/ext/matrix_clip_space.hpp"
#include "glm/glm/ext/matrix_transform.hpp"


class Camera
{
public:
	const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0);

private:
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	
	float rotation;
	glm::vec3 position;
	glm::vec3 front;

private:

	void RecalculateMVP()
	{
		view = lookAt(position, position + front, up);
	}

	void UpdateProjection(float left, float right, float bottom, float top)
	{
		proj = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
		proj[1][1] *= -1;

		RecalculateMVP();
	}

public:

	Camera(float left, float right, float bottom, float top) { UpdateProjection(left, right, bottom, top); }

	const glm::mat4& GetModel() { return model; }
	const glm::mat4& GetView() { return view; }
	const glm::mat4& GetProj() { return proj; }
	const glm::vec3& GetFront() { return front; }

	void SetModel(glm::mat4 model) { this->model = model; } //RecalculateMVP(); }
	void SetView(glm::mat4 view) { this->view = view; } //RecalculateMVP(); }
	void SetProj(glm::mat4 proj) { this->proj = proj; } //RecalculateMVP(); }

	void SetPosition(glm::vec3 pos)
	{
		position = pos;
		RecalculateMVP();
	}

	void UpdatePosition(glm::vec3 change)
	{
		position += change;
		RecalculateMVP();
	}

	void SetFront(glm::vec3 front)
	{
		this->front = front;
		RecalculateMVP();
	}

	void SetRotation(float rot)
	{
		rotation = rot;
		RecalculateMVP();
	}
};
