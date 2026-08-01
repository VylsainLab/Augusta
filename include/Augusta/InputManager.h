#pragma once
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <glm/glm.hpp>

namespace aug
{
	enum EInputEvent
	{
		INPUT_EVENT_NONE,
		INPUT_EVENT_PRESSED,
		INPUT_EVENT_RELEASED,
	};

	class InputManager
	{
	public:

		static void ProcessInputs(GLFWwindow* pWindow);

		static bool InputPressed(uint32_t uiCode) { return m_mInputEvent[uiCode] == INPUT_EVENT_PRESSED; }
		static bool InputReleased(uint32_t uiCode) { return m_mInputEvent[uiCode] == INPUT_EVENT_RELEASED; }
		static glm::dvec2 GetMousePosition() { return m_vCursorPosition; }

	protected:
		static std::unordered_map<uint32_t, uint8_t> m_mInputEvent;
		static glm::dvec2 m_vCursorPosition;
	};
}

