#pragma once

#include <cstdint>
#include <memory>


// Making 0's and 1's more readable
enum class Channel
{
	LEFT = 0,
	RIGHT = 1,
	ANY = 0 // If any channel is allowed, why not choose the left one?
};


// Class works only with 16 bits amplitudes :(
class IEffect
{
public:
	// You know what this proc does
	static void setSampleRate(uint32_t sampleRate);

	// Function gets dry input (which can actually contain wet sound, if needed)
	// And writes to wetOutput PRESERVING everything that already there
	// Both parameters are stereo arrays of length "length"
	virtual void processSample(const float** dryInput, float** wetOutput, size_t length) = 0;

	// Clear all your effect data and run it from the beginning
	virtual void clear() = 0;

protected:
	inline static uint32_t m_sampleRate;
};