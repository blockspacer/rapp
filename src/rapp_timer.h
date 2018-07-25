//--------------------------------------------------------------------------//
/// Copyright (c) 2018 Milos Tosic. All Rights Reserved.                   ///
/// License: http://www.opensource.org/licenses/BSD-2-Clause               ///
//--------------------------------------------------------------------------//

#ifndef RTM_RAPP_TIMER_H
#define RTM_RAPP_TIMER_H

#include <rbase/inc/cpu.h>

namespace rapp {

	class FrameStep
	{
		uint64_t	m_startClock;
		float		m_accumulator;
		float		m_currentTime;
		float		m_step;

	public:
		FrameStep(uint32_t _fps = 60)
			: m_startClock(rtm::CPU::clock())
			, m_accumulator(0.0f)
			, m_currentTime(0.0f)
			, m_step(1.0f / float(_fps))
		{
			m_currentTime = rtm::CPU::time(m_startClock);
		}

		bool update()
		{
			if (m_accumulator <= m_step)
			{
				float newTime = rtm::CPU::time(m_startClock);
				float frameTime = newTime - m_currentTime;
				if (frameTime > 0.25f)
					frameTime = 0.25f;

				m_currentTime = newTime;
				m_accumulator += frameTime;
			}

			if (m_accumulator > m_step)
			{
				m_accumulator  -= m_step;
				return true;
			}
			return false;
		}

		inline float step()
		{
			return m_step;
		}

		inline float alpha()
		{
			return m_accumulator / m_step;
		}
	};
	
} // namespace rapp

#endif // RTM_RAPP_TIMER_H
