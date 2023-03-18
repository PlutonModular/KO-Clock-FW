/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cmath>

/// @brief Clock Timing Manager Class
class Chronos
{
	private:
	public:
		/// @brief Initializes Chronos' state, must be called before updating
		void Init();
		/// @brief High Speed "Audio Rate" update function, for accurate timing requirements.
		/// @param deltaMicros microseconds since the last time this was run
		void FastUpdate(uint32_t deltaMicros);
};