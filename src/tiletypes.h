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

#undef X_DEFAULT
#ifndef X
	#ifndef TILETYPES_H
		#define TILETYPES_H
		#define X_DEFAULT
		#define X(id, col, solid) id,
		enum TileTypes {
	#else
		#define X(id, col, solid)
	#endif
#endif

X(EMPTY,		0x202060, -1)
X(NORMAL,		0x506070, 0)
X(COLLAPSABLE,	0x408040, 0)
X(COLLAPSE_DOOR,0xa0f0a0, 1)
X(TRAMPOLINE,	0x603060, 0)
X(SPINNER,		0x784040, 0)
X(WALL,			0x000080, 1)
X(COLLAPSABLE2,	0x408080, 0)
X(COLLAPSE_DOOR2,0xa0f0f0, 1)
X(GUN,			0xb0a040, 0)
X(TRAP,			0x000000, 0)
X(COLLAPSABLE3,	0x202020, 0)
X(BUILDER,		0x009000, 0)
X(SWITCH,		0x004000, 0)
X(FLOATING_BALL,0xa00050, 0)
X(LIFT_DOWN,	0x7850a0, 0)
X(LIFT_UP,		0x7850a0, 1)

#undef X

#ifdef X_DEFAULT
			NumTileTypes
		};
#undef X_DEFAULT
#endif

