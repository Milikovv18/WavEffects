#include "Menu.h"
#include "Graphics.h" // Fuck c++


const char MENU_NAMES[5][6][18]
{
	{
		"General          ",
		"Dry sound        ",
		"Wet sound        "
	},
	{
		"Graphics         ",
		"Peak sensitivity ",
		"Max particles    "
	},
	{
		"Binaurality      ",
		"Enabled          ",
		"Animated         ",
		"Azimuth          ",
		"Elevation        "
	},
	{
		"Particles        ",
		"Gravity          ",
		"dTime as 10^     "
	},
	{
		"Reverberation    ",
		"Enabled          ",
		"Room size        ",
		"Wave damping     ",
		"Wet gain         ",
		"Sound width      "
	}
};


Menu::Menu(WavPlayer& player, Graphics& graph, Binaurality& bin, Freeverb& reverb) :
	m_curMenu(0), m_curSubMenu(-1), m_isShown(false), m_isPressed(false),
	m_player(player), m_graph(graph), m_bin(bin), m_reverb(reverb)
{
	// Hello
}


void Menu::updateInput()
{
	// Arrow up (prev menu item)
	if (GetAsyncKeyState(VK_UP) & 0x8000)
	{
		if (m_isPressed)
			return;
		m_isPressed = true;

		if (m_curMenu > 0 && m_curSubMenu == -1)
			m_curMenu--;
		else if (m_curSubMenu > 0)
			m_curSubMenu--;
	}
	// Arrow down (next menu item)
	else if (GetAsyncKeyState(VK_DOWN) & 0x8000)
	{
		if (m_isPressed)
			return;
		m_isPressed = true;

		if (m_curMenu != std::size(MENU_NAMES) - 1 && m_curSubMenu == -1)
			m_curMenu++;
		// Each submenu has custom number of params
		else if (m_curSubMenu != -1 && m_curSubMenu != getSubMenuItemsCount() - 1)
			m_curSubMenu++;
	}
	// Left arrow (from submenu to main menu)
	else if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		if (m_isPressed)
			return;
		m_isPressed = true;

		if (m_curSubMenu != -1) {
			m_curSubMenu = -1;
		}
	}
	// Right arrow (from menu to submenu)
	else if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		if (m_isPressed)
			return;
		m_isPressed = true;

		if (m_curSubMenu == -1) {
			m_curSubMenu = 0;
		}
	}
	// Escape key (show/hide menu)
	else if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
	{
		if (m_isPressed)
			return;
		m_isPressed = true;

		m_isShown = !m_isShown;
	}
	else
	{
		m_isPressed = false;
	}

	// Changing values with numpad
	if (m_curSubMenu == -1)
		return;

	if (GetAsyncKeyState(VK_NUMPAD4) & 0x8000)
	{
		changeValue(DEC);
	}
	else if (GetAsyncKeyState(VK_NUMPAD6) & 0x8000)
	{
		changeValue(INC);
	}
}

// Fuck c++ 2.0
// Uniform function for values changing
void Menu::changeValue(Direction dir)
{
	switch (m_curMenu)
	{
	case 0: // General
	{
		switch (m_curSubMenu)
		{
		case 0: // Dry sound
			if (canChange(m_player.getDry(), 0.05f, 0.0f, 1.0f, dir))
				m_player.setDry(m_player.getDry() + dir * 0.05f);
			return;
		case 1: // Wet sound
			if (canChange(m_player.getWet(), 0.05f, 0.0f, 1.0f, dir))
				m_player.setWet(m_player.getWet() + dir * 0.05f);
			return;
		default:
			return;
		}
	}
	case 1: // Graphics
	{
		switch (m_curSubMenu)
		{
		case 0: // Peak sensitivity
			if (canChange(m_graph.getPeakSensitivity(), 5.0f, 0.0f, 200.0f, dir))
				m_graph.setPeakSensitivity(m_graph.getPeakSensitivity() + dir * 5.0f);
			return;
		case 1: // Max particles
			if (canChange((float)m_graph.getMaxParticles(), 10.0f, 0.0f, 200.0f, dir))
				m_graph.setMaxParticles(m_graph.getMaxParticles() + dir * 10ULL);
			return;
		default:
			return;
		}
	}
	case 2: // Binaurality
	{
		switch (m_curSubMenu)
		{
		case 0: // Enabled
			if (canChange(m_bin.isEnabled(), 1.0f, 0.0f, 1.0f, dir))
				m_bin.enable(dir > 0 ? true : false);
			return;
		case 1: // Animated
			if (canChange(m_bin.isAnime(), 1.0f, 0.0f, 1.0f, dir))
				m_bin.animeOn(dir > 0 ? true : false);
			return;
		case 2: // Azimuth
			m_bin.setAzimuth(m_bin.getAzimuth() + dir * 5.0f);
			return;
		case 3: // Elevation
			m_bin.setElevation(m_bin.getElevation() + dir * 15.0f);
			return;
		default:
			return;
		}
	}
	case 3: // Particles
	{
		switch (m_curSubMenu)
		{
		case 0: // Gravity
			if (canChange(GasParticle::gravity, 10'000.0f, -1'000'000.0f, 0.0f, dir))
				GasParticle::gravity += dir * 10'000.0f;
			return;
		case 1: // Delta time
			if (canChange(log10(GasParticle::DT), -1.0f, -8.0f, -1.0f, dir))
				GasParticle::DT = pow(10.0f, log10(GasParticle::DT) + dir);
			return;
		default:
			return;
		}
	}
	case 4: // Reverberation
	{
		switch (m_curSubMenu)
		{
		case 0: // Enabled
			if (canChange(m_reverb.isEnabled(), 1.0f, 0.0f, 1.0f, dir))
				m_reverb.enable(dir > 0 ? true : false);
			return;
		case 1: // Room size
			if (canChange(m_reverb.getRoomSize(), 0.05f, 0.0f, 1.0f, dir))
				m_reverb.setRoomSize(m_reverb.getRoomSize() + dir * 0.05f);
			return;
		case 2: // Wave dumping
			if (canChange(m_reverb.getDamp(), 0.1f, 0.0f, 2.5f, dir))
				m_reverb.setDamp(m_reverb.getDamp() + dir * 0.1f);
			return;
		case 3: // Wet gain
			if (canChange(m_reverb.getWet(), 0.01f, 0.0f, 1.0f, dir))
				m_reverb.setWet(m_reverb.getWet() + dir * 0.01f);
			return;
		case 4: // Sound width
			if (canChange(m_reverb.getWidth(), 0.05f, 0.0f, 10.0f, dir))
				m_reverb.setWidth(m_reverb.getWidth() + dir * 0.05f);
			return;
		default:
			return;
		}
	}
	default:
		return;
	}
}


short Menu::getCurMenu() const
{
	return m_curMenu;
}

short Menu::getCurSubMenu() const
{
	return m_curSubMenu;
}

short Menu::getSubMenuItemsCount() const
{
	return m_numberOfItemsInSubMenu[m_curMenu];
}

// Uniform function for values getting
const char* Menu::getCurrentValue(short menuId, short subMenuId) const
{
	static char printedValue[16]{};

	switch (menuId)
	{
	case 0: // General
	{
		switch (subMenuId)
		{
		case 1: // Dry sound
			sprintf_s(printedValue, "%f", m_player.getDry());
			return printedValue;
		case 2: // Wet sound
			sprintf_s(printedValue, "%f", m_player.getWet());
			return printedValue;
		default:
			return "0.0";
		}
	}
	case 1: // Graphics
	{
		switch (subMenuId)
		{
		case 1: // Peak sensitivity
			sprintf_s(printedValue, "%f", m_graph.getPeakSensitivity());
			return printedValue;
		case 2: // Max particles
			sprintf_s(printedValue, "%llu", m_graph.getMaxParticles());
			return printedValue;
		default:
			return "0.0";
		}
	}
	case 2: // Binaurality
	{
		switch (subMenuId)
		{
		case 1: // Enabled
			return m_bin.isEnabled() ? "true" : "false";
		case 2: // Animated
			return m_bin.isAnime() ? "true" : "false";
		case 3: // Azimuth
			sprintf_s(printedValue, "%f", m_bin.getAzimuth());
			return printedValue;
		case 4: // Elevation
			sprintf_s(printedValue, "%f", m_bin.getElevation());
			return printedValue;
		default:
			return "0.0";
		}
	}
	case 3: // Particles
	{
		switch (subMenuId)
		{
		case 1: // Gravity
			sprintf_s(printedValue, "%f", GasParticle::gravity);
			return printedValue;
		case 2: // Delta time
			sprintf_s(printedValue, "%f", GasParticle::DT);
			return printedValue;
		default:
			return "0.0";
		}
	}
	case 4: // Reverberation
	{
		switch (subMenuId)
		{
		case 1: // Enabled
			return m_reverb.isEnabled() ? "true" : "false";
		case 2: // Room size
			sprintf_s(printedValue, "%f", m_reverb.getRoomSize());
			return printedValue;
		case 3: // Wave dumping
			sprintf_s(printedValue, "%f", m_reverb.getDamp());
			return printedValue;
		case 4: // Wet gain
			sprintf_s(printedValue, "%f", m_reverb.getWet());
			return printedValue;
		case 5: // Sound width
			sprintf_s(printedValue, "%f", m_reverb.getWidth());
			return printedValue;
		default:
			return "0.0";
		}
	}
	default:
		return "0.0";
	}
}

bool Menu::isShown() const
{
	return m_isShown;
}


bool Menu::canChange(float var, float step, float min, float max, Direction dir) const
{
	if (dir > 0) {
		return (var + step) - max < 0.0001f;
	} else {
		return (var - step) - min > -0.0001f;
	}
}