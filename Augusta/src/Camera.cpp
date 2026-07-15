#include <Augusta/Camera.h>
#include <glm/gtc/matrix_transform.hpp>

namespace aug
{
	Camera::Camera(SCameraDesc desc)
	{
		m_Position = desc._position;
		m_Direction = desc._direction;
		m_Up = desc._up;
		m_Right = desc._right;
		m_dZNear = desc._znear;
		m_dZFar = desc._zfar;
		m_ddegVFov = desc._deg_vfov;
		m_dAspect = desc._aspect;
		m_fSpeed = desc._speed;
		m_fSensitivity = desc._sensitivity;

		ComputeCamera();
	}

	void Camera::ComputeCamera()
	{
		ComputeViewMatrix();
		ComputeProjectionMatrix();
	}

	void Camera::ProcessEvents(GLFWwindow* window, float fDeltaT)
	{
		double x, y, deltax, deltay;
		glfwGetCursorPos(window, &x, &y);

		if (m_Input.firstInput)
		{
			m_Input.firstInput = false;
			m_Input.cursor_x = x;
			m_Input.cursor_y = y;
		}

		deltax = x - m_Input.cursor_x;
		deltay = y - m_Input.cursor_y;
		m_Input.cursor_x = x;
		m_Input.cursor_y = y;
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
		{
			m_ddegYaw += deltax * m_dAspect * m_fSensitivity;
			m_ddegPitch += deltay * m_fSensitivity;
		}

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_4) == GLFW_PRESS)
			m_Input.mouseSpeed += 1;

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_5) == GLFW_PRESS)
			m_Input.mouseSpeed -= 1;

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
			m_Input.mouseSpeed = 0;

		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
			m_Input.moveUpFlag = 0;
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_RELEASE)
			m_Input.moveUpFlag = 0;

		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			m_Input.moveUpFlag = 1;
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			m_Input.moveUpFlag = -1;

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE)
			m_Input.moveForwardFlag = 0;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE)
			m_Input.moveForwardFlag = 0;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE)
			m_Input.moveRightFlag = 0;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE)
			m_Input.moveRightFlag = 0;

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			m_Input.moveForwardFlag = 1;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			m_Input.moveForwardFlag = -1;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			m_Input.moveRightFlag = -1;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			m_Input.moveRightFlag = 1;

		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			m_ddegRoll += 1.;
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			m_ddegRoll -= 1.;

		ComputeMovement(fDeltaT);
	}

	void Camera::ComputeViewMatrix()
	{
		ApplyMovement();

		glm::dmat4 rot(1.);
		rot = glm::rotate(rot, glm::radians(-m_ddegYaw), glm::dvec3(0., 1., 0.));
		rot = glm::rotate(rot, glm::radians(-m_ddegPitch), glm::dvec3(1., 0., 0.));
		rot = glm::rotate(rot, glm::radians(m_ddegRoll), glm::dvec3(0., 0., 1.));
		m_OrientationMatrix = rot;

		glm::dvec3 dir = glm::dvec3(rot * glm::dvec4(m_Direction, 0.));
		glm::dvec3 up = glm::dvec3(rot * glm::dvec4(m_Up, 0.));

		m_ViewMatrix = glm::lookAt(m_Position, m_Position + dir, up);
	}

	void Camera::ComputeProjectionMatrix()
	{
		m_ProjectionMatrix = glm::perspective(glm::radians(m_ddegVFov), m_dAspect, m_dZNear, m_dZFar);
		m_ProjectionMatrix[1][1] *= -1; //GLM was designed for OpenGL and Y clip coordinates is reversed
	}

	void Camera::ComputeMovement(float fDeltaT)
	{
		glm::dvec3 vDir = glm::dvec3(m_OrientationMatrix * glm::dvec4(m_Direction, 0.));
		glm::dvec3 vUp = glm::dvec3(m_OrientationMatrix * glm::dvec4(m_Up, 0.));
		glm::dvec3 vRight = glm::cross(vDir, vUp);
		m_Movement = static_cast<double>(m_fSpeed) * fDeltaT *
			(static_cast<double>(m_Input.mouseSpeed + m_Input.moveForwardFlag) * vDir 
				+ static_cast<double>(m_Input.moveUpFlag) * vUp 
				+ static_cast<double>(m_Input.moveRightFlag) * vRight);
	}

	void Camera::ApplyMovement()
	{
		m_Position += m_Movement;
	}
}
