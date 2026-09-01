#include <Augusta/Camera.h>
#include <Augusta/InputManager.h>
#include <glm/gtc/matrix_transform.hpp>

namespace aug
{
	Camera::Camera(SCameraDesc desc)
	{
		m_Position = desc._position;
		m_ddegYaw = desc._yaw;
		m_ddegPitch = desc._pitch;
		m_ddegRoll = desc._roll;
		m_dZNear = desc._znear;
		m_dZFar = desc._zfar;
		m_ddegVFov = desc._deg_vfov;
		m_dAspect = desc._aspect;
		m_fSpeed = desc._speed;
		m_fSensitivity = desc._sensitivity;

		ComputeCamera();

		Debug::RegisterDebugee("Debug", "Camera", std::bind(&Camera::DrawDebug, this));
	}

	Camera::~Camera()
	{
	}

	void Camera::ComputeCamera()
	{
		ComputeViewMatrix();
		ComputeProjectionMatrix();
	}

	void Camera::ProcessEvents(float fDeltaT)
	{
		double deltax, deltay;
		glm::dvec2 pos = InputManager::GetMousePosition();

		if (m_Input.firstInput)
		{
			m_Input.firstInput = false;
			m_Input.cursor_x = pos.x;
			m_Input.cursor_y = pos.y;
		}

		deltax = pos.x - m_Input.cursor_x;
		deltay = pos.y - m_Input.cursor_y;
		m_Input.cursor_x = pos.x;
		m_Input.cursor_y = pos.y;
		if (InputManager::InputPressed(GLFW_MOUSE_BUTTON_1))
		{
			m_ddegYaw += deltax * m_dAspect * m_fSensitivity;
			m_ddegPitch += deltay * m_fSensitivity;
		}

		if (InputManager::InputPressed(GLFW_MOUSE_BUTTON_4))
			m_Input.mouseSpeed += 1;

		if (InputManager::InputPressed(GLFW_MOUSE_BUTTON_5))
			m_Input.mouseSpeed -= 1;

		if (InputManager::InputPressed(GLFW_MOUSE_BUTTON_2))
			m_Input.mouseSpeed = 0;

		if (InputManager::InputReleased(GLFW_KEY_LEFT_SHIFT))
			m_Input.moveUpFlag = 0;
		if (InputManager::InputReleased(GLFW_KEY_LEFT_CONTROL))
			m_Input.moveUpFlag = 0;

		if (InputManager::InputPressed(GLFW_KEY_LEFT_SHIFT))
			m_Input.moveUpFlag = 1;
		if (InputManager::InputPressed(GLFW_KEY_LEFT_CONTROL))
			m_Input.moveUpFlag = -1;

		if (InputManager::InputReleased(GLFW_KEY_W))
			m_Input.moveForwardFlag = 0;
		if (InputManager::InputReleased(GLFW_KEY_S))
			m_Input.moveForwardFlag = 0;
		if (InputManager::InputReleased(GLFW_KEY_A))
			m_Input.moveRightFlag = 0;
		if (InputManager::InputReleased(GLFW_KEY_D))
			m_Input.moveRightFlag = 0;

		if (InputManager::InputPressed(GLFW_KEY_W))
			m_Input.moveForwardFlag = 1;
		if (InputManager::InputPressed(GLFW_KEY_S))
			m_Input.moveForwardFlag = -1;
		if (InputManager::InputPressed(GLFW_KEY_A))
			m_Input.moveRightFlag = -1;
		if (InputManager::InputPressed(GLFW_KEY_D))
			m_Input.moveRightFlag = 1;

		if (InputManager::InputPressed(GLFW_KEY_Q))
			m_ddegRoll += 1.;
		if (InputManager::InputPressed(GLFW_KEY_E))
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

	void Camera::DrawDebug(void* pObject)
	{
		Camera* pCam = reinterpret_cast<Camera*>(pObject);
		if(pCam)
		{
			ImGui::SliderFloat("Speed", &pCam->m_fSpeed, 0.1f, 1000.f);
			ImGui::SliderFloat("Sensitivity", &pCam->m_fSensitivity, 0.001f, 1.f);

			float fFOV = static_cast<float>(pCam->m_ddegVFov);
			if (ImGui::SliderFloat("FOV", &fFOV, 10.f, 120.f))
				pCam->m_ddegVFov = fFOV;

			float fZNear = static_cast<float>(pCam->m_dZNear);
			if (ImGui::SliderFloat("ZNear", &fZNear, 0.00001f, 0.1f))
				pCam->m_dZNear = fZNear;

			float fZFar = static_cast<float>(pCam->m_dZFar);
			if (ImGui::SliderFloat("ZFar", &fZFar, 10.f, 10000.f))
				pCam->m_dZFar = fZFar;
		}
	}
}
