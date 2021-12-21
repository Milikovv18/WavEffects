#pragma once

#include <Windows.h>
#include <cmath>

#include "Binaurality.h"
#include "ReverbModel.h"
#include "GasParticle.h"
#include "WavPlayer.h"

// Windows default lib
#pragma comment(lib, "User32.lib")

extern const char MENU_NAMES[5][6][18];
class Graphics;


class Menu
{
	// Direction for changing param value
	// (decrease or increase)
	enum Direction
	{
		DEC = -1,
		INC = 1
	};

public:
	// Currently supported only this effect list
	Menu(WavPlayer& player, Graphics& graph, Binaurality& bin, Freeverb& reverb);

	// Check for user keyboard input
	void updateInput();
	// Change some avail param value
	void changeValue(Direction dir);

	short getCurMenu() const;
	short getCurSubMenu() const;
	short getSubMenuItemsCount() const;
	const char* getCurrentValue(short menuId, short subMenuId) const;

	// Is menu displayed on the screen
	bool isShown() const;

private:
	short m_curMenu;
	short m_curSubMenu;
	bool m_isShown;
	bool m_isPressed;

	WavPlayer& m_player;
	Graphics& m_graph;
	Binaurality& m_bin;
	Freeverb& m_reverb;

	const short m_numberOfItemsInSubMenu[std::size(MENU_NAMES)]{ 2,2,4,2,5 };

	// Help function for detecting min/max boundaries
	bool canChange(float var, float step, float min, float max, Direction dir) const;
};