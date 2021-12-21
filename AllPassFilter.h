#pragma once

#include "Filters.h"


// Provides a phase shift for a sound sample (amplitude isnt changed)
class AllPassFilter : public IFilter
{
public:
	AllPassFilter();

	inline float process(float input) override;
};


// For freeverb only
class CustomAllPassFilter : public IFilter
{
public:
	CustomAllPassFilter();

	inline float process(float input) override;
};