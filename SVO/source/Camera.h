#pragma once
#include <glm/glm.hpp>

class Camera
{
public:
	Camera() = default;

	Camera(const Camera& other) = default;
	Camera& operator=(const Camera& other) = default;

	Camera(Camera&& other) = default;
	Camera& operator=(Camera&& other) = default;

	void update(float deltaTheta, float deltaPhi);

	const glm::vec3& forward() const;
	const glm::vec3& right() const;
	const glm::vec3& up() const;

private:
	glm::vec3 _forward{ 0, 0, 1 };
	glm::vec3 _right{ 1, 0, 0 };
	glm::vec3 _up{ 0, 1, 0 };
};

