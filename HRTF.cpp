#include "HRTF.h"


HRTF::HRTF(const char* hrirSphereFileName)
{
	// GENERAL INFO //
	std::ifstream sphereMesh(hrirSphereFileName, std::ios::binary);
	if (!sphereMesh.is_open())
		throw "Where are my meshes?";

	// Header
	char magic[4]; sphereMesh.read(magic, 4); // HRIR
	uint32_t sampleRate(0); sphereMesh.read((char*)&sampleRate, sizeof(uint32_t));
	sphereMesh.read((char*)&m_HRIR_length, sizeof(uint32_t));
	m_FftNormFactor = 1.0f / m_HRIR_length;
	sphereMesh.read((char*)&m_vertexCount, sizeof(uint32_t));
	uint32_t indexCount(0); sphereMesh.read((char*)&indexCount, sizeof(uint32_t));

	// Indices
	uint32_t* indices = new uint32_t[indexCount]{};
	sphereMesh.read((char*)indices, indexCount * sizeof(uint32_t));

	// LOCAL INFO //
	m_curRealFftData[(int)Channel::LEFT] = fftwf_alloc_real(m_HRIR_length);
	m_curRealFftData[(int)Channel::RIGHT] = fftwf_alloc_real(m_HRIR_length);

	m_curComplexFftData[(int)Channel::LEFT] = fftwf_alloc_complex(m_HRIR_length);
	m_curComplexFftData[(int)Channel::RIGHT] = fftwf_alloc_complex(m_HRIR_length);

	// Forward (real to complex)
	m_forwardFFT[(int)Channel::LEFT] = fftwf_plan_dft_r2c_1d((int)m_HRIR_length,
		m_curRealFftData[(int)Channel::LEFT], m_curComplexFftData[(int)Channel::LEFT], FFTW_ESTIMATE);
	m_forwardFFT[(int)Channel::RIGHT] = fftwf_plan_dft_r2c_1d((int)m_HRIR_length,
		m_curRealFftData[(int)Channel::RIGHT], m_curComplexFftData[(int)Channel::RIGHT], FFTW_ESTIMATE);

	// Backward (complex to real)
	m_backwardFFT[(int)Channel::LEFT] = fftwf_plan_dft_c2r_1d((int)m_HRIR_length,
		m_curComplexFftData[(int)Channel::LEFT], m_curRealFftData[(int)Channel::LEFT], FFTW_ESTIMATE);
	m_backwardFFT[(int)Channel::RIGHT] = fftwf_plan_dft_c2r_1d((int)m_HRIR_length,
		m_curComplexFftData[(int)Channel::RIGHT], m_curRealFftData[(int)Channel::RIGHT], FFTW_ESTIMATE);

	// Accumulator
	m_accumulator[(int)Channel::LEFT] = new fftwf_complex[m_HRIR_length]{};
	m_accumulator[(int)Channel::RIGHT] = new fftwf_complex[m_HRIR_length]{};

	m_prevInput = new float[m_HRIR_length]{};

	// GENERAL INFO //
	float* tempArr[2];
	tempArr[(int)Channel::LEFT] = new float[m_HRIR_length];
	tempArr[(int)Channel::RIGHT] = new float[m_HRIR_length];
	m_vertices = new Vector[m_vertexCount]{};

	for (unsigned i(0); i < m_vertexCount; ++i)
	{
		sphereMesh.read((char*)&m_vertices[i].x, sizeof(float));
		sphereMesh.read((char*)&m_vertices[i].y, sizeof(float));
		sphereMesh.read((char*)&m_vertices[i].z, sizeof(float));

		sphereMesh.read((char*)tempArr[(int)Channel::LEFT], m_HRIR_length * sizeof(float));
		sphereMesh.read((char*)tempArr[(int)Channel::RIGHT], m_HRIR_length * sizeof(float));

		// Left channel HRTF
		m_vertices[i].HRTF[(int)Channel::LEFT] = new fftwf_complex*[2];
		for (int partNo(0); partNo < 2; ++partNo)
		{
			memset(m_curRealFftData[(int)Channel::LEFT], 0x0, m_HRIR_length); // Zero padding
			memcpy(m_curRealFftData[(int)Channel::LEFT],
				tempArr[(int)Channel::LEFT] + partNo * m_HRIR_length / 2,
				m_HRIR_length / 2 * sizeof(float));
			fftwf_execute(m_forwardFFT[(int)Channel::LEFT]);

			m_vertices[i].HRTF[(int)Channel::LEFT][partNo] = new fftwf_complex[m_HRIR_length]{};
			memcpy(m_vertices[i].HRTF[(int)Channel::LEFT][partNo],
				m_curComplexFftData[(int)Channel::LEFT], m_HRIR_length * sizeof(fftwf_complex));
		}

		// Right channel HRTF
		m_vertices[i].HRTF[(int)Channel::RIGHT] = new fftwf_complex*[2];
		for (int partNo(0); partNo < 2; ++partNo)
		{
			memset(m_curRealFftData[(int)Channel::RIGHT], 0x0, m_HRIR_length); // Zero padding
			memcpy(m_curRealFftData[(int)Channel::RIGHT],
				tempArr[(int)Channel::RIGHT] + partNo * m_HRIR_length / 2,
				m_HRIR_length / 2 * sizeof(float));
			fftwf_execute(m_forwardFFT[(int)Channel::RIGHT]);

			m_vertices[i].HRTF[(int)Channel::RIGHT][partNo] = new fftwf_complex[m_HRIR_length]{};
			memcpy(m_vertices[i].HRTF[(int)Channel::RIGHT][partNo],
				m_curComplexFftData[(int)Channel::RIGHT], m_HRIR_length * sizeof(fftwf_complex));
		}
	}

	delete[] tempArr[(int)Channel::LEFT];
	delete[] tempArr[(int)Channel::RIGHT];

	sphereMesh.close();
}


void HRTF::processSample(const float** dryInput,
	float** wetOutput, size_t length)
{
	// Finding intersection
	Vector* nearestVertices[3]{}; // Composing triangle
	float smallestLength(1.0);	 // Bc vertices are normalized
	for (uint32_t i(0); i < m_vertexCount; ++i)
	{
		if ((m_vertices[i] - m_soundSource).squaredLength() < smallestLength)
		{
			nearestVertices[2] = nearestVertices[1];
			nearestVertices[1] = nearestVertices[0];
			nearestVertices[0] = m_vertices + i;
		}
	}

	if (nearestVertices[0] == nullptr)
		return;

	// Frequency domain
	for (size_t inpBlockId(0); inpBlockId < length; inpBlockId += m_HRIR_length / 2)
	{
		// Input is mono channel
		memcpy(m_prevInput + 0,
			m_prevInput + m_HRIR_length / 2,
			m_HRIR_length / 2 * sizeof(float));
		memcpy(m_prevInput + m_HRIR_length / 2,
			dryInput[(int)Channel::ANY] + size_t(inpBlockId),
			m_HRIR_length / 2 * sizeof(float));

		memcpy(m_curRealFftData[(int)Channel::ANY], m_prevInput, m_HRIR_length * sizeof(float));

		// Input subframe FFT
		fftwf_execute(m_forwardFFT[(int)Channel::ANY]);
		float tempReal, tempImg;
		for (int partNo(0); partNo < 2; ++partNo)
		{
			for (int i(0); i < m_HRIR_length; ++i)
			{
				tempReal = m_curComplexFftData[(int)Channel::ANY][i][REAL];
				tempImg =  m_curComplexFftData[(int)Channel::ANY][i][IMG];

				// Left channel
				m_accumulator[(int)Channel::LEFT][i][REAL] +=
					(tempReal * nearestVertices[0]->HRTF[(int)Channel::LEFT][partNo][i][REAL]) -
					(tempImg  * nearestVertices[0]->HRTF[(int)Channel::LEFT][partNo][i][IMG]);
				m_accumulator[(int)Channel::LEFT][i][IMG] +=
					(tempReal * nearestVertices[0]->HRTF[(int)Channel::LEFT][partNo][i][IMG]) +
					(tempImg  * nearestVertices[0]->HRTF[(int)Channel::LEFT][partNo][i][REAL]);

				// Right channel
				m_accumulator[(int)Channel::RIGHT][i][REAL] +=
					(tempReal * nearestVertices[0]->HRTF[(int)Channel::RIGHT][partNo][i][REAL]) -
					(tempImg  * nearestVertices[0]->HRTF[(int)Channel::RIGHT][partNo][i][IMG]);
				m_accumulator[(int)Channel::RIGHT][i][IMG] +=
					(tempReal * nearestVertices[0]->HRTF[(int)Channel::RIGHT][partNo][i][IMG]) +
					(tempImg  * nearestVertices[0]->HRTF[(int)Channel::RIGHT][partNo][i][REAL]);
			}
		}

		memcpy(m_curComplexFftData[(int)Channel::LEFT],
			m_accumulator[(int)Channel::LEFT], m_HRIR_length * sizeof(fftwf_complex));
		memcpy(m_curComplexFftData[(int)Channel::RIGHT],
			m_accumulator[(int)Channel::RIGHT], m_HRIR_length * sizeof(fftwf_complex));

		// Lets pretend that algo requires to compute IFFT right here
		// Loop is important for normalizeing FFT result
		fftwf_execute(m_backwardFFT[(int)Channel::LEFT]);
		for (size_t i(0); i < m_HRIR_length / 2; ++i)
			wetOutput[(int)Channel::LEFT][inpBlockId + i] =
			m_FftNormFactor * m_curRealFftData[(int)Channel::LEFT][m_HRIR_length / 2 + i];

		fftwf_execute(m_backwardFFT[(int)Channel::RIGHT]);
		for (size_t i(0); i < m_HRIR_length / 2; ++i)
			wetOutput[(int)Channel::RIGHT][inpBlockId + i] =
			m_FftNormFactor * m_curRealFftData[(int)Channel::RIGHT][m_HRIR_length / 2 + i];
	}


	/*for (size_t inpBlockId(0); inpBlockId < length; inpBlockId += m_HRIR_length)
	{
		memcpy(m_curRealFftData[(int)Channel::ANY],
			dryInput[(int)Channel::ANY] + inpBlockId, sizeof(float) * m_HRIR_length);
		fftwf_execute(m_forwardFFT[(int)Channel::ANY]);

		// Multiplying input with HRTF (using m_curComplexFftData for performance reasons)
		float tempReal, tempImg;
		for (int i(0); i < m_HRIR_length; ++i)
		{
			// Left channel
			tempReal = m_curComplexFftData[(int)Channel::ANY][i][REAL];
			tempImg =  m_curComplexFftData[(int)Channel::ANY][i][IMG];

			m_curComplexFftData[(int)Channel::LEFT][i][REAL] =
				(tempReal * nearestVertices[0]->HRTF[(int)Channel::LEFT][0][i][REAL]) -
				(tempImg  * nearestVertices[0]->HRTF[(int)Channel::LEFT][0][i][IMG]);
			m_curComplexFftData[(int)Channel::LEFT][i][IMG] =
				(tempReal * nearestVertices[0]->HRTF[(int)Channel::LEFT][0][i][IMG]) +
				(tempImg  * nearestVertices[0]->HRTF[(int)Channel::LEFT][0][i][REAL]);

			// Right channel
			m_curComplexFftData[(int)Channel::RIGHT][i][REAL] =
				(tempReal * nearestVertices[0]->HRTF[(int)Channel::RIGHT][0][i][REAL]) -
				(tempImg  * nearestVertices[0]->HRTF[(int)Channel::RIGHT][0][i][IMG]);
			m_curComplexFftData[(int)Channel::RIGHT][i][IMG] =
				(tempReal * nearestVertices[0]->HRTF[(int)Channel::RIGHT][0][i][IMG]) +
				(tempImg  * nearestVertices[0]->HRTF[(int)Channel::RIGHT][0][i][REAL]);
		}

		// Lets pretend that algo requires to compute IFFT right here
		// Loop is important for normalizeing FFT result
		fftwf_execute(m_backwardFFT[(int)Channel::LEFT]);
		for (size_t i(0); i < m_HRIR_length; ++i)
			wetOutput[(int)Channel::LEFT][inpBlockId + i] =
			m_FftNormFactor * m_curRealFftData[(int)Channel::LEFT][i];

		fftwf_execute(m_backwardFFT[(int)Channel::RIGHT]);
		for (size_t i(0); i < m_HRIR_length; ++i)
			wetOutput[(int)Channel::RIGHT][inpBlockId + i] =
			m_FftNormFactor * m_curRealFftData[(int)Channel::RIGHT][i];
	}*/
}


void HRTF::clear()
{

}


bool HRTF::isFinished()
{
	return true;
}