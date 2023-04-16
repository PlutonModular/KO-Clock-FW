/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cmath>
#include "IO/IOHelper.hpp"

/// @brief Clock Timing Manager Class
class Chronos
{
	private:
		/// IO Helper Instance Pointer
		IOHelper *io;

		/// @brief between 0 and 1024, used in CalcGate
		int gateLen = 512;

		/// @brief The current musical time in 512th notes
		uint32_t beatTime = 0;
		uint32_t last_beatTime = 0;

		/// @brief The number of microseconds between incrementations of "time" variable; calculated in SetBPM
		uint16_t microsPerTimeGradation = 0;
		/// @brief Used to prevent setting BPM to its current value
		float currentExactBPM = 0;
		
		/// @brief Microseconds in this time Gradation; when > microsPerTimeGradation, reset and increment beatTime
		uint16_t timeInThisGradation = 0;


		/// @brief Calculate whether a gate should be on
		/// @param divisor number of 512th notes the gate cycle should last
		/// @param gateLen number between 0 and 1024 indicating the gate length
		/// @return whether the gate should be on
		bool CalcGate(uint32_t thisBeatTime, uint16_t divisor, uint16_t gateLen);
		
			/// @brief Calculate whether a gate should be on, according to barTime's current value
			/// @param divisor number of 512th notes the gate cycle should last
			/// @param gateLen number between 0 and 1024 indicating the gate length
			/// @return whether the gate should be on currently
			bool CalcGate(uint16_t divisor, uint16_t gateLen)
			{ return CalcGate(beatTime, divisor, gateLen); }

	public:
		
		// -------- VARIABLES --------

		/// @brief True when clock should be running on its own; Overridden by isFollowMode
		bool isPlayMode = false;
		/// @brief True when clock should be synchronized to clock input; Higher priority than isPlayMode
		bool isFollowMode = false;

		// --------  METHODS  --------

		/// @brief Initializes Chronos' state, must be called before updating
		void Init(IOHelper *io);
		/// @brief High Speed "Audio Rate" update function, for accurate timing requirements.
		/// @param deltaMicros microseconds since the last time this was run
		void FastUpdate(uint32_t deltaMicros);
		/// @brief Low Speed "CV Rate" update function, for more time-expensive operations.
		/// @param deltaMicros microseconds since the last time this was run
		void SlowUpdate(uint32_t deltaMicros);

		/// @brief Sets the BPM and calculates microsPerTimeGradation (slow!)
		/// @param exactBPM the target BPM
		void SetBPM(float exactBPM);
};
