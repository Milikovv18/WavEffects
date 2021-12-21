#pragma once

#include <memory>

// If you have like very small number (e-40), its better to make that equals to 0
#define undenormalise(sample) if(((*(unsigned int*)&sample)&0x7f800000)==0) sample=0.0f


// Filters for audio signal are very useful
// E.g. you can remove unwanted frequencies
// or even delay sound for some time
class IFilter
{
public:
	IFilter();

	// Process 1 sample from your signal
	virtual inline float process(float inp) = 0;
	// Clears your effect buffers, so effect does nothing to the input signal
	void mute();

	// Set new effect buffer
	void setBuffer(float* buf, int size);
	// Set how much of amplitude of input signal you want to save
	void setFeedback(float val);

	// Get...
	float getFeedback() const;
	// Get...
	float* getBuffer() const; // For deletion

	~IFilter();

protected:
	float m_feedback{ 0 }; // Check setFeedback
	float* m_buffer{ nullptr }; // Some effect internal data
	int	m_bufsize{ 0 }; // Size of m_buffer
	int	m_bufidx; // Id of a current element (sample) in m_buffer
};
