#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include <array>


namespace Athena
{
	class ATHENA_API ProfilingPanel
	{
	public:
		void OnImGuiRender();

	private:
		bool m_IsPlottingFrameRate = false;
		bool m_IsShowRenderer2D = true;

		std::array<float, 64> m_FrameRateStack;
		SIZE_T m_FrameRateIndex = 0;

		Time m_FrameTime;
		Timer m_Timer;
		Time m_LastTime = 0;

		const Time m_UpdateInterval = Time::Seconds(0.05f);
	};
}