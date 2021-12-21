#pragma once

#include <filesystem>
#include <Windows.h>
#include <fstream>
#include <thread>
#include <mutex>

#include "Effects.h"

#pragma comment(lib, "winmm.lib") // Standard audio lib import


// Constructor accepts only mono 16 bits wav audio recordings
class WavPlayer
{
public:
	// Wav file name, and on/off looping audio
	WavPlayer(const char* name, std::wstring sOutputDevice, bool loop);

	// Play wav file and effects
	void play();
	// Pause audio (applies only to wav file samples, not effects, you cant pause them)
	void pause();
	// Apply any IEffect or combinations of them (order is NOT important)
	void applyEffect(IEffect& effect);

	// Set dry wav file samples gain
	void setDry(float percent);
	// Set wet effects samples gain (applies to all effects simultaneously)
	void setWet(float percent);

	// Hope you dont search for explanation
	float getDry() const;
	float getWet() const;

	// External access to last read wav samples
	void getLastSamples(float* receiver) const;
	// How much samples are processed and sent to an audiocard at a time
	size_t getFrameSize() const;

	// Check if all samples from wav are read
	// and start from the beginning if needed
	void isFinished();

	~WavPlayer();

	// List all devices connected to your PC that can emit sound
	static std::vector<std::wstring> getDevices();

private:
	// Check getFrameSize
	static constexpr size_t FRAME_SIZE = 2'048;
	
	// Wav file related info
	std::ifstream m_file;
	std::string m_fileNameWithoutPath;
	int16_t* m_wavSamples;

	// Dry sound and effects related data
	std::vector<IEffect*> m_effects{}; // All effects that are currently applied to dry sound
	std::atomic<float*> m_dryBuffer[2]; // Stereo dry buffer with original sound
	float* m_wetBuffer[2]; // Stereo wet buffer with effects
	float* m_mixBuffer[2]; // Stereo dry + wet buffer, used internally and externally
	float m_dry{ 1.0f }; // Dry signal gain (check setDry)
	float m_wet{ 1.0f }; // Wet signal gain (check setWet)

	WAVEHDR m_waveHeaders[2]{}; // Multi-buffering
	HWAVEOUT m_hwDevice; // Choosen output device
	uint64_t m_samplesPlayed; // How much samples from wav file has been played

	/* WAV HEADER */
	char chunkId[4]{};        // Sign "RIFF"
	uint32_t chunkSize{};     // Wav file size minus 8 bytes
	char format[4]{};         // Sign "WAVE"
	char subchunk1Id[4]{};    // Sign "fmt "
	uint32_t subchunk1Size{}; // Number 16 (only PCM format supported)
	uint16_t audioFormat{};   // Number 1 (no compression for PCM)
	uint16_t numChannel{};    // Number 1 (only mono supported)
	uint32_t sampleRate{};    // Any sample rate
	uint32_t byteRate{};      // Bytes read per second (2 * sampleRate)
	uint16_t blockAlign{};    // Number 2 (bytes ber 1 sample and mono)
	uint16_t bitsPerSample{}; // Number 16 (bits ber 1 sample)
	char subchunk2Id[4]{};    // Sign "data"
	uint32_t subchunk2Size{}; // Data size (actually no - "random" number)
	/* WAV HEADER */

	// Windows handler for user to know when audiocard finished playing 1 frame of samples
	HANDLE m_done;
	std::thread m_playingThread; // Main thread, that process all audio
	std::mutex soundBufferMux[2]{}; // Mutex for m_waveHeaders audio buffer
	std::atomic<bool> isLooped; // Is audio from wav file looped
	std::atomic<bool> isStopped{ false }; // Is audio from wav file is not playing (paused)

	// Function for m_playingThread
	void playingThreadFunc();

	// Load next samples frame from wav file
	void loadNextSamples();
	// Combine dry and wet sound, changing the coord system from float to 16 bits int
	void generateWave(int bufferId);
};
