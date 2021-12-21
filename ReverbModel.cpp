#include "ReverbModel.h"


Freeverb::Freeverb()
{
	// Allocating memory
	bufcombL1 = new float[reverb_const::combTuningL1];
	bufcombR1 = new float[reverb_const::combTuningR1];
	bufcombL2 = new float[reverb_const::combTuningL2];
	bufcombR2 = new float[reverb_const::combTuningR2];
	bufcombL3 = new float[reverb_const::combTuningL3];
	bufcombR3 = new float[reverb_const::combTuningR3];
	bufcombL4 = new float[reverb_const::combTuningL4];
	bufcombR4 = new float[reverb_const::combTuningR4];
	bufcombL5 = new float[reverb_const::combTuningL5];
	bufcombR5 = new float[reverb_const::combTuningR5];
	bufcombL6 = new float[reverb_const::combTuningL6];
	bufcombR6 = new float[reverb_const::combTuningR6];
	bufcombL7 = new float[reverb_const::combTuningL7];
	bufcombR7 = new float[reverb_const::combTuningR7];
	bufcombL8 = new float[reverb_const::combTuningL8];
	bufcombR8 = new float[reverb_const::combTuningR8];

	bufallpassL1 = new float[reverb_const::allPassTuningL1];
	bufallpassR1 = new float[reverb_const::allPassTuningR1];
	bufallpassL2 = new float[reverb_const::allPassTuningL2];
	bufallpassR2 = new float[reverb_const::allPassTuningR2];
	bufallpassL3 = new float[reverb_const::allPassTuningL3];
	bufallpassR3 = new float[reverb_const::allPassTuningR3];
	bufallpassL4 = new float[reverb_const::allPassTuningL4];
	bufallpassR4 = new float[reverb_const::allPassTuningR4];

	// Tie the components to their buffers
	m_combL[0].setBuffer(bufcombL1, reverb_const::combTuningL1);
	m_combR[0].setBuffer(bufcombR1, reverb_const::combTuningR1);
	m_combL[1].setBuffer(bufcombL2, reverb_const::combTuningL2);
	m_combR[1].setBuffer(bufcombR2, reverb_const::combTuningR2);
	m_combL[2].setBuffer(bufcombL3, reverb_const::combTuningL3);
	m_combR[2].setBuffer(bufcombR3, reverb_const::combTuningR3);
	m_combL[3].setBuffer(bufcombL4, reverb_const::combTuningL4);
	m_combR[3].setBuffer(bufcombR4, reverb_const::combTuningR4);
	m_combL[4].setBuffer(bufcombL5, reverb_const::combTuningL5);
	m_combR[4].setBuffer(bufcombR5, reverb_const::combTuningR5);
	m_combL[5].setBuffer(bufcombL6, reverb_const::combTuningL6);
	m_combR[5].setBuffer(bufcombR6, reverb_const::combTuningR6);
	m_combL[6].setBuffer(bufcombL7, reverb_const::combTuningL7);
	m_combR[6].setBuffer(bufcombR7, reverb_const::combTuningR7);
	m_combL[7].setBuffer(bufcombL8, reverb_const::combTuningL8);
	m_combR[7].setBuffer(bufcombR8, reverb_const::combTuningR8);
	m_allpassL[0].setBuffer(bufallpassL1, reverb_const::allPassTuningL1);
	m_allpassR[0].setBuffer(bufallpassR1, reverb_const::allPassTuningR1);
	m_allpassL[1].setBuffer(bufallpassL2, reverb_const::allPassTuningL2);
	m_allpassR[1].setBuffer(bufallpassR2, reverb_const::allPassTuningR2);
	m_allpassL[2].setBuffer(bufallpassL3, reverb_const::allPassTuningL3);
	m_allpassR[2].setBuffer(bufallpassR3, reverb_const::allPassTuningR3);
	m_allpassL[3].setBuffer(bufallpassL4, reverb_const::allPassTuningL4);
	m_allpassR[3].setBuffer(bufallpassR4, reverb_const::allPassTuningR4);

	// Set default values
	m_allpassL[0].setFeedback(0.5f);
	m_allpassR[0].setFeedback(0.5f);
	m_allpassL[1].setFeedback(0.5f);
	m_allpassR[1].setFeedback(0.5f);
	m_allpassL[2].setFeedback(0.5f);
	m_allpassR[2].setFeedback(0.5f);
	m_allpassL[3].setFeedback(0.5f);
	m_allpassR[3].setFeedback(0.5f);

	// Activating effect immediately
	m_enabled = true;

	// Why? Authors say so
	m_dry = reverb_const::initialDry * reverb_const::scaleDry;

	// Setting initial params
	setWet(reverb_init::initialWet);
	setRoomSize(reverb_init::roomSize);
	setDamp(reverb_init::dampAmount);
	setWidth(reverb_init::width);

	// Buffer will be full of rubbish - so we MUST mute (clear) them
	clear();
}


void Freeverb::clear()
{
	// Clearing all effect buffers
	for (int i = 0; i < reverb_const::numCombs; i++)
	{
		m_combL[i].mute();
		m_combR[i].mute();
	}

	for (int i = 0; i < reverb_const::numAllPasses; i++)
	{
		m_allpassL[i].mute();
		m_allpassR[i].mute();
	}
}


void Freeverb::processSample(const float** dryInput,
	float** wetOutput, size_t length)
{
	if (!isEnabled())
		return;

	float outL, outR, inL, inR;
	const float* inputL(dryInput[(int)Channel::LEFT]), *inputR(dryInput[(int)Channel::RIGHT]);
	float* outputL(wetOutput[0]), *outputR(wetOutput[1]);

	while (length-- > 0)
	{
		outL = outR = 0;
		inL = *inputL * m_gain;
		inR = *inputR * m_gain;

		// Accumulate comb filters in parallel
		for (int i = 0; i < reverb_const::numCombs; i++)
		{
			outL += m_combL[i].process(inL);
			outR += m_combR[i].process(inR);
		}

		// Feed through allpasses in series
		for (int i = 0; i < reverb_const::numAllPasses; i++)
		{
			outL = m_allpassL[i].process(outL);
			outR = m_allpassR[i].process(outR);
		}

		// Calculate output MIXING with anything already there
		*outputL += outL * m_wet1 + outR * m_wet2 + **dryInput * m_dry;
		*outputR += outR * m_wet1 + outL * m_wet2 + **dryInput * m_dry;

		// Increment sample pointers, allowing for interleave (if any)
		inputL++;
		inputR++;
		outputL++;
		outputR++;
	}
}


void Freeverb::update()
{
	// Recalculate internal values after parameter change

	m_wet1 = m_wet * (m_width / 2 + 0.5f);
	m_wet2 = m_wet * ((1 - m_width) / 2);

	m_damp1 = m_damp;
	m_gain = reverb_const::fixedGain;

	for (int i(0); i < reverb_const::numCombs; i++)
	{
		m_combL[i].setFeedback(m_roomSize);
		m_combR[i].setFeedback(m_roomSize);
	}

	for (int i(0); i < reverb_const::numCombs; i++)
	{
		m_combL[i].setDamp(m_damp1);
		m_combR[i].setDamp(m_damp1);
	}
}


// The following get/set functions are not inlined, because
// speed is never an issue when calling them, and also
// because as you develop the reverb model, you may
// wish to take dynamic action when they are called.

void Freeverb::enable(bool value)
{
	m_enabled = value;
}

void Freeverb::setRoomSize(float value)
{
	m_roomSize = (value * reverb_const::scaleRoom) + reverb_const::offsetRoom;
	update();
}

void Freeverb::setDamp(float value)
{
	m_damp = value * reverb_const::scaleDamp;
	update();
}

void Freeverb::setWet(float value)
{
	m_wet = value * reverb_const::scaleWet;
	update();
}

void Freeverb::setWidth(float value)
{
	m_width = value;
	update();
}

bool Freeverb::isEnabled() const
{
	return m_enabled;
}

float Freeverb::getRoomSize() const
{
	return (m_roomSize - reverb_const::offsetRoom) / reverb_const::scaleRoom;
}

float Freeverb::getDamp() const
{
	return m_damp / reverb_const::scaleDamp;
}

float Freeverb::getWet() const
{
	return m_wet / reverb_const::scaleWet;
}

float Freeverb::getWidth() const
{
	return m_width;
}


Freeverb::~Freeverb()
{
	delete[] bufcombL1;
	delete[] bufcombR1;
	delete[] bufcombL2;
	delete[] bufcombR2;
	delete[] bufcombL3;
	delete[] bufcombR3;
	delete[] bufcombL4;
	delete[] bufcombR4;
	delete[] bufcombL5;
	delete[] bufcombR5;
	delete[] bufcombL6;
	delete[] bufcombR6;
	delete[] bufcombL7;
	delete[] bufcombR7;
	delete[] bufcombL8;
	delete[] bufcombR8;

	delete[] bufallpassL1;
	delete[] bufallpassR1;
	delete[] bufallpassL2;
	delete[] bufallpassR2;
	delete[] bufallpassL3;
	delete[] bufallpassR3;
	delete[] bufallpassL4;
	delete[] bufallpassR4;
}