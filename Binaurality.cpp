#include "Binaurality.h"


Binaurality::Binaurality(const char* hrirFileName, size_t signalLength)
{
	// GENERAL INFO //

	// Opening file with HRIR data
	std::ifstream database(hrirFileName, std::ios::binary);
	if (!database.is_open())
		throw "Where are my impulses?";

	// LOCAL INFO //
	m_fftBufferLength = signalLength;
	m_FftNormFactor = 1.0f / m_fftBufferLength;

	// Allocating buffers for FFT
	m_curRealFftData[(int)Channel::LEFT] = fftwf_alloc_real(m_fftBufferLength);
	m_curRealFftData[(int)Channel::RIGHT] = fftwf_alloc_real(m_fftBufferLength);

	m_curComplexFftData[(int)Channel::LEFT] = fftwf_alloc_complex(m_fftBufferLength);
	m_curComplexFftData[(int)Channel::RIGHT] = fftwf_alloc_complex(m_fftBufferLength);

	// Forward FFT (real to complex)
	m_forwardFFT[(int)Channel::LEFT] = fftwf_plan_dft_r2c_1d((int)m_fftBufferLength,
		m_curRealFftData[(int)Channel::LEFT], m_curComplexFftData[(int)Channel::LEFT], FFTW_ESTIMATE);
	m_forwardFFT[(int)Channel::RIGHT] = fftwf_plan_dft_r2c_1d((int)m_fftBufferLength,
		m_curRealFftData[(int)Channel::RIGHT], m_curComplexFftData[(int)Channel::RIGHT], FFTW_ESTIMATE);

	// Backward FFT (complex to real)
	m_backwardFFT[(int)Channel::LEFT] = fftwf_plan_dft_c2r_1d((int)m_fftBufferLength,
		m_curComplexFftData[(int)Channel::LEFT], m_curRealFftData[(int)Channel::LEFT], FFTW_ESTIMATE);
	m_backwardFFT[(int)Channel::RIGHT] = fftwf_plan_dft_c2r_1d((int)m_fftBufferLength,
		m_curComplexFftData[(int)Channel::RIGHT], m_curRealFftData[(int)Channel::RIGHT], FFTW_ESTIMATE);

	// Previous [dry] input buffer storage
	m_prevInput = new float[m_fftBufferLength]{};

	// GENERAL INFO //
	double* rawHRIR = new double[binaural_const::HRIR_Length]{}; // Temp
	// Vertices, that form our "virtual sources" sphere
	m_vertices = new Vector[binaural_const::vertexCount]{};
	int vertexCount(0);

	// Filling all vertices with info from file
	for (short elevation : binaural_const::elevations) // Elevations are not uniformely distributed
	{
		for (short azimuth(0); azimuth < 360; azimuth += 5) // These are OK
		{
			m_vertices[vertexCount].azimuth = azimuth;
			m_vertices[vertexCount].elevation = elevation;
			m_vertices[vertexCount].distance = 0.76f; // Distance is constant

			for (int chan(0); chan < 2; ++chan) // Stereo
			{
				// Reading full HRTF for this position from file
				database.read((char*)rawHRIR, binaural_const::HRIR_Length * sizeof(double));
				memset(m_curRealFftData[chan], 0x0, m_fftBufferLength);

				// Converting sample rate and impulse length to desired
				for (size_t sampleNo(0); sampleNo < m_fftBufferLength; ++sampleNo)
					m_curRealFftData[chan][sampleNo] = (float)rawHRIR[sampleNo << 1];

				// Applying FFT
				fftwf_execute(m_forwardFFT[chan]);

				// Filling in vertex info
				m_vertices[vertexCount].HRTF[chan] = new fftwf_complex[m_fftBufferLength]{};
				memcpy(m_vertices[vertexCount].HRTF[chan],
					m_curComplexFftData[chan], m_fftBufferLength * sizeof(fftwf_complex));
			}

			++vertexCount;
		}
	}

	// Immediately enable the effect
	m_enabled = true;
	// Initial sound source pos
	m_sourcePos = { binaural_init::azimuth, binaural_init::elevation, binaural_init::distance };
	m_sourceAnimation = binaural_init::animated;

	delete[] rawHRIR;
	database.close();
}


void Binaurality::processSample(const float** dryInput,
	float** wetOutput, size_t length)
{
	if (!isEnabled())
		return;

	// Animation
	if (m_sourceAnimation)
	{
		m_sourcePos.azimuth += 2.0f;
		if (m_sourcePos.azimuth > 355) {
			m_sourcePos.azimuth = 0;
		}
	}

	// No interpolation actually, just nearest neighbour from sphere
	Vector hrtfData = interpolate();

	// Overlap-save algorithm loop
	for (size_t inpBlockId(0); inpBlockId < length; inpBlockId += binaural_const::desired_HRIR_Length)
	{
		// Overlapped regions, so memmove
		memmove(m_prevInput, m_prevInput + binaural_const::desired_HRIR_Length,
			(length - binaural_const::desired_HRIR_Length) * sizeof(float));
		memcpy(m_prevInput + length - binaural_const::desired_HRIR_Length,
			dryInput[(int)Channel::ANY] + size_t(inpBlockId),
			binaural_const::desired_HRIR_Length * sizeof(float));

		// False positive C6385?
		memcpy(m_curRealFftData[(int)Channel::ANY], m_prevInput, m_fftBufferLength * sizeof(float));

		// Dry input from subframe
		fftwf_execute(m_forwardFFT[(int)Channel::ANY]);
		float tempReal, tempImg;
		// Convolving original and HR impulses in a frequency domain
		for (int i(0); i < m_fftBufferLength; ++i)
		{
			tempReal = m_curComplexFftData[(int)Channel::ANY][i][REAL];
			tempImg  = m_curComplexFftData[(int)Channel::ANY][i][IMG];

			// Pair-wise complex multiplication in frequency domain
			// m_curComplexFftData[chan][i] * HRTF[chan][i]
			// (a+bi)*(c+di) = (ac-bd)+(ad+cb)i

			// Left channel
			m_curComplexFftData[(int)Channel::LEFT][i][REAL] =
				(tempReal * hrtfData.HRTF[(int)Channel::LEFT][i][REAL]) -
				(tempImg  * hrtfData.HRTF[(int)Channel::LEFT][i][IMG]);
			m_curComplexFftData[(int)Channel::LEFT][i][IMG] =
				(tempReal * hrtfData.HRTF[(int)Channel::LEFT][i][IMG]) +
				(tempImg  * hrtfData.HRTF[(int)Channel::LEFT][i][REAL]);

			// Right channel
			m_curComplexFftData[(int)Channel::RIGHT][i][REAL] =
				(tempReal * hrtfData.HRTF[(int)Channel::RIGHT][i][REAL]) -
				(tempImg  * hrtfData.HRTF[(int)Channel::RIGHT][i][IMG]);
			m_curComplexFftData[(int)Channel::RIGHT][i][IMG] =
				(tempReal * hrtfData.HRTF[(int)Channel::RIGHT][i][IMG]) +
				(tempImg  * hrtfData.HRTF[(int)Channel::RIGHT][i][REAL]);
		}

		// Loop is important for normalizeing FFT result
		// We need 2 channels at the same time for performance
		fftwf_execute(m_backwardFFT[(int)Channel::LEFT]);
		fftwf_execute(m_backwardFFT[(int)Channel::RIGHT]);
		for (size_t i(0); i < binaural_const::desired_HRIR_Length; ++i)
		{
			// Just saving what we have got from loop
			// Note: saving only @desired_HRIR_Length@ samples
			// Thats what overlap-save algorithm tells us to do
			wetOutput[(int)Channel::LEFT][inpBlockId + i] +=
				m_FftNormFactor * m_curRealFftData[(int)Channel::LEFT]
				[m_fftBufferLength - binaural_const::desired_HRIR_Length + i];
			wetOutput[(int)Channel::RIGHT][inpBlockId + i] +=
				m_FftNormFactor * m_curRealFftData[(int)Channel::RIGHT]
				[m_fftBufferLength - binaural_const::desired_HRIR_Length + i];
		}
	}
}


Binaurality::Vector Binaurality::interpolate()
{
	Vector* nearVert;
	// Binary search
	int64_t left(0), right(binaural_const::vertexCount);
	while (true)
	{
		int64_t middle = (right + left) >> 1;
		nearVert = &m_vertices[middle];

		// Because we dont have exact needed source pos
		// on a sphere, we iterate until interval becomes
		// 1 unit in length (can be optimised) TODO
		if (abs(left - right) <= 1) {
			break;
		}
		else if (nearVert->elevation < m_sourcePos.elevation ||
			nearVert->azimuth < m_sourcePos.azimuth) {
			left = middle;
		}
		else {
			right = middle;
		}
	}

	// Some bugs here maybe?
	if (!nearVert->HRTF[(int)Channel::LEFT] || !nearVert->HRTF[(int)Channel::RIGHT])
		throw "Not found";

	// Tadam
	return *nearVert;
}


void Binaurality::clear()
{
	// Cant clear 2 channels at the same time -- pointers become NULL
	memset(m_curRealFftData[(int)Channel::LEFT], 0x0, m_fftBufferLength * sizeof(float));
	memset(m_curRealFftData[(int)Channel::RIGHT], 0x0, m_fftBufferLength * sizeof(float));
	memset(m_curComplexFftData[(int)Channel::LEFT], 0x0, m_fftBufferLength * sizeof(fftwf_complex));
	memset(m_curComplexFftData[(int)Channel::RIGHT], 0x0, m_fftBufferLength * sizeof(fftwf_complex));
	memset(m_prevInput, 0x0, m_fftBufferLength * sizeof(float));
}


void Binaurality::enable(bool value)
{
	m_enabled = value;
}

void Binaurality::animeOn(bool value)
{
	m_sourceAnimation = value;
}

void Binaurality::setAzimuth(float value)
{
	// We can have literally any azimuth angle
	// we just need to wrap it around 360
	value = fmod(value, 360.0f);
	value = value < 0.0f ? 355 + value : value;
	m_sourcePos.azimuth = value;
}

void Binaurality::setElevation(float value)
{
	// We need to wrap elevation around [-90; 90]

	// Elevation from -90 to 90 deg
	value = fmod(value + 90, 180.0f);
	value = value < 0.0f ? 180.0f + value : value;
	m_sourcePos.elevation = value - 90;
}

bool Binaurality::isEnabled() const
{
	return m_enabled;
}

bool Binaurality::isAnime() const
{
	return m_sourceAnimation;
}

float Binaurality::getAzimuth() const
{
	return m_sourcePos.azimuth;
}

float Binaurality::getElevation() const
{
	return m_sourcePos.elevation;
}

float Binaurality::getDistance() const
{
	return 0.76f; // Always const because of HRIR data
}


Binaurality::~Binaurality()
{
	// Clearing FFT data
	fftwf_destroy_plan(m_forwardFFT[(int)Channel::LEFT]);
	fftwf_destroy_plan(m_forwardFFT[(int)Channel::RIGHT]);
	fftwf_destroy_plan(m_backwardFFT[(int)Channel::LEFT]);
	fftwf_destroy_plan(m_backwardFFT[(int)Channel::RIGHT]);

	fftwf_free(m_curRealFftData[(int)Channel::LEFT]);
	fftwf_free(m_curRealFftData[(int)Channel::RIGHT]);
	fftwf_free(m_curComplexFftData[(int)Channel::LEFT]);
	fftwf_free(m_curComplexFftData[(int)Channel::RIGHT]);

	// Previous input storage
	delete[] m_prevInput;

	// Clearing all vertices from our sphere
	for (int vId(0); vId < binaural_const::vertexCount; ++vId)
	{
		m_vertices[vId].HRTF[(int)Channel::LEFT];
		m_vertices[vId].HRTF[(int)Channel::RIGHT];
	}
	delete[] m_vertices;
}