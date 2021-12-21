#pragma once

#include "Filters.h"


// At each new iteration adds to current samples
// some previous (saved and attenuated) samples.
// So creates some kind of echo
class CombFilter : public IFilter
{
public:
	CombFilter();

	inline float process(float input) override;

	// Set attenuation factor
	void setDamp(float val);
	// Get...
	float getDamp() const;

private:
	// Contains dumped sum of previous samples
	float m_filterstore;
	float m_damp1;
	float m_damp2;
};

