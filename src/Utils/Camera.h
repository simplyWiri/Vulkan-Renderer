#pragma once
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>


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

private:

	void RecalculateMVP()
	{
		glm::mat4 transform = translate(glm::mat4(1.0f), position) * rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0, 0, 1));

		view = inverse(transform);
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

	void SetModel(glm::mat4 model) { this->model = model; } //RecalculateMVP(); }
	void SetView(glm::mat4 view) { this->view = view; } //RecalculateMVP(); }
	void SetProj(glm::mat4 proj) { this->proj = proj; } //RecalculateMVP(); }

	void SetPosition(glm::vec3 pos)
	{
		position = pos;
		RecalculateMVP();
	}

	void SetRotation(float rot)
	{
		rotation = rot;
		RecalculateMVP();
	}
};
