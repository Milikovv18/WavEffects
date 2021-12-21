#pragma once

#include "Effects.h"

#include "AllPassFilter.h"
#include "CombFilter.h"


namespace reverb_const
{
	constexpr int	numCombs = 8; // Number of used comb filters (check CombFilter.h)
	constexpr int	numAllPasses = 4; // Number of used allpass filters (check AllPassFilter.h)
	constexpr float fixedGain = 0.015f; // Input (dry) sound gain (before processing)
	constexpr float scaleWet = 3; // initialWet mutiplies by this param to become actual wet gain value
	constexpr float scaleDry = 2; // initialDry... like above
	constexpr float scaleDamp = 0.4f; // initialDamp... like above
	constexpr float scaleRoom = 0.28f; // initialRoom... like above
	constexpr float offsetRoom = 0.7f; // initialRoom not only multiplies by scaleRoom, but also is summed up with this const
	constexpr float initialDry = 0; // Dry sound gain
	constexpr int	stereoSpread = 23; // Some constant, that is used in every reverb engine

	// These values assume 44.1KHz sample rate
	// they will probably be OK for 48KHz sample rate
	// but would need scaling for 96KHz (or other) sample rates.
	// The values were obtained by listening tests.
	constexpr int combTuningL1 = 1116;
	constexpr int combTuningR1 = 1116 + stereoSpread;
	constexpr int combTuningL2 = 1188;
	constexpr int combTuningR2 = 1188 + stereoSpread;
	constexpr int combTuningL3 = 1277;
	constexpr int combTuningR3 = 1277 + stereoSpread;
	constexpr int combTuningL4 = 1356;
	constexpr int combTuningR4 = 1356 + stereoSpread;
	constexpr int combTuningL5 = 1422;
	constexpr int combTuningR5 = 1422 + stereoSpread;
	constexpr int combTuningL6 = 1491;
	constexpr int combTuningR6 = 1491 + stereoSpread;
	constexpr int combTuningL7 = 1557;
	constexpr int combTuningR7 = 1557 + stereoSpread;
	constexpr int combTuningL8 = 1617;
	constexpr int combTuningR8 = 1617 + stereoSpread;
	constexpr int allPassTuningL1 = 556;
	constexpr int allPassTuningR1 = 556 + stereoSpread;
	constexpr int allPassTuningL2 = 441;
	constexpr int allPassTuningR2 = 441 + stereoSpread;
	constexpr int allPassTuningL3 = 341;
	constexpr int allPassTuningR3 = 341 + stereoSpread;
	constexpr int allPassTuningL4 = 225;
	constexpr int allPassTuningR4 = 225 + stereoSpread;
}

namespace reverb_init
{
	// Wet sound gain (aka reverb loudness)
	constexpr float initialWet = 1 / reverb_const::scaleWet;
	// Room size (how long sound waves should travel)
	constexpr float roomSize = 0.85f;
	// Damping sound waves with time (loosing energy)
	constexpr float dampAmount = 0.5f;
	// How much reverb from 1 channel leaks to other channel
	constexpr float width = 1.0f;
}


class Freeverb : public IEffect
{
public:
	Freeverb();

	void clear() override;
	void processSample(const float** dryInput, float** wetOutput, size_t length) override;

	void enable(bool value);
	void setRoomSize(float value);
	void setDamp(float value);
	void setWet(float value);
	void setWidth(float value);

	bool isEnabled() const;
	float getRoomSize() const;
	float getDamp() const;
	float getWet() const;
	float getWidth() const;

	~Freeverb();

private:
	// On/Off effect
	bool m_enabled;

	// Check reverb_const for definitions
	float m_gain;
	float m_roomSize;
	float m_damp, m_damp1;
	float m_wet, m_wet1, m_wet2;
	float m_dry;
	float m_width;

	// Comb filters
	CombFilter m_combL[reverb_const::numCombs];
	CombFilter m_combR[reverb_const::numCombs];

	// Allpass filters
	CustomAllPassFilter m_allpassL[reverb_const::numAllPasses];
	CustomAllPassFilter m_allpassR[reverb_const::numAllPasses];

	// Buffers for the combs
	float* bufcombL1;
	float* bufcombR1;
	float* bufcombL2;
	float* bufcombR2;
	float* bufcombL3;
	float* bufcombR3;
	float* bufcombL4;
	float* bufcombR4;
	float* bufcombL5;
	float* bufcombR5;
	float* bufcombL6;
	float* bufcombR6;
	float* bufcombL7;
	float* bufcombR7;
	float* bufcombL8;
	float* bufcombR8;

	// Buffers for the allpasses
	float* bufallpassL1;
	float* bufallpassR1;
	float* bufallpassL2;
	float* bufallpassR2;
	float* bufallpassL3;
	float* bufallpassR3;
	float* bufallpassL4;
	float* bufallpassR4;

	// If some parameters changed by user, internal info should be recalculated
	void update();
};

