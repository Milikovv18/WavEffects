#include "CombFilter.h"


CombFilter::CombFilter() :
	IFilter(),
	m_filterstore(0), m_damp1(0.5f), m_damp2(0.5f)
{
	// Hello
}


inline float CombFilter::process(float input)
{
	// Getting what we already have in a buffer
	float output = m_buffer[m_bufidx];
	undenormalise(output);

	// Creating new sound from what we had before and what we have now
	// Works like an echo
	m_filterstore = (output * m_damp2) + (m_filterstore * m_damp1);
	undenormalise(m_filterstore);

	// Storing new input value in buffer + damped old value
	m_buffer[m_bufidx] = input + (m_filterstore * m_feedback);

	// If we reached the end of circular buffer
	// return to the beginning
	if (++m_bufidx >= m_bufsize)
		m_bufidx = 0;

	// Return what already was in a buffer
	return output;
}


void CombFilter::setDamp(float val)
{
	// We dont want to loose energy on this
	// So the sum always equals to 1
	m_damp1 = val;
	m_damp2 = 1 - val;
}

float CombFilter::getDamp() const
{
	// Dunno :)
	return m_damp1;
}