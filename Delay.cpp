#include "Delay.h"


Delay::Delay(int64_t delayDistance, Channel ch)
	: m_distance(delayDistance)
{
	m_delayTo = ch;

	// Circular delaying buffer (we know exacly what size we want
	// cause we can delay on a const number)
	float* buffer = new float[delayDistance] {};
	m_circularBuffer.setBuffer(buffer, (int)delayDistance);
}

Delay::Delay(double delayTime, Channel ch)
	: Delay(int64_t(delayTime* m_sampleRate), ch)
{
	// Hello
}


void Delay::processSample(const float** dryInput,
	float** wetOutput, size_t length)
{
	if (m_delayTo < Channel::LEFT || m_delayTo > Channel::RIGHT)
		return;

	for (int i(0); i < length; ++i)
	{
		// Adding to wet buffer everything that was delayed previously
		wetOutput[(int)m_delayTo][i] +=
			m_circularBuffer.process(dryInput[(int)Channel::ANY][i]);

		if (wetOutput[(int)m_delayTo][i] < 0.0f)
			wetOutput[(int)m_delayTo][i] = 0.0f;
	}
}


void Delay::clear()
{
	// Just zeroing out buffer
	m_circularBuffer.mute();
}


Delay::~Delay()
{
	delete[] m_circularBuffer.getBuffer();
}