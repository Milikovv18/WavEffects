#pragma once

#include <fstream>
#include "FFTW/fftw3.h"

#include "Effects.h"


namespace binaural_const
{
	constexpr uint32_t sampleRate = 96'000; // HRIR files sample rate
	constexpr size_t   HRIR_Length = 4096/*samples*/;
	constexpr size_t   desired_HRIR_Length = 512/*samples*/;
	constexpr uint32_t vertexCount = 72/*deg, azimuths*/ * 9/*deg, elevations*/;

	// All available elevation angles
	constexpr short elevations[] = { -57, -30, -15, 0, 15, 30, 45, 60, 75 };
}

namespace binaural_init
{
	// Is rotating around head
	constexpr bool animated = true;
	// Sound source pos (on a unit radius sphere)
	constexpr float azimuth = 0.0f/*deg*/;
	constexpr float elevation = 0.0f/*deg*/;
	constexpr float distance = 0.76f/*meters*/;
}


class Binaurality : public IEffect
{
	// Single point on a normalized sphere,
	// that has 2 spherical coords and distance from the center in meters,
	// also it contains HRTF for both channels in frequency domain
	struct Vector
	{
		float azimuth{ 0.0f }, elevation{ 0.0f }, distance{ 0.0f };
		fftwf_complex* HRTF[2]{};
	};

	// Simplifying access to fftw complex number parts
	enum Complex
	{
		REAL = 0,
		IMG = 1
	};

public:
	// File, that contains only HRIRs, that satisfy conditions above.
	// Signal length is actually frame size, but with some eloquence
	Binaurality(const char* hrirFileName, size_t signalLength);

	void processSample(const float** dryInput, float** wetOutput, size_t length) override;
	void clear() override;

	void animeOn(bool value);
	void enable(bool value);
	void setAzimuth(float value);
	void setElevation(float value);

	bool isAnime() const;
	bool isEnabled() const;
	float getAzimuth() const;
	float getElevation() const;
	float getDistance() const;

	~Binaurality();

private:
	// Array, sorted by elevation and [then] azimuth
	// For binary search
	Vector* m_vertices{ nullptr };
	size_t m_fftBufferLength; // aka frame length
	float m_FftNormFactor; // 1/m_fftBufferLength

	bool m_enabled;
	Vector m_sourcePos; // Virtual sound source pos
	bool m_sourceAnimation; // Is sound source rotating (auto azimuth changes)
	float* m_curRealFftData[2]{}; // Time domain data
	fftwf_complex* m_curComplexFftData[2]{}; // Frequency domain data

	float* m_prevInput{}; // Overlap-Save algorithm requires previous input buffers

	fftwf_plan m_forwardFFT[2]{}; // Just some FFTW reqs
	fftwf_plan m_backwardFFT[2]{};// Just some FFTW reqs
	
	Vector interpolate(); // Interpolation between normalized sphere points, but actually no
};
