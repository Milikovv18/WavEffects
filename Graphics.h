#pragma once

#include <random>
#include <Windows.h>
#include <functional>
#include <corecrt_math_defines.h>
#include "FFTW/fftw3.h"

#include "GasParticle.h"
#include "Effects.h"
#include "Menu.h"


// Ready to go ASCII image of the sphere,
// we dont want to waste time on ray tracing
constexpr char sphereImg[17][36]
{
	"             `.------.`            ",
	"         .:+syhdddhyo+//:-.        ",
	"      `-+ydmdhyso+++////////-`     ",
	"     -+sdddyo///::////////////.    ",
	"    :+yddyo/:::::::::////+++///-   ",
	"   :+oddy+:::----::::///oyhyo///-  ",
	"  .//ohy/::-------::://+hmmmdo//:. ",
	"  ://+o+:---------::::/+hmmmds//:- ",
	"  ::::::---.....---::://ohddy+//:- ",
	"  -:::----.......---:::///++////:- ",
	"  .:-----........----:::://///:::` ",
	"   -----..........-----::::::::--  ",
	"    .-.......`......------:::--.   ",
	"     `.......``.......--------`    ",
	"       `.....````...........`      ",
	"        ``.....```......``         ",
	"             ``````````            ",
};

// Text used in subtitles above ray traced sphere
constexpr char subtitlesText[22][3][65]
{
	{
		"Welcome to The WavEffects!",
		"Finally it is finished.",
		"This whole project took about 1 month and a lot of research."
	},
	{
		"But now, when its ready,",
		"you can enjoy your beautiful songs again.",
		"Oh, and maybe a should explain this concept a bit."
	},
	{
		"Lets start from basics and think about why only",
		"weird mono .wav files with no compression and",
		"16 bit sample depth are availble?"
	},
	{
		"Well, because this Proga only needs some samples",
		"to test different cool audio effects.",
		"Its not a music player or smth."
	},
	{
		"So you only requred to pass this program any sample container.",
		"Yeah, you are right, if you convert any of your sound",
		"to requested format it can easily be played with all FX enabled!"
	},
	{
		"Hmm, \"FX\"? Oh this smart words again...",
		"Ok.. I'll try to explaing further.",
		"You can hear 2 different audio effects now. Guess what?"
	},
	{
		"No, you are wrong, but thats not a problem.",
		"Right now you can hear reverberation and binauralization",
		"Wasnt so hard to guess I think."
	},
	{
		"Reverberation is a very simple effect,",
		"you just need working tuned algorithm on hand.",
		"Or you can tune it by yourself."
	},
	{
		"Reverberation always happens when you sitting in your room,",
		"or, a better example, in a special acustic hall.",
		"It just represents sound waves bounced from walls around you."
	},
	{
		"At first you can hear sound directly from the source.",
		"Next, you hear almost exacly the same sound (about 5ms long),",
		"but it gets to your ears after a little travel across the room."
	},
	{
		"That's called \"early reflections\".",
		"Everyting past that (5ms+) is called \"late reflections\"",
		"that consist of noize and doesnt even look like initial sound"
	},
	{
		"Ultimately, this exact Proga is using \"Freeverb\" algorithm.",
		"You can find all info about it on the official cite.",
		"But we are going deeper and the difficulty increases."
	},
	{
		"The second effect is binauralization.",
		"It allows you to hear the sound like it's somewhere",
		"in your room. In other words, it creates \"virtual source\""
	},
	{
		"This shit can't be done via any algorithms in near feature.",
		"If you think it only requires panning, you are totally wrong.",
		"And you can prove it by listening only one channel of audio."
	},
	{
		"Even without reverb it always plays something.",
		"So how can it even be possible? Imagine you'r sitting.",
		"And there is a sphere of speakers around you."
	},
	{
		"Also you have 2 microphones in your ears very close",
		"to eardrums. The noize sequentially plays from the speakers",
		"and microphones in your ears are constantly recording."
	},
	{
		"When every speaker expressed its opinion, you are done.",
		"After that you find the difference between original sound",
		"and the sound that you heared. You save this stuff in a file"
	},
	{
		"From that moment you can apply this difference (called HRIR -",
		"Head related impulse response) to any sound you want by",
		"convolving original sound with HRIR"
	},
	{
		"Of course the convolution is applied in a frequency domain",
		"(by Fourier transform) for performance. But here is a catch.",
		"As I mentioned, the HRIR is measured for 1 human specifically."
	},
	{
		"And it can sound not so natural for another human.",
		"So, if you dont like what you hear tell me to send you",
		"another HRIR file."
	},
	{
		"Thats all about FX, hope you understood something.",
		"Also these effects are highly customizable.",
		"Finally, press ESC to open the effects menu."
	}
};


namespace graph_const
{
	// Making FFT look great on the screen
	constexpr double fftAmplifier = 0.2;
	// Red dot representating speaker on the normalized sphere
	constexpr float posRadius = 8.0f;
	// Smoothness of FFT amplitude changes on the screen
	constexpr float smoothness = 0.3f;
}

namespace graph_init
{
	// When frequency leap occurs - some particles spawned
	constexpr float peakSensitivity = 50.0f;
	// Max amount of particles on the screen at the same time
	constexpr uint64_t maxParticles = 50;
}


class Graphics
{
	// New custom color names for console
	enum Colors
	{
		LIGHT_GRAY = 1,
		DARK_GRAY = 2,
		PALE_GRAY = 3,

		LIGHT_RED = 4,
		DARK_RED = 5,
		ORANGE = 6,
		FIRE_RED_1 = 7,
		FIRE_RED_2 = 8,

		WHITE = 15
	};

	// Titles are appering from the dark
	// And disappearing in the dark
	enum class TitleStates
	{
		APPEARING_1,
		APPEARING_2,
		APPEARING_3,
		READABLE,
		DISAPPEARING_1,
		DISAPPEARING_2,
		DISAPPEARING_3,
		HIDDEN
	};

	// Current [dis]appearing title state
	struct CurrentState
	{
		uint64_t curTextId;
		TitleStates tState;
		int64_t timeRemains;
	};

public:
	// Preffered console window dimensions
	// and engine sound frame size in samples
	Graphics(COORD windowSize, size_t frameSize);

	// Process one frame
	void process(const Menu& menu, uint64_t dTime);

	void setAzimuth(float value);
	void setElevation(float value);
	void setPeakSensitivity(float value);
	void setMaxParticles(uint64_t value);

	float getAzimuth() const;
	float getElevation() const;
	float getPeakSensitivity() const;
	uint64_t getMaxParticles() const;
	float* getSamplesArray();

private:
	// Ctor-time constants
	const size_t FRAME_SIZE;
	double m_logBase;
	float m_sphereCenterX;
	float m_sphereCenterY;
	short m_menuX, m_menuY; // Menu pos
	short m_subMenuX, m_subMenuY; // Submenu pos

	// Console output stuff
	SMALL_RECT m_windowRect;
	COORD m_windowSize;

	HANDLE m_outputHandle;
	CHAR_INFO* m_graphicsBuffer;

	// Render buffer to the screen
	void renderBuffer();

	// Digital signal processing stuff
	float* m_samples;
	float* m_smoothFft;
	bool* m_isLeap;
	fftwf_complex* m_freqs;
	fftwf_plan m_plan;

	float m_azimuth{ 0.0f }, m_elevation{ 0.0f };

	// Particles simulation
	GasParticle* m_parts;
	uint64_t* m_freePartPlaces; // For performance
	uint64_t* m_ptrToLastFreePlaceIdx;
	uint64_t m_maxPartDisplay;
	float m_peakDifference;
	
	void partsUpdate(); // Calls all 3 functions below internally
	void computeDensityPressure();
	void computeForces();
	void integrate();

	std::default_random_engine re{ 0 };
	const std::uniform_real_distribution<float> partPosDist{ -0.5f, 0.5f };
	const std::uniform_real_distribution<float> partDecayTimelDist{ 0.006f, 0.04f };
	const std::uniform_int_distribution<int> partColDist{ 0, 1 };

	// Menu
	void drawMenu(const Menu& menu);

	// Titles
	const int64_t appearingTime = (uint64_t)100e+6;
	const int64_t readingTime = (uint64_t)10e+9;
	const int64_t disappearingTime = (uint64_t)100e+6;
	const int64_t hiddenTime = (uint64_t)100e+6;

	CurrentState m_curState;
	// Draw title text
	void drawTitles(int64_t dTime); // Delta time in nanosecs from prev frame
};
