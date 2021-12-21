#include "WavPlayer.h"


WavPlayer::WavPlayer(const char* name, std::wstring sOutputDevice, bool loop)
	: isLooped(loop)
{
	// READING WAV FILE
	m_file = std::ifstream(name, std::ios::binary);
	if (!m_file.is_open())
		throw "Bad file";

	// Didnt even used the song name :(
	m_fileNameWithoutPath = std::filesystem::path(name).filename().string();

	// Reading wav header
	m_file.read((char*)&chunkId, 4);
	m_file.read((char*)&chunkSize, 4);
	m_file.read((char*)&format, 4);
	m_file.read((char*)&subchunk1Id, 4);
	m_file.read((char*)&subchunk1Size, 4);
	m_file.read((char*)&audioFormat, 2);
	m_file.read((char*)&numChannel, 2);
	m_file.read((char*)&sampleRate, 4);
	m_file.read((char*)&byteRate, 4);
	m_file.read((char*)&blockAlign, 2);
	m_file.read((char*)&bitsPerSample, 2);
	m_file.read((char*)&subchunk2Id, 4);
	m_file.read((char*)&subchunk2Size, 4);

	// Checking requirements
	if (numChannel != 1)
		throw "Only mono";

	if (subchunk1Size != 16)
		throw "Only 16 bits";

	// Setting up winmm
	std::vector<std::wstring> devices = getDevices();
	auto d = std::find(devices.begin(), devices.end(), sOutputDevice);
	if (d != devices.end())
	{
		// Device is available
		size_t nDeviceID = distance(devices.begin(), d);
		WAVEFORMATEX waveFormat;
		waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		waveFormat.nSamplesPerSec = sampleRate;
		waveFormat.wBitsPerSample = 16; // bitsPerSample
		waveFormat.nChannels = 2; // Stereo 3d positioning
		waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
		waveFormat.cbSize = 0;

		// Windows callback when audiocard finished its work
		m_done = CreateEvent(0, false, false, 0);

		// Open Device if valid
		if (waveOutOpen(&m_hwDevice, (uint32_t)nDeviceID, &waveFormat, (DWORD_PTR)m_done, (DWORD_PTR)this, CALLBACK_EVENT) != S_OK)
		{
			throw "Invalid device";
			return;
		}
	}

	// Allocate Wave|Block Memory
	for (int i(0); i < std::size(m_waveHeaders); ++i)
	{
		m_waveHeaders[i].lpData = new char[2 * sizeof(int16_t) * FRAME_SIZE]; // Stereo
		m_waveHeaders[i].dwBufferLength = 2 * sizeof(int16_t) * FRAME_SIZE; // Stereo
		if (!m_waveHeaders[i].lpData)
		{
			throw "No memory allocated";
			return;
		}
		ZeroMemory(m_waveHeaders[i].lpData, 2 * sizeof(int16_t) * FRAME_SIZE); // Stereo

		waveOutPrepareHeader(m_hwDevice, &m_waveHeaders[i], sizeof(WAVEHDR));
		waveOutWrite(m_hwDevice, &m_waveHeaders[i], sizeof(WAVEHDR));
	}

	// Dry raw samples from file
	m_wavSamples = new int16_t[FRAME_SIZE]{}; // Mono (from file)

	// Initializing sound buffers
	m_dryBuffer[(int)Channel::LEFT]  = new float[FRAME_SIZE]; // Stereo
	m_dryBuffer[(int)Channel::RIGHT] = new float[FRAME_SIZE];
	m_wetBuffer[(int)Channel::LEFT]  = new float[FRAME_SIZE]; // Stereo
	m_wetBuffer[(int)Channel::RIGHT] = new float[FRAME_SIZE];
	m_mixBuffer[(int)Channel::LEFT]  = new float[FRAME_SIZE]; // Stereo
	m_mixBuffer[(int)Channel::RIGHT] = new float[FRAME_SIZE];

	// Yeah
	IEffect::setSampleRate(sampleRate);

	// Start playing
	m_playingThread = std::thread(&WavPlayer::playingThreadFunc, this);
	m_playingThread.detach();
}


void WavPlayer::play()
{
	isStopped = false;
}

void WavPlayer::pause()
{
	isStopped = true;
}


void WavPlayer::applyEffect(IEffect& effect)
{
	// Add effect to queue
	m_effects.push_back(&effect);
}


void WavPlayer::playingThreadFunc()
{
	// Who even needs this?
	double dTimeStep = 1.0 / sampleRate;

	// Playing forever
	while (true)
	{
		// Waiting for audiocard to finish its work
		WaitForSingleObject(m_done, INFINITE);
		ResetEvent(m_done);

		// Search which buffer became free
		for (int bufId(0); bufId < std::size(m_waveHeaders); ++bufId)
		{
			if (m_waveHeaders[bufId].dwFlags & WHDR_DONE)
			{
				soundBufferMux[bufId].lock();

				loadNextSamples();

				// Dry MONO samples
				for (int sampleNo(0); sampleNo < FRAME_SIZE; ++sampleNo)
				{
					// Making 16 bit samples uniform [-1.0; 1.0]
					m_dryBuffer[(int)Channel::LEFT][sampleNo] =
					m_dryBuffer[(int)Channel::RIGHT][sampleNo] =
						float(0.000030517578125/*one over 32768*/ * m_wavSamples[sampleNo]);
				}

				// Wet STEREO sampling
				ZeroMemory(m_wetBuffer[0], sizeof(float) * FRAME_SIZE);
				ZeroMemory(m_wetBuffer[1], sizeof(float) * FRAME_SIZE);
				for (IEffect* e : m_effects) {
					// Вот сам он не справится...
					e->processSample((const float**)m_dryBuffer, (float**)m_wetBuffer, FRAME_SIZE);
				}

				// Generate 16 bit samples back from float samples
				generateWave(bufId);
				// Sending samples to audiocard
				waveOutWrite(m_hwDevice, &m_waveHeaders[bufId], sizeof(WAVEHDR));

				soundBufferMux[bufId].unlock();

				// Checking if song is finished
				isFinished();
			}
		}
	}
}

void WavPlayer::loadNextSamples()
{
	// If not playing no new samples
	if (isStopped) {
		memset(m_wavSamples, 0x0, sizeof(int16_t) * FRAME_SIZE);
	} else {
		m_file.read((char*)m_wavSamples, sizeof(int16_t) * FRAME_SIZE);
	}
}

void WavPlayer::generateWave(int bufferId)
{
	for (size_t blockPos(0); blockPos < FRAME_SIZE; ++blockPos)
	{
		// Mixing dry and wet signals
		m_mixBuffer[(int)Channel::LEFT][blockPos] =
			m_dry * m_dryBuffer[(int)Channel::LEFT][blockPos] +
			m_wet * m_wetBuffer[(int)Channel::LEFT][blockPos];
		m_mixBuffer[(int)Channel::RIGHT][blockPos] =
			m_dry * m_dryBuffer[(int)Channel::RIGHT][blockPos] +
			m_wet * m_wetBuffer[(int)Channel::RIGHT][blockPos];

		// Converting to signed 16 bit
		((int16_t&)m_waveHeaders[bufferId].lpData[sizeof(int16_t) * ((blockPos << 1) + 0ULL)]) =
			int16_t(32'768 * m_mixBuffer[(int)Channel::LEFT][blockPos]);

		((int16_t&)m_waveHeaders[bufferId].lpData[sizeof(int16_t) * ((blockPos << 1) + 1ULL)]) =
			int16_t(32'768 * m_mixBuffer[(int)Channel::RIGHT][blockPos]);
	}
}


void WavPlayer::setDry(float percent)
{
	m_dry = percent;

	if (percent < 0.0f)
		m_dry = 0.0;
	else if (percent > 1.0f)
		m_dry = 1.0;
}

void WavPlayer::setWet(float percent)
{
	m_wet = percent;

	if (percent < 0.0f)
		m_wet = 0.0;
	else if (percent > 1.0f)
		m_wet = 1.0;
}

float WavPlayer::getDry() const
{
	return m_dry;
}

float WavPlayer::getWet() const
{
	return m_wet;
}


void WavPlayer::getLastSamples(float* receiver) const
{
	if (!receiver)
		return;

	memcpy(receiver, m_dryBuffer[(int)Channel::ANY], FRAME_SIZE * sizeof(float));
}


size_t WavPlayer::getFrameSize() const
{
	return FRAME_SIZE;
}


void WavPlayer::isFinished()
{
	// If wav file is ended
	if (m_file.eof())
	{
		// Pause
		isStopped = true;

		if (isLooped)
		{
			// Clear everything and start from the beginning
			m_samplesPlayed = 0;
			isStopped = false;

			m_file.clear();
			m_file.seekg(44, m_file.beg);

			for (IEffect* e : m_effects)
				e->clear();
		}
		else // Not looped
		{
			pause();
			return;
		}
	}
}


WavPlayer::~WavPlayer()
{
	// Stopping thread
	CloseHandle(m_done);
	pause();
	if (m_playingThread.joinable())
		m_playingThread.join();

	// Delete things that thread used
	m_file.close();
	for (int i(0); i < std::size(m_waveHeaders); ++i)
		delete[] m_waveHeaders[i].lpData;
	delete[] m_dryBuffer[0];
	delete[] m_dryBuffer[1];
	delete[] m_wetBuffer[0];
	delete[] m_wetBuffer[1];
	delete[] m_wavSamples;
}


std::vector<std::wstring> WavPlayer::getDevices()
{
	int nDeviceCount = waveOutGetNumDevs();
	std::vector<std::wstring> sDevices;
	WAVEOUTCAPS woc;
	// Storing all available audio output devices
	for (int n = 0; n < nDeviceCount; n++)
		if (waveOutGetDevCaps(n, &woc, sizeof(WAVEOUTCAPS)) == S_OK)
			sDevices.push_back(woc.szPname);
	return sDevices;
}