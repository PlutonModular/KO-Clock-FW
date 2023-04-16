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
		/// @brief The current musical time in 256th notes
		uint32_t time = 0;
		uint32_t last_time = 0;
		/// @brief The number of microseconds between incrementations of "time" variable; calculated in SetBPM
		uint16_t microsPerTimeGradation = 1;

	public:
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
