/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cmath>
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#include "IO/IOHelper.hpp"
#include "debug.h"

#define CLOCKIN_BUFFER_SIZE 64

#define CLOCKIN_MIN_WAIT 100'000
#define CLOCKIN_WAIT_MULT 32

enum PPQNType
{
	PPQN_1 = 1,
	PPQN_4 = 4,
	PPQN_8 = 8,
	PPQN_16 = 16,
	PPQN_32 = 32,
	PPQN_24 = 24,
	PPQN_48 = 48
};

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


		//-------- EXT CLOCK IN VARIABLES --------

		/// @brief Current PPQN setting. Set by SetPPQN
		PPQNType clockPPQN = PPQNType::PPQN_24;
		/// @brief Circular buffer of the time between the most recent clock pulses, used to estimate BPM for interpolation
		uint64_t clockInDiffBuffer[CLOCKIN_BUFFER_SIZE];
		/// @brief Write pointer for clockInBuffer
		uint16_t clockInDiffBufferIterator = 0;
		/// @brief Current estimated BPM based on clock in timings
		float estimatedBPM = 120;
		/// @brief Time of last clock pulse
		uint64_t lastClockTime = 0;
		/// @brief Reset to CLOCK_KEEPALIVE_TIME on each clock in pulse. Used to detect when an external clock is stopped.
		int32_t externalClockKeepaliveCountdown = 0;
		/// @brief Used to keep track of when a full quarter note has elapsed
		uint16_t clockPulseCounter = 0;
		/// @brief Called when clock in goes high, used to update and calculate the input BPM estimate.
		void AddBeatToBPMEstimate();
		/// @brief Called when clock in goes high, used to update and calculate the input BPM estimate.
		/// @note This version only sets "lastClockTime", with its intended use being to capture the first pulse after
		/// a long delay that would otherwise incorrectly lower the average BPM estimate.
		void AddBeatToBPMEstimateLastOnly();

		// -------- Methods --------

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
