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

#include <string>

struct Menu;
Menu* activeMenu = 0;
Menu* deadMenu = 0;

static void HackKeyPress(int key, int mod)
{
	int k = keyState[key];
	StateMakerBase::current->KeyPressed(key, mod);
	keyState[key] = k;
}

struct Menu {
	bool renderBG;

	Menu() : renderBG(true), under(activeMenu), time(0) { activeMenu = this; }
	
	virtual ~Menu() { 
		if(this!=deadMenu) FATAL("this!=deadMenu"); 
		deadMenu=under; 
	}

	static void Pop()
	{
		if (!activeMenu) return;
		Menu* m = activeMenu;
		activeMenu = activeMenu->under;
		m->under = deadMenu;
		deadMenu = m;
	}

	virtual bool KeyPressed(int key, int /*mod*/)
	{
		if (key==SDLK_UP){
			PlaySound(HHOP_SOUND_UI_MENU);
			Move(-1), noMouse=1;
		}
		else if (key==SDLK_DOWN){
			PlaySound(HHOP_SOUND_UI_MENU);
			Move(1), noMouse=1;
		}
		else if (key==SDLK_a)
		{
			PlaySound(HHOP_SOUND_UI_MENU);
			Select();
			noMouse=1;
		}
		else if (key==SDLK_b){
			PlaySound(HHOP_SOUND_UI_MENU);
			Cancel();
		}
		else
			return false;
		return true;
	}
	virtual void Mouse(int /*x*/, int /*y*/, int /*dx*/, int /*dy*/, int buttons_pressed, int /*buttons_released*/, int /*buttons*/) 
	{
		if (buttons_pressed==4 || buttons_pressed==2)
			Cancel();
	}
	
	virtual void Move(int /*dir*/) {}
	virtual void Select() {}
	virtual void Cancel() {}

	virtual void Update(double timedelta) {time+=timedelta;}
	virtual void Render() = 0;

	Menu * under;
	double time;
};

const char * hint[] = {
	
/*EMPTY*/
//_("Basic controls:|Move around with the keys Q,W,E,A,S,D or the numeric  keypad. Alternatively, you can use the mouse and  click on the tile you'd like to move to.    Use 'U', backspace or the right mouse button to  undo mistakes.    The 'Esc' key (or middle mouse button) brings up a  menu from which you can restart if you get stuck."),
_("Basic controls:|Move around with the DPAD.  Press the B button to undo mistakes.    The Select button brings up a  menu from which you can restart if you get stuck."),


/*NORMAL*/
0,
/*COLLAPSABLE*/
_("Objective:|Your goal is to break all the green tiles.    You mainly do this by jumping on them.    They will crack when you land on them, and  only disintegrate when you jump off.    Try not to trap yourself!"), 
/*COLLAPSE_DOOR*/
_("The coloured walls flatten themselves when there  are no matching coloured tiles remaining."),
/*TRAMPOLINE*/
_("You can bounce on the purple trampoline tiles to  get around.    But try not to fall in the water.    If you do, remember you can undo with 'U',  backspace or the right mouse button!"),
/*SPINNER*/
_("A red spinner tile will rotate the pieces around  it when you step on it."),
/*WALL*/
0,
/*COLLAPSABLE2*/
_("You don't need to destroy blue tiles to complete  the level.    But they'll turn green when you step off them, and  you know what you have to do to green tiles..."),
/*COLLAPSE_DOOR2*/
0,
/*GUN*/
_("Yellow laser tiles fire when you step on them.    Shooting other laser tiles is more destructive."),
/*TRAP*/
_("Ice is slippery!    Please be careful!!"),
/*COLLAPSABLE3*/
0,
/*BUILDER*/
_("The dark grey tiles with arrows on are builders.    Landing on one creates green tiles in any adjacent  empty tile, and turns green tiles into walls."),
/*SWITCH*/
0,
/*FLOATING_BALL*/
/* TRANSLATORS: pop means vanish and Emy drowns (you loose) */
_("You can ride on the pink floating boats to get  across water.    They'll pop if you try and float off the edge of the  screen though, so look where you're going."),
/*LIFT_DOWN*/
_("The blue lifts go up or down when you land on them."),
/*LIFT_UP*/
0,

0,0,0,0,
//Item 0 (21)
_("The spiky anti-ice pickups turn icy tiles into blue ones.    They get used automatically when you land on ice."),
//Item 1 (22)
/* TRANSLATORS: Normally you jump from one plate to another. The golden jump (a
   pickup) allows you to jump and land on the *same* plate */
_("Collecting the golden jump pickups will allow you to  do a big vertical jump.    Try it out on different types of tiles.    Use the space bar or return key to jump. Or click  on the tile you're currently on with the mouse."),

0,0,

// Map (25)
_("Map Screen:|You can choose which level to attempt next from  the map screen.    Silver levels are ones you've cleared.    Black levels are ones you haven't completed yet,  but are available to play."),

// Scoring (26)
/* TRANSLATORS: Levels are depicted as black balls. Once you passed them, they
   turn silver. If you reached the par, they turn golden (with a crown), and if
   you beat the par, they turn their shape/color once more */
_("New feature unlocked!|Each level has an efficiency target for you to try  and beat.    Every move you make and each non-green tile  you destroy counts against you.    Why not try replaying some levels and going  for gold?"),

0,0,0,

// End of help (30)
//_("Thanks for playing this little game.    I hope you  enjoy it!    -- --    All content is Copyright 2005 Tom Beaumont    email: tombeaumont@yahoo.com  Any constructive criticism gratefully received!"),
_("The red Iwatas sued their own fans...    Still pissed off about it ?    Why don't you watch some child porn instead ?    At least you won't get a DMCA for that.      Gameblabla      ==========    Copyright 2005 Tom Beaumont"),

// First help page (31)
/* TRANSLATORS: This string is copied twice into the POT file to workaround a
   gettext limitation (no macro expansion). The extracted string "Welcome to "
   will not be used. */
_("Welcome to " GAMENAME "!    This is a puzzle game based on hexagonal tiles.  There is no time limit and no real-time element, so  take as long as you like.    Use the cursor keys or click on the arrows to  scroll through the help pages. More pages will be  added as you progress through the game."),
};

struct HintMessage : public Menu
{
	static int flags;

	SDL_Rect InnerTextWindowRect;
	SDL_Rect OuterTextWindowRect;
	const char * msg;
	char title[500];
	int state;

	const SDL_Rect &GetInnerWindowRect() const
	{
		return InnerTextWindowRect;
	}

	const SDL_Rect &GetOuterWindowRect() const
	{
		return OuterTextWindowRect;
	}

	static bool FlagTile(int t, bool newStuff=true)
	{
		if (t==LIFT_UP) t=LIFT_DOWN;
		if (newStuff && (flags & (1<<t))) return false;
		if (!newStuff && !(flags & (1<<t))) return false;
		if (t>31) return false;
		if (!hint[t]) return false;

		flags |= 1<<t;
		new HintMessage(hint[t]);
		return true;
	}

	HintMessage(const char * m) { Init(m); }

	void Init(const char * m)
	{
		state = 0; time = 0;
		memset(title, 0, sizeof(title));
		const char * x = strstr(m, "|");
		if (!x)
		{
			msg = m;
			strcpy(title, _("Info:"));
		}
		else
		{
			strncpy(title, m, x-m);
			msg=x+1;
		}

		char msg_utf8[5000];
		ConvertToUTF8(msg, msg_utf8, sizeof(msg_utf8)/sizeof(char));

		std::string text(msg_utf8);
		while (text.find("  ") != std::string::npos)
			text.replace(text.find("  "), 2, "\n");

		InnerTextWindowRect.w = SCREEN_W-TILE_W1*2; 
		InnerTextWindowRect.h = TextHeight(text, InnerTextWindowRect.w - 2*FONT_SPACING);
		InnerTextWindowRect.h += FONT_SPACING;
		OuterTextWindowRect = InnerTextWindowRect;
		OuterTextWindowRect.w += 4;
		OuterTextWindowRect.h += FONT_SPACING+4;
	}

	virtual void Render()
	{
		int y = SCREEN_H/4 + int(SCREEN_H*MAX(1-time*5, 0)*3/4);
		if (state)
			y = SCREEN_H/4 + int(SCREEN_H*(-time*5)*3/4);

		Render(0, y);
		
		if (!state && time>0.2)
			PrintR(SCREEN_W-FONT_SPACING, SCREEN_H-FONT_SPACING, _("Press any key"));
	}

	void Render(int x, int y)
	{
	printf("\n");
		//if (y<0) {
		//  std::cout << "Error in Render: " << x << " " << y << "\n"; // CHECKME
		//  y = 0;
		//}
		InnerTextWindowRect.x = x+TILE_W1;
		InnerTextWindowRect.y = y;
		OuterTextWindowRect.x = InnerTextWindowRect.x-2;
		OuterTextWindowRect.y = InnerTextWindowRect.y-2-FONT_SPACING;
		// Height is reduced in SDL_FillRect!!? Why? ==> Use a copy:
		SDL_Rect r2 = InnerTextWindowRect;
		SDL_Rect r = OuterTextWindowRect;
		SDL_FillRect(screen, &r, SDL_MapRGB(screen->format, 60,90,90));
		SDL_FillRect(screen, &r2, SDL_MapRGB(screen->format, 20,50,50));
		Print(OuterTextWindowRect.x+FONT_SPACING/4, y-FONT_SPACING, "%s", title);
		/* TRANSLATORS: This specifies how the text in the help dialog should
		   be aligned. Do *not* translate the text itself but use one of "left",
       "center" or "right" (untranslated!). The default is "center". */
		std::string alignment = _("text alignment");
		if (alignment == "right")
			Print_Aligned(true, InnerTextWindowRect.x + InnerTextWindowRect.w - FONT_SPACING, y+FONT_SPACING/2, InnerTextWindowRect.w - 2*FONT_SPACING, msg, 2);
		else if (alignment == "left")
			Print_Aligned(true, InnerTextWindowRect.x + FONT_SPACING, y+FONT_SPACING/2, InnerTextWindowRect.w - 2*FONT_SPACING, msg, 0);
		else
			Print_Aligned(true, x+SCREEN_W/2, y+FONT_SPACING/2, InnerTextWindowRect.w - 2*FONT_SPACING, msg, 1);
	}

	virtual void Mouse(int /*x*/, int /*y*/, int /*dx*/, int /*dy*/, int buttons_pressed, int /*buttons_released*/, int /*buttons*/) 
	{
		if (buttons_pressed && state==0 && time>0.2)
			state = 1, time=0;
	}

	bool KeyPressed(int /*key*/, int /*mod*/)
	{
		if (state==0 && time>0.2)
			state = 1, time=0;
		return true;
	}

	virtual void Update(double timedelta)
	{
		Menu::Update(timedelta);
		if(state && time > 0.2)
			Pop();
	}
};

int HintMessage::flags = 1<<31 | 1<<30;

struct HintReview : public HintMessage
{
	int page;
	int page_dir;
	int page_count;
	int page_display;
	HintReview() : HintMessage(hint[31]), page(31), page_dir(0), page_display(0)
	{
		page_count=0;
		for (int i=0; i<32; i++)
			if (flags & (1<<i))
				page_count++;
	}

	void Cancel() { Pop(); }
	void Select() { Pop(); }


	virtual void Mouse(int x, int y, int dx, int dy, int buttons_pressed, int buttons_released, int buttons) 
	{
		if (buttons_pressed==1)
		{
			if (y < SCREEN_H/4-FONT_SPACING+2)
				Move(-1);
			else if (y > HintMessage::GetOuterWindowRect().y+
					HintMessage::GetOuterWindowRect().h)
				Move(1);
			else
				Cancel();
		}
		else if (buttons_pressed==8)
			Move(-1);
		else if (buttons_pressed==16)
			Move(1);
		else
			Menu::Mouse(x,y,dx,dy,buttons_pressed, buttons_released, buttons);	
	}

	bool KeyPressed(int key, int mod)
	{
		if (key==SDLK_LEFT)
			Move(-1);
		else if (key==SDLK_RIGHT)
			Move(1);
		else
			return Menu::KeyPressed(key, mod);
		return true;
	}

	virtual void Render()
	{
		const double SPD = 10;

#ifdef EDIT
		sprintf (title, _("Help (Page --)"), page_display+1, page_count);
#else
		sprintf (title, _("Help (Page %d/%d)"), page_display+1, page_count);
#endif

		int y=SCREEN_H/4;
		if (state==0)
			y = SCREEN_H/4+int(MAX(0, time*SPD)*-page_dir*SCREEN_H);
		if (state==1)
			y = SCREEN_H/4+int(MAX(0, 1-time*SPD)*page_dir*SCREEN_H);

		//if (!noMouse)
		{
			PrintC(false, SCREEN_W/2, y-FONT_SPACING*2, "^");
			PrintC(false, SCREEN_W/2, HintMessage::GetOuterWindowRect().y+
					HintMessage::GetOuterWindowRect().h, "_");
		}

		HintMessage::Render(0,y);

		if (time > 1.0/SPD && page_dir && state==0)
		{
			do {
				page = (page+page_dir) & 31;
#ifdef EDIT
				if (hint[page]) break;
#endif
			} while (hint[page]==0 || !(flags&(1<<page)));
			Init(hint[page]);

			page_display = (page_display + page_count + page_dir) % page_count;

			time = 0;
			state=1;
		}
		if (time>1.0/SPD && state==1)
			state=0, page_dir = 0;
	}
	virtual void Update(double timedelta)
	{
		Menu::Update(timedelta);
	}
	void Move(int dir)
	{
		if (page_dir)
			return;
		time = 0;
		page_dir = dir;
		state = 0;
	}
};

#define MAX_GAMESLOT 4
enum option {
	OPT_GAMESLOT_0,
	OPT_GAMESLOT_LAST = OPT_GAMESLOT_0 + MAX_GAMESLOT - 1,
	OPT_RESUME,
	OPT_RESTART,
	OPT_GOTO_MAP,
	OPT_GOTO_MAP_CONTINUE,
	/*PT_FULLSCREEN,*/
#ifndef DISABLE_SOUND
	OPT_MUSIC,
	OPT_EFFECTS,
#endif
	OPT_OPTIONS,
	OPT_QUIT,
	OPT_QUIT_CONFIRM,
	OPT_QUIT_CANCEL,
	OPT_QUIT_MENU_CONFIRM,
	OPT_HELP,
	OPT_GAMESLOT_NEW,
	OPT_DELETE_CONFIRM,
	OPT_DELETE_CANCEL,
	OPT_UNDO,
	OPT_BACK,
	OPT_END, OPT_END2,
};

char optionSlotName[MAX_GAMESLOT][400] = { {0} };
char currentSlot[800] = "";
int freeSlot = -1;
char* GetSlotName(int i, char * t)
{
	 sprintf(t, "save%d.dat", i+1);
	 return t;
}

const char * optionString[] = {
	optionSlotName[0],
	optionSlotName[1],
	optionSlotName[2],
	optionSlotName[3],
	_("Resume"),
	_("Restart Level"),
	_("Return to Map"),
	_("Continue"),
	/*_("Toggle Fullscreen"),*/
#ifndef DISABLE_SOUND
	_("Toggle Music"),
	_("Toggle Effects"),
#endif
	_("Options"),
	_("Quit"),
	_("Yes"),
	_("No"),
	_("Return to Title"),
	_("Help"),
	_("Start New Game"),
	_("Yes, really delete it!"),
	_("Don't do it!"),
	_("Undo Last Move"),
	_("OK"),
	_("View Credits Sequence"), _("View Credits Sequence"),
};


struct OptMenu : public Menu
{
	int select;
	int num_opt;
	int opt[10];
	bool left_align;
	const char * title;
	SDL_Rect r, r2;

	OptMenu(const char * t) : select(0), title(t)
	{
		left_align = false;
		num_opt = 0;
	}

	void Init()
	{
		r.w=SCREEN_W/2;
		r.x=(SCREEN_W-r.w)/2;
		r.y=SCREEN_H/3;
		
		r2 = r;
		
		const int SPACE = int(FONT_SPACING * 1.5);

		r2.h = SPACE*num_opt + FONT_SPACING/2;
		r.h = r2.h + (FONT_SPACING+2*2);
		r.y -= FONT_SPACING+2;
		r.w += 2*2;
		r.x -= 2;
	}

	void RenderOption(int o, const char * s)
	{
		int y = r2.y + FONT_SPACING/2 + int(FONT_SPACING * 1.5) * o;
		if (left_align)
		{
			int x = r.x + TextWidth(" ");
			int x1 = x + TextWidth("> ");
			if (select==o)
			{
				//x += int( sin(time*9)*2.5 );
				//y += int( sin(time*9 + 1.5)*1.5 );
				Print(x, y, "> %s", s);
			}
			else
			{
				Print(x1, y, "%s", s);
			}
		}
		else
		{
			int x = r.x + r.w/2;
			if (select==o)
			{
				//x += int( sin(time*9)*2.5 );
				//y += int( sin(time*9 + 1.5)*1.5 );
				PrintC(false, x, y, "> %s <", s);
			}
			else
			{
				PrintC(false, x, y, "%s", s);
			}
		}
	}
	
	void Move(int dir)
	{
		select += dir;
		if (select<0) select = num_opt-1;
		if (select>=num_opt) select = 0;
	}
	virtual void Mouse(int x, int y, int dx, int dy, int buttons_pressed, int buttons_released, int buttons) 
	{
		if (1)
		{
			if (x<r2.x || y<r2.y || x>r2.x+r2.w || y>r2.y+r2.h)
			{
				if (buttons_pressed!=4 && buttons_pressed!=2)
					return;
			}
			else
			{
				select = (y-r2.y-FONT_SPACING/3) / int(FONT_SPACING*1.5);
				if (select<0) select = 0;
				if (select>=num_opt) select = num_opt-1;
			}
		}
		if (buttons_pressed==1)
			Select();
		Menu::Mouse(x, y, dx, dy, buttons_pressed, buttons_released, buttons);
	}
	void Select();

	void Render()
	{
		RenderBG();
		RenderTitle();
		RenderOptions();
	}
	void RenderBG()
	{
		SDL_FillRect(screen, &r, SDL_MapRGB(screen->format, 60,90,90));
		SDL_FillRect(screen, &r2, SDL_MapRGB(screen->format, 20,50,50));
	}
	void RenderTitle()
	{
		if (left_align)
			Print(r2.x+TextWidth(" "), r.y+4, title);
		else
			PrintC(false, r2.x+r2.w/2, r.y+4, title);
	}
	void RenderOptions()
	{
		for (int i=0; i<num_opt; i++)
			RenderOption(i, optionString[opt[i]]);
	}
	void Cancel()
	{
		Pop();
	}
};

struct WinLoseScreen : public OptMenu
{
	bool win;
	int score, par, best_score;
	WinLoseScreen(bool _win, int _score=0, int _par=0, int _prev_score=0) : 
		OptMenu(_win ? _("Level Complete!") : _("Emi can't swim...")),
		win(_win),
		score(_score),
		par(_par),
		best_score(_prev_score)
	{
		if (!win)
			opt[num_opt++] = OPT_UNDO;
		if (!win)
			opt[num_opt++] = OPT_RESTART;
		opt[num_opt++] = win ? OPT_GOTO_MAP_CONTINUE : OPT_GOTO_MAP;

		Init();

		if (win)
		{
			r.h += FONT_SPACING * 3 + FONT_SPACING/2;
			r2.h += FONT_SPACING * 3 + FONT_SPACING/2;
		}
	}

	void Render()
	{
		OptMenu::RenderBG();
		OptMenu::RenderTitle();

		if (win)
			r2.y += FONT_SPACING * 3 + FONT_SPACING/2;
		
		OptMenu::RenderOptions();
		
		if (win)
			r2.y -= FONT_SPACING * 3 + FONT_SPACING/2;

		if (win)
		{
			int x = r.x+r.w/2;
			int y = r2.y + FONT_SPACING/2;
			if (score < best_score && score <= par)
				PrintC(true, x, y, _("New Best Score: %d  Par Score: %d  Par Beaten!"), score, par);
			else if (score < best_score)
				PrintC(true, x, y, _("New Best Score: %d  Par Score: %d"), score, par);
			else if (par && best_score)
				PrintC(true, x, y, _("Score: %d  Previous Best: %d  Par Score: %d"), score, best_score, par);	
			else
				PrintC(true, x, y+FONT_SPACING/2, _("Well Done!  Level Completed!"));
		}
	}

	static void Undo()
	{
		Pop();
		HackKeyPress('z', 0);
	}
	bool KeyPressed(int key, int mod)
	{
		if (key=='z' || key=='u' || key==SDLK_DELETE || key==SDLK_BACKSPACE || key==SDLK_LALT)
			return Undo(), true;
		if (key=='r' && (mod & KMOD_CTRL))
		{
			Pop();
			HackKeyPress(key, mod);
			return true;
		}
		return OptMenu::KeyPressed(key, mod);
	}
	virtual void Mouse(int x, int y, int dx, int dy, int buttons_pressed, int buttons_released, int buttons) 
	{
		if (buttons_pressed==4)
		{
			Undo();
			return;
		}
		
		OptMenu::Mouse(x, y, dx, dy, buttons_pressed, buttons_released, buttons);
	}
	void Cancel()
	{
		if (win)
			select=0, Select();
		else
			Undo();
	}
};

struct OptMenuTitle : public OptMenu
{
	OptMenuTitle(const char * t) : OptMenu(t)
	{
		renderBG = false;
		//left_align = true;
	}

	void Render()
	{
		SDL_Rect a = {0,0,SCREEN_W,SCREEN_H};
//		SDL_FillRect(screen, &a, SDL_MapRGB(screen->format, 10,25,25));

		
		SDL_BlitSurface(titlePage, &a, screen, &a);

		OptMenu::RenderTitle();
		OptMenu::RenderOptions();
	}

	void Init()
	{
		OptMenu::Init();

		int xw = SCREEN_W/6;
		r.w += xw; r2.w+=xw;
		int x = SCREEN_W - r.x - r.w;// - FONT_SPACING;
		int y = SCREEN_H - r.y - r.h + FONT_SPACING/4;// - FONT_SPACING;
		r.x += x; r2.x += x;
		r.y += y; r2.y += y;
		r.w+=20; r2.w+=20; r.h+=20; r2.h+=20;

		r.h = r2.h = SCREEN_H;
		r2.y = SCREEN_H/2;
		r.y = r2.y - FONT_SPACING - 2;
	}
};

const char *ending[] = {
	_(" Very Well Done! "),
	"", "", "", "", "", "",
	
	"", "", "*15,4", "", "", 
	
	_("All Levels Cleared!"),
	
	"", "", "*5,7", "", "", 
	
	_("Not a single green hexagon is left unbroken."),
	"", 
	_("Truly, you are a master of hexagon hopping!"),
	
	"", "", "*9,10", "", "", 
	
	"", _("Credits"), "", "", "",
	_("<Design & Direction:"), ">Tom Beaumont", "", "",
	_("<Programming:"), ">Tom Beaumont", "", "",
	_("<Graphics:"), ">Tom Beaumont", "", "",

#ifndef DISABLE_SOUND
	_("<Music:"), ">remaxim", "", "",
	_("<Sound:"), ">remaxim", ">Stephen Cameron", ">Tomasz Mazurek", ">Michel Baradari",
	">Sander M", ">Freesound.org", "", "",
#endif

	_("<Thanks to:"), ">Kris Beaumont", "",  "",
//	"", "<Some useless facts...", "",
//	_("<Tools and libraries used:"), "", ">Photoshop LE", ">Inno Setup", ">Wings 3D", ">MSVC", ">SDL", "", 
//	_("<Fonts used:"), "", ">Copperplate gothic bold", ">Verdana", "", 

	"", "", "*12,14", "", "", 

	_("Thanks for playing!")
};

const char *ending2[] = {
	_(" Absolutely Amazing! "),
	"", "", "", "", "", 
	
	"", "", "*15,4", "", "", 
	
	_("All Levels Mastered!!"),
	
	"", "", "*5,7", "", "", 
	
	_("You crushed every last green hexagon with"),
	_("breathtaking efficiency!"), 
	"",
	_("You truly are a grand master of hexagon hopping!"),
};

const int endingLen = sizeof(ending)/sizeof(ending[0]);
const int endingLen2 = sizeof(ending2)/sizeof(ending2[0]);
const int scrollMax = SCREEN_H + (endingLen+1) * FONT_SPACING;

struct Ending : public Menu
{
	struct Particle{
		double x,y,xs,ys,xa,ya;
		double time;
		int type;

		void Update(double td)
		{
			if (type==EMPTY) return;
			
			time -= td;
			x += xs*td;
			y += ys*td;
			xs += xa*td;
			ys += ya*td;
			if (type==TILE_LASER_HEAD && time<0.3)
				type=TILE_FIRE_PARTICLE_1;
			if (type==TILE_FIRE_PARTICLE_1 && time<0.1)
				type=TILE_FIRE_PARTICLE_2;

//			if (type==COLLAPSABLE || type==COLLAPSE_DOOR)
//				for (int i=int((time)*40); i<int((time+td)*40); i++)
//					new Particle(TILE_GREEN_FRAGMENT_1, x+32-rand()%63, y+20-rand()%40);

			if (y>SCREEN_H && type==TILE_GREEN_FRAGMENT)
				xa=0, ys=-ys/2, y=SCREEN_H, type=TILE_GREEN_FRAGMENT_1, time+=rand()%100*0.01;
			if (y>SCREEN_H && type==TILE_GREEN_FRAGMENT_1)
				xa=0, ys=-ys/2, y=SCREEN_H, type=TILE_GREEN_FRAGMENT_2, time+=rand()%100*0.01;
			if (y>SCREEN_H && type==TILE_GREEN_FRAGMENT_2)
				type = EMPTY;

			if (time<=0)
			{
				if (type < 10)
				{
					for (int i=0; i<40; i++)
						new Particle(TILE_LASER_HEAD, x, y);
					for (int i=0; i<40; i++)
						new Particle(TILE_GREEN_FRAGMENT + rand() % 3, x+32-rand()%63, y+20-rand()%40);
				}
				if (type==COLLAPSE_DOOR)
				{
					type = COLLAPSABLE;
					time += 1;
				}
				else
				{
					type = EMPTY;
				}
			}
			
			if (type==EMPTY) return;

			if (type<0.05 && type<15)
				RenderTile(false, tileSolid[type] ? TILE_WHITE_WALL : TILE_WHITE_TILE, int(x)+scrollX, int(y)+scrollY);
			else
				RenderTile(false, type, int(x)+scrollX, int(y)+scrollY);
		}

		Particle() : type(EMPTY) {}
		Particle(int t, int _x) : x(_x), type(t)
		{
			xa=ys=xs=0; ya=400;
			//if (t==1)
			{
				xs = rand()%100-50;
				ys=-400-rand()%200;
				//x=rand() % SCREEN_W;
				//y=rand() % SCREEN_H;
				y = SCREEN_H+20;
				time = ys/-ya;
			}
		}
		Particle(int t, double _x, double _y) : type(t)
		{
			x=_x; y=_y;
			xa=0; ya=-100;
			double r = (rand() % 2000) * PI / 1000;
			double d = rand() % 50 + 250;
			
			xs = sin(r)*d;
			ys = cos(r)*d;
			time = 1 + (rand() & 255) /255.0;
			if (t==TILE_WATER_PARTICLE || t==TILE_GREEN_FRAGMENT || t==TILE_GREEN_FRAGMENT_1)
				time = 2;
			xa=-xs/time; ya=-ys/time;
			if (t==TILE_WATER_PARTICLE || t==TILE_GREEN_FRAGMENT || t==TILE_GREEN_FRAGMENT_1)
				ya += 500, ya/=2, xa/=2, xs/=2, ys/=2;
		}
		~Particle() { type = EMPTY; }
		void* operator new(size_t sz);
	};

	Particle p[1000];

	double scroll;
	double t;
	bool goodEnding;
	static Ending* ending;
	
	Ending(bool _goodEnd) : goodEnding(_goodEnd)
	{
		memset(p, 0, sizeof(p));
		ending = this;
		renderBG = false;
		scroll = 0;
		t=0;
		PlayMusic(HHOP_MUSIC_ENDING);
	}
	
	void Render()
	{
		SDL_Rect a = {0,0,SCREEN_W,SCREEN_H};
		SDL_FillRect(screen, &a, SDL_MapRGB(screen->format, 10,25,25));

		for (unsigned int i=0; i<sizeof(p)/sizeof(p[0]); i++)
			p[i].Update(t);

		int x = a.x + a.w/2;
		int xl = SCREEN_W/5;
		int xr = SCREEN_W*4/5;
		int y = SCREEN_H - int(scroll);
		for (int i=0; i<endingLen; i++)
		{
			if (y>-FONT_SPACING*2 && y<SCREEN_H+FONT_SPACING)
			{
				const char * xx = (i<endingLen2 && goodEnding) ? ending2[i] : ::ending[i];
				if (xx[0]=='<')
					Print(xl, y+FONT_SPACING, xx+1);
				else if (xx[0]=='>')
					PrintR(xr, y, xx+1);
				else if (xx[0]=='*')
				{
					RenderTile(false, atoi(xx+1), (xl+x)/2+scrollX, y+FONT_SPACING/2+scrollY);
					RenderTile(false, atoi(strchr(xx, ',')+1), (xr+x)/2+scrollX, y+FONT_SPACING/2+scrollY);
				}
				else
					PrintC(false, x, y, xx);
			}
			y+=FONT_SPACING;
		}
		if (scroll > scrollMax + FONT_SPACING*10)
			PrintC(true, x, SCREEN_H/2-FONT_SPACING/2, _("The End"));
	}
	
	void Cancel();
	
	bool KeyPressed(int key, int mod)
	{
		if (key=='r' && (mod & KMOD_CTRL))
		{
			time = 0;
			memset(p, 0, sizeof(p));
		}
		else
			return Menu::KeyPressed(key, mod);
		return true;
	}

	void Update(double td)
	{
		noMouse = 1;

		double old = time;

		t = td;
		if (keyState[SDLK_LSHIFT]) 
			t = td*5;
		if (keyState[SDLK_0]) 
			t = MAX(-td*5, -time);

		UpdateSound(-1.0);
		Menu::Update(t);
		scroll = time * 50;
//		if (scroll > scrollMax)
//		scroll = fmod(scroll, scrollMax);
//			scroll = 0, time = 0;

		if (old>4 && time > 4)
		{
			if (scroll < scrollMax + FONT_SPACING*17)
			{
				for (int i=int( old/2.5); i<int(time/2.5); i++)
				{
					int xs = (rand()%SCREEN_W * 6 + 1) / 8;
					for (int j=rand()%3+1; j; j--)
						new Particle(rand()&1 ? COLLAPSABLE : COLLAPSE_DOOR, xs+j*64);
				}
			}
			if (scroll > scrollMax + FONT_SPACING*27)
				Cancel();
		}

	}
};

Ending* Ending::ending = 0;
void* Ending::Particle::operator new(size_t /*sz*/)
{
	static int start = 0;
	const int max = sizeof(ending->p)/sizeof(ending->p[0]);
	for (int i=0; i<max; i++)
	{
		start++;
		if (start==max) start=0;
		if (ending->p[start].type==EMPTY)
			return &ending->p[start];
	}
	return &ending->p[rand() % max];
}

struct TitleMenu : public OptMenuTitle
{
	TitleMenu() : OptMenuTitle("")
	{
		//left_align = 1;
		PlayMusic(HHOP_MUSIC_TITLE);

		SaveState p;
		freeSlot = -1;
		for (int i=0; i<MAX_GAMESLOT; i++)
		{
			char tmp[80];
			GetSlotName(i, tmp);
			FILE* f = file_open(tmp, "rb");
			if (f)
			{
				p.LoadSave(f, false);
				fclose(f);

				if (p.general.completionPercentage==100 && p.general.masteredPercentage==100)
					sprintf(optionSlotName[i], _("Continue game %d (All Clear!)"), i+1);
				else if (p.general.completionPercentage==100)
					sprintf(optionSlotName[i], _("Continue game %d (%d%% + %d%%)"), i+1, p.general.completionPercentage, p.general.masteredPercentage);
				else
					sprintf(optionSlotName[i], _("Continue game %d (%d%% complete)"), i+1, p.general.completionPercentage);
	
				opt[num_opt++] = OPT_GAMESLOT_0 + i;
			}
			else
			{
//				sprintf(optionSlotName[i], "Start new game (slot %d)", i+1);
//				opt[num_opt++] = OPT_GAMESLOT_0 + i;

				if (freeSlot==-1)
					freeSlot = i;
			}
		}
		
		
		if (num_opt < MAX_GAMESLOT)
			opt[num_opt++] = OPT_GAMESLOT_NEW;

		opt[num_opt++] = OPT_OPTIONS;
#ifdef EDIT
		opt[num_opt++] = OPT_END;
		opt[num_opt++] = OPT_END2;
#endif
		opt[num_opt++] = OPT_QUIT;

		Init();
#ifdef EDIT
		r.y-=FONT_SPACING*2;
		r2.y-=FONT_SPACING*2;
#else
		if (num_opt==3)
			r.y+=FONT_SPACING+FONT_SPACING/2, r2.y+=FONT_SPACING+FONT_SPACING/2;
#endif
	}
	bool KeyPressed(int key, int mod);
};

struct QuitConfirmMenu : public OptMenuTitle
{
	QuitConfirmMenu() : OptMenuTitle(_("Quit: Are you sure?"))
	{
		opt[num_opt++] = OPT_QUIT_CONFIRM;
		opt[select=num_opt++] = OPT_QUIT_CANCEL;
		Init();

		r.y += FONT_SPACING*1;
		r2.y += FONT_SPACING*2;
	}

};

struct DeleteConfirmMenu : public OptMenuTitle
{
	char tmp[800];
	int num;
	DeleteConfirmMenu(int _num) : OptMenuTitle(&tmp[0]), num(_num)
	{
		//left_align = 1;

		sprintf(tmp, _("Really delete game %d?"), num+1);
		opt[num_opt++] = OPT_DELETE_CONFIRM;
		opt[select=num_opt++] = OPT_DELETE_CANCEL;
		Init();

		r.y += FONT_SPACING*1;
		r2.y += FONT_SPACING*2;
	}
	void Select()
	{
		if (select<0 || select>=num_opt)
			return;
		if (opt[select] == OPT_DELETE_CONFIRM)
		{
			GetSlotName(num, tmp);
			remove(tmp);
		}
		Pop();
		Pop();
		new TitleMenu();
	}
};

bool TitleMenu::KeyPressed(int key, int mod)
{
	if (key==SDLK_DELETE || key==SDLK_BACKSPACE || key==SDLK_F2 || key==SDLK_LALT)
	{
		if (select<0 || select>=num_opt || opt[select]<OPT_GAMESLOT_0 || opt[select]>OPT_GAMESLOT_LAST)
			return true;
		int i = opt[select] - OPT_GAMESLOT_0;

		new DeleteConfirmMenu(i);

		return true;
	}
	return OptMenu::KeyPressed(key, mod);
}

struct PauseMenu : public OptMenu
{
	PauseMenu(bool isMap, bool allowGotoMap, int allowEnd, int allowEnd2) : OptMenu(_("Paused"))
	{
		opt[num_opt++] = OPT_RESUME;
		if (!isMap)
			opt[num_opt++] = OPT_RESTART;
		opt[num_opt++] = OPT_OPTIONS;
		opt[num_opt++] = OPT_HELP;
		if (allowEnd || allowEnd2)
			opt[num_opt++] = allowEnd2 ? OPT_END2 : OPT_END;
		opt[num_opt++] = (isMap || !allowGotoMap) ? OPT_QUIT_MENU_CONFIRM : OPT_GOTO_MAP;
		Init();
	}
	virtual bool KeyPressed(int key, int mod)
	{
		if (key=='p' || key==SDLK_PAUSE)
		{
			Cancel();
			return true;
		}
		return Menu::KeyPressed(key, mod);
	}
	
};

struct OptionMenu : public OptMenuTitle
{
	bool title;
	OptionMenu(bool _title) : OptMenuTitle(_("Options")), title(_title)
	{
		/*opt[num_opt++] = OPT_FULLSCREEN;*/

#ifndef DISABLE_SOUND
		opt[num_opt++] = OPT_MUSIC;
		opt[num_opt++] = OPT_EFFECTS;
#endif
		
		opt[num_opt++] = OPT_BACK;
		
		if (title)
		{
			OptMenuTitle::Init(), renderBG=false;
			r.y += FONT_SPACING;
			r2.y += FONT_SPACING*2;
		}
		else
			OptMenu::Init(), renderBG=true;
	}
	void Render()
	{
		if (title)
			OptMenuTitle::Render();
		else
			OptMenu::Render();
	}
};

void RenderFade(double time, int dir, int seed);

struct Fader : public Menu
{
	int dir;
	double speed;
	int result;
	Fader(int _dir, int _result, double _speed=1) : dir(_dir), speed(_speed), result(_result)
	{
		renderBG = under ? under->renderBG : true;
		PlaySound(HHOP_SOUND_UI_FADE);
	}
	void Render()
	{
		if (under)
			under->Render();

		RenderFade(time, dir, (long)this);
	}
	void Update(double timedelta)
	{
		Menu::Update(timedelta * speed);
		if (result==-2)
		{
			if (time > 0.7)
				quitting = 1;
		}
		else if (time >= 0.5)
		{
			Pop();
			if (result==-1)
			{
				new TitleMenu();
				new Fader(1, -3);
			}
			if (result==-4)
			{
				Pop();	// Remove old menu
				HackKeyPress(SDLK_ESCAPE, KMOD_CTRL | KMOD_SHIFT);	// Reload map combination!
			}
			if (result==-6)
			{
				Pop();	// Remove old menu
				new Fader(1, 0, speed);
			}
			if (result==-5 || result==-7)
			{
				new Ending(result==-7);
				new Fader(1, 0, speed);
			}
		}
	}
};

void Ending::Cancel()
{
	if (isMap)
		PlayMusic(HHOP_MUSIC_MAP);
	else
		PlayMusic(HHOP_MUSIC_GAME);
	new Fader(-1, -6, 0.3);
//	Pop();
}

void ToggleFullscreen();

void OptMenu::Select()
{
	if (select<0 || select>=num_opt)
		return;
//	PlaySound(HHOP_SOUND_UI_MENU);
	switch(opt[select])
	{
		case OPT_RESUME:
			Cancel();
			break;

		case OPT_RESTART:
			Cancel();
			HackKeyPress('r', KMOD_CTRL);
			break;

		case OPT_GOTO_MAP:
		case OPT_GOTO_MAP_CONTINUE:
			Pop();
			HackKeyPress(SDLK_ESCAPE, KMOD_CTRL);
			break;

		case OPT_QUIT:
			new QuitConfirmMenu();
			break;

		/*case OPT_FULLSCREEN:
			ToggleFullscreen();
			break;*/

#ifndef DISABLE_SOUND
		case OPT_MUSIC:
			ToggleMusic ();
			break;
		case OPT_EFFECTS:
			ToggleEffects ();
			break;
#endif

		case OPT_QUIT_CONFIRM:
			new Fader(-1, -2);
			break;

		case OPT_QUIT_MENU_CONFIRM:
			Pop();
			new Fader(-1, -1);
			break;

		case OPT_OPTIONS:
			new OptionMenu(!activeMenu->renderBG);
			break;

		case OPT_HELP:
			new HintReview();
			break;

		case OPT_QUIT_CANCEL:
		case OPT_BACK:
			Pop();
			break;

		case OPT_END:
			new Fader(-1, -5, 0.3);
			break;

		case OPT_END2:
			new Fader(-1, -7, 0.3);
			break;

		case OPT_UNDO:
			Pop();
			HackKeyPress('z', 0);
			break;

		default:
			if ((opt[select]>=OPT_GAMESLOT_0 && opt[select]<=OPT_GAMESLOT_LAST) ||
			    (opt[select]==OPT_GAMESLOT_NEW && freeSlot>=0))
			{
				if (opt[select]==OPT_GAMESLOT_NEW)
					GetSlotName(freeSlot, currentSlot);
				else
					GetSlotName(opt[select]-OPT_GAMESLOT_0, currentSlot);
				new Fader(-1, -4);
			}
			break;
	}
}
