/*
    Copyright (C) 2005-2007 Tom Beaumont

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef __HHOP_SFX_H__
#define __HHOP_SFX_H__

#include "state.h"

#define HHOP_EFFECT_CHANNELS 16
#define HHOP_FADE_MUSIC_IN 200
#define HHOP_FADE_MUSIC_OUT 400

enum
{
	HHOP_MUSIC_ENDING,        // Ending sequence after clearing all levels.
	HHOP_MUSIC_GAME,          // Puzzle mode music.
	HHOP_MUSIC_MAP = 1,       // Map screen music.
	HHOP_MUSIC_TITLE = 1,     // Title screen music.
	HHOP_MUSIC_WIN = 1,       // Victory music after clearing a level.

	HHOP_MUSIC_MAX
};

enum
{
	HHOP_SOUND_BUILDER,       // Builder generates new tiles.
	HHOP_SOUND_COLLAPSE,      // Green or blue walls collapse to floor tiles.
	HHOP_SOUND_CRACK,         // Girl steps on an a green or blue tile.
	HHOP_SOUND_DEATH,         // Girl falls in water.
	HHOP_SOUND_DISINTEGRATE,  // Tile is removed when girl leaves it.
	HHOP_SOUND_EXPLODE_BIG,   // Laser hits a laser tile.
	HHOP_SOUND_EXPLODE_SMALL, // Laser hits any other tile.
	HHOP_SOUND_FLOATER_ENTER, // Girl steps on a floater.
	HHOP_SOUND_FLOATER_MOVE,  // Floater moves one tile.
	HHOP_SOUND_FOUND_ANTIICE, // Girl picks up anti-ice item.
	HHOP_SOUND_FOUND_JUMP,    // Girl picks up jump item.
	HHOP_SOUND_ICE,           // Girl slides through ice tile.
	HHOP_SOUND_LASER,         // Girl steps on a laser tile.
	HHOP_SOUND_LIFT_UP,       // Elevator goes up.
	HHOP_SOUND_LIFT_DOWN,     // Elevator goes down.
	HHOP_SOUND_SPINNER,       // Spinner tile spins.
	HHOP_SOUND_STEP,          // Girl moves one step.
	HHOP_SOUND_TRAMPOLINE,    // Girl steps on a trampoline.
	HHOP_SOUND_UI_FADE,       // Screen fade begins.
	HHOP_SOUND_UI_MENU,       // Menu item activated.
	HHOP_SOUND_USED_ANTIICE,  // Girl steps on ice and uses anti-ice.
	HHOP_SOUND_USED_JUMP,     // Girl jumps.
	HHOP_SOUND_WIN,           // Level is completed.

	HHOP_SOUND_MAX
};

void InitSound(const char* path);
void FreeSound();
void PlayMusic(int type);
void PlaySound(int type);
void QueueSound(int type, double time);
void ToggleMusic();
void ToggleEffects();
void UndoSound();
void UpdateSound(double time);

#endif

