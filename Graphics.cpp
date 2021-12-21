#include "Graphics.h"


Graphics::Graphics(COORD windowSize, size_t frameSize) :
	FRAME_SIZE(frameSize),
	m_windowRect{ 0,0,windowSize.X - 1, windowSize.Y - 1 },
	m_windowSize(windowSize)
{
	// Getting our favourite output handle for console
	m_outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	// Correcting console palette
	CONSOLE_SCREEN_BUFFER_INFOEX csbi{};
	csbi.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	GetConsoleScreenBufferInfoEx(m_outputHandle, &csbi);
	csbi.ColorTable[LIGHT_GRAY]  = RGB(200, 200, 200);
	csbi.ColorTable[DARK_GRAY]  = RGB(130, 130, 130);
	csbi.ColorTable[PALE_GRAY]  = RGB(80,  80,  80);
	csbi.ColorTable[LIGHT_RED]  = RGB(150, 30,  30);
	csbi.ColorTable[DARK_RED]   = RGB(100, 30,  30);
	csbi.ColorTable[ORANGE]     = RGB(255, 165, 0);
	csbi.ColorTable[FIRE_RED_1] = RGB(255, 30,  0);
	csbi.ColorTable[FIRE_RED_2] = RGB(255, 80,  0);
	SetConsoleScreenBufferInfoEx(m_outputHandle, &csbi);

	// Changing console dimensions (scroll bars problem, so call func 2 times)
	// Even after this the problem appears sometimes
	SetConsoleWindowInfo(m_outputHandle, TRUE, &m_windowRect);
	SetConsoleScreenBufferSize(m_outputHandle, m_windowSize);
	SetConsoleWindowInfo(m_outputHandle, TRUE, &m_windowRect);

	// Digital signal processing settings
	m_graphicsBuffer = new CHAR_INFO[(size_t)windowSize.X * windowSize.Y]{};
	m_samples = new float[FRAME_SIZE]{};
	m_smoothFft = new float[FRAME_SIZE]{};
	m_isLeap = new bool[FRAME_SIZE / 2]{}; // Is peak big enough (there is no signal after FRAME_SIZE/2

	// Setting up FFT for visuals
	m_freqs = fftwf_alloc_complex(FRAME_SIZE);
	m_plan = fftwf_plan_dft_r2c_1d((int)FRAME_SIZE, m_samples, m_freqs, FFTW_ESTIMATE);

	// Particles settings
	m_peakDifference = graph_init::peakSensitivity;
	m_maxPartDisplay = graph_init::maxParticles;

	m_parts = new GasParticle[200/*cant be more particles in any case*/]{};
	m_freePartPlaces = new uint64_t[200/*same*/];
	m_ptrToLastFreePlaceIdx = m_freePartPlaces + m_maxPartDisplay/*custom user number*/ - 1;
	for (int i(0); i < 200/*same*/; ++i)
		m_freePartPlaces[i] = i;

	// We use logarithmic scale, so to compact FRAME_SIZE/2 samples into m_windowSize
	// we compute our custom log base
	m_logBase = pow(FRAME_SIZE / 2, 1.0 / m_windowSize.X); // Root

	// Center of the sphere
	m_sphereCenterX = 0.5f * m_windowSize.X;
	m_sphereCenterY = 0.5f * m_windowSize.Y;

	// Center of menus
	m_menuX = short(0.22f * m_windowSize.X);
	m_menuY = short(0.4f * m_windowSize.Y);
	m_subMenuX = short(0.67f * m_windowSize.X);
	m_subMenuY = short(0.4f * m_windowSize.Y);

	// Current title state and info
	m_curState.curTextId = 0;
	m_curState.tState = TitleStates::APPEARING_1;
	m_curState.timeRemains = appearingTime;
}


void Graphics::setAzimuth(float value)
{
	m_azimuth = value;
}

void Graphics::setElevation(float value)
{
	m_elevation = value;
}

void Graphics::setPeakSensitivity(float value)
{
	m_peakDifference = value;
}

void Graphics::setMaxParticles(uint64_t value)
{
	m_maxPartDisplay = min(value, 200);
}

float Graphics::getAzimuth() const
{
	return m_azimuth;
}

float Graphics::getElevation() const
{
	return m_elevation;
}

float Graphics::getPeakSensitivity() const
{
	return m_peakDifference;
}

uint64_t Graphics::getMaxParticles() const
{
	return m_maxPartDisplay;
}

float* Graphics::getSamplesArray()
{
	return m_samples;
}


void Graphics::process(const Menu& menu, uint64_t dTime)
{
	// Clearing screen
	ZeroMemory(m_graphicsBuffer, (size_t)m_windowSize.X * m_windowSize.Y * sizeof(float));

	// FFT
	fftwf_execute(m_plan);

	// Smoothing for better animation
	for (int i(0); i < FRAME_SIZE; ++i)
	{
		float newFreq = (float)sqrt(pow(m_freqs[i][0], 2) + pow(m_freqs[i][1], 2));

		// Detecting sudden leaps (peaks)
		if (i < (FRAME_SIZE >> 1))
			m_isLeap[i] = newFreq - m_smoothFft[i] > m_peakDifference;

		// Smooting amplitude of frequency by using previous ones
		m_smoothFft[i] = (graph_const::smoothness) * m_smoothFft[i] +
				   (1.0f - graph_const::smoothness) * newFreq;
	}

	// Drawing audio spectrum + generating particles
	for (int h(0); h < 0.3 * m_windowSize.Y; ++h)
	{
		// Border (how high amplitudes should be)
		double level = FRAME_SIZE/*normalizing FFT*/ *
			graph_const::fftAmplifier * pow((float)h / m_windowSize.Y, 2);

		// Drawing spectrum
		for (int w(0); w < m_windowSize.X; ++w)
		{
			// Getting index for frequncies array from window horizontal pos
			double value = 0.5/*half if empty*/ * FRAME_SIZE * w / m_windowSize.X;
			uint64_t freqIdx = uint64_t(log(value) / log(m_logBase)); // Quick math
			// If frequncy loud enough
			if (m_smoothFft[freqIdx] >= level)
			{
				uint64_t bufIdx = ((size_t)m_windowSize.Y - h) * m_windowSize.X + w;
				m_graphicsBuffer[bufIdx].Char.AsciiChar = '@';
				m_graphicsBuffer[bufIdx].Attributes = ORANGE;
			}

			// Generating particles
			if (w > 0 && m_isLeap[freqIdx])
			{
				// Computing particles height
				int spawnHeight = m_windowSize.Y - 1; // Appearing from the root of the fire

				// Adding several particles to form a cloud
				for (int i(0); i < 5; ++i)
				{
					if (m_ptrToLastFreePlaceIdx != m_freePartPlaces)
					{
						m_parts[*m_ptrToLastFreePlaceIdx].m_pos =
							new Vec2(float(w + partPosDist(re)), float(spawnHeight + partPosDist(re)));
						m_parts[*m_ptrToLastFreePlaceIdx].m_decay = partDecayTimelDist(re);

						--m_ptrToLastFreePlaceIdx;
					}
				}

				// Zeroing peak to not generate too much particles
				m_isLeap[freqIdx] = false;
			}
		}
	}

	// Drawing particles
	partsUpdate(); // Updating fire simulation
	for (int i(0); i < m_maxPartDisplay; ++i)
	{
		// If particle doesnt exist
		if (!m_parts[i].m_pos)
			continue;

		uint64_t idx = uint64_t((uint64_t)m_parts[i].m_pos->m_y * m_windowSize.X +
			(uint64_t)m_parts[i].m_pos->m_x);
		m_graphicsBuffer[idx].Char.AsciiChar = '#';

		// Particles have custom fire colors and can be faded
		if (m_parts[i].m_decay > 0.0f) {
			m_graphicsBuffer[idx].Attributes = FIRE_RED_1 + partColDist(re);
		} else {
			m_graphicsBuffer[idx].Attributes = PALE_GRAY;
		}
	}

	// Drawing sphere with slider
	double curPosProjAz(fmod(m_azimuth + 90.0, 360.0)); // 0 angle should be to the left
	double curPosProjEl(0.0f);
	WORD colors;

	// From sphrical coordinates to cartesian
	double sphereWidth = (std::size(sphereImg[0]) - 2) *
		cos(M_PI * m_elevation / 180);

	// Azimuthal position
	if (curPosProjAz > 180) // Behind the head
	{
		curPosProjAz = ((curPosProjAz - 180) / 180 - 0.5) * sphereWidth + 0.5 * std::size(sphereImg[0]);
		colors = 17 * LIGHT_RED;
	}
	else // In front of the head
	{
		curPosProjAz = ((180 - curPosProjAz) / 180 - 0.5) * sphereWidth + 0.5 * std::size(sphereImg[0]);
		colors = 16 * DARK_RED + DARK_GRAY;
	}
	// Elevation
	curPosProjEl = (90.0 - m_elevation) * std::size(sphereImg) / 180;

	// Correcting sphere center (can be done in ctor)
	uint64_t sphereOffsetX = uint64_t(m_sphereCenterX - 0.5 * std::size(sphereImg[0]));
	uint64_t sphereOffsetY = uint64_t(m_sphereCenterY - 0.5 * std::size(sphereImg));
	for (int h(0); h < std::size(sphereImg); ++h)
	{
		for (int w(0); w < std::size(sphereImg[0]) - 1; ++w)
		{
			// Dont draw spaces, so sphere in round but not squared
			if (sphereImg[h][w] == ' ')
				continue;

			size_t idx = (h + sphereOffsetY) * (size_t)m_windowSize.X + w + sphereOffsetX;
			m_graphicsBuffer[idx].Char.AsciiChar = sphereImg[h][w];

			// Sphere is gray and virtual source pointer is red
			if (pow(w - curPosProjAz, 2) + pow(2 * (h - curPosProjEl), 2) < graph_const::posRadius) {
				m_graphicsBuffer[idx].Attributes = colors;
			} else {
				m_graphicsBuffer[idx].Attributes = DARK_GRAY;
			}
		}
	}

	// Drawing menu
	drawMenu(menu);

	// Drawing titles
	drawTitles(dTime);

	renderBuffer();
}


void Graphics::renderBuffer()
{
	static COORD coordBufCoord{ 0, 0 };

	// Rendering frame to the screen
	WriteConsoleOutput(m_outputHandle,
		m_graphicsBuffer, // buffer to copy from
		m_windowSize,     // col-row size of chiBuffer
		coordBufCoord,    // top left src cell in chiBuffer
		&m_windowRect);   // dest. screen buffer rectangle
}


void Graphics::partsUpdate()
{
	// Some simulation things that are better explained on the internet
	computeDensityPressure();
	computeForces();
	integrate();
}


void Graphics::computeDensityPressure()
{
	for (int i(0); i < m_maxPartDisplay; ++i)
	{
		if (!m_parts[i].m_pos)
			continue;

		m_parts[i].m_dens = 0.0f;
		for (int j(0); j < m_maxPartDisplay; ++j)
		{
			if (!m_parts[j].m_pos)
				continue;

			Vec2 rij = *m_parts[j].m_pos + -*m_parts[i].m_pos;
			float r2 = rij.squaredLength();

			if (r2 < particle_const::HSQ)
			{
				// This computation is symmetric
				m_parts[i].m_dens += particle_const::MASS *
					particle_const::POLY6 * pow(particle_const::HSQ - r2, 3.0f);
			}
		}
		m_parts[i].m_pres = particle_const::GAS_CONST * (m_parts[i].m_dens - particle_const::REST_DENS);
	}
}


void Graphics::computeForces()
{
	for (int i(0); i < m_maxPartDisplay; ++i)
	{
		if (!m_parts[i].m_pos)
			continue;

		Vec2 fpress{};
		Vec2 fvisc{};

		for (int j(0); j < m_maxPartDisplay; ++j)
		{
			if (!m_parts[j].m_pos || i == j)
				continue;

			Vec2 rij = *m_parts[j].m_pos + -*m_parts[i].m_pos;
			float r = rij.length();

			if (r < particle_const::H)
			{
				// compute pressure force contribution
				fpress = fpress + -rij.normalize() * particle_const::MASS *
					(m_parts[i].m_pres + m_parts[j].m_pres) *
					(1 / (2.0f * m_parts[j].m_dens)) * particle_const::SPIKY_GRAD
					* pow(particle_const::H - r, 2.0f);
				// compute viscosity force contribution
				fvisc = fvisc + (m_parts[j].m_vel + -m_parts[i].m_vel) *
					particle_const::VISC * particle_const::MASS *
					(1 / m_parts[j].m_dens) * particle_const::VISC_LAP *
					(particle_const::H - r);
			}
		}
		Vec2 fgrav = Vec2(0.0f, GasParticle::gravity) * m_parts[i].m_dens;
		Vec2 part2sphere(m_parts[i].m_pos->m_x - m_sphereCenterX, m_parts[i].m_pos->m_y - m_sphereCenterY);
		// Some big bois here
		Vec2 fsphere = part2sphere.normalize() * (60000000 / (float)pow(part2sphere.length(), 2));

		m_parts[i].m_force = fpress + fvisc + fgrav + fsphere;
	}
}


void Graphics::integrate()
{
	for (int i(0); i < m_maxPartDisplay; ++i)
	{
		// What the hell do you want from me
		auto& p = m_parts[i];

		// If particle does not exist
		if (!p.m_pos)
			continue;

		// forward Euler integration
		p.m_vel = p.m_vel + p.m_force * (GasParticle::DT / p.m_dens);
		*p.m_pos = *p.m_pos + p.m_vel * GasParticle::DT;
		p.m_decay -= GasParticle::DT;

		auto eps = particle_const::EPS;

		// Enforce boundary conditions
		// And delete particles to save memory if needed
		if (p.m_pos->m_x - eps < 0.0f ||
			p.m_pos->m_x + eps > m_windowSize.X ||
			p.m_pos->m_y - eps < 0.0f ||
			p.m_pos->m_y + eps > m_windowSize.Y)
		{
			delete p.m_pos;
			ZeroMemory(m_parts + i, sizeof(GasParticle));

			m_ptrToLastFreePlaceIdx++;
			*m_ptrToLastFreePlaceIdx = i;
		}
	}
}


void Graphics::drawMenu(const Menu& menu)
{
	if (!menu.isShown())
		return;

	short menuCount = menu.getCurMenu();
	short subMenuCount = menu.getCurSubMenu();

	// Main menu items
	for (short i(0); i < std::size(MENU_NAMES); ++i)
	{
		for (short letter(0); letter < std::size(MENU_NAMES[0][0]); ++letter)
		{
			short bufIdx = (i + m_menuY) * m_windowSize.X + letter + m_menuX;
			m_graphicsBuffer[bufIdx].Char.AsciiChar = MENU_NAMES[i][0][letter];
			m_graphicsBuffer[bufIdx].Attributes = WHITE;
		}
	}

	// Submenu items
	for (short i(0); i < menu.getSubMenuItemsCount(); ++i)
	{
		for (short letter(0); letter < std::size(MENU_NAMES[0][0]); ++letter)
		{
			short bufIdx = (i + m_subMenuY) * m_windowSize.X + letter + m_subMenuX;
			m_graphicsBuffer[bufIdx].Char.AsciiChar =
				MENU_NAMES[menuCount][i + 1][letter];
			m_graphicsBuffer[bufIdx].Attributes = WHITE;
		}

		// The 0's one is the name of submenu, so start counting from 1
		auto buf = menu.getCurrentValue(menuCount, i + 1);

		// sprintf can shit into your buffer so the second condition is needed
		for (short letter(0); letter < 8 && buf[letter] != '\0'; ++letter)
		{
			short bufIdx = (i + m_subMenuY) * m_windowSize.X +
				(short)std::size(MENU_NAMES[0][0]) + letter + m_subMenuX;
			m_graphicsBuffer[bufIdx].Char.AsciiChar = buf[letter];
			m_graphicsBuffer[bufIdx].Attributes = WHITE;
		}
	}

	// Arrows
	short bufIdx;
	if (subMenuCount == -1) {
		bufIdx = (menuCount + m_menuY) * m_windowSize.X + m_menuX - 2;
	} else {
		bufIdx = (subMenuCount + m_subMenuY) * m_windowSize.X + m_subMenuX - 2;
	}
	m_graphicsBuffer[bufIdx].Char.AsciiChar = '>';
	m_graphicsBuffer[bufIdx].Attributes = WHITE;
}


void Graphics::drawTitles(int64_t dTime)
{
	// If titles are finished, return
	if (m_curState.curTextId >= std::size(subtitlesText))
		return;

	WORD titleColor(0);
	m_curState.timeRemains -= dTime;

	// Choosing in what state titles are now
	// And updating info if needed
	switch (m_curState.tState)
	{
	case TitleStates::APPEARING_1:
		titleColor = PALE_GRAY;
		if (m_curState.timeRemains <= 0) {
			m_curState.tState = TitleStates::APPEARING_2;
			m_curState.timeRemains = appearingTime;
		}
		break;
	case TitleStates::APPEARING_2:
		titleColor = DARK_GRAY;
		if (m_curState.timeRemains <= 0) {
			m_curState.tState = TitleStates::APPEARING_3;
			m_curState.timeRemains = appearingTime;
		}
		break;
	case TitleStates::APPEARING_3:
		titleColor = LIGHT_GRAY;
		if (m_curState.timeRemains <= 0) {
			m_curState.tState = TitleStates::READABLE;
			m_curState.timeRemains = readingTime;
		}
		break;
	case TitleStates::READABLE:
		titleColor = WHITE;
		if (m_curState.timeRemains <= 0) {
			m_curState.tState = TitleStates::DISAPPEARING_1;
			m_curState.timeRemains = disappearingTime;
		}
		break;
	case TitleStates::DISAPPEARING_1:
		titleColor = LIGHT_GRAY;
		if (m_curState.timeRemains <= 0) {
			m_curState.tState = TitleStates::DISAPPEARING_2;
			m_curState.timeRemains = disappearingTime;
		}
		break;
	case TitleStates::DISAPPEARING_2:
		titleColor = DARK_GRAY;
		if (m_curState.timeRemains <= 0) {
			m_curState.tState = TitleStates::DISAPPEARING_3;
			m_curState.timeRemains = disappearingTime;
		}
		break;
	case TitleStates::DISAPPEARING_3:
		titleColor = PALE_GRAY;
		if (m_curState.timeRemains <= 0) {
			m_curState.tState = TitleStates::HIDDEN;
			m_curState.timeRemains = hiddenTime;
		}
		break;
	case TitleStates::HIDDEN:
		if (m_curState.timeRemains <= 0) {
			m_curState.tState = TitleStates::APPEARING_1;
			m_curState.timeRemains = appearingTime;
			m_curState.curTextId++;
		}
		return;
	}

	// Drawing text from titles
	for (short line(0); line < 3; ++line)
	{
		short printPosX = short(0.5f * (m_windowSize.X -
			strlen(subtitlesText[m_curState.curTextId][line])));

		for (short letter(0); letter <
			std::size(subtitlesText[m_curState.curTextId][line]); ++letter)
		{
			short bufIdx = (line + 1) * m_windowSize.X + printPosX + letter;
			m_graphicsBuffer[bufIdx].Char.AsciiChar =
				subtitlesText[m_curState.curTextId][line][letter];
			m_graphicsBuffer[bufIdx].Attributes = titleColor;
		}
	}
}