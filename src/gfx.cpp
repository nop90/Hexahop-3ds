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

#include "i18n.h"

#include "state.h"
#include "sfx.h"
#include "text.h"
#include "system-relative.h"
#include "3ds.h"
#include <cassert>
#include <sys/stat.h>

#undef USE_BBTABLET

#include <algorithm>
#include <string>

StateMakerBase* StateMakerBase::first = 0;
State* StateMakerBase::current = 0;

int SDL_focus = SDL_APPACTIVE | SDL_APPINPUTFOCUS;	// Initial focus state



char* LoadSaveDialog(bool /*save*/, bool /*levels*/, const char * /*title*/)
{
	return 0;
}

extern void test();

int mouse_buttons = 0;
int mousex= 10, mousey = 10;
int noMouse = 1;
int quitting = 0;

double stylusx= 0, stylusy= 0;
int stylusok= 0;
float styluspressure = 0;
SDL_Surface * screen = 0;
SDL_Surface * realScreen = 0;

extern State* MakeWorld();

bool fullscreen = true;

void ScrFlip(void){
	SDL_Flip(realScreen);
}

void InitScreen()
{
#ifdef USE_OPENGL
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

//	printf("SDL_SetVideoMode (OpenGL)\n");
	realScreen = SDL_SetVideoMode(
		SCREEN_W, SCREEN_H, // Width, Height
		0, // Current BPP
		SDL_OPENGL | (fullscreen ? SDL_FULLSCREEN : 0) );
#else
//	printf("SDL_SetVideoMode (non-OpenGL)\n");
	realScreen = SDL_SetVideoMode(
		SCREEN_W, SCREEN_H, // Width, Height
		16, // Current BPP
//		SDL_HWSURFACE | (fullscreen ? SDL_FULLSCREEN : 0));
		SDL_HWSURFACE | (fullscreen ? SDL_FULLSCREEN : 0) | SDL_TOPSCR | SDL_CONSOLEBOTTOM);
#endif

/*
	if (screen)
		SDL_FreeSurface(screen);

	SDL_Surface* tempscreen = SDL_CreateRGBSurface(
		SDL_HWSURFACE, 
		SCREEN_W, SCREEN_H,
		16, 0xf800, 0x07e0, 0x001f, 0);

	screen = SDL_DisplayFormat(tempscreen);
	SDL_FreeSurface(tempscreen);
*/ 
	screen = realScreen;	
}

void ToggleFullscreen()
{
	fullscreen = !fullscreen;
	InitScreen();
	StateMakerBase::current->ScreenModeChanged();
}
String base_path;

int TickTimer()
{
	static int time = SDL_GetTicks();
	int cap=40;

	int x = SDL_GetTicks() - time;
	time += x;
	if (x<0) x = 0, time = SDL_GetTicks();
	if (x>cap) x = cap;

	return x;
}

String GetBasePath()
{
	String base_path;

	base_path =  "romfs:/";

	return base_path;	
}

int main(int /*argc*/, char * /*argv*/[])
{
	mkdir("/3ds", 0777);
	mkdir("/3ds/Hexahop", 0777);
	osSetSpeedupEnable(true);
	romfsInit();
	base_path = GetBasePath();

/*
	// Experimental - create a splash screen window whilst loading
	SDL_Init(SDL_INIT_VIDEO);
	screen = SDL_SetVideoMode( 200,200,0,SDL_NOFRAME );
	SDL_Rect r = {0,0,200,200};
	SDL_FillRect(screen, &r, SDL_MapRGB(screen->format, 0, 0, 50));
	SDL_Flip(screen);
*/

	SDL_Init(SDL_INIT_VIDEO);
	SDL_ShowCursor(0);
	if (!TextInit(base_path))
		return 1;

	SDL_N3DSKeyBind(KEY_CPAD_UP|KEY_CSTICK_UP, SDLK_UP);
	SDL_N3DSKeyBind(KEY_CPAD_DOWN|KEY_CSTICK_DOWN, SDLK_DOWN);
	SDL_N3DSKeyBind(KEY_CPAD_LEFT|KEY_CSTICK_LEFT, SDLK_LEFT);
	SDL_N3DSKeyBind(KEY_CPAD_RIGHT|KEY_CSTICK_RIGHT, SDLK_RIGHT);

	InitScreen();
	printf("Loading resources. Please wait.\n");
 	InitSound(base_path);

	int videoExposed = 1;

#ifdef USE_BBTABLET
	bbTabletDevice &td = bbTabletDevice::getInstance( );
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
#endif
	
videoExposed = 1;

//SDL_Flip(realScreen); //nop90: flipscreen to avoid citra emulator freezing

	StateMakerBase::GetNew();

printf("\e[2J\e[H");
	while(!quitting)
	{
		SDL_Event e;
		UpdateSound(-1.0);
		while(!SDL_PollEvent(&e) && !quitting)
		{
			int x = 0;

			if ((SDL_focus & 6)==6)
			{
				videoExposed = 1;
				x = TickTimer();

				while (x<10)
				{
					SDL_Delay(10-x);
					x += TickTimer();
				}
				StateMakerBase::current->Update(x / 1000.0);
			}
/*			else
			{
				// Not focussed. Try not to eat too much CPU!
				SDL_Delay(150);
			}
*/			// experimental...
			if (!noMouse)
				StateMakerBase::current->Mouse(mousex, mousey, 0, 0, 0, 0, mouse_buttons);

			if (videoExposed)
			{

				StateMakerBase::current->Render();

				#ifdef USE_OPENGL
					SDL_GL_SwapBuffers();
				#else
/*					if (screen && realScreen!=screen)
					{
						SDL_Rect r = {0,0,SCREEN_W,SCREEN_H};
						SDL_BlitSurface(screen, &r, realScreen, &r);
					}
*/
					SDL_Flip(realScreen);
				#endif
//				videoExposed = 0;
			}

//			SDL_Delay(10);

#ifdef USE_BBTABLET
			// Tablet ////////////////////////
			bbTabletEvent evt;
			while(hwnd!=NULL && td.getNextEvent(evt))
			{
				stylusok = 1;
				RECT r;
				if (tablet_system)
				{
					GetWindowRect(hwnd, &r);
					stylusx = evt.x * GetSystemMetrics(SM_CXSCREEN);
					stylusy = (1.0 - evt.y) * GetSystemMetrics(SM_CYSCREEN);
					stylusx -= (r.left + GetSystemMetrics(SM_CXFIXEDFRAME));
					stylusy -= (r.top + GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION));;
				}
				else
				{
					GetClientRect(hwnd, &r);
					stylusx = evt.x * r.right;
					stylusy = (1.0 - evt.y) * r.bottom;
				}
				styluspressure = (evt.buttons & 1) ? evt.pressure : 0;
 
				/*
				printf("id=%d csrtype=%d b=%x (%0.3f, %0.3f, %0.3f) p=%0.3f tp=%0.3f\n", 
					   evt.id,
					   evt.type,
					   evt.buttons,
					   evt.x,
					   evt.y,
					   evt.z,
					   evt.pressure,
					   evt.tpressure
					   );
				*/
			}

#endif
		}

		switch (e.type)
		{
			case SDL_VIDEOEXPOSE:
				videoExposed = 1;
				break;

			case SDL_ACTIVEEVENT:
			{
				int gain = e.active.gain ? e.active.state : 0;
				int loss = e.active.gain ? 0 : e.active.state;
				SDL_focus = (SDL_focus | gain) & ~loss;
				if (gain & SDL_APPACTIVE)
					StateMakerBase::current->ScreenModeChanged();
				if (loss & SDL_APPMOUSEFOCUS)
					noMouse = 1;
				else if (gain & SDL_APPMOUSEFOCUS)
					noMouse = 0;

				break;
			}

			case SDL_MOUSEMOTION:
				noMouse = false;
				StateMakerBase::current->Mouse(e.motion.x, e.motion.y, e.motion.x-mousex, e.motion.y-mousey, 0, 0, mouse_buttons);
				mousex = e.motion.x; mousey = e.motion.y;
				break;
			case SDL_MOUSEBUTTONUP:
				noMouse = false;
				mouse_buttons &= ~(1<<(e.button.button-1));
				StateMakerBase::current->Mouse(e.button.x, e.button.y, e.button.x-mousex, e.button.y-mousey, 
										0, 1<<(e.button.button-1), mouse_buttons);
				mousex = e.button.x; mousey = e.button.y ;
				break;
			case SDL_MOUSEBUTTONDOWN:
				noMouse = false;
				mouse_buttons |= 1<<(e.button.button-1);
				StateMakerBase::current->Mouse(e.button.x, e.button.y, e.button.x-mousex, e.button.y-mousey, 
										1<<(e.button.button-1), 0, mouse_buttons);
				mousex = e.button.x; mousey = e.button.y ;
				break;

			case SDL_KEYUP:
				StateMakerBase::current->KeyReleased(e.key.keysym.sym);
				break;

			case SDL_KEYDOWN:
			{
				SDL_KeyboardEvent & k = e.key;

				if (k.keysym.sym==SDLK_ESCAPE)
				{
					quitting = 1;
				}
				else if (k.keysym.sym==SDLK_x)//SDLK_F12)	
				{
					// Toggle system pointer controlled by tablet or not
					#ifdef USE_BBTABLET
						if (td.isValid())
						{
							tablet_system = !tablet_system;
							td.setPointerMode(tablet_system ? bbTabletDevice::SYSTEM_POINTER : bbTabletDevice::SEPARATE_POINTER);
						}
					#endif
				}
				else if (k.keysym.sym==SDLK_RETURN)
				{
					/*ToggleFullscreen();*/
				}
				else if (StateMakerBase::current->KeyPressed(k.keysym.sym, k.keysym.mod))
				{
				}
				else if ((k.keysym.mod & (KMOD_ALT | KMOD_CTRL))==0)
				{
					StateMakerBase::GetNew(k.keysym.sym);
				}
			}
			break;

			case SDL_QUIT:
				quitting = 1;
				break;
		}
	}

	TextFree();
	FreeSound();
	romfsExit();
	SDL_Quit();
	return 0;
}
