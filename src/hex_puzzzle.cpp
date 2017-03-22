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

#include "config.h"
#include "i18n.h"
#include "sfx.h"
#include <string>
#include <iostream>
#include <cctype> // TODO: remove it later
#include <errno.h>

//////////////////////////////////////////////////////
// Config


#ifdef _DEBUG
#define EDIT
#endif

//#define MAP_LOCKED_VISIBLE

#ifdef EDIT
//	#define MAP_EDIT_HACKS
	#define MAP_EDIT_HACKS_DISPLAY_UNLOCK 0
	#define CHEAT
	#define BMP_SUFFIX ".bmp"
#else
	#define USE_LEVEL_PACKFILE
	#define BMP_SUFFIX ".dat"
#endif



#ifdef EDIT
#define GAMENAME PACKAGE_NAME " (EDIT MODE)"
#endif
#ifndef GAMENAME
#define GAMENAME PACKAGE_NAME
#endif

#define IMAGE_DAT_OR_MASK 0xff030303 // Reduce colour depth of images slightly for better compression (and remove useless top 8 bits!)
#define STARTING_LEVEL "Levels\\0_green\\triangular.lev"
#define UNLOCK_SCORING 75
const char * mapname = "Levels\\map_maybe\\map.lev";

//////////////////////////////////////////////////////



#ifndef USE_OPENGL

#include "state.h"

#include "tiletypes.h"

#ifdef USE_LEVEL_PACKFILE
#include "packfile.h"
#endif

#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef PATH_MAX 
#define PATH_MAX 4096 
#endif 

//extern void ScrFlip(void);

void RenderTile(bool reflect, int t, int x, int y, int cliplift=-1);

int keyState[SDLK_LAST] = {0};

FILE *file_open( const char *file, const char *flags )
{
	extern String base_path;
	static String exec_path = "/3ds/Hexahop/" ;
	static String filename; // static to reduce memory alloc/free calls.

	extern void* realScreen;
//	ScrFlip(); //nop90: flipscreen to avoid citra emulator freezing

	if (strncmp(file, "save", 4) == 0)
	{

		filename = exec_path + file;
	}
	else
		filename = base_path + file;

	filename.fix_backslashes();
	FILE* f = fopen( filename, flags );

/*
	if (!f && strncmp(file, "save", 4) != 0)
	{
		printf("Warning: unable to open file \"%s\" for %s\n", (const char*)filename, strchr(flags, 'r') ? "reading" : "writing");
	}
*/
	return f;
}


#ifdef MAP_EDIT_HACKS
	static const short value_order[]={
		//WALL,
		//COLLAPSE_DOOR2,
		//COLLAPSABLE3
		//SWITCH
		//EMPTY, NORMAL,

		COLLAPSABLE,
		TRAMPOLINE,
		COLLAPSE_DOOR, COLLAPSABLE2,
		GUN,
		FLOATING_BALL,
		SPINNER,
		TRAP,
		0x100,
		LIFT_DOWN, LIFT_UP,
		BUILDER,
		0x200,
	};
#endif

//#define PROGRESS_FILE "progress.dat"

#define PI (3.1415926535897931)
#define PI2 (PI*2)
#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define ABS(a) ((a)<0 ? -(a) : (a))

#define WATER_COLOUR 31 | ((IMAGE_DAT_OR_MASK>>16)&255), 37 | ((IMAGE_DAT_OR_MASK>>8)&255), 135 | ((IMAGE_DAT_OR_MASK>>0)&255)

#define ROTATION_TIME 0.25
#define BUILD_TIME 1
#define LASER_LINE_TIME 0.7
#define LASER_FADE_TIME 0.1
#define LASER_SEGMENT_TIME 0.01
#define LIFT_TIME 0.5
#define JUMP_TIME 0.4

#define X(NAME,FILE,ALPHA) SDL_Surface* NAME = 0;
#include "gfx_list.h"
int scrollX=0, scrollY=0, initScrollX=0, initScrollY=0;
int mapRightBound = 0;
int mapScrollX = 0;
bool showScoring = false;
bool hintsDone = false;

enum {
	TILE_SPLASH_1 = 17,
	TILE_SPLASH_2,
	TILE_SPLASH_3,

	TILE_SPHERE = 20,
	TILE_SPHERE_OPEN,
	TILE_SPHERE_DONE,
	TILE_SPHERE_PERFECT,
	TILE_LOCK,

	TILE_LIFT_BACK,
	TILE_LIFT_FRONT,
	TILE_LIFT_SHAFT,
	TILE_BLUE_FRONT,
	TILE_GREEN_FRONT,

	TILE_LINK_0 = 30,
	TILE_LINK_1,
	TILE_LINK_2,
	TILE_LINK_3,
	TILE_LINK_4,
	TILE_LINK_5,
	TILE_GREEN_FRAGMENT,
	TILE_GREEN_FRAGMENT_1,
	TILE_GREEN_FRAGMENT_2,
	TILE_ITEM2,

	TILE_WATER_MAP = 40,
	TILE_GREEN_CRACKED,
	TILE_GREEN_CRACKED_WALL,
	TILE_BLUE_CRACKED,
	TILE_BLUE_CRACKED_WALL,
	TILE_LASER_HEAD,
	TILE_FIRE_PARTICLE_1,
	TILE_FIRE_PARTICLE_2,
	TILE_WATER_PARTICLE,

	TILE_LASER_0 = 50,
	TILE_LASER_FADE_0 = 53,
	TILE_BLUE_FRAGMENT = 56,
	TILE_BLUE_FRAGMENT_1,
	TILE_BLUE_FRAGMENT_2,
	TILE_ITEM1,
	TILE_LASER_REFRACT = 60,
	TILE_ICE_LASER_REFRACT = TILE_LASER_REFRACT+6,
	TILE_WHITE_TILE,
	TILE_WHITE_WALL,
	TILE_BLACK_TILE,

};

const int colours[] = {
	#define X(n,col, solid) col,
	#include "tiletypes.h"
};

const int tileSolid[] = {
	#define X(n,col, solid) solid,
	#include "tiletypes.h"
};

void ChangeSuffix(char* filename, char* newsuffix)
{
	int len = strlen(filename);
	int i = len-1;
	while (i>=0 && filename[i]!='\\' && filename[i]!='.' && filename[i]!='/') 
		i--;
	if (filename[i]=='.')
		strcpy(filename+i+1, newsuffix);
	else
	{
		strcat(filename, ".");
		strcat(filename, newsuffix);
	}
}

bool isMap=false, isRenderMap=false;
int isFadeRendering=0;

/*
	 |--|     |--|   TILE_W1
	 |--------|	 	 TILE_W2
		|-----|	 	 TILE_WL
	 |-----------|	 TILE_W3

		*-----*		-			-
	   /       \    |TILE_H1	|TILE_H2
	  /         \	|			|
	 *           *	-			|
	  \         /				|
	   \       /				|
		*-----*					-

	WL = sqrt(h1*h1 + w1*w1)
	wl**2 = h1**2 + w1**2

	w1 = sin60.wL
	
*/

#if 1
	#define TILE_W1 18
	#define TILE_W3	64
	#define GFX_SIZE TILE_W3
	#define TILE_W2 (TILE_W3-TILE_W1)
	#define TILE_H1 TILE_W1
	#define TILE_HUP 22	//extra visible height of wall (used for determining whether a wall was clicked on)
	#define TILE_H2 (TILE_H1*2)
	#define TILE_WL (TILE_W2-TILE_W1)
	#define TILE_H_LIFT_UP   26
	#define TILE_H_REFLECT_OFFSET 24
	#define TILE_HUP2 TILE_H_LIFT_UP	// Displacement of object on top of wall
	#define FONT_SPACING 25
	#define FONT_X_SPACING (-1)	// -1 in order to try and overlap the black borders of adjacent characters
#else
	#define TILE_WL 30
	#define TILE_W1 (TILE_WL/2)
	#define TILE_W2 (TILE_W1+TILE_WL)
	#define TILE_W3 (TILE_W1+TILE_W2)
	#define TILE_H1 (TILE_WL*0.8660254037844386)
	#define TILE_H2 (TILE_H1*2)
#endif

#define MAX_DIR 6

SDL_Rect tile[2][70];
short tileOffset[2][70][2];
int Peek(SDL_Surface* i, int x, int y)
{
	if (x<0 || y<0 || x>=i->w || y>=i->h)
		return 0;
	unsigned int p=0;
	const int BytesPerPixel = i->format->BytesPerPixel;
	const int BitsPerPixel = i->format->BitsPerPixel;
	if (BitsPerPixel==8)
		p = ((unsigned char*)i->pixels)[i->pitch*y + x*BytesPerPixel];
	else if (BitsPerPixel==15 || BitsPerPixel==16)
		p = *(short*)(((char*)i->pixels) + (i->pitch*y + x*BytesPerPixel));
	else if (BitsPerPixel==32)
		p = *(unsigned int*)(((char*)i->pixels) + (i->pitch*y + x*BytesPerPixel));
	else if (BitsPerPixel==24)
		p = (int)((unsigned char*)i->pixels)[i->pitch*y + x*BytesPerPixel]
		  | (int)((unsigned char*)i->pixels)[i->pitch*y + x*BytesPerPixel] << 8
		  | (int)((unsigned char*)i->pixels)[i->pitch*y + x*BytesPerPixel] << 16;
	
	return p;
}
bool IsEmpty(SDL_Surface* im, int x, int y, int w, int h)
{
	for (int i=x; i<x+w; i++)
		for (int j=y; j<y+h; j++)
			if (Peek(im,i,j) != Peek(im,0,im->h-1)) 
				return false;
	return true;
}

void MakeTileInfo()
{
	for (int i=0; i<140; i++)
	{
		SDL_Rect r = {(i%10)*GFX_SIZE, ((i/10)%7)*GFX_SIZE, GFX_SIZE, GFX_SIZE};
		short * outOffset = tileOffset[i/70][i%70];
		SDL_Surface * im = (i/70) ? tileGraphicsR : tileGraphics;

		outOffset[0] = outOffset[1] = 0;

		while (r.h>1 && IsEmpty(im, r.x, r.y, r.w, 1)) r.h--, r.y++, outOffset[1]++;
		while (r.h>1 && IsEmpty(im, r.x, r.y+r.h-1, r.w, 1)) r.h--;
		while (r.w>1 && IsEmpty(im, r.x, r.y, 1, r.h)) r.w--, r.x++, outOffset[0]++;
		while (r.w>1 && IsEmpty(im, r.x+r.w-1, r.y, 1, r.h)) r.w--;

		tile[i/70][i%70] = r;
	}
}

#include "text.h"
#include "savestate.h"
#include "menus.h"
#include "level_list.h"

void SaveState::GetStuff()
{
	general.hintFlags = HintMessage::flags;
}
void SaveState::ApplyStuff()
{
	HintMessage::flags = general.hintFlags;
}


// somewhere else Tile map[][] is assigned to an unsigned char not int32_t
// but the data file format expects it to be 32 bit wide!??
typedef int32_t Tile;
typedef int Dir;
struct Pos{
	int32_t x,y;
	Pos() : x(0), y(0) {}
	Pos(int a, int b) : x(a), y(b) {}
	bool operator == (Pos const & p) const 
	{
		return x==p.x && y==p.y;
	}
	Pos operator + (Dir const d) const
	{
		return Pos(
			x + ((d==1 || d==2) ?  1 : (d==4 || d==5) ? -1 : 0),
			y + ((d==0 || d==1) ? -1 : (d==3 || d==4) ?  1 : 0)
		);
	}
	int getScreenX() const {
		return x*TILE_W2;
	}
	int getScreenY() const {
		return x*TILE_H1 + y*TILE_H2;
	}
	static Pos GetFromWorld(double x, double y)
	{
		x += TILE_W3/2;
		y += TILE_H1;
		int tx, ty;
		tx = (int)floor(x/TILE_W2);
		y -= tx*TILE_H1;
		ty = (int)floor(y/TILE_H2);

		y -= ty * TILE_H2;
		x -= tx * TILE_W2;

		if (x < TILE_W1 && y < TILE_H1)
			if (x*TILE_H1 + y * TILE_W1 < TILE_H1*TILE_W1)  
				tx--;
		if (x < TILE_W1 && y > TILE_H1)
			if (x*TILE_H1 + (TILE_H2-y) * TILE_W1 < TILE_H1*TILE_W1)  
				tx--, ty++;

		return Pos(tx, ty);
	}
};
Pos mousep(0,0), keyboardp(4,20);

class RenderObject;

struct RenderStage
{
	virtual ~RenderStage() {}
	virtual void Render(RenderObject* r, double time, bool reflect) = 0;
	virtual int GetDepth(double /*time*/) { return 1; }
};

class RenderObject
{
	RenderStage** stage;
	double* time;
	int numStages;
	int maxStages;
	int currentStage;
public: 
	double seed;
	double currentTime;
private:

	void Reserve()
	{
		if (maxStages <= numStages)
		{
			maxStages = maxStages ? maxStages*2 : 4;
			stage = (RenderStage**)	realloc(stage, sizeof(stage[0])*maxStages);
			time  = (double*)		realloc(time, sizeof(time[0])*maxStages);
		}
	}
public:
	RenderObject() : stage(0), time(0), numStages(0), maxStages(0), currentStage(0)
	{
		// TODO:	use a random number with better range
		//			or maybe make seed an int or float...
		seed = rand() / (double)RAND_MAX;
	}
	~RenderObject()
	{
		free(stage); free(time);
	}
	bool Active(double t)
	{
		if (numStages==0) return false;
		if (t < time[0]) return false;
		return true;
	}
	void UpdateCurrent(double t)
	{
		if (currentStage >= numStages) currentStage = numStages-1;
		if (currentStage < 0) currentStage = 0;

		while (currentStage>0 && time[currentStage]>t)
			currentStage--;
		while (currentStage<numStages-1 && time[currentStage+1]<=t)
			currentStage++;

		currentTime = t;
	}
	RenderStage* GetStage(double t)
	{
		if (t==-1 && numStages>0)
			return stage[numStages-1];

		if (!Active(t)) return 0;
		UpdateCurrent(t);
		return stage[currentStage];
	}
	double GetLastTime()
	{
		return numStages>0 ? time[numStages-1] : -1;
	}
	void Render(double t, bool reflect)
	{
		if (!Active(t)) 
			return;
		UpdateCurrent(t);
		stage[currentStage]->Render(this, t - time[currentStage], reflect);
	}
	int GetDepth(double t)
	{
		if (!Active(t)) 
			return -1;
		UpdateCurrent(t);
		return stage[currentStage]->GetDepth(t - time[currentStage]);
	}
	void Reset(double t)
	{
		if (t<0)
			numStages = currentStage = 0;
		else
		{
			while (numStages > 0 && time[numStages-1] >= t)
				numStages--;
		}
	}
	void Wipe()
	{
		if (currentStage > 0 && numStages > 0)
		{
			memmove(&time[0], &time[currentStage], sizeof(time[0]) * (numStages-currentStage));
			memmove(&stage[0], &stage[currentStage], sizeof(stage[0]) * (numStages-currentStage));
			numStages -= currentStage;
			currentStage = 0;
		}
	}
	void Add(RenderStage* s, double t)
	{
		int i=0;
		
		if (currentStage<numStages && time[currentStage]<=t)
			i = currentStage;

		while (i<numStages && time[i]<t)
			i++;

		if (i<numStages && time[i]==t)
			stage[i]=s;
		else
		{
			Reserve();

			if (i<numStages)
			{
				memmove(&time[i+1], &time[i], (numStages-i) * sizeof(time[0]));
				memmove(&stage[i+1], &stage[i], (numStages-i) * sizeof(stage[0]));
			}

			numStages++;
			time[i] = t;
			stage[i] = s;
		}
	}
};

class WorldRenderer
{
	#define SIZE 30
	#define FX 10
	RenderObject tile[SIZE][SIZE][2];
	RenderObject fx[FX];
	int fxPos;

public:
	RenderObject player;
	RenderObject dummy;

	WorldRenderer()
	{
		Reset();
	}

	void Reset(double t = -1)
	{
		fxPos = 0;
		player.Reset(t);
		dummy.Reset(-1);

		for (int i=0; i<SIZE; i++)
			for (int j=0; j<SIZE; j++)
				for (int q=0; q<2; q++)
					tile[i][j][q].Reset(t);

		for (int j=0; j<FX; j++)
			fx[j].Reset(t);
	}

	void Wipe()
	{
		player.Wipe();
		dummy.Reset(-1);

		for (int i=0; i<SIZE; i++)
			for (int j=0; j<SIZE; j++)
				for (int q=0; q<2; q++)
					tile[i][j][q].Wipe();

		for (int j=0; j<FX; j++)
			fx[j].Wipe();
	}

	bool Visible(Pos p)
	{
		int x0 = (scrollX+TILE_W2) / TILE_W2;
		int x1 = (scrollX+SCREEN_W+TILE_W3+TILE_W1) / TILE_W2;
		if (p.x<0 || p.y<0 || p.x>=SIZE || p.y>=SIZE) return false;
		if (p.x<x0) return false;
		if (p.x>=x1-1) return false;
		for (int j0=0; j0<SIZE*3; j0++)
		{
			if (j0 * TILE_H1 < scrollY-TILE_H1) continue;
			if (j0 * TILE_H1 > scrollY+SCREEN_H+TILE_H1) break;
			int i = j0&1;
			int j = j0>>1;
			j -= (x0-i)/2;
			i += (x0-i)/2*2;
			if (j>=SIZE) i+=(j+1-SIZE)*2, j=SIZE-1;
			for (; i<x1 && j>=0; i+=2, j--)
			{
				if (Pos(i,j)==p)
					return true;
			}
		}
		return false;
	}

	void Render(double t, bool reflect)
	{
		dummy.Reset(-1);

		int playerDepth = player.GetDepth(t);
		if (reflect) playerDepth-=4;
		if (playerDepth<0)
			player.Render(t, reflect);

		int x0 = (scrollX+TILE_W2) / TILE_W2;
		int x1 = (scrollX+SCREEN_W+TILE_W3+TILE_W1) / TILE_W2;
		x0 = MAX(x0, 0);
		x1 = MIN(x1, SIZE);
		for (int j0=0; j0<SIZE*3; j0++)
		{
			if (j0 * TILE_H1 < scrollY-TILE_H1) continue;
			if (j0 * TILE_H1 > scrollY+SCREEN_H+TILE_H1) break;
			int i = j0&1;
			int j = j0>>1;
			j -= (x0-i)/2;
			i += (x0-i)/2*2;
			if (j>=SIZE) i+=(j+1-SIZE)*2, j=SIZE-1;
			for (; i<x1 && j>=0; i+=2, j--)
			{
				for (int q=reflect?1:0; q!=2 && q!=-1; q += (reflect ? -1 : 1))
					if (tile[i][j][q].Active(t))
					{
						tile[i][j][q].Render(t, reflect);
					}
			}

			if (playerDepth==j0 || (j0==SIZE*3 && playerDepth>j0))
				player.Render(t, reflect);
		}

		for (int j=0; j<FX; j++)
			if(fx[j].Active(t))
			{
				fx[j].Render(t, reflect);
			}

	}
	RenderObject & operator () ()
	{
		fxPos++;
		if (fxPos==FX) fxPos = 0;
		return fx[fxPos];
	}
	RenderObject & operator () (Pos const & p, bool item=false)
	{
		if (p.x<0 || p.y<0 || p.x>=SIZE || p.y>=SIZE)
			return dummy;
		return tile[p.x][p.y][item ? 1 : 0];
	}
};

void RenderTile(bool reflect, int t, int x, int y, int cliplift)
{
	SDL_Rect src = tile[reflect][t];
	SDL_Rect dst = {x-scrollX-GFX_SIZE/2, y-scrollY-GFX_SIZE+TILE_H1, 0, 0};
	dst.x += tileOffset[reflect][t][0];
	dst.y += tileOffset[reflect][t][1];
	if (reflect)
		dst.y += TILE_H_REFLECT_OFFSET;
	if (cliplift==-1 || reflect)
	{
	//	dst.w=src.w; dst.h=src.h;
	//	SDL_FillRect(screen, &dst, rand());
		SDL_BlitSurface(reflect ? tileGraphicsR : tileGraphics, &src, screen, &dst);
	}
	else
	{
		src.h -= cliplift;
		if (src.h > TILE_W1)
		{
			src.h -= TILE_W1/2;
			SDL_BlitSurface(tileGraphics, &src, screen, &dst);
			src.y += src.h;
			dst.y += src.h;
			src.h = TILE_W1/2;
		}
		if (src.h > 0)
		{
			src.w -= TILE_W1*2, src.x += TILE_W1;
			dst.x += TILE_W1;
			SDL_BlitSurface(tileGraphics, &src, screen, &dst);
		}
	}	
}
void RenderGirl(bool reflect, int r, int frame, int x, int y, int h)
{
	int sx = r * 64;
	int sy = frame * 80*2;
	if (reflect) 
		y += TILE_H_REFLECT_OFFSET+20+h, sy += 80;
	else
		y -= h;
	SDL_Rect src = {sx, sy, 64, 80};
	SDL_Rect dst = {x-scrollX-32, y-scrollY-65, 0, 0};
	SDL_BlitSurface(girlGraphics, &src, screen, &dst);
}

struct ItemRender : public RenderStage
{
	int item;
	Pos p;
	int water;
	
	ItemRender(int i2, int _water, Pos const & _p) :  item(i2), p(_p), water(_water)
	{}

	double Translate(double seed, double time)
	{
		double bob = time*2 + seed*PI2;
		return sin(bob)*4;
	}

	void Render(RenderObject* r, double time, bool reflect)
	{
		if (item==0)
			return;

		int y = -5 + (int)Translate(r->seed, r->currentTime + time);
		if (reflect) 
			y=-y;
		if (!reflect && !water)
			RenderTile( false, TILE_SPHERE, p.getScreenX(), p.getScreenY());
		RenderTile(
			reflect,
			item==1 ? TILE_ITEM1 : TILE_ITEM2,
			p.getScreenX(), p.getScreenY()+y
		);
	}
};

void RenderFade(double time, int dir, int seed)
{
	int ys=0;
	srand(seed);
	for(int x=rand()%22-11; x<SCREEN_W+22; x+=32, ys ^= 1)
	{			
		for (int y=ys*20; y<SCREEN_H+30; y+=40)
		{
			double a = (rand()&0xff)*dir;
			double b = (time * 0x400 + (y - SCREEN_H) * 0x140/SCREEN_H)*dir;
			if (a >= b)
			{
				RenderTile(false, TILE_BLACK_TILE, x+scrollX, y+scrollY);
			}
		}
	}
}

struct FadeRender : public RenderStage
{
	int seed;
	int dir;
	FadeRender(int d=-1) : seed(rand()), dir(d) 
	{
		isFadeRendering = d;
	}

	void Render(RenderObject* /*r*/, double time, bool reflect)
	{
		if (reflect) return;
		if (time > 0.5)
		{
			if (dir==1) dir=0, isFadeRendering=0;
			return;
		}
		RenderFade(time, dir, seed);
	}
};

struct ScrollRender : public RenderStage
{
	int x,y;
	bool done;
	ScrollRender(int a,int b) : x(a), y(b), done(false) {}

	void Render(RenderObject* /*r*/, double /*time*/, bool /*reflect*/)
	{
		if (done) return;
		scrollX = x, scrollY = y;
		isRenderMap = isMap;
		done = true;
	}
};

struct LevelSelectRender : public RenderStage
{
	Pos p;
	int item;
	int adj;
#ifdef MAP_EDIT_HACKS
	int magic;
#endif
	
	LevelSelectRender(Pos const & _p, int i2, int adj) : p(_p), item(i2), adj(adj)
	{}

	void Render(RenderObject* /*r*/, double /*time*/, bool reflect)
	{
		if (item==0)
			return;

	#ifndef MAP_LOCKED_VISIBLE
		if (item==1) return;
	#endif

	if (!reflect && adj)
	{
		for (int i=0; i<MAX_DIR; i++)
		{
			if (adj & (1 << i))
				RenderTile( false, TILE_LINK_0+i, p.getScreenX(), p.getScreenY());
		}
	}

	if (item < 0)
		return;

	if (!reflect)
	{
		RenderTile(
			reflect, 
			TILE_SPHERE + item-1, 
			p.getScreenX(), p.getScreenY()
		);

		#ifdef MAP_EDIT_HACKS
			int x = p.getScreenX()-scrollX, y = p.getScreenY()-scrollY;
			Print(x+5,y-25,"%d",magic);
		#endif
	}
	}
};

struct ItemCollectRender : public ItemRender
{
	ItemCollectRender(int i2, Pos const & p) :  ItemRender(i2, 0, p)
	{}

	void Render(RenderObject* /*r*/, double /*time*/, bool /*reflect*/)
	{
	}
};

int GetLiftHeight(double time, int t)
{	
	if (t==LIFT_UP)
		time = LIFT_TIME-time;
	time = time / LIFT_TIME;
	if (time > 1) 
		time = 1;
	if (time < 0)
		time = 0;
	time = (3 - 2*time)*time*time;
	if (t==LIFT_UP)
		time = (3 - 2*time)*time*time;
	if (t==LIFT_UP)
		return (int)((TILE_H_LIFT_UP+4) * time);
	else
		return (int)((TILE_H_LIFT_UP-4) * time) + 4;
}

struct TileRender : public RenderStage
{
	int special;
	int t;
	Pos p;
	double specialDuration;
	
	TileRender(int i, Pos const & _p, int _special=0) : special(_special), t(i), p(_p), specialDuration(LASER_LINE_TIME)
	{}

	void Render(RenderObject* r, double time, bool reflect)
	{
		if (t==0 && special==0)
			return;

		if (special && (t==LIFT_UP || t==LIFT_DOWN) && time<LIFT_TIME)
		{
			int y = GetLiftHeight(time, t);
			if (!reflect)
			{
				RenderTile(reflect, TILE_LIFT_BACK, p.getScreenX(), p.getScreenY());
				RenderTile(reflect, TILE_LIFT_SHAFT, p.getScreenX(), p.getScreenY()+y, y-8);
				RenderTile(reflect, TILE_LIFT_FRONT, p.getScreenX(), p.getScreenY());
			}
			else
			{
				RenderTile(reflect, TILE_LIFT_SHAFT, p.getScreenX(), p.getScreenY()-y, y);
				RenderTile(reflect, LIFT_DOWN, p.getScreenX(), p.getScreenY());
			}
		}
		else if (special && (t==EMPTY || t==TRAP) && !reflect && time < specialDuration)
		{
			if (t == TRAP)
			{
				if (time < specialDuration-LASER_FADE_TIME)
					RenderTile(reflect, TILE_ICE_LASER_REFRACT, p.getScreenX(), p.getScreenY());
				else
					RenderTile(reflect, t, p.getScreenX(), p.getScreenY());
			}
			int base = ((t==EMPTY) ? TILE_LASER_0 : TILE_LASER_REFRACT);
			if (t==EMPTY && time >= specialDuration-LASER_FADE_TIME)
				base = TILE_LASER_FADE_0;
			
			int foo=special;
			for(int i=0; foo; foo>>=1, i++)
				if (foo & 1)
					RenderTile(reflect, base+i, p.getScreenX(), p.getScreenY());
		}
		else if (t==FLOATING_BALL)
		{
			int y = int(1.8 * sin(r->seed*PI + time*4));
			if (special==512)
			{
				if (time > 2) return;
				if (reflect) return;
				srand(int(r->seed * 0xfff));
				for (int i=0; i<20 - int(time*10); i++)
				{
					int x = int((((rand() & 0xfff) - 0x800) / 10) * time);
					int y = int((((rand() & 0xfff) - 0x800) / 10) * time);
					RenderTile(true, 19 + ((i+int(time*5))&1)*10, p.getScreenX() + x, p.getScreenY() - 14 + y);
				}

				if (time < 0.05)
					RenderTile(true, 18, p.getScreenX(), p.getScreenY() - 14);
			}
			else if (special)
				RenderBoat(reflect, int(special)&255,  p.getScreenX(), p.getScreenY(), y);
			else
				RenderTile(reflect, t, p.getScreenX(), p.getScreenY() + (reflect ? -y : y));
		}
		else if (t != EMPTY)
			RenderTile(reflect, t, p.getScreenX(), p.getScreenY());
	}
	static void RenderBoat(bool reflect, int d, int x, int y, int yo)
	{
		if (reflect)
			RenderGirl(reflect, d, 0, x, y, -yo);
		RenderTile(reflect, FLOATING_BALL, x, y+yo);
		if (!reflect)
		{
			RenderGirl(reflect, d, 0, x, y, -yo);
			RenderTile(true, 17, x, y+yo-TILE_H_REFLECT_OFFSET);
		}
	}
};

struct TileRotateRender : public TileRender
{
	Dir d;
//	int range;
	int mode;
	TileRotateRender(int i, Pos const & p, Dir _d, int m) : TileRender(i, p), d(_d), mode(m)
	{}
	void Render(RenderObject* r, double time, bool reflect)
	{
		if (t==0)
			return;
		double f = time / ROTATION_TIME;

		if (mode & 1) f += 0.5;
		if (f<1 && f>0)
		{
			if (mode & 2)
				;
			else
				f = (3-2*f)*f*f;
		}

		if (mode & 1) f=1-f; else f=f;
		if (f<0) f=0;

		if (f >= 1)
			TileRender::Render(r, time, reflect);
		else
		{
			Pos dd = (Pos(0,0)+d);
			int x = p.getScreenX() + int(dd.getScreenX()*(f));
			int y = p.getScreenY() + int(dd.getScreenY()*(f));

			if (mode & 2)
				RenderBoat(reflect, (mode&1) ? (d+MAX_DIR/2)%MAX_DIR : d, x, y, 2);
			else
				RenderTile(reflect, t, x, y);
		}
	}
};

struct LaserRender : public RenderStage
{
	Pos p;
	Dir d;
	int range;

	LaserRender(Pos _p, int dir, int r) : p(_p), d(dir), range(r)
	{}

	void Render(RenderObject* /*r*/, double /*time*/)
	{
	}
};

struct ExplosionRender : public RenderStage
{
	Pos p;
	int seed;
	int power;
	int type;

	ExplosionRender(Pos _p, int _pow=0, int t=0) : p(_p), power(_pow), type(t)
	{
		seed = rand();
	}

	virtual int GetDepth(double /*time*/) 
	{
		return p.x + p.y*2;
	}

	void Render(RenderObject* /*r*/, double time, bool reflect)
	{
		if (type==1 && time > 2.5)
			type = -1, new WinLoseScreen(false);

	//	if (reflect) return;
		if (time > 3) return;
		srand(seed);
		int q = 50 - int(time * 35);
		if (power) q*=2;
		if (type) q = 50;
		for (int i=0; i<q; i++)
		{
			int x = p.getScreenX();
			int y = p.getScreenY() + (rand() & 31)-16;
			int xs = ((rand() & 63) - 32);
			int ys = (-10 - (rand() & 127)) * (1+power);
			if (type) ys*=2, xs/=2;
			x += int(xs * (1+time*(2+power)));
			int yo = int(time*time*128 + ys*time);
			//if (yo > 0) yo=-yo;//continue;
			if (type)
			{
				
				if (yo > 0)
				{
					if (!reflect && ys<-60)
					{
						const double T = 0.06;
						double ct = -ys / 128.0;
						if (time < ct+T*4)
						{
							x = p.getScreenX() + int(xs * (1+ct*(2+power)));
							RenderTile(
								reflect, 
								time > ct+3*T ? TILE_SPLASH_3 : time > ct+2*T ? TILE_SPLASH_2 :  time > ct+T ?  TILE_SPLASH_1 : TILE_WATER_PARTICLE+1, 
								x, y);
						}
					}
				}
				else
					RenderTile(
						reflect, 
						time - i*0.003 < 0.2 ? TILE_WATER_PARTICLE+1 : TILE_WATER_PARTICLE, 
						x, y+(reflect?-1:1)*yo);
			}
			else
			{
				if (yo > 0)
					;
				else
					RenderTile(
						reflect, 
						i<q-20 || time<0.3 ? TILE_LASER_HEAD : i<q-10 || time<0.6 ? TILE_FIRE_PARTICLE_1 : TILE_FIRE_PARTICLE_2, 
						x, y+(reflect?-1:1)*yo);
			}
		}
	}
};
struct DisintegrateRender : public RenderStage
{
	Pos p;
	int seed;
	int height;
	int type;

	DisintegrateRender(Pos _p, int _pow=0, int _t=0) : p(_p), height(_pow), type(_t)
	{
		seed = rand();
	}

	void Render(RenderObject* /*r*/, double time, bool reflect)
	{
		if (type)
			RenderTile(reflect, height ? COLLAPSE_DOOR : COLLAPSABLE, p.getScreenX(), p.getScreenY());

		if (time > 50.0/70.0) return;
		if (reflect) return;
		srand(seed);
		int q = 50 - int(time * 70);
		if (height) q*=2;
		for (int i=0; i<q; i++)
		{
			int x = (rand() % (TILE_W3-8))-TILE_W3/2+4;
			int y = (rand() % (TILE_H2-8))-TILE_H1+4;
			if (x<-TILE_WL/2 && ABS(y)<-TILE_WL/2-x) continue;
			if (x>TILE_WL/2 && ABS(y)>x-TILE_WL/2) continue;
			int yo=0;
			if (height) yo -= rand() % TILE_HUP;
			x += p.getScreenX();
			y += p.getScreenY() + 4;
			int xs = 0;//((rand() & 63) - 32);
			int ys = (- (rand() & 31));
			x += int(xs * (1+time*(2)));
			if (type) yo = -yo;
			yo += int(time*time*128 + ys*time);
			if (type) yo = -yo*2;
			//if (yo > 0) yo=-yo;//continue;
			int t = type ? TILE_BLUE_FRAGMENT : TILE_GREEN_FRAGMENT;
			if (i>q-20) t++;
			if (i>q-10) t++;
			if (yo > 5) yo = 5;
			RenderTile(false, t, x, y+(reflect?-yo:yo));
		}
	}
};
struct BuildRender : public RenderStage
{
	Pos p;
	Dir dir;
	int reverse;
	int height;
	int type;

	BuildRender(Pos _p, Dir _d, int _h, int _r=0, int _type=0) : p(_p), dir(_d), reverse(_r), height(_h), type(_type)
	{
	}

	void Render(RenderObject* /*r*/, double time, bool reflect)
	{
		if (time >= BUILD_TIME)
			RenderTile(reflect, height ^ reverse ? (type ? COLLAPSE_DOOR2 : COLLAPSE_DOOR) : (type ? COLLAPSABLE2 : COLLAPSABLE), p.getScreenX(), p.getScreenY());
		else 
		{
			if (height)
				RenderTile(reflect, type ? COLLAPSABLE2 : COLLAPSABLE, p.getScreenX(), p.getScreenY());

			double dist = time * 2 / BUILD_TIME;
			if (dir>-1)
			{
				Pos from = p + ((dir+MAX_DIR/2)%MAX_DIR);
				if (dist <= 1)
				//if (dist > 1)
				{
					double offset = (dist*0.7) + 0.3;
					int x = from.getScreenX() + int((p.getScreenX()-from.getScreenX()) * offset);
					int y = from.getScreenY() + int((p.getScreenY()-from.getScreenY()) * offset - dist*(1-dist)*(TILE_HUP*4));
					RenderTile(reflect, TILE_GREEN_FRAGMENT, x, y);				
				}
				dist -= 1;
			}
			else
			{
				if (reverse) dist = 1-dist;
			}
			if (dist > 0 && !height)
			{
				if (!reflect)
				{
					for (int i=0; i<=int(dist*15); i++)
					{
						int x = p.getScreenX(), y = p.getScreenY();
						double d = (i + fmod(dist*15, 1))/10.0;
						int x1 = int(sin(d*5+time)*MIN(d,1)*TILE_W2/2);
						int y1 = int(cos(d*5+time)*MIN(d,1)*TILE_H1*0.7);
						RenderTile(reflect, TILE_GREEN_FRAGMENT, x+x1, y+y1+4);
						RenderTile(reflect, TILE_GREEN_FRAGMENT, x-x1, y-y1+4);
					}
				}
			}
			if (dist > 0 && height)
			{
				int yo = int((1-dist)*(TILE_HUP*1.3));
				if (yo > TILE_HUP*1.1)
					RenderTile(reflect, TILE_WHITE_TILE, p.getScreenX(), p.getScreenY());
				else if (!reflect)
				{
					RenderTile(reflect, type ? COLLAPSABLE2 : COLLAPSABLE, p.getScreenX(), p.getScreenY());
					RenderTile(reflect, type ? COLLAPSE_DOOR2 : COLLAPSE_DOOR, p.getScreenX(), p.getScreenY()+(reflect ? -yo : yo), yo+6);
					RenderTile(reflect, type ? TILE_BLUE_FRONT : TILE_GREEN_FRONT, p.getScreenX(), p.getScreenY());
				}
				else
				{
					if (yo < TILE_HUP/2)
					{
						RenderTile(reflect, type ? COLLAPSE_DOOR2 : COLLAPSE_DOOR, p.getScreenX(), p.getScreenY()+(reflect ? -yo : yo), yo);
					
					}
					RenderTile(reflect, type ? COLLAPSABLE2 : COLLAPSABLE, p.getScreenX(), p.getScreenY());
					
				}
			}
		}
	}
};

struct PlayerRender : public RenderStage
{
	Pos p;
	Pos target;
	int p_h, target_h;
	int r;
	int type;
	double speed;
	bool dead;
	
	PlayerRender(Pos a, int h, bool d) : p(a), target(a), p_h(h), target_h(h), r(3), type(0), speed(0), dead(d)
	{}
	PlayerRender(int _r, Pos a, int h1, Pos t, int h2, bool d) : p(a), target(t), p_h(h1), target_h(h2), r(_r), type(0), speed(JUMP_TIME), dead(d)
	{
		int dist = MAX(ABS(p.x-target.x), ABS(p.y-target.y));
		if (dist > 1)
			speed *= 1.5;
		if(dist==0)
			speed = 0;
	}

	virtual int GetDepth(double time)
	{
		double f = speed ? time / speed : 1;
		if (f>1) f=1;
		if (f==1) dead = this->dead;

		if (f==1 || (f>0.5 && p_h>target_h))
			return target.x+target.y*2;
		return MAX(target.x+target.y*2 , p.x+p.y*2);
	}

	void Render(RenderObject* /*ro*/, double time, bool reflect)
	{
		bool dead = false;
		double f = speed ? time / speed : 1;
		if (f>1) f=1;
		if (f==1) dead = this->dead;

		int x = p.getScreenX();
		int y = p.getScreenY();
		int x2 = target.getScreenX();
		int y2 = target.getScreenY();
		int h = 0;
		int shadow_h = (int)((p_h+(target_h-p_h)*f)*TILE_HUP2);

		if (x==x2 && y==y2 && p_h!=target_h)
		{
			h = TILE_H_LIFT_UP - GetLiftHeight(time, p_h ? LIFT_DOWN : LIFT_UP);
		}
		else
		{
			
			int dist = MAX(ABS(p.x-target.x), ABS(p.y-target.y));
			int arc = dist*dist;
			int h1 = p_h * TILE_HUP2;;
			int h2 = target_h * TILE_HUP2;
			if (dist==2 && h1!=0) 
			{
				arc += h2 ? 1 : 3;
				h1 = 0;
				shadow_h = f>=0.7 ? int(shadow_h*(f-0.7)/0.3) : 0;
			}
			if (dist==0)
				arc = speed > JUMP_TIME ? 7 : 2;

			h = (int)(h1+(h2-h1)*f);
		//	if (x==x2 && y==y2)
		//		;
		//	else
			{
				//h += int(TILE_H_LIFT_UP/3 * (1-f));
				h += (int)(f*(1-f)*TILE_HUP2*arc);
			}

			if (type==2)
				h=0;
		}

		if (!dead)
		{
			int frame = 0;
			if (type==2 && f<1)
			{
				//frame = ((int)(f*4) % 4);
				//if (frame==2) frame=0; else if (frame==3) frame=2;
				frame = 0;
			}
			else if (f==1 || (x==x2 && y==y2))	// stationary
				frame = 0;
			else if (f > 0.7)
				frame = 0;
			else
			{
				frame = type ? 2 : 1;
				if (f<0.1 || f>0.6)
					frame += 2;
			}

			if (!reflect)
				RenderTile( false, TILE_SPHERE,
					(int)(x+(x2-x)*f),
					(int)(y+(y2-y)*f) - shadow_h
				);

			RenderGirl(
				reflect,
				r, frame, 
				(int)(x+(x2-x)*f),
				(int)(y+(y2-y)*f),
				h
				);

		}
	/*	RenderTile(
			dead ? TILE_SPHERE_OPEN : TILE_SPHERE_DONE, 
			(int)(x+(x2-x)*f),
			(int)(y+(y2-y)*f),
			true
			);*/
	}
};


struct HexPuzzle : public State
{
	struct Undo
	{
		#define MAX_TILECHANGE 64 // TODO: don't have a magic upper limit
		struct TileChange
		{
			Pos p;
			Tile t;
			int item;
			
			TileChange()
			{}
			TileChange(Pos _p, Tile _t, int _i) : p(_p), t(_t), item(_i)
			{}
			void Restore(HexPuzzle* w)
			{
				w->SetTile(p,t,false,false);
				w->SetItem(p,item,false,false);
			}
		};

		TileChange t[MAX_TILECHANGE];
		Pos playerPos;
		Dir playerMovement;
		int numT;
		int numItems[2];
		int score;
		double time;
		double endTime;

		void Add(TileChange const & tc)
		{
			for (int i=0; i<numT; i++)
				if (t[i].p==tc.p)
					return;
			if (numT>=MAX_TILECHANGE)
				FATAL("numT>=MAX_TILECHANGE"); 
			else 
				t[numT++] = tc;
		}
		void New(Dir pmove, Pos & pp, int* items, double t, int sc)
		{
			numItems[0] = items[0];
			numItems[1] = items[1];
			playerPos = pp;
			playerMovement = pmove;
			score = sc;
			time = t;
			numT = 0;
		}
		void Restore(HexPuzzle* w)
		{
			for (int i=numT-1; i>=0; i--)
				t[i].Restore(w);
			w->dead = false;
			w->win = false;
			w->player = playerPos;
			w->player_items[0] = numItems[0];
			w->player_items[1] = numItems[1];
			w->player_score = score;

			//w->renderer.player.Add(new PlayerRender(playerPos, w->GetHeight(playerPos), false), w->time);
		}
	};

	#define MAP_SIZE 30
	char* special[MAP_SIZE][MAP_SIZE];
	Tile map[MAP_SIZE][MAP_SIZE];
	int32_t map_item[MAP_SIZE][MAP_SIZE];
	int tileCount[NumTileTypes];
	int32_t levelPar, levelDiff;
	int turboAnim;
	Pos player;
	int player_items[2];
	int player_score;
	int numComplete, numLevels, numMastered, numLevelsFound;
	bool dead;
	bool win;
	int winFinal;

	SaveState progress;

	WorldRenderer renderer;
	double time;
	double undoTime;

	#define MAX_UNDO 50
	Undo undo[MAX_UNDO];
	int numUndo;
	LevelInfo* currentLevelInfo;

	char currentFile[1000];

	~HexPuzzle()
	{
		FreeGraphics();
	}

	LevelInfo* GetLevelInfo(const char* f)
	{
		if (strstr(f, "Levels\\") == f)
			f += 7;
		if (currentLevelInfo!=0 && strcmp(currentLevelInfo->file, f)==0)
			return currentLevelInfo;
		
		if (f[0]=='_')
		{
			int t = atoi(f+1);
			if (t <= numComplete)
				return 0;

			static char tmp1[1000];
			static LevelInfo tmp = {0, "", tmp1};
			sprintf(tmp1, ngettext("Complete 1  more level  to unlock!", "Complete %d  more levels  to unlock!", t-numComplete), t-numComplete);
			return &tmp;
		}

		for (unsigned int i=0; i<sizeof(levelNames)/sizeof(levelNames[0]); i++)
			if (strcmp(f, levelNames[i].file)==0)
				return &levelNames[i];
		static LevelInfo tmp = {0, "", _("<<NO NAME>>")};
		return &tmp;
	}

#ifdef MAP_EDIT_HACKS
	int GetAutoTile(const char * level, bool tiletype)
	{
		FILE* f = file_open(filename, "rb");
		int tile = EMPTY;
		int version;

		if (f && fscanf(f, "%d", &version)==1 && (version==3 || version==4))
		{
			if (strstr(level,"mk"))
				level+=0;

			fgetc(f); // Remove '\n' character

			int32_t par, diff;
			unsigned char bounds[4];
			Pos playerStart;
			fread(&par, sizeof(par), 1, f);
			par = SWAP32(par);

			if (version >= 4) {
				fread(&diff, sizeof(diff), 1, f);
  			diff = SWAP32(diff);
      }
			fread(bounds, sizeof(bounds), 1, f);
			fread(&playerStart, sizeof(playerStart), 1, f);
			playerStart.x = SWAP32(playerStart.x);
			playerStart.y = SWAP32(playerStart.y);

			int highval=0;

			for (int i=bounds[0]; i<=bounds[1]; i++)
				for (int j=bounds[2]; j<=bounds[3]; j++)
				{
					unsigned char comp = map[i][j] | (map_item[i][j]<<5);
					fread(&comp, sizeof(comp), 1, f);
					int t = comp & 0x1f;
					int item = (comp >> 5) & 3;
					for (int i=highval+1; i<sizeof(value_order)/sizeof(value_order[0]); i++)
						if (t!=0 && t==value_order[i] 
						 ||	item!=0 && item==(value_order[i]>>8))
							highval = i;
				}

			if (tiletype)
			{
				tile = value_order[highval];
				if (tile==0x100) tile = COLLAPSABLE3;
				if (tile==0x200) tile = SWITCH;
				if (tile==LIFT_UP) tile = LIFT_DOWN;
			}
			else
			{
				if (value_order[highval] == LIFT_UP)
					tile = highval-1;
				else
					tile = highval;
			}
		}
		else
		{
			level+=0;
		}
		if (f)
			fclose(f);
		return tile;
	}
#endif

	void InitSpecials()
	{
		numComplete = numLevels = numMastered = numLevelsFound = 0;
		for (int i=0; i<MAP_SIZE; i++)
			for (int j=0; j<MAP_SIZE; j++)
				ActivateSpecial(Pos(i,j), 0);
		for (int i=0; i<MAP_SIZE; i++)
			for (int j=0; j<MAP_SIZE; j++)
				ActivateSpecial(Pos(i,j), 2);
		numComplete = numLevels = numMastered = numLevelsFound = 0;
		for (int i=0; i<MAP_SIZE; i++)
			for (int j=0; j<MAP_SIZE; j++)
				ActivateSpecial(Pos(i,j), 0);

	}
	void DoHints()
	{
		#ifndef EDIT
			if (strcmp(mapname, currentFile)==0)
			{
//				for (int i=0; i<32; i++)
//					HintMessage::FlagTile(i);
				if (numComplete >= UNLOCK_SCORING && !progress.general.scoringOn)
				{
					HintMessage::FlagTile(26);
					progress.general.scoringOn = 1;
					InitSpecials(); // Re-initialise with gold ones available
				}
				HintMessage::FlagTile(25);
			}
			else
			{
				for (int i=0; i<MAP_SIZE; i++)
					for (int j=0; j<MAP_SIZE; j++)
					{
						int t = GetTile(Pos(i,j));
						int item = GetItem(Pos(i,j));
						if (t)
							HintMessage::FlagTile(t);
						if (item)
							HintMessage::FlagTile(item+20);
					}
				HintMessage::FlagTile(EMPTY);
			}
		#endif
		hintsDone = true;
	}
	void ResetLevel()
	{
		hintsDone = false;

		UpdateCursor(Pos(-1,-1));

		isMap = false;

		player_score = 0;

		numUndo = 0;
		undoTime = -1;

		dead = false;
		win = false;
		winFinal = false;
		player_items[0] = player_items[1] = 0;
//		time = 0;
		if (strlen(currentSlot) == 0)
		{
			new TitleMenu();
			new Fader(1, -3);
		}
		else
		{
			if (!isFadeRendering && time!=0)
			{
				renderer().Add(new FadeRender(-1), time);
				time += 0.5;
			}
		}

		// Reset renderer
		renderer.Reset(time);
		renderer.Wipe();

		for (int t=0; t<NumTileTypes; t++)
			tileCount[t] = 0;

		for (int i=0; i<MAP_SIZE; i++)
			for (int j=0; j<MAP_SIZE; j++)
			{
				Pos p(i,j);
				int item = GetItem(p);
				//if (item)
					renderer(p,true).Add(new ItemRender(item, GetTile(p)==EMPTY, p), time);
			}

		InitSpecials();

		for (int i=0; i<MAP_SIZE; i++)
			for (int j=0; j<MAP_SIZE; j++)
			{
				Pos p(i,j);
				int t = GetTile(p);
				tileCount[t]++;

				if (isMap)
					t = EMPTY;
				
				//if (t)
					renderer(p).Add(new TileRender(t, p), time);
			}

		if (!isMap)
			renderer.player.Add(new PlayerRender(player, GetHeight(player), dead), time);
		else
			renderer.player.Add(new PlayerRender(Pos(-100,-100), 0, true), time);
			

		int bounds[4] = {player.getScreenX(),player.getScreenX(),player.getScreenY(),player.getScreenY()};
		for (int i=0; i<MAP_SIZE; i++)
			for (int j=0; j<MAP_SIZE; j++)
			{
				Pos p(i,j);
				if (map[i][j] !=0 || map_item[i][j]!=0)
				{
					int x1 = p.getScreenX();
					int y1 = p.getScreenY();
					int x2 = x1 + TILE_W3;
					int y2 = y1 + TILE_H2;
					y1 -= TILE_H2;	// Make sure objects/player will be properly visible

					if (x1<bounds[0]) bounds[0] = x1;
					if (x2>bounds[1]) bounds[1] = x2;
					if (y1<bounds[2]) bounds[2] = y1;
					if (y2>bounds[3]) bounds[3] = y2;
				}
			}

		int sx, sy;
		if (isMap)
		{
			sx = bounds[0] - int(TILE_W2*6.35);
			sy = (bounds[3] + bounds[2] - SCREEN_H) / 2 - TILE_H2/2;
		}
		else
		{
			sx = (bounds[1] + bounds[0] - SCREEN_W) / 2 - TILE_W3/2;
			sy = (bounds[3] + bounds[2] - SCREEN_H) / 2 - TILE_H2/2;
		}
		if (isMap) 
		{
			initScrollX = sx;
			initScrollY = sy;
			if (mapScrollX==0)
				mapScrollX = sx;
			else
				sx = mapScrollX;
		}

//		time = 1;	// Guarantee we can't try and do things at time=0

		renderer().Add(new ScrollRender(sx, sy), time);
		renderer().Add(new FadeRender(1), time);
		if (time != 0)
			time -= 0.5;
	}

	char* ReadAll(FILE* f)
	{
		int size;
    // FIXME: According to http://userpage.fu-berlin.de/~ram/pub/pub_jf47ht20Ht/c_faq_de
    // undefined for binary streams! (POSIX does not differ between ascii and binary, so
    // we are on the save side in Linux)
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
		char* c = loadPtr = new char [size];
		endLoad = loadPtr + size;
		fread(c, 1, size, f);
		return c;
	}

	static char *loadPtr, *endLoad;
	static unsigned int fread_replace(void* d, unsigned int size, unsigned int num, FILE*)
	{
		unsigned int remain = (endLoad - loadPtr) / size;
		if (remain < num) num = remain;
		memcpy(d, loadPtr, size*num);
		loadPtr += size*num;
		return num;
	}

	int GetPar(const char * level, bool getdiff=false)
	{
		if (strcmp(level, currentFile)==0)
			return getdiff ? levelDiff : levelPar;

		#ifdef USE_LEVEL_PACKFILE
			PackFile1::Entry* e = levelFiles.Find(level);
			if (!e) return 999;
			loadPtr = (char*)e->Data();
			endLoad = loadPtr + e->DataLen();
			FILE* f = 0;
		#else
			loadPtr = 0;
			FILE* f = file_open(level, "rb");
		#endif

		typedef unsigned int _fn(void*, unsigned int, unsigned int, FILE*);
		_fn * fn = (loadPtr ? (_fn*)fread_replace : (_fn*)fread);

		int32_t par = 99999, diff = 0;
		int16_t version;
		
		if (!f && !loadPtr)
			return getdiff ? diff : par;

		fn(&version, 2, 1, f); // skip to relevant point

		if (fn(&par, sizeof(par), 1, f) != 1)
			par = 99999;
		else
			par = SWAP32(par);
		size_t ret = fn(&diff, sizeof(diff), 1, f);
		diff = SWAP32(diff);
		if (ret != 1 || diff<0 || diff>10)
			diff = 0;

		#ifdef USE_LEVEL_PACKFILE
			loadPtr = endLoad = 0;
		#else
			if (f)
				fclose(f);
		#endif

		return getdiff ? diff : par;
	}

	bool LoadSave(const char * filename, bool save)
	{
		if (!filename) 
			return false;

		if (!save)
		{
			showScoring = false;
			LevelSave* l = progress.GetLevel(filename, true);
			if (progress.general.scoringOn && l && l->Completed() )
				showScoring = true;
		}

		#ifdef USE_LEVEL_PACKFILE
			if (!save)
			{
				PackFile1::Entry* e = levelFiles.Find(filename);
				if (!e) return false;

				if (currentFile != filename) // equal (overlapping) strings are forbidden
					strcpy(currentFile, filename);
				currentLevelInfo = GetLevelInfo(currentFile);
				
				loadPtr = (char*)e->Data();
				endLoad = loadPtr + e->DataLen();
				_LoadSave(NULL, save);
				loadPtr = endLoad = 0;

				if (!isMap && !activeMenu)
					PlayMusic(HHOP_MUSIC_GAME);

				return true;
			}
		#else
			loadPtr = 0;
			FILE* f = file_open(filename, save ? "wb" : "rb");
			if (f)
			{
				strcpy(currentFile, filename);
				if (!save)
					currentLevelInfo = GetLevelInfo(currentFile);

				if (!save)
				{
					char* data = ReadAll(f);
					_LoadSave(f, save);
					delete [] data;
					loadPtr = endLoad = 0;
				}
				else
				{
					_LoadSave(f, save);
				}
				fclose(f);

				if (!isMap && !activeMenu && !save)
					PlayMusic(HHOP_MUSIC_GAME);

				return true;
			}
		#endif
			
		return false;
	}

  /** \brief Writes/reads game status to/from a file
   *
   *  The game data file is written in little endian so it can be shared
   *  across different machines.
   */
	void _LoadSave(FILE* f, bool save)
	{
		typedef unsigned int _fn(void*, unsigned int, unsigned int, FILE*);
		_fn * fn = save ? (_fn*)fwrite : (loadPtr ? (_fn*)fread_replace : (_fn*)fread);

		#define SAVEVERSION 4
		int version = SAVEVERSION; // 1--9
		if (save)
			fprintf(f, "%d\n", version);
		else
		{
			char c;
			if (fn(&c, 1, 1, f) != 1)
				return;
			version = c-'0';

			// Remove '\n' character
			fn(&c, 1, 1, f);
		}

		if (!save)
		{
			for (int i=0; i<MAP_SIZE; i++)
				for (int j=0; j<MAP_SIZE; j++)
				{
					delete [] special[i][j];
					special[i][j] = 0;
				}
		}

		if (version==1)
		{
			for (int i=0; i<MAP_SIZE; i++)
				for (int j=0; j<MAP_SIZE; j++) {
					map[i][j] = SWAP32(map[i][j]);
					fn(&map[i][j], sizeof(map[i][j]), 1, f);
					map[i][j] = SWAP32(map[i][j]);
				}

			player.x = SWAP32(player.x);
			player.y = SWAP32(player.y);
			fn(&player, sizeof(player), 1, f);
			player.x = SWAP32(player.x);
			player.y = SWAP32(player.y);

			for (int i=0; i<MAP_SIZE; ++i)
				for (int j=0; j<MAP_SIZE; ++j)
					map_item[i][j] = SWAP32(map_item[i][j]);
			if (fn(map_item, sizeof(map_item), 1, f) == 0)
				memset(map_item, 0, sizeof(map_item));
			for (int i=0; i<MAP_SIZE; ++i)
				for (int j=0; j<MAP_SIZE; ++j)
					map_item[i][j] = SWAP32(map_item[i][j]);
		}
		else if (version>=2 && version<=4)
		{
			unsigned char bounds[4];
			if (save)
			{
				bounds[0]=bounds[1]=player.x;
				bounds[2]=bounds[3]=player.y;
				for (int i=0; i<MAP_SIZE; i++)
					for (int j=0; j<MAP_SIZE; j++)
						if (map[i][j] !=0 || map_item[i][j]!=0 || special[i][j]!=0)
						{
							if (i<bounds[0]) bounds[0] = i;
							if (i>bounds[1]) bounds[1] = i;
							if (j<bounds[2]) bounds[2] = j;
							if (j>bounds[3]) bounds[3] = j;
						}
			}
			else
			{
				memset(map, 0, sizeof(map));
				memset(map_item, 0, sizeof(map_item));
			}

			if (version>=3) {
				levelPar = SWAP32(levelPar);
				fn(&levelPar, 1, sizeof(levelPar), f);
				levelPar = SWAP32(levelPar);
			}
			else if (!save)
				levelPar = 0;

			if (version>=4) {
				levelDiff = SWAP32(levelDiff);
				fn(&levelDiff, 1, sizeof(levelDiff), f);
				levelDiff = SWAP32(levelDiff);
			}
			else if (!save)
				levelDiff = 0;

			fn(bounds, sizeof(bounds), 1, f);
			player.x = SWAP32(player.x);
			player.y = SWAP32(player.y);
			fn(&player, sizeof(player), 1, f);
			player.x = SWAP32(player.x);
			player.y = SWAP32(player.y);

			int offsetx=0, offsety=0;

			if (!save && bounds[1]-bounds[0]<15) // Hacky - don't recenter map...
			{
				// Re-position map to top left (but leave a bit of space)
				// (This ensures the laser/boat effects don't clip prematurely against the edges of the screen)
				offsetx = SCREEN_W/2/TILE_W2 + 1 - (bounds[0]+bounds[1]/2);
				offsety = SCREEN_H/2/TILE_H2 + SCREEN_W/2/TILE_W2 - (bounds[2]+bounds[3]/2);
				offsetx = MAX(0, offsetx);
				offsety = MAX(0, offsety);
//				if (bounds[0] > 2)
//					offsetx = 2 - bounds[0];
//				if (bounds[2] > 2)
//					offsety = 2 - bounds[2];
			}
			bounds[0] += offsetx;
			bounds[1] += offsetx;
			bounds[2] += offsety;
			bounds[3] += offsety;
			player.x += offsetx;
			player.y += offsety;

			for (int i=bounds[0]; i<=bounds[1]; i++)
				for (int j=bounds[2]; j<=bounds[3]; j++)
				{
					unsigned char comp = map[i][j] | (map_item[i][j]<<5);
					fn(&comp, sizeof(comp), 1, f);
					map[i][j] = comp & 0x1f;
					map_item[i][j] = (comp >> 5) & 3;
				}

			if (save)
			{
				for (int i=bounds[0]; i<=bounds[1]; i++)
					for (int j=bounds[2]; j<=bounds[3]; j++)
						if (special[i][j])
						{
							int16_t len = strlen(special[i][j]);
							unsigned char x=i, y=j;
							fn(&x, sizeof(x), 1, f);
							fn(&y, sizeof(y), 1, f);
							len = SWAP16(len);
							fn(&len, sizeof(len), 1, f);
							len = SWAP16(len);
							fn(special[i][j], 1, len, f);
						}
			}
			else
			{
				while(1){
					int16_t len;
					unsigned char x, y;
					if (!fn(&x, sizeof(x), 1, f))
						break;
					fn(&y, sizeof(y), 1, f);
					x += offsetx; y += offsety;
					fn(&len, sizeof(len), 1, f);
					len = SWAP16(len);
					if (len<0) break;
					char* tmp = new char[len+1];
					tmp[len] = 0;
					fn(tmp, 1, len, f);

					SetSpecial(Pos(x,y), tmp, true, false);
				}
			}
		}
		else
			return;	// Unsupported version!

		ResetLevel();

		// Save when returning to map!
		if (isMap)
		{
			progress.general.completionPercentage = numComplete*100/numLevels;
			progress.general.masteredPercentage = numMastered*100/numLevels;			
			LoadSaveProgress(true);
		}
	}

	void SetTile(Pos const & p, Tile t, bool updateRenderer=true, bool undoBuffer=true)
	{
		if (p.x<0 || p.x>MAP_SIZE)
			return;
		if (p.y<0 || p.y>MAP_SIZE)
			return;
		if (map[p.x][p.y] == t)
			return;
		if (map[p.x][p.y] == t)
			return;

		tileCount[map[p.x][p.y]]--;
		tileCount[t]++;

		if (undoBuffer)
			undo[numUndo].Add(Undo::TileChange(p,GetTile(p),GetItem(p)));

		map[p.x][p.y] = t;

		if (updateRenderer)
			renderer(p).Add(new TileRender(t, p), time);
	}

	Tile GetTile(Pos const & p)
	{
		if (p.x<0 || p.x>=MAP_SIZE)
			return EMPTY;
		if (p.y<0 || p.y>=MAP_SIZE)
			return EMPTY;
		return map[p.x][p.y];
	}

	int GetHeight(Pos const & p)
	{
		return tileSolid[GetTile(p)]==1;
	}

	char* GetSpecial(Pos const & p)
	{
		if (p.x<0 || p.x>=MAP_SIZE)
			return NULL;
		if (p.y<0 || p.y>=MAP_SIZE)
			return NULL;
		return special[p.x][p.y];
	}

	void SetSpecial(Pos const & p, char * d, bool use_pointer=false, bool auto_activate=true)
	{
		if (p.x<0 || p.x>=MAP_SIZE || p.y<0 || p.y>=MAP_SIZE)
		{
			if (use_pointer)
				delete [] d;
			return;
		}

		delete [] special[p.x][p.y];
		if (!use_pointer && d)
		{
			
			special[p.x][p.y] = new char [strlen(d) + 1];
			strcpy(special[p.x][p.y], d);
		}
		else
			special[p.x][p.y] = d;

		if (special[p.x][p.y]==0)
			renderer(p,true).Add(new ItemRender(GetItem(p), GetTile(p)==EMPTY, p), time);
		else if (auto_activate)
			ActivateSpecial(p, 0);
	}

	int GetLevelState(Pos const & p, int recurse=0)
	{
		char* x = GetSpecial(p);
		if (!x) return 0;

		LevelSave* l = progress.GetLevel(x, false);

		int t = 1;

		if (strcmp(x, STARTING_LEVEL)==0)
			t = 2;
		if (x[0]=='_' && l && l->unlocked)
			t=3;

		if (l && l->Completed())
		{
			t = 3;
			
			if (recurse) 
				return t;

			int par = GetPar(x);
			if (progress.general.scoringOn && l->PassesPar( par ))
				t = l->BeatsPar( par ) ? 40 : 4;
		}
		if (recurse) 
			return t;

		int adj=0;
		for (Dir d=0; d<MAX_DIR; d++)
		{
			int i = GetLevelState(p+d, 1);
//			if (i>1 || i==1 && t>1)
			if ((i>=1 && t>2) || (t>=1 && i>2))
			{
				adj |= 1<<d;
				if (t==1)
					t = 2;
			}
		}

		return t | adj<<8;
	}

	void ActivateSpecial(Pos const & p, int type)
	{
		if (p.x<0 || p.x>=MAP_SIZE || p.y<0 || p.y>=MAP_SIZE)
			return;

		char * x = special[p.x][p.y];

		if (x==0 || x[0]==0)
			return;

		if (type==2 && x[0]=='_') // Phase2 init - unlock
		{
			int t = GetLevelState(p);
			int target = atoi(x+1), targetM = 0;
			if (target>1000) targetM=target=target-100;
			if (t > 1 && numComplete >= target && numMastered >= targetM)
			{
				LevelSave* l = progress.GetLevel(x, true);
				if (!l->unlocked)
				{
					l->unlocked = true;

					renderer(p, true).Add(new LevelSelectRender(p, 5, GetLevelState(p)>>8), time+0.01);
					renderer().Add(new ExplosionRender(p, 0), time + 0.6);
					renderer().Add(new ExplosionRender(p, 1), time + 1.1);
					renderer(p, true).Add(new LevelSelectRender(p, -1, GetLevelState(p)>>8), time + 1.1);
				}
			}
		}
		
		if (type==0) // Init & count levels
		{
			if (x[0]=='_')
			{
				int t = GetLevelState(p);
				int unlock = progress.GetLevel(x, true)->unlocked;
				LevelSelectRender* lsr = new LevelSelectRender( p, unlock ? -1 : (t>>8) ? 5 : 1, t>>8 );
				if ((t>>8) && p.x > mapRightBound) mapRightBound = p.x;
				#ifdef MAP_EDIT_HACKS
					lsr->magic = -atoi(x+1);
					SetTile(p, LIFT_DOWN, true, false);
				#else
					SetTile(p, EMPTY, true, false);
				#endif
				renderer(p,true).Add(lsr, time);
			}
			else
			{
				//printf("Level: %s\n", x);

				int t = GetLevelState(p);
				numLevels++;
				if (t && !GetItem(p))
				{
					if (!isMap)
					{
						isMap = true;
						mapRightBound = 0;
					}
					currentLevelInfo = 0;

					if ((t&0xff)>=2)
					{
						LevelSave* l = progress.GetLevel(x, true);
						if (!l->unlocked)
						{
							l->unlocked = true;

							renderer(p, true).Add(new LevelSelectRender(p, -1, 0), time+0.01);
							renderer().Add(new ExplosionRender(p, 0), time + 0.6);
							renderer(p, true).Add(new LevelSelectRender(p, t & 0xff, t>>8), time + 0.6);
						}

						numLevelsFound++;
						if (p.x > mapRightBound) mapRightBound = p.x;
					}
					if ((t&0xff)>=3)
						numComplete++;
					if ((t&0xff)>=4)
						numMastered++;

					LevelSelectRender* lsr = new LevelSelectRender( p, t & 0xff, t>>8 );

					#ifdef MAP_EDIT_HACKS
						lsr->magic = 0;
						int t = GetAutoTile(x, true);
						int v = GetAutoTile(x, false);
						if (MAP_EDIT_HACKS_DISPLAY_UNLOCK)
							lsr->magic = v;
						else
							lsr->magic = GetPar(x, true);
						t = 1;
						SetTile(p, t, true, false);
					#else
						SetTile(p, EMPTY, true, false);
					#endif

					renderer(p,true).Add(lsr, time);
				}
			}
		}
		
		if (type==1 && x[0]!='_') // Clicked on
		{
			int t = GetLevelState(p);
			if (t>1)
			{
				LoadSave(x, false);
			}
		}
	}

	void SetItem(Pos const & p, int t, bool updateRenderer=true, bool undoBuffer=true)
	{
		if (p.x<0 || p.x>MAP_SIZE)
			return;
		if (p.y<0 || p.y>MAP_SIZE)
			return;
		if (map_item[p.x][p.y] == t)
			return;

		if (undoBuffer)
			undo[numUndo].Add(Undo::TileChange(p,GetTile(p),GetItem(p)));

		map_item[p.x][p.y] = t;

		if (updateRenderer)
			renderer(p,true).Add(new ItemRender(t, GetTile(p)==EMPTY, p), time);
	}

	Tile GetItem(Pos const & p)
	{
		if (p.x<0 || p.x>=MAP_SIZE)
			return EMPTY;
		if (p.y<0 || p.y>=MAP_SIZE)
			return EMPTY;
		return map_item[p.x][p.y];
	}

	void LoadSaveProgress(bool save)
	{
		FILE* f = file_open(currentSlot, save ? "wb" : "rb");
		if (f)
		{
			progress.LoadSave(f, save);
			fclose(f);
		}
		else
		{
			if (!save)
				progress.Clear();
		}
	}
	void LoadProgress()
	{
		LoadSaveProgress(false);
	}
	void SaveProgress()
	{
		LoadSaveProgress(true);
	}

	SDL_Surface* Load(const char * bmp, bool colourKey=true)
	{
		typedef unsigned int uint32;
		uint32* tmp = 0;

		SDL_Surface * g = 0;

#ifdef EDIT
		if (strstr(bmp, ".bmp"))
		{
			g = SDL_LoadBMP(bmp);

			char out[1024];
			strcpy(out, bmp);
			strcpy(strstr(out, ".bmp"), ".dat");

//			SDL_PixelFormat p;
//			p.sf = 1;
//			SDL_Surface* tmp = SDL_ConvertSurface(g, &p, SDL_SWSURFACE);

			short w=g->w, h=g->h;
			char* buf = (char*) g->pixels;
			if (colourKey)
			{
				while (IsEmpty(g, w-1, 0, 1, h) && w>1)
					w--;
				while (IsEmpty(g, 0, h-1, w, 1) && h>1)
					h--;
			}

			FILE* f = file_open(out, "wb");
			fwrite(&w, sizeof(w), 1, f);
			fwrite(&h, sizeof(h), 1, f);

			uint32 mask = IMAGE_DAT_OR_MASK;
			for (int i=0; i<(int)w*h; )
			{
				uint32 c = (*(uint32*)&buf[(i%w)*3 + (i/w)*g->pitch] | mask);
				int i0 = i;
				while (i < (int)w*h && c == (*(uint32*)&buf[(i%w)*3 + (i/w)*g->pitch] | mask))
					i++;
				c &= 0xffffff;
				i0 = i-i0-1;
				if (i0 < 0xff)
					c |= i0 << 24;
				else
					c |= 0xff000000;
				
				fwrite(&c, sizeof(c), 1, f);
				
				if (i0 >= 0xff)
					fwrite(&i0, sizeof(i0), 1, f);
			}
			fclose(f);

			SDL_FreeSurface(g);

			bmp = out;
		}
#endif			

		FILE* f = file_open(bmp, "rb");
		if (!f) FATAL("Unable to open file", bmp);

		int16_t w,h;
		fread(&w, sizeof(w), 1, f);
		fread(&h, sizeof(h), 1, f);
		w = SWAP16(w);
		h = SWAP16(h);
		if (w>1500 || h>1500 || w<=0 || h<=0) FATAL("Invalid file", bmp);

		tmp = new uint32[(int)w*h];
		
		uint32 c = 0;
		uint32 cnt = 0;
		for (int p=0; p<(int)w*h; p++)
		{
			if (cnt)
				cnt -= 0x1;
			else
			{
				fread(&c, sizeof(c), 1, f);
				c = SWAP32(c);
				cnt = c >> 24;
				if (cnt==255) {
					fread(&cnt, sizeof(cnt), 1, f);
					cnt = SWAP32(cnt);
				}
			}
			tmp[p] = c | 0xff000000;
		}

		g = SDL_CreateRGBSurfaceFrom(tmp, w, h, 32, w*4, 
			0xff0000,
			0xff00,
			0xff,
			0xff000000 );

		fclose(f);


		if (!g) FATAL("Unable to create SDL surface");
		if (colourKey)
			SDL_SetColorKey(g, SDL_SRCCOLORKEY, SDL_MapRGB(g->format, WATER_COLOUR));
		SDL_Surface * out = SDL_DisplayFormat(g);
		SDL_FreeSurface(g);
		delete [] tmp;
		if (!out) FATAL("Unable to create SDL surface (SDL_DisplayFormat)");
		return out;
	}

	#ifdef USE_LEVEL_PACKFILE
		PackFile1 levelFiles;
	#endif
	HexPuzzle()
	{
		SDL_WM_SetCaption(GAMENAME, 0);

		time = 0;

		#ifdef USE_LEVEL_PACKFILE
			FILE* f = file_open("levels.dat", "rb");
			if (!f)
				FATAL("Unable to open file", "levels.dat");
			levelFiles.Read(f);
			fclose(f);
		#endif

		LoadGraphics();

		isMap = false;
		editMode = false;

		currentLevelInfo = 0;

		editTile = 0;
		levelPar = 0;
		levelDiff = 5;
		turboAnim = 0;

		memset(map, 0, sizeof(map));
		memset(map_item, 0, sizeof(map_item));
		memset(special, 0, sizeof(special));
		
		LoadProgress();

//		player = Pos(1,11);

//		ResetLevel();

		LoadMap();
	}

	void LoadMap()
	{
		#ifndef EDIT
			progress.GetLevel(STARTING_LEVEL, true)->unlocked = 1;
			if (!progress.GetLevel(STARTING_LEVEL, true)->Completed())
			{
				LoadSave(STARTING_LEVEL, false);
				return;
			}
		#endif
		
		//editMode = false;
		LoadSave(mapname, false);
		PlayMusic(HHOP_MUSIC_MAP);
	}

	void Render()
	{
		if (!activeMenu || activeMenu->renderBG)
		{
			SDL_Rect src  = {0,0,screen->w,screen->h};
			SDL_Rect dst  = {0,0,screen->w,screen->h};
			if (isRenderMap)
			{
				int boundW = mapBG->w;
	#ifndef EDIT
				boundW = MIN(boundW, (mapRightBound+4) * TILE_W2 - TILE_W1);
	#endif
				src.x = scrollX - initScrollX;
				if (src.x+src.w > boundW)
				{
					int diff = src.x+src.w - boundW;
					src.x -= diff;
					if (isMap)
						scrollX -= diff;
				}
				if (src.x < 0)
				{
					if (isMap)
						scrollX -= src.x;
					src.x = 0;
				}
				//scrollY = initScrollY;

				if (isMap)
					mapScrollX = scrollX;

				SDL_BlitSurface(mapBG, &src, screen, &dst);
			}
			else
				SDL_BlitSurface(gradient, &src, screen, &dst);

			renderer.Render(time, true);

			if (!hintsDone && !isFadeRendering)
			{
				DoHints();
			}

			if (1)
			{
				SDL_Rect src = {0,SCREEN_H-1,SCREEN_W,1};
				SDL_Rect dst = {0,SCREEN_H-1,SCREEN_W,1};
				for (int i=0; i<SCREEN_H; i++)
				{
					dst.x = src.x = 0;
					dst.y = src.y = SCREEN_H-1-i;
					src.w = SCREEN_W;
					src.h = 1;

					if (isRenderMap)
					{
						src.x += (int)( sin(i*0.9 + time*3.7) * sin(i*0.3 + time*0.7)*4 );
						src.y += (int)( (sin(i*0.3 - time*2.2) * sin(i*0.48 + time*0.47) - 1) * 1.99 );
					}
					else
					{
						src.x += (int)( sin(i*0.5 + time*6.2) * sin(i*0.3 + time*1.05) * 5 );
						src.y += (int)( (sin(i*0.4 - time*4.3) * sin(i*0.08 + time*1.9) - 1) * 2.5 );
					}
					SDL_BlitSurface(screen, &src, screen, &dst);
				}
			}

			if(isRenderMap)
				SDL_BlitSurface(mapBG2, &src, screen, &dst);

			renderer.Render(time, false);

			if (!isRenderMap && !isMap && !isFadeRendering)
			{
				int v[3] = {player_items[0], player_items[1], player_score};
				if (numUndo > 1 && time < undo[numUndo-2].endTime)
				{
					int i = numUndo-1;
					while (i>1 && time<undo[i-1].time)
						i--;
					v[0] = undo[i].numItems[0];
					v[1] = undo[i].numItems[1];
					v[2] = undo[i].score;
				}
				if (numUndo>1 && time < undo[0].time)
					v[0]=v[1]=v[2]=0;
	#ifdef EDIT
        /* TRANSLATORS: Anti-Ice are pickups, which turn ice plates into solid
           plates once you step on them. Each pickup changes one ice plate */
				Print(0,0,_("Anti-Ice: %d"), v[0]);
				Print(0,FONT_SPACING,_("Jumps: %d"), v[1]);
				Print(0,FONT_SPACING*2,_("Score: %d (%d)"), v[2], player_score);
        /* TRANSLATORS: Par is similar to golf, a pre defined score which you
           can attempt to beat */
				Print(0,FONT_SPACING*3,_("Par:   %d"), levelPar);
				Print(0,FONT_SPACING*4,_("Diff:  %d"), levelDiff);
	#else
				if (showScoring)
					Print(0, SCREEN_H-FONT_SPACING, _(" Par: %d   Current: %d"), levelPar, v[2]);

				if (v[0])
					Print(0,0,_(" Anti-Ice: %d"), v[0]);
				else if (v[1])
					Print(0,0,_(" Jumps: %d"), v[1]);
	#endif
			}
			if (isRenderMap && isMap && !isFadeRendering)
			{
	#if 0//def EDIT
				Print(0,0,_("Points: %d"), numComplete+numMastered);
				Print(0,FONT_SPACING,_("Discovered: %d%% (%d/%d)"), numLevelsFound*100/numLevels, numLevelsFound, numLevels);
				Print(0,FONT_SPACING*2,_("Complete: %d%% (%d)"), numComplete*100/numLevels, numComplete);
				Print(0,FONT_SPACING*3,_("Mastered: %d%% (%d)"), numMastered*100/numLevels, numMastered);
	#else
				if (numComplete==numLevels && progress.general.endSequence>0)
					Print(0, SCREEN_H-FONT_SPACING, _(" %d%% Mastered"), numMastered*100/numLevels);
				else
					Print(0, SCREEN_H-FONT_SPACING, _(" %d%% Complete"), numComplete*100/numLevels);

				if (numMastered >= numLevels && progress.general.endSequence < 2)
				{
					progress.general.endSequence = 2;
					LoadSaveProgress(true);

					new Fader(-1, -7, 0.3);
				}
				if (numComplete >= numLevels && progress.general.endSequence < 1)
				{
					progress.general.endSequence = 1;
					LoadSaveProgress(true);
					
					new Fader(-1, -5, 0.3);
				}
	#endif
			}
			if ((currentLevelInfo || noMouse) && isMap && isRenderMap && !activeMenu && isFadeRendering<=0)
			{
				Pos p;
				if (noMouse)
					p = keyboardp;
				else
					p = mousep;
				int pad = SCREEN_W/80;
	//			SDL_Rect src = {0, 0, uiGraphics->w, uiGraphics->h};
				SDL_Rect dst = {pad, SCREEN_H-TILE_H2-pad, 0, 0};
		//		dst.x = p.getScreenX() + TILE_W3/2 - scrollX;
		//		dst.y = p.getScreenY() - src.h/2 - scrollY;
				dst.x = p.getScreenX() - scrollX;
				dst.y = p.getScreenY() - scrollY - FONT_SPACING*3 - FONT_SPACING/2;
		//		if (dst.x > SCREEN_W*2/3) dst.x -= TILE_W3 + src.w;
		//		if (dst.y+src.h > screen->h-pad) dst.y = screen->h-pad - src.h;

				RenderTile(false, 0, p.getScreenX(), p.getScreenY());
			//	SDL_BlitSurface(uiGraphics, &src, screen, &dst);

		//		dst.x += src.w/2;

				if (currentLevelInfo)
				{
					keyboardp = p;

					PrintC(true, dst.x, dst.y - FONT_SPACING/4, currentLevelInfo->name);

					if (currentLevelInfo->file[0]!=0)
					{
						if (player_score > 0)
						{
							if (progress.general.scoringOn)
							{
								PrintC(false, dst.x, dst.y + FONT_SPACING*4 - FONT_SPACING/4, _("Best:% 3d"), player_score);
								PrintC(false, dst.x, dst.y + FONT_SPACING*5 - FONT_SPACING/4, _("Par:% 3d"), levelPar);
							}
							else
								PrintC(false, dst.x, dst.y + FONT_SPACING*4 - FONT_SPACING/4, _("Completed"), player_score);
						}
						else
							PrintC(false, dst.x, dst.y + FONT_SPACING*4 - FONT_SPACING/4, _("Incomplete"), player_score);
					}
				}
			}

			// "Win"
			if (win && numUndo > 0 && time > undo[numUndo-1].endTime + 2)
			{
				if (currentFile[0] && winFinal==0)
				{
					LevelSave* l = progress.GetLevel(currentFile, true);

					new WinLoseScreen(true, player_score, showScoring ? levelPar : 0, l && showScoring && l->Completed() ? l->GetScore() : 0);

					if (l->IsNewCompletionBetter(player_score))
					{
						l->SetScore(player_score);

						l->SetSolution(numUndo);

						for (int i=0; i<numUndo; i++)
							l->SetSolutionStep(i, undo[i].playerMovement);
					}

					SaveProgress();
				}

				winFinal = 1;
			}
			else
				winFinal = 0;		

			// Move up "level complete" writing so it doesn't feel like everything's ground to a halt...
			if (win && numUndo > 0 && time > undo[numUndo-1].endTime && !winFinal)
			{
				double t = (time - undo[numUndo-1].endTime) / 2;
				t=1-t;
				t*=t*t;
				t=1-t;
				int y = SCREEN_H/3 - FONT_SPACING + 1;
				y = SCREEN_H + int((y-SCREEN_H)*t);
				PrintC(true, SCREEN_W/2, y, _("Level Complete!"));
			}
		}

		if (activeMenu)
			activeMenu->Render();

		if (!noMouse)
		{
			// Edit cursor
			if (editMode)
			{
				RenderTile(false, editTile, mousex+scrollX, mousey+scrollY);
			}
		}
	}

	int Count(Tile t)
	{
		return tileCount[t];
	}
	int Swap(Tile t, Tile t2)
	{
		const int num = Count(t) + Count(t2);
		if (t==t2 || num==0)
			return Count(t);	// Nothing to do...
		
		int count=0;
		for (int x=0; x<MAP_SIZE; x++)
			for (int y=0; y<MAP_SIZE; y++)
			{
				if (GetTile(Pos(x,y))==t)
				{
					count++;
					SetTile(Pos(x,y), t2);
				}
				else if (GetTile(Pos(x,y))==t2)
				{
					count++;
					SetTile(Pos(x,y), t);
				}
				if (count==num)
					return count;
			}
		return count;
	}
	int Replace(Tile t, Tile t2)
	{
		const int num = Count(t);
		if (t==t2 || num==0)
			return num;	// Nothing to do...

		int count=0;
		for (int x=0; x<MAP_SIZE; x++)
			for (int y=0; y<MAP_SIZE; y++)
			{
				Pos p(x,y);
				if (GetTile(p)==t)
				{
					count++;

					SetTile(p, t2, false);

					if (t==COLLAPSE_DOOR && t2==COLLAPSABLE)
						renderer(p).Add(new BuildRender(p, -1, 1, 1), time + (rand() & 255)*0.001);
					else if (t==COLLAPSE_DOOR2 && t2==COLLAPSABLE2)
						renderer(p).Add(new BuildRender(p, -1, 1, 1, 1), time + (rand() & 255)*0.001);
					else
						SetTile(p, t2);

					if (count==num)
						return count;
				}
			}
		return count;
	}

	Tile editTile;
	bool editMode;
	void ResetUndo()
	{
		UndoDone();
		undoTime = -1;
		numUndo = 0;
		win = false;
	}

	void UpdateCursor(Pos const & s)
	{
		static Pos _s;
		if (s.x!=_s.x || s.y!=_s.y)
		{
			_s = s;
		
			char* sp = GetSpecial(s);
			char tmp[1000];
			tmp[0]='\0';
			if (sp)
			{
				if (isMap)
				{
					currentLevelInfo = 0;
					levelPar = player_score = -1;
					if (GetLevelState(s)>=2)
					{
						LevelSave* l = progress.GetLevel(sp, true);
						if (l)
						{
							currentLevelInfo = GetLevelInfo(sp);
							levelPar = GetPar(sp);
							player_score = l->GetScore();
						}
					}
				}

#ifdef EDIT
				sprintf(tmp, _("Special(%d,%d): %s (%d)"), s.x, s.y, sp ? sp : _("<None>"), GetPar(sp));
				SDL_WM_SetCaption(tmp, NULL);
#endif
			}
			else if (currentFile[0])
			{
#ifdef EDIT
				SDL_WM_SetCaption(currentFile, NULL);
#endif
				if (isMap)
					currentLevelInfo = 0;
			}
		}
	}

	virtual void Mouse(int x, int y, int dx, int dy, int button_pressed, int button_released, int button_held) 
	{
		if (activeMenu)
		{
			activeMenu->Mouse(x,y,dx,dy,button_pressed,button_released,button_held);
			return;	
		}

		if (isFadeRendering)
			return;


#ifndef EDIT
		if (button_pressed==2 || (button_pressed==4 && isMap))
		{
			KeyPressed(SDLK_ESCAPE, 0);
			keyState[SDLK_ESCAPE] = 0;
			return;
		}
#endif

		x += scrollX;
		y += scrollY;

		Pos s = Pos::GetFromWorld(x,y);
		if (tileSolid[GetTile(Pos::GetFromWorld(x,y+TILE_HUP))] == 1)
			s = Pos::GetFromWorld(x,y+TILE_HUP);

		mousep = s;

		UpdateCursor(s);

#ifdef EDIT
		if (button_held & ~button_pressed & 4)
		{
			scrollX -= dx;
			scrollY -= dy;
		}
#endif

		if (!editMode)
		{
			if (isMap && (button_pressed & 1))
			{
				ActivateSpecial(s, 1);
				return;
			}
			if (!isMap && win && winFinal)
			{
				if (button_pressed & 1)
				{
					LoadMap();
					return;
				}
			}
			if(!isMap)
			{
				if((button_pressed & 1) || ((button_held & 1) && (numUndo==0 || time>=undo[numUndo-1].endTime)))
				{
					if(s.x==player.x && s.y==player.y)
					{
						// Don't activate jump powerup without a new click
						if (button_pressed & 1)
							Input(-1);
					}
					else if(s.x==player.x && s.y<player.y)
						Input(0);
					else if(s.x==player.x && s.y>player.y)
						Input(3);
					else if(s.y==player.y && s.x<player.x)
						Input(5);
					else if(s.y==player.y && s.x>player.x)
						Input(2);
					else if(s.y+s.x==player.y+player.x && s.x>player.x)
						Input(1);
					else if(s.y+s.x==player.y+player.x && s.x<player.x)
						Input(4);
				}
				if ((button_pressed & 4) || ((button_held & 4) && (undoTime < 0)))
					Undo();
			}
			return;
		}

#ifdef EDIT
		if (!button_pressed && !button_held)
			return;

		if (button_pressed==1)
			if (editTile<0)
				editTile = GetItem(s)==1 ? -3 : GetItem(s)==2 ? -2 : -1;

		if (button_held==1 || button_pressed==1)
		{
			ResetUndo();
			if (editTile>=0)
				SetTile(s, editTile, true, false);
			else
				SetItem(s, editTile==-2 ? 0 : editTile==-1 ? 1 : 2, true, false);
		}

		if (button_pressed==2)
		{
			editTile = GetTile(s);
		}

		if (button_pressed==8)
		{
			editTile=editTile-1;
			if (editTile<=0) editTile=NumTileTypes-1;
		}

		if (button_pressed==16)
		{
			editTile=editTile+1;
			if (editTile<=0) editTile=1;
			if (editTile==NumTileTypes) editTile=0;
		}

		if (button_pressed==64)
		{
			ResetUndo();
			player = s;
			dead = false;
			renderer.player.Reset(-1);
			renderer.player.Add(new PlayerRender(player, GetHeight(player), dead), 0);
		}

		if (button_pressed==256)
		{
			char* fn = LoadSaveDialog(false, true, _("Select level"));
			if (fn)
			{
				char * l = strstr(fn, "Levels");
				if(l)
				{
					FILE * f = file_open(l,"rb");
					if (f) 
						fclose(f);
					if (f)
						SetSpecial(s, l);
					else if (l[6]!=0 && l[7]=='_')
						SetSpecial(s, l+7);
				}
				UpdateCursor(Pos(-1,-1));
			}
		}
		if (button_pressed==512)
		{
			SetSpecial(s, NULL);
			UpdateCursor(Pos(-1,-1));
		}
		if (button_pressed==1024)
		{
			static char x[1000] = "";
			if (!(s.x<0 || s.x>=MAP_SIZE || s.y<0 || s.y>=MAP_SIZE))
			{
				char tmp[1000];
				strcpy(tmp, x);
				if (GetSpecial(s))
					strcpy(x, GetSpecial(s));
				else
					x[0] = 0;
				SetSpecial(s, tmp[0] ? tmp : 0);
				if (!tmp[0])
					SetTile(s, EMPTY, true, false);
			}
		}

		if (button_pressed==32)
		{
			editTile = editTile<0 ? 1 : -1;
		}
#endif // EDIT
	}

	void CheckFinished()
	{
		bool slow = false;
		if (Count(COLLAPSABLE)==0)
		{
			if (Replace(COLLAPSE_DOOR, COLLAPSABLE) == 0)
				win = true;
			else
				slow = true;
			Replace(SWITCH, NORMAL);
		}
		else
			win = false;

		if (Count(COLLAPSABLE2)==0)
			if (Replace(COLLAPSE_DOOR2, COLLAPSABLE2))
				slow = true;

		if (slow)
		{
			QueueSound(HHOP_SOUND_COLLAPSE, time);
			time += BUILD_TIME;
		}
	}
	bool Collide(Pos p, bool high)
	{
		Tile t = GetTile(p);
//		switch(t)
//		{
//		default:
			if (!high)
				return tileSolid[t]==1;
			else
				return false;
//		}
	}
	void Undo()
	{
		if (numUndo==0) return;
		
		UndoDone(); // Complete previous undo...

		numUndo--;

		if (time > undo[numUndo].endTime)
			time = undo[numUndo].endTime;
		undoTime = undo[numUndo].time;
		
		undo[numUndo].Restore(this);

		// Cancel all queued sounds.
		UndoSound();

		// Restore game music if undid winning move.
		PlayMusic(HHOP_MUSIC_GAME);
	}
	void UndoDone()
	{
		if (undoTime < 0) 
			return;
		renderer.Reset(undoTime);
		time = undoTime;
		undoTime = -1;
	}
	void ScoreDestroy(Pos p)
	{
		Tile t = GetTile(p);
		if (t==COLLAPSABLE || t==COLLAPSE_DOOR)
		{}
		else if (t != EMPTY)
		{
			player_score += 10;
		}
	}

	bool LaserTile(Pos p, int mask, double fireTime)
	{
		if (&renderer(p) == &renderer(Pos(-1,-1)))
			return false;
		//if (!renderer.Visible(p))
		//	return false;

		TileRender* tr = 0;
		if (time <= renderer(p).GetLastTime())
			if (fireTime < renderer(p).GetLastTime())
			{
				renderer(p).Add(tr = new TileRender(GetTile(p), p, mask), fireTime);
				((TileRender*)renderer(p).GetStage(time+10/*HACKY!*/))->special |= mask;
			}
			else
			{
				tr = new TileRender(GetTile(p), p, mask | ((TileRender*)renderer(p).GetStage(time+10/*HACKY!*/))->special);
				renderer(p).Add(tr, fireTime);
			}
		else
			renderer(p).Add(tr = new TileRender(GetTile(p), p, mask), fireTime);

		if (tr)
		{
			tr->specialDuration = time + LASER_LINE_TIME - fireTime + LASER_FADE_TIME;
		}
		return true;
	}
	void FireGun(Pos newpos, Dir d, bool recurse, double fireTime)
	{
		static Pos hits[100];
		static Dir hitDir[100];
		static unsigned int numHits=0;
		if (!recurse)
			numHits = 0;

		double starttime = fireTime;
		for (Dir fd=((d<0)?0:d); fd<((d<0)?MAX_DIR:d+1); fd++)
		{
			fireTime = starttime;
		//	starttime += 0.03;

			Pos p = newpos + fd;
			int range = 0;
			for (; range<MAP_SIZE; range++, p=p+fd)
			{
				Tile t = GetTile(p);
				if (tileSolid[t]!=-1)
				{
					if (t!=TRAP)
						renderer(p).Add(new TileRender(tileSolid[t]==1 ? TILE_WHITE_WALL : TILE_WHITE_TILE, p), fireTime+0.1);

					unsigned int i;
					for (i=0; i<numHits; i++)
						if (hits[i]==p)
							break;
					if (i==numHits || 
						(t==TRAP && (hitDir[i]&(1<<fd))==0)
					   )
					{
						if (i==numHits)
						{
							if (i >= sizeof(hits)/sizeof(hits[0]))
								return;
							hitDir[i] = 1 << fd;
							hits[i] = p;
							numHits++;
						}
						else
						{
							hitDir[i] |= 1 << fd;
						}
						if (t==TRAP)
						{
							int dirmask = 
								  1<<((fd+2) % MAX_DIR)
								| 1<<((fd+MAX_DIR-2) % MAX_DIR);

							if (LaserTile(p, dirmask, fireTime))
								fireTime += (time+LASER_LINE_TIME - fireTime) / 40;
//							fireTime += LASER_SEGMENT_TIME;

							FireGun(p, (fd+1) % MAX_DIR, true, fireTime);
							FireGun(p, (fd+MAX_DIR-1) % MAX_DIR, true, fireTime);
						}
					}
					break;
				}
				else
				{
					LaserTile(p, 1<<(fd%3), fireTime);

					fireTime += (time+LASER_LINE_TIME - fireTime) / 40;
//					fireTime += LASER_SEGMENT_TIME;
				}
			}
			
//			renderer().Add(new LaserRender(newpos, fd, range), time);
		}

		if (!recurse)
		{
			//double _time = time;
			time += LASER_LINE_TIME;
			for (unsigned int i=0; i<numHits; i++)
			{
				Pos p = hits[i];
				Tile t = GetTile(p);

				if (t==TRAP)
					continue;

				ScoreDestroy(p);

				renderer(p).Add(new ExplosionRender(p, t==GUN), time);
				//renderer(p).Add(new TileRender(EMPTY, p), time+2);
				SetTile(p, EMPTY, false);

				if (GetItem(p))
					renderer(p,true).Add(new ItemRender(GetItem(p), 1, p), time);

				if (t==GUN)
					QueueSound(HHOP_SOUND_EXPLODE_BIG, time);
				else
					QueueSound(HHOP_SOUND_EXPLODE_SMALL, time);

				if (t==GUN)
				{
					for (Dir j=0; j<MAX_DIR; j++)
					{
						if (GetTile(p+j)!=EMPTY)
						{
							renderer(p+j).Add(new TileRender(tileSolid[GetTile(p+j)]==1 ? TILE_WHITE_WALL : TILE_WHITE_TILE, p+j), time+0.05);
							renderer(p+j).Add(new ExplosionRender(p+j), time+0.2);

							if (GetItem(p+j))
								renderer(p+j,true).Add(new ItemRender(GetItem(p+j), 1, p), time);

							//renderer(p+j).Add(new TileRender(EMPTY, p+j), time+2.2);
						}
						ScoreDestroy(p + j);
						SetTile(p + j, EMPTY, false);
					}
				}
			}

			time += MAX(LASER_FADE_TIME, 0.15);
			//time = _time;
			CheckFinished();
		}
	}
	int GetLastPlayerRot()
	{
		RenderStage* rs = renderer.player.GetStage(-1);
		if (!rs) return 3;
		return ((PlayerRender*)rs)->r;
	}
	bool Input(Dir d)
	{
		if (dead || win || isMap)
			return false;

		// Complete undo
		UndoDone();

		// Jump forwards in time to last move finishing
		if (numUndo > 0 && time < undo[numUndo-1].endTime)
			time = undo[numUndo-1].endTime;

		double realTime = time;
		double endAnimTime = time;
		bool high = (tileSolid[GetTile(player)] == 1);
		Pos playerStartPos = player;
		Pos oldpos = player;
		int oldPlayerHeight = GetHeight(oldpos);
		Pos newpos = player + d;

		int playerRot = GetLastPlayerRot();
		if (d!=-1 && d!=playerRot)
		{
			while (d!=playerRot)
			{
				if ((d+6-playerRot) % MAX_DIR < MAX_DIR/2)
					playerRot = (playerRot+1) % MAX_DIR;
				else
					playerRot = (playerRot+MAX_DIR-1) % MAX_DIR;

				time += 0.03;

				if (GetTile(oldpos) == FLOATING_BALL)
				{
					TileRender* t = new TileRender(FLOATING_BALL, oldpos);
					t->special = playerRot + 256;
					renderer(oldpos).Add(t, time);

					renderer.player.Add(new PlayerRender(playerRot, Pos(-20,-20), oldPlayerHeight, Pos(-20,-20), oldPlayerHeight, dead), time);
				}
				else
				{
					PlayerRender *p = new PlayerRender(playerRot, player, oldPlayerHeight, player, oldPlayerHeight, dead);
					p->speed = 0;
					renderer.player.Add(p, time);
				}
			}

			time += 0.03;
		}

		if (d<0 && player_items[1]==0)
			return false;

		if (d >= 0)
		{
			if (tileSolid[GetTile(newpos)] == -1)
			{
				time = realTime;
				return false;
			}
			if (Collide(newpos, high))
			{
				time = realTime;
				return false;
			}
		}

		// Don't change any real state before this point!
		if (numUndo >= MAX_UNDO)
		{
			numUndo--;
			for(int i=0; i<MAX_UNDO-1; i++)
				undo[i] = undo[i+1];
		}
		undo[numUndo].New(d, player, player_items, time, player_score);

		if (d<0)
		{
			QueueSound(HHOP_SOUND_USED_JUMP, time);
			player_items[1]--;
		}

		double time0 = time;
		time += 0.15;	//Time for leave-tile fx

		if (d>=0)
		{
			QueueSound(HHOP_SOUND_STEP, time);
			switch (GetTile (newpos))
			{
				case COLLAPSABLE:
				case COLLAPSABLE2:
				case COLLAPSE_DOOR:
				case COLLAPSE_DOOR2:
					QueueSound(HHOP_SOUND_CRACK, time + 0.28);
					break;
			}
		}

		switch (GetTile(oldpos))
		{
			case COLLAPSABLE:
				QueueSound(HHOP_SOUND_DISINTEGRATE, time);
				SetTile(oldpos, EMPTY);
				renderer(oldpos).Add(new DisintegrateRender(oldpos), time);
				CheckFinished();
				break;

			case COLLAPSE_DOOR:
				// Don't need to CheckFinished - can't be collapse doors around
				//  unless there're still collapsable tiles around.
				QueueSound(HHOP_SOUND_DISINTEGRATE, time);
				SetTile(oldpos, EMPTY);
				renderer(oldpos).Add(new DisintegrateRender(oldpos, 1), time);
				break;

			case COLLAPSABLE2:
				QueueSound(HHOP_SOUND_DISINTEGRATE, time);
				SetTile(oldpos, COLLAPSABLE, false);
				renderer(oldpos).Add(new DisintegrateRender(oldpos, 0, 1), time);
				player_score += 10; 
				CheckFinished();
				break;

			case COLLAPSE_DOOR2:
				QueueSound(HHOP_SOUND_DISINTEGRATE, time);
				SetTile(oldpos, COLLAPSE_DOOR, false);
				renderer(oldpos).Add(new DisintegrateRender(oldpos, 1, 1), time);
				player_score += 10; 
				break;

			case COLLAPSABLE3:
				SetTile(oldpos, COLLAPSABLE2);
				break;
		}

		time = time0;	//End of leave-tile fx

		int retry_pos_count=0;
retry_pos:
		retry_pos_count++;
		
		if (GetItem(newpos)==1)
		{
			QueueSound(HHOP_SOUND_FOUND_ANTIICE, time);
			renderer(newpos, true).Add(new ItemCollectRender(GetItem(newpos), newpos), time + JUMP_TIME/2);
			SetItem(newpos, 0, false);
			player_items[0]++;
		}
		if (GetItem(newpos)==2)
		{
			QueueSound(HHOP_SOUND_FOUND_JUMP, time);
			renderer(newpos, true).Add(new ItemCollectRender(GetItem(newpos), newpos), time + JUMP_TIME/2);
			SetItem(newpos, 0, false);
			player_items[1]++;
		}

		if (GetTile(player) == FLOATING_BALL)
		{
			TileRender* t = new TileRender(FLOATING_BALL, player);
			t->special = 0;
			renderer(oldpos).Add(t, time);
		}

		PlayerRender *p = new PlayerRender(playerRot, player, oldPlayerHeight, newpos, GetHeight(newpos), dead);

		// alternate leg (hacky!)
		if (1)
		{
			static int l=0;
			l++;
			p->type = l & 1;
		}

		if (retry_pos_count!=0 && GetTile(player)==TRAP)
		{
			p->speed /= 1.5;
			p->type = 2;
		}
		if (d==-1)
			p->speed = JUMP_TIME * 1.5;
		renderer.player.Add(p, time);

		endAnimTime = MAX(endAnimTime, time + p->speed+0.001);
		time += p->speed;

		player = newpos;

		switch (GetTile(newpos))
		{
			case COLLAPSABLE:
				renderer(newpos).Add(new TileRender(TILE_GREEN_CRACKED, newpos), time);
				break;
			case COLLAPSE_DOOR:
				renderer(newpos).Add(new TileRender(TILE_GREEN_CRACKED_WALL, newpos), time);
				break;
			case COLLAPSABLE2:
				renderer(newpos).Add(new TileRender(TILE_BLUE_CRACKED, newpos), time);
				break;
			case COLLAPSE_DOOR2:
				renderer(newpos).Add(new TileRender(TILE_BLUE_CRACKED_WALL, newpos), time);
				break;

			case EMPTY:
				dead = true;
				break;

			case BUILDER:
			{
				double pretime = time;
				bool done = false;
				time += 0.15;
				for (Dir fd=0; fd<MAX_DIR; fd++)
				{
					Tile t2 = GetTile(newpos + fd);
					if (t2==EMPTY)
					{
						done = true;
						SetTile(newpos+fd, COLLAPSABLE, false);
						renderer(newpos+fd).Add(new BuildRender(newpos+fd, fd, 0), time);
					}
					else if (t2==COLLAPSABLE)
					{
						done = true;
						SetTile(newpos+fd, COLLAPSE_DOOR, false);
						renderer(newpos+fd).Add(new BuildRender(newpos+fd, fd, 1), time);
					}
				}
				if (done)
				{
					QueueSound(HHOP_SOUND_BUILDER, time);
					time += BUILD_TIME;
				}
				else
					time = pretime;
				CheckFinished();
				endAnimTime = MAX(endAnimTime, time + 0.1);
			}
			break;

			case SWITCH:
				// FIXME SOUND: No switches in the game currently?
				Swap(COLLAPSE_DOOR, COLLAPSABLE);
				break;

			case FLOATING_BALL:
			{
				int step=0;
				QueueSound(HHOP_SOUND_FLOATER_ENTER, time);
				renderer.player.Add(new PlayerRender(playerRot, Pos(-30,-30), 0, Pos(-30,-30), 0, dead), time);
				while (tileSolid[GetTile(newpos+d)]==-1)
				{
					step++;

					if (!renderer.Visible(newpos+d))
					{
						TileRender* r = new TileRender(FLOATING_BALL, newpos);
						r->special = 512;
						renderer(newpos).Add(r, time);

						PlayerRender* pr = new PlayerRender(playerRot, newpos, 0, newpos, 0, dead);
						pr->speed = JUMP_TIME*1;
						renderer.player.Add(pr, time);

						time += pr->speed;

						dead = 1;
						break;
					}
					oldpos = newpos;
					newpos = oldpos + d;

					SetTile(oldpos, EMPTY, false);
					SetTile(newpos, FLOATING_BALL, false);

					renderer(oldpos).Add(new TileRotateRender(FLOATING_BALL, oldpos, d, 2), time);
					renderer(oldpos).Add(new TileRender(EMPTY, oldpos), time + ROTATION_TIME/2);
					renderer(newpos).Add(new TileRotateRender(FLOATING_BALL, newpos, (d+3)%MAX_DIR, 3), time + ROTATION_TIME/2);

//					PlayerRender *p = new PlayerRender(playerRot, oldpos, 0, newpos, 0, dead);
//					p->speed = ROTATION_TIME*0.9;
//					renderer.player.Add(p, time);

					endAnimTime = MAX(endAnimTime, time + ROTATION_TIME + ROTATION_TIME/2);
					time += ROTATION_TIME;
					QueueSound(HHOP_SOUND_FLOATER_MOVE, time);
				}
				player = newpos;
//				renderer.player.Add(new PlayerRender(playerRot, player, 0, player, 0, 0), time);
				if (dead)
				{
				}
				else
				{
					TileRender* r = new TileRender(FLOATING_BALL, newpos);
					r->special = playerRot + 256;
					renderer(newpos).Add(r, time);
				}
			}
			break;

			case LIFT_DOWN:
			case LIFT_UP:
			{
				if (GetTile(newpos)==LIFT_UP)
					QueueSound(HHOP_SOUND_LIFT_DOWN, time);
				else
					QueueSound(HHOP_SOUND_LIFT_UP, time);
				SetTile(newpos, GetTile(newpos)==LIFT_UP ? LIFT_DOWN : LIFT_UP, false);
				renderer(newpos).Add(new TileRender(GetTile(newpos), newpos, 1), time);

				PlayerRender *p = new PlayerRender(playerRot, newpos, 1-GetHeight(newpos), newpos, GetHeight(newpos), dead);
				renderer.player.Add(p, time);
				endAnimTime = MAX(endAnimTime, time + JUMP_TIME);
			}
			break;

			case TRAMPOLINE:
				if (d<0) break;
				QueueSound(HHOP_SOUND_TRAMPOLINE, time);

				oldpos = newpos;
				if (Collide(newpos + d, high)) 
					break;
				if (Collide((newpos + d) + d, high) == 1) 
					newpos = (newpos + d);
				else
					newpos = (newpos + d) + d;
				if (tileSolid[GetTile(newpos)] == -1) 
					dead=1;
				//player = newpos;
				goto retry_pos;

			case SPINNER:
			{
				QueueSound(HHOP_SOUND_SPINNER, time);
				for (Dir d=0; d<MAX_DIR; d++)
				{
					Tile tmp = GetTile(newpos + d);
					renderer(newpos + d).Add(new TileRotateRender(tmp, newpos + d, (d+2)%MAX_DIR, false), time);
				}
				Tile tmp = GetTile(newpos + Dir(MAX_DIR-1));
				for (Dir d=0; d<MAX_DIR; d++)
				{
					Tile t2 = GetTile(newpos + d);
					SetTile(newpos + d, tmp, false);
					renderer(newpos + d).Add(new TileRotateRender(tmp, newpos + d, (d+4)%MAX_DIR, true), time + ROTATION_TIME/2);
					if (GetItem(newpos + d))
						renderer(newpos + d,true).Add(new ItemRender(GetItem(newpos + d), GetTile(newpos + d)==EMPTY, newpos+d), time + ROTATION_TIME/2);

					tmp = t2;
				}
				endAnimTime = MAX(endAnimTime, time+ROTATION_TIME);
//				renderer(newpos).Add(new TileRotateRender(SPINNER, Dir(0), 0), time);					
			}
			break;

			case TRAP:
			{
				if (d<0) break;

				if (player_items[0]==0)
				{
					QueueSound(HHOP_SOUND_ICE, time);
					if (tileSolid[GetTile(newpos + d)] == 1) 
						break;
					newpos = newpos + d;
					if (tileSolid[GetTile(newpos)] == -1) 
						dead=1;
					//player = newpos;
					goto retry_pos;
				}
				else
				{
					QueueSound(HHOP_SOUND_USED_ANTIICE, time);
					SetTile(newpos, COLLAPSABLE3);
					player_items[0]--;
				}
			}
			break;

			case GUN:
			{
				QueueSound(HHOP_SOUND_LASER, time);
				FireGun(newpos, d, false, time);

				endAnimTime = MAX(endAnimTime, time);

				if (GetTile(newpos)==EMPTY)
				{
					PlayerRender* pr = new PlayerRender(playerRot, player, 0, player, 0, dead);
					pr->speed = JUMP_TIME*1;
					renderer.player.Add(pr, time);

					time += pr->speed;
					dead = 1;
				}

				/*
				Pos hits[MAX_DIR];
				int numHits=0;

				for (Dir fd=((d<0)?0:d); fd<((d<0)?MAX_DIR:d+1); fd++)
				{
					Pos p = newpos + fd;
					int range = 0;
					for (range; range<MAP_SIZE; range++, p=p+fd)
					{
						Tile t = GetTile(p);
						if (tileSolid[t]!=-1)
						{
							hits[numHits++] = p;
							break;
						}
					}
					
					renderer().Add(new LaserRender(newpos, fd, range), time);
				}

				double _time = time;
				time += 0.25;
				for (int i=0; i<numHits; i++)
				{
					Pos p = hits[i];
					Tile t = GetTile(p);

					renderer().Add(new ExplosionRender(p), time);
					ScoreDestroy(p);
					SetTile(p, EMPTY);

					if (t==GUN)
					{
						for (Dir j=0; j<MAX_DIR; j++)
						{
							ScoreDestroy(p + j);
							SetTile(p + j, EMPTY);
						}
						if (GetTile(newpos)==EMPTY)
							dead = 1;
					}
				}
				endAnimTime = MAX(endAnimTime, time);

				time = _time;
				
				CheckFinished();
*/				
				break;
			}
		}

		endAnimTime = MAX(endAnimTime, time);

		if (dead)
		{
			QueueSound(HHOP_SOUND_DEATH, time);
			win = false;
			
			PlayerRender* pr = new PlayerRender(player, 0, dead);
			pr->speed = 0; // Don't sit around before disappearing!
			renderer.player.Add(pr, time);

			// If the tile we're drowning on isn't visible, give the ownership of the splash effect to the player, rather than a tile.
			if (renderer.Visible(player))
				renderer(player).Add(new ExplosionRender(player, 0, 1), time);
			else
				renderer.player.Add(new ExplosionRender(player, 0, 1), time);

			endAnimTime = MAX(endAnimTime, time+2);
		}
		if (win)
		{
			QueueSound(HHOP_SOUND_WIN, time);
			PlayMusic(HHOP_MUSIC_WIN);
		}

		time = realTime;

		player_score += 1;

		undo[numUndo].endTime = endAnimTime;
		numUndo++;
		
		return true;
	}
	void Update(double timedelta)
	{
		while(deadMenu)
			delete deadMenu;

		if (activeMenu)
		{
			activeMenu->Update(timedelta);
		}
		else
			UpdateKeys();

		for (int i=0; i<SDLK_LAST; i++)
			if (keyState[i])
				keyState[i] = 1;

		if (activeMenu)
			return;

		if (isMap && isRenderMap)
		{
			double min = 50;
			static double scrollHi = 0;
			double x = 0;
#ifndef EDIT
//			if (!noMouse)
			{
				int xx = noMouse ? keyboardp.getScreenX()-scrollX : mousex;
				if (xx > SCREEN_W) xx = SCREEN_W;
				int w = TILE_W2*4;
				if (xx < w)
					x = (double)xx / (w) - 1;
				if (xx > SCREEN_W - w)
					x = 1 - (double)(SCREEN_W-xx) / (w);
				x *= 500;
				if (x<-min || x>min)
				{
					scrollHi += timedelta * x;
					scrollX += (int)scrollHi;
					scrollHi -= (int)scrollHi;
				}
			}
#endif
		}
		if (undoTime>=0 && undoTime < time)
		{
			double acc = (time - undoTime) / 2;
			if (acc < 3) acc = 3;
			time -= timedelta * acc;
			if (undoTime >= time)
				UndoDone();
		}
		else
		{
			time += timedelta;
			if (turboAnim)
				time += timedelta * 20;
			UpdateSound(time);
		}
	}
	void FileDrop(const char* filename)
	{
		LoadSave(filename, false);
	}
	void UpdateKeys()
	{
#ifdef EDIT
		if (keyState[SDLK_LALT] || keyState[SDLK_LCTRL])
			return;
#endif

		if (!isMap && !editMode && undoTime < 0)
		{
			if (keyState[SDLK_b]) 
			{
				Undo();
				return;
			}
		}
		if (isMap && !editMode)
		{

/*			if ((keyState[SDLK_q] | keyState[SDLK_KP7]) & 2) keyboardp.x--;
			else if ((keyState[SDLK_d] | keyState[SDLK_KP3]) & 2) keyboardp.x++;
			else if ((keyState[SDLK_e] | keyState[SDLK_KP9]) & 2) keyboardp.x++, keyboardp.y--;
			else if ((keyState[SDLK_a] | keyState[SDLK_KP1]) & 2) keyboardp.x--, keyboardp.y++;
			else */ if (keyState[SDLK_UP] & 2) keyboardp.y--;
			else if (keyState[SDLK_DOWN] & 2) keyboardp.y++;
			else if (keyState[SDLK_LEFT] & 2) keyboardp.x--, keyboardp.y+=keyboardp.x&1;
			else if (keyState[SDLK_RIGHT] & 2) { if (keyboardp.x < mapRightBound) keyboardp.y-=keyboardp.x&1, keyboardp.x++; }
			else if (keyState[SDLK_a] & 2) 
			{
				// Simulate user clicking on it...
				Mouse(keyboardp.getScreenX()-scrollX, keyboardp.getScreenY()-scrollY, 0, 0, 1, 0, 0);
				noMouse = 1;
				return;
			}
			else
			{
				if (noMouse)
					UpdateCursor(keyboardp);
				return;
			}
			int min[21] = { 17, 16, 15, 14, 13, 13, 13, 13, 13, 13, 12, 11, 11, 13, 12, 11,  8,  8,  7,  6,  7 };
			int max[21] = { 20, 20, 19, 19, 19, 19, 18, 21, 20, 20, 19, 19, 18, 18, 17, 16, 16, 16, 15, 15, 14 };
			if (keyboardp.x < 3) keyboardp.x = 3;
			if (keyboardp.x > mapRightBound) keyboardp.x = mapRightBound;

			if (keyboardp.y < min[keyboardp.x-3]) keyboardp.y = min[keyboardp.x-3];
			if (keyboardp.y > max[keyboardp.x-3]) keyboardp.y = max[keyboardp.x-3];
			noMouse = 1;
			UpdateCursor(keyboardp);
		}
		else if (!editMode && (numUndo==0 || time>=undo[numUndo-1].endTime))
		{
			static int usedDiag = 0;

			if (keyState[SDLK_UP] && keyState[SDLK_LEFT]) HandleKey('q', 0), usedDiag=1;
			else if (keyState[SDLK_UP] && keyState[SDLK_RIGHT]) HandleKey('e', 0), usedDiag=1;
			else if (keyState[SDLK_DOWN] && keyState[SDLK_LEFT]) HandleKey('a', 0), usedDiag=1;
			else if (keyState[SDLK_DOWN] && keyState[SDLK_RIGHT]) HandleKey('d', 0), usedDiag=1;
			else if (keyState[SDLK_UP] && !usedDiag) HandleKey('w', 0);
			else if (keyState[SDLK_DOWN] && !usedDiag) HandleKey('s', 0);

			else usedDiag = 0;
		}
	}
	void KeyReleased(int key)
	{
		keyState[key] = 0;
	}
	bool KeyPressed(int key, int mod)
	{
		keyState[key] = 2;

		if (activeMenu)
		{
			bool eat = activeMenu->KeyPressed(key, mod);
			if (!activeMenu)
				memset(keyState, 0, sizeof(keyState));
			return eat;
		}
		else
		{
			if (key==SDLK_ESCAPE)
			{
				if (mod & KMOD_SHIFT)
				{
					time = 0;
					renderer.Reset();
					LoadSaveProgress(false);
				}

				LoadMap();
			}

			if (isFadeRendering)
				return false;

			return HandleKey(key, mod);
		}
	}
	bool HandleKey(int key, int mod)
	{
		turboAnim = 0;

#ifdef CHEAT
		if (isMap && key=='r' && (mod & KMOD_ALT))
		{
			progress.Clear();
			LoadMap();
		}
#endif

		if (key==SDLK_y && !editMode)
		{
			noMouse = 1;
			new PauseMenu(isMap, progress.GetLevel(STARTING_LEVEL, true)->Completed(), progress.general.endSequence>=1, progress.general.endSequence>=2);
		}

#ifdef EDIT
		else if (key=='e' && (mod & KMOD_ALT)) 
			editMode = !editMode;
		
		else if (key=='p' && (mod & KMOD_ALT) && numUndo>0
		      || key>='0' && key<='9' && (mod & KMOD_SHIFT) && !isMap)
		{
			if (key>='0' && key<='9')
				levelDiff = (key=='0') ? 10 : key-'0';

			if (key=='p' && levelPar==0)
				levelPar = player_score;

			if (numUndo)
			{
				do 
					undo[numUndo-1].Restore(this);
				while (--numUndo);
			}
			time = 0;
			if (LoadSave(currentFile, true))
			{
				if (key>='0' && key<='9')
					LoadMap();
			}
		}
#endif

		/////////////////////////////////////////////////////////////////////////
		if (isMap && !editMode)
			return false;

		else if (key==SDLK_KP9 || key=='e') Input(1), noMouse=1;
		else if (key==SDLK_KP3 || key=='d') Input(2), noMouse=1;
		else if (key==SDLK_KP1 || key=='a') Input(4), noMouse=1;
		else if (key==SDLK_KP7 || key=='q') Input(5), noMouse=1;
		else if (key==SDLK_KP8 || key=='w') Input(0), noMouse=1;
		else if (key==SDLK_KP2 || (key=='s' && (((mod & (KMOD_CTRL|KMOD_ALT))==0)||!editMode))) Input(3), noMouse=1;
		else if (key==SDLK_KP5 || key==SDLK_SPACE || key==SDLK_RETURN || key==SDLK_KP_ENTER || key==SDLK_LCTRL)
		{
			noMouse=1;
			if (win && winFinal)
				LoadMap(), memset(keyState, 0, sizeof(keyState));
			else
				Input(-1);
		}

		else if (key=='r' && (mod & KMOD_CTRL))
			LoadSave(currentFile, false);

#ifdef EDIT
		else if (key=='z' && (mod & KMOD_ALT)) 
		{
			if (numUndo>0 && !isMap)
			{
				time = undo[numUndo-1].endTime;
				undoTime = undo[0].time;
				
				do 
					undo[numUndo-1].Restore(this);
				while (--numUndo);
			}
		}
#endif
		else if (key=='z' || key==SDLK_BACKSPACE || key==SDLK_DELETE || key=='u' || key==SDLK_LALT) 
		{
			if (!isMap)
				Undo();
		}

#ifdef EDIT
		else if (key=='s' && (mod & KMOD_ALT)){
			if (win && strlen(currentFile)>0 && !isMap)
			{
				char tmp[1000];
				strcpy(tmp, currentFile);
				ChangeSuffix(tmp, "sol");
				FILE* f = file_open(tmp, "wb");
				if (f)
				{
					for (int i=0; i<numUndo; i++)
					{
						fputc(undo[i].playerMovement, f);
					}
					fclose(f);
				}
			}
		}
#endif

#ifdef CHEAT
		else if (key=='/' && (mod & KMOD_ALT)){
			turboAnim = 1;
			if (!isMap)
			{
				while (numUndo)
					Undo();
				ResetLevel();

				if (mod & KMOD_SHIFT)
				{
					LevelSave* l = progress.GetLevel(currentFile, false);
					if (l && l->Completed())
					{
						for (int i=0; i<l->bestSolutionLength; i++)
							Input(l->bestSolution[i]);
						time = 0;
					}
					if (!win && l)
						l->Clear();
				}
				else
				{
					char tmp[1000];
					strcpy(tmp, currentFile);
					ChangeSuffix(tmp, "sol");
					FILE* f = file_open(tmp, "rb");
					if (f)
					{
						int dir;
						while ((dir = fgetc(f)) != -1)
						{
							if (dir==0xff)
								dir = -1;
							Input(dir);
						}
						time = 0;
						fclose(f);

						if (!win)
							remove(tmp);
					}
				}
			}
		}
#endif

#ifdef EDIT
		else if (!editMode)
			return false;
		
		else if (key>='0' && key<='9' && (mod & KMOD_ALT) && !isMap)
			levelPar = levelPar*10 + key-'0';
		else if (key==SDLK_BACKSPACE  && (mod & KMOD_ALT) && !isMap)
			levelPar /= 10;

		else if (key=='i')
			Mouse(mousex, mousey, 0, 0, 32, 0, mouse_buttons);
		else if (key=='p' && !(mod & KMOD_ALT))
			Mouse(mousex, mousey, 0, 0, 64, 0, mouse_buttons);
		else if (key=='x')
			Mouse(mousex, mousey, 0, 0, 128, 0, mouse_buttons);
		else if (key==SDLK_RETURN || key==SDLK_LCTRL)
			Mouse(mousex, mousey, 0, 0, 256, 0, mouse_buttons);
		else if (key==SDLK_BACKSPACE)
			Mouse(mousex, mousey, 0, 0, 512, 0, mouse_buttons);
		else if (key=='c')
			Mouse(mousex, mousey, 0, 0, 1024, 0, mouse_buttons);

		else if (key=='s' && (mod & KMOD_CTRL)){
			char *fn = LoadSaveDialog(true, true, _("Save level"));
			LoadSave(fn, true);
			SDL_WM_SetCaption(currentFile, NULL);
		}
		
		else if (key=='o' && (mod & KMOD_CTRL)){
			char* fn = LoadSaveDialog(false, true, _("Open level"));
			LoadSave(fn, false);
			SDL_WM_SetCaption(currentFile, NULL);
		}
#endif

		else 
			return false;

		return true;
	}
	void LoadGraphics()
	{
		#define X(NAME,FILE,ALPHA) NAME = Load(String(FILE) + BMP_SUFFIX, ALPHA);
		#include "gfx_list.h"

		static int first = 1;
		if (first)
		{
			first = false;
			MakeTileInfo();
		}

	//	unsigned int d = {

	//	};
	//	static SDL_Cursor * c = SDL_CreateCursor(data, mask, 32, 32, 1, 1);
	//	SDL_SetCursor(c);
		//SDL_ShowCursor(1);
	}
	void FreeGraphics()
	{
		#define X(NAME,FILE,ALPHA) if (NAME) SDL_FreeSurface(NAME), NAME=0;
		#include "gfx_list.h"
	}
	virtual void ScreenModeChanged() 
	{
//		FreeGraphics();
//		LoadGraphics();
	}
};

MAKE_STATE(HexPuzzle, SDLK_F1, false);

char * HexPuzzle::loadPtr = 0;
char * HexPuzzle::endLoad = 0;

#endif //USE_OPENGL
