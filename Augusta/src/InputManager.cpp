#include <Augusta/InputManager.h>

namespace aug
{
	std::unordered_map<uint32_t, uint8_t> InputManager::m_mInputEvent;
	glm::dvec2 InputManager::m_vCursorPosition;

	void InputManager::ProcessInputs(GLFWwindow* pWindow)
	{
		glfwPollEvents();	

		glfwGetCursorPos(pWindow, &m_vCursorPosition.x, &m_vCursorPosition.y);

		for (uint32_t uiButton = GLFW_MOUSE_BUTTON_1; uiButton < GLFW_MOUSE_BUTTON_LAST; ++uiButton)
		{
			int iState = glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_1);
			if (iState == GLFW_PRESS)
				m_mInputEvent[uiButton] = INPUT_EVENT_PRESSED;
			else if (iState == GLFW_RELEASE)
			{
				if (m_mInputEvent[uiButton] == INPUT_EVENT_PRESSED)
				{
					m_mInputEvent[uiButton] = INPUT_EVENT_RELEASED;
				}
			}
		}

		for (uint32_t uiCode = GLFW_KEY_SPACE; uiCode < GLFW_KEY_LAST; ++uiCode)
		{
			//clear state released from previous iteration
			if (m_mInputEvent[uiCode] == INPUT_EVENT_RELEASED)
				m_mInputEvent[uiCode] = INPUT_EVENT_NONE;

			int iState = glfwGetKey(pWindow, uiCode);
			if (iState == GLFW_PRESS)
				m_mInputEvent[uiCode] = INPUT_EVENT_PRESSED;
			else if (iState == GLFW_RELEASE)
			{
				if (m_mInputEvent[uiCode] == INPUT_EVENT_PRESSED)
				{
					m_mInputEvent[uiCode] = INPUT_EVENT_RELEASED;
				}
			}
		}
		
	}
}