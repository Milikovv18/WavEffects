#include "AllPassFilter.h"


AllPassFilter::AllPassFilter()
	: IFilter()
{
	// Hello
}


inline float AllPassFilter::process(float input)
{
	// Getting what we already have in a buffer
	float bufout = m_buffer[m_bufidx];
	undenormalise(bufout);

	// Storing new input value in buffer + damped old value
	m_buffer[m_bufidx] = input + (bufout * m_feedback);

	// If we reached the end of circular buffer
	// return to the beginning
	if (++m_bufidx >= m_bufsize)
		m_bufidx = 0;
	
	// Return what already was in a buffer
	return bufout;
}



// FREEVERB PART //
CustomAllPassFilter::CustomAllPassFilter()
	: IFilter()
{
	// Hello
}


// The same as the one above, but with 1 feature
inline float CustomAllPassFilter::process(float input)
{
	float output;
	float bufout;

	bufout = m_buffer[m_bufidx];
	undenormalise(bufout);

	output = -input + bufout; // Removing metallic effect from reverb
	m_buffer[m_bufidx] = input + (bufout * m_feedback);

	if (++m_bufidx >= m_bufsize)
		m_bufidx = 0;

	return output;
}