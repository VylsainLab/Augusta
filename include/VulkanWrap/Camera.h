#ifndef VKW_CAMERA_H
#define VKW_CAMERA_H

//#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <VulkanWrap/Application.h>

namespace vkw
{
	struct SCameraDesc
	{
		glm::dvec3 position = glm::dvec3(0., 0., 0.);
		glm::dvec3 direction = glm::dvec3(0., 0., -1.);
		glm::dvec3 up = glm::dvec3(0., 1., 0.);
		glm::dvec3 right = glm::dvec3(-1., 0., 0.);

		double znear = 0.0001;
		double zfar = 100.;
		double aspect = 1.;
		double deg_vfov = 60.;

		float speed = 1.f;
		float sensitivity = 1.f;
	};

	class Camera : public IGLFWEventObserver
	{
	public:
		Camera() {}
		Camera(SCameraDesc desc);
		
		void ComputeCamera();
		virtual void ProcessEvents(GLFWwindow* window) override;

		void SetSpeed(float s) { m_fSpeed = s; }	

		glm::dmat4 GetViewMatrix() { return m_ViewMatrix; }
		glm::dmat4 GetProjectionMatrix() { return m_ProjectionMatrix; }

	private:

		virtual void ComputeViewMatrix();
		virtual void ComputeProjectionMatrix();

		virtual void ComputeMovement();
		virtual void ApplyMovement();

		glm::dvec3 m_Position = glm::dvec3(0., 0., 0.);
		glm::dvec3 m_Direction = glm::dvec3(0., 0., -1.);
		glm::dvec3 m_Up = glm::dvec3(0., 1., 0.);
		glm::dvec3 m_Right = glm::dvec3(-1., 0., 0.);
		glm::dmat4 m_ViewMatrix = glm::dmat4(1.);
		glm::dmat4 m_OrientationMatrix = glm::dmat4(1.);

		double m_ddegPitch = 0.;
		double m_ddegRoll = 0.;
		double m_ddegYaw = 0.;

		double m_dZNear = 0.0001;
		double m_dZFar = 100.;
		double m_dAspect = 0.;
		double m_ddegVFov = 60.;
		glm::dmat4 m_ProjectionMatrix = glm::dmat4(1.);

		float m_fSpeed = 1.f;
		float m_fSensitivity = 0.1f;
		glm::dvec3 m_Movement = glm::dvec3(0.);

		struct Input
		{
			bool firstInput = true;
			int moveUpFlag = 0;
			int moveForwardFlag = 0;
			int moveRightFlag = 0;
			int mouseSpeed = 0;
			double cursor_x;
			double cursor_y;
		};

		Input m_Input;
	};
}

#endif