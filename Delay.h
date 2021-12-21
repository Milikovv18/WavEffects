#pragma once

#include "Effects.h"
#include "AllPassFilter.h"


// Works like allpass filter, but have more human-friendly name
// (Actually this is an audio effect, not a filter)
// Not used in this project
class Delay : public IEffect
{
public:
	Delay(double delayTime, Channel ch); // Time is seconds
	Delay(int64_t delayDistance, Channel ch); // Or distance in samples

	void processSample(const float** dryInput, float** wetOutput, size_t length) override;
	void clear() override;

	~Delay();

private:
	// Here we go, actual allpass filter
	AllPassFilter m_circularBuffer;
	uint64_t m_distance = 0; // Delay distance in samples
	Channel m_delayTo; // Can delay either in left or right ear (not both)
};

