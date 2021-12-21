#pragma once

#include <fstream>
#include "FFTW/fftw3.h"

#include "Effects.h"


// https://dsp.stackexchange.com/questions/72572/hrir-interpolation-using-vbap

class HRTF : public IEffect
{
	struct Vector
	{
		float x{ 0.0f }, y{ 0.0f }, z{ 0.0f };
		fftwf_complex** HRTF[2]{};

		Vector operator-(const Vector& vec)
		{
			return Vector{ x - vec.x, y - vec.y, z - vec.z, nullptr, nullptr };
		}

		float squaredLength()
		{
			return x * x + y * y + z * z;
		}
	};

	enum Complex
	{
		REAL = 0,
		IMG = 1
	};

public:
	HRTF(const char* hrirSphereFileName);

	void processSample(const float** dryInput, float** wetOutput, size_t length) override;
	void clear();

	bool isFinished();

private:
	Vector* m_vertices{ nullptr };
	uint32_t m_vertexCount;
	size_t m_HRIR_length; // And eponymous with FFT size
	float m_FftNormFactor; // 1 / m_HRIR_length

	Vector m_soundSource{ -1.0, 0.0, 0.0 }; // Not using HRTF fields
	float* m_curRealFftData[2]{}; // E.g. Impulse response is scalar in time domain
	// Its also a sliding window, that Overlap-Save method requires
	fftwf_complex* m_curComplexFftData[2]{}; // E.g. Transfer function is complex in frequency domain

	fftwf_complex* m_accumulator[2]{}; // Overlap-Save algorithm requires
	float* m_prevInput{}; // Overlap-Save algorithm requires half of previous input buffer, but we save full buffer

	fftwf_plan m_forwardFFT[2]{};
	fftwf_plan m_backwardFFT[2]{};

	const size_t OVERLAP_PARTS = 2;
};

