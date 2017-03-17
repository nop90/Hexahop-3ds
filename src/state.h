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

#ifndef __HHOP_STATE_H__
#define __HHOP_STATE_H__

#include <SDL.h>
#ifdef USE_OPENGL
#include <SDL_OpenGL.h>
#endif
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "video.h"

#ifdef WIN32
	// Trigger debugger
//	#define FATAL(string, string2) do{__asm{int 3};}while(0)
	static inline void FATAL(const char * string="Unknown", const char * string2="") { fprintf(stderr, "Fatal error: %s \"%s\"\n", string, string2); exit(0); }
#else
	static inline void FATAL(const char * string="Unknown", const char * string2="") { fprintf(stderr, "Fatal error: %s \"%s\"\n", string, string2); exit(0); }
#endif

class String
{
	int len;
	char* data;
public:
	void reserve(int i) { if (i<0) FATAL("-ve string length."); if (i<=len) return; len=i; data=(char*)realloc(data, (len+1)*sizeof(char)); }
	String() : len(0), data(NULL) { reserve(32); *data = '\0'; }
	String(String const & s) : len(0), data(NULL) { reserve(s.len); strcpy(data, s.data); }
	String(const char* s) : len(0), data(NULL) { *this = s; }
	~String() { free(data); }
	operator const char* () const {return data ? data : "";}
	void operator = (String const & a) { *this = (const char*)a; }
	void operator = (const char * a) { reserve(strlen(a)); strcpy(data, a); }
	void operator += (const char * a) { reserve(strlen(a)+len); strcat(data, a); }
	String operator + (const char * a) const { String ret(*this); ret += a; return ret; }
	void truncate (int pos) { data[pos] = '\0'; }
	void fix_backslashes() { if(data) { for (int i=0; data[i]; i++) { if (data[i]=='\\') data[i]='/'; } } }
};

class State
{
public:
	virtual ~State() {}

	virtual bool KeyPressed(int key, int mod) = 0;
	virtual void KeyReleased(int /*key*/) {};
	virtual void Mouse(int x, int y, int dx, int dy, int buttons_pressed, int buttons_released, int buttons) = 0;
	virtual void Update(double timedelta) = 0;
	virtual void Render() = 0;
	virtual void FileDrop(const char* filename) = 0;
	virtual void ScreenModeChanged() {};
};

/************************************************************************
// TEMPLATE - copy & paste

#define ClassName NEWSTATE
class ClassName : public State
{
public:
	virtual bool KeyPressed(int key, int mod) 
	{ 
		return false; 
	}
	virtual void KeyReleased(int key) 
	{ 
	}
	virtual void Mouse(int x, int y, int dx, int dy, int buttons_pressed, int buttons_released, int buttons) 
	{
	}
	virtual void FileDrop(const char* filename) 
	{
	}
	virtual void Update(double timedelta)
	{
		// TODO
	}
	virtual void Render()
	{
		// TODO
	}
	virtual void ScreenModeChanged() 
	{
		// TODO
	}
};
MAKE_STATE(ClassName, _KEY_, false);

************************************************************************/

class StateMakerBase
{
	static StateMakerBase* first;
	StateMakerBase* next;
	int key;
	bool start;
protected:
	State* state;
public:
	static State* current;

public:
	StateMakerBase(int key, bool start) : state(NULL)
	{
		for (StateMakerBase* s = first; s; s=s->next)
			if(key == s->key)
			{
				FATAL("Duplicate key in StateMakerBase::StateMakerBase");
				return;
			}
		this->key = key;
		this->start = start;
		next = first;
		first = this;
	}
	virtual ~StateMakerBase() {}
	virtual State* Create() = 0;
	void Destroy()
	{
		delete state;
		state = 0;
	}
	static State* GetNew(int k)
	{
		for (StateMakerBase* s = first; s; s=s->next)
			if(k==s->key)
				return current = s->Create();
		return current;
	}
	static State* GetNew()
	{
		if (!first)
		{
			FATAL("StateMakerBase::GetNew - first is NULL");
			return 0;
		}
		for (StateMakerBase* s = first; s; s=s->next)
			if(s->start)
				return current = s->Create();
		return current = first->Create();
	}
	static void DestroyAll()
	{
		for (StateMakerBase* s = first; s; s=s->next)
			s->Destroy();
		current = 0;
	}
};

template<class X>
class StateMaker : public StateMakerBase
{
public:
	StateMaker(int key, bool start=false) : StateMakerBase(key, start)
	{}
	State* Create() 
	{ 
		if (!state) state = new X;
		return state;
	}
};

#define MAKE_STATE(x,key,start) static StateMaker<x> _maker_##x(key, start);

extern int mouse_buttons, mousex, mousey, noMouse;
extern double stylusx, stylusy;
extern float styluspressure;
extern int stylusok;
extern int quitting;

char* LoadSaveDialog(bool save, bool levels, const char * title);

#endif

