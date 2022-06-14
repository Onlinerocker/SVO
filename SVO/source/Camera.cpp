#pragma warning(disable: 4201)

#include "Camera.h"
#include "glm/gtx/quaternion.hpp"

//stolen from myself https://github.com/Onlinerocker/SokobanGameCPP/blob/main/OpenGL_Game/Camera.cpp
void Camera::update(float deltaTheta /* yaw */, float deltaPhi /* pitch */)
{
	glm::quat yaw = glm::angleAxis(glm::radians(deltaTheta / 2), glm::vec3(0, 1, 0));

	_forward = glm::conjugate(yaw) * _forward * yaw;
	_right = glm::conjugate(yaw) * _right * yaw;

	glm::quat pitch = glm::angleAxis(glm::radians(deltaPhi / 2), _right);
	_forward = glm::conjugate(pitch) * _forward * pitch;

	_up = glm::normalize(glm::cross(_forward, _right));
}

const glm::vec3& Camera::forward() const
{
	return _forward;
}

const glm::vec3& Camera::right() const
{
	return _right;
}

const glm::vec3& Camera::up() const
{
	return _up;
}