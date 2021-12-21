#include "Filters.h"


IFilter::IFilter() :
	m_buffer(nullptr),
	m_bufsize(0),
	m_bufidx(0),
	m_feedback(0.0)
{
	// Hello
}


void IFilter::mute()
{
	// Just zeroing out buffer
	memset(m_buffer, 0x0, sizeof(float) * m_bufsize);
}


void IFilter::setBuffer(float* buf, int size)
{
	m_buffer = buf;
	m_bufsize = size;
}

void IFilter::setFeedback(float val)
{
	m_feedback = val;
}

float IFilter::getFeedback() const
{
	return m_feedback;
}

float* IFilter::getBuffer() const
{
	return m_buffer;
}


IFilter::~IFilter()
{
	if (m_buffer) {
		delete[] m_buffer;
	}
}