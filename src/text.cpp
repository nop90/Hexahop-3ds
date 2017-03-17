#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <errno.h>
#include "i18n.h"
#include "text.h"
#include "video.h"

#ifndef ENABLE_PANGO

#include <SDL_ttf.h>

static TTF_Font* font;

/**
 * \brief Splits a string into one or more lines.
 *
 * The string is altered by the algorithm and must thus be writable. This is
 * the case since the UTF8 conversion is done to all strings passed to this.
 *
 * \param str UTF8 string.
 * \param width Maximum line width.
 * \param split True if should interpret double spaces as line breaks.
 * \return Array of lines.
 */
static std::vector<std::string> TextWrapString(char* str, int width, bool split)
{
	int val;
	char tmp;
	char* start;
	char* end;
	char* word;
	std::vector<std::string> lines;

	if (*str == '\0')
		return lines;
	word = NULL;
	start = str;
	end = start + 1;
	while (*end != '\0')
	{
		tmp = *end;
		*end = '\0';

		if (tmp == '\n')
		{
			// Normal newline.
			lines.push_back(std::string(start));
			start = end;
			word = NULL;
		}
		else if (split && tmp == ' ' && end[1] == ' ')
		{
			// Double space break.
			// FIXME: Should really get rid of this and just use newlines.
			lines.push_back(std::string(start));
			//lines.push_back(std::string(""));
			start = end;
			word = NULL;
			*end = tmp;
			end += 2;
			continue;
		}
		else
		{
			// Character wrap.
			if (tmp == ' ')
				word = end;
			TTF_SizeUTF8(font, start, &val, NULL);
			if (val > width)
			{
				if (word != NULL)
				{
					*end = tmp;
					end = word;
					tmp = *end;
					*end = '\0';
				}
				lines.push_back(std::string(start));
				start = end;
				word = NULL;
			}
		}

		*end = tmp;
		end++;
	}
	lines.push_back(std::string(start));

	return lines;
}

static void TextPrintUTF8(int x, int y, const char* str)
{
	SDL_Color fg = { 255, 255, 255, 255 };
	SDL_Surface* surface = TTF_RenderUTF8_Blended (font, str, fg);
	SDL_Rect dst = {x, y, 1, 1};
	SDL_BlitSurface(surface, NULL, screen, &dst);
	SDL_FreeSurface(surface);
}

static void TextPrintRAW(int x, int y, const char* str)
{
	char tmp[5000];

	ConvertToUTF8(str, tmp, 5000);
	SDL_Color fg = { 255, 255, 255, 255 };
	SDL_Surface* surface = TTF_RenderUTF8_Blended (font, tmp, fg);
	SDL_Rect dst = {x, y, 1, 1};
	SDL_BlitSurface(surface, NULL, screen, &dst);
	SDL_FreeSurface(surface);
}

#else

#include <SDL_Pango.h>

static SDLPango_Context *context = 0;

/// determine length of longest line with current font (wrapping allowed if text_width != -1)
static int SDLPangoTextHeight(const std::string &text_utf8, int text_width)
{
	// SDLPango_SetMinimumSize limits indeed the maximal size! See
	// http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=438691
	SDLPango_SetMinimumSize(context, text_width, 0);
	SDLPango_SetText(context, text_utf8.c_str(), -1);
	return SDLPango_GetLayoutHeight(context);
}

/** \brief Determine length of longest line with current font
 *
 * Whether line breaks are allowed or not needs to be set before using
 * SDLPango_SetMinimumSize!
 */
static int SDLPangoTextWidth(const std::string &text_utf8)
{
	SDLPango_SetText(context, text_utf8.c_str(), -1);
	return SDLPango_GetLayoutWidth(context);
}

/// Display the specified UTF-8 text left aligned at (x,y)
static void Print_Pango(int x, int y, const std::string &text_utf8)
{
	// Workaround for possible crash, see
	// http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=439071
	if (text_utf8.size() == 0 || (text_utf8.size() == 1 && text_utf8[0]==127))
		return;
	assert(text_utf8.find("\n") == std::string::npos);
	SDLPango_SetMinimumSize(context, SCREEN_W, 0);
	SDLPango_SetText(context, text_utf8.c_str(), -1);
	SDL_Surface *surface = SDLPango_CreateSurfaceDraw(context);
	SDL_Rect dst = {x, y, 1, 1};
	SDL_BlitSurface(surface, NULL, screen, &dst);
	SDL_FreeSurface(surface);
}

/** \brief Display the specified UTF-8 text according to the alignment
 *
 *  If line breaks are already properly set (manually) the will be respected
 *  and no new line breaks will be added. This assumes that th text is not too
 *  wide.
 *
 *  \param x the displayed text is horizontally centered around x
 *  \param y the displayed text starts at y
 *  \param width background window size into which the text needs to fit
 *  \param text_utf8 the text to be displayed, in UTF8 encoding
 *  \param align=1: horizontally centered around (x,y)
 * */
static void Print_Pango_Aligned(int x, int y, int width, const std::string &text_utf8, int align)
{
	// Workaround for possible crash, see
	// http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=439071
	if (text_utf8.size() == 0 || (text_utf8.size() == 1 && text_utf8[0]==127))
		return;
	if (width<=0)
		return;
	SDLPango_SetMinimumSize(context, width, 0);
	int real_width = SDLPangoTextWidth(text_utf8);
	// Workaround for a crash in SDL Pango, see
	// http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=439855
	if (real_width>width)
		SDLPango_SetMinimumSize(context, real_width, 0);

  SDLPango_Alignment alignment;
  if (align==0)
    alignment = SDLPANGO_ALIGN_LEFT;
  else if (align==2) {
    alignment = SDLPANGO_ALIGN_RIGHT;
    x -= width;
  } else {
    alignment = SDLPANGO_ALIGN_CENTER;
    x -= width/2;
  }
	// SDLPango_SetText_GivenAlignment is not (yet?) part of the official Pango
	// distribution, see http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=437865
	SDLPango_SetText_GivenAlignment(context, text_utf8.c_str(), -1, alignment);
	SDL_Surface *surface = SDLPango_CreateSurfaceDraw(context);
	SDL_Rect dst = {x, y, 1, 1};
	SDL_BlitSurface(surface, NULL, screen, &dst);
	SDL_FreeSurface(surface);
}

#endif

/*****************************************************************************/

#ifndef ENABLE_PANGO

bool TextInit(const char* base)
{
	std::string dir(base);
	std::string name(dir + "font.ttf");

	TTF_Init();
	font = TTF_OpenFont(name.c_str(), 24);
	if (font == NULL)
	{
		fprintf (stderr, "Cannot load font `%s'.\n", name.c_str());
		return false;
	}
	//TTF_SetFontStyle(font, TTF_STYLE_BOLD);
	TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
	
	return true;
}

void TextFree()
{
	TTF_CloseFont(font);
	TTF_Quit();
}

int TextWidth(const std::string &text_utf8)
{
	int val;

	TTF_SizeUTF8(font, text_utf8.c_str(), &val, NULL);
	return val;
}

int TextHeight(const std::string &text_utf8, int width)
{
	int h;
	int val = 0;
	char* tmp = strdup(text_utf8.c_str());

	std::vector<std::string> lines = TextWrapString(tmp, width, false);
	std::vector<std::string>::iterator iter;
	for (iter = lines.begin() ; iter != lines.end() ; iter++)
	{
		TTF_SizeUTF8(font, iter->c_str(), NULL, &h);
		val += h;
	}
	free (tmp);

	return val;
}

/// Prints a left aligned string (a single line) beginning at (x,y)
// TODO: Check that the maximal text width is already set
void Print(int x, int y, const char * string, ...)
{
	va_list marker;
	va_start( marker, string );     /* Initialize variable arguments. */

	char tmp[1000];
	vsnprintf((char*)tmp, 1000, string, marker);

	TextPrintRAW(x, y, tmp);

	va_end( marker );              /* Reset variable arguments.      */
}

/// Prints a string right aligned so that it ends at (x,y)
// TODO: Check that the maximal text width is already set
void PrintR(int x, int y, const char * string, ...)
{
	va_list marker;
	va_start( marker, string );     /* Initialize variable arguments. */

	char tmp[1000];
	vsnprintf((char*)tmp, 1000, string, marker);

	TextPrintRAW(x-TextWidth(tmp), y, tmp);

	va_end( marker );              /* Reset variable arguments.      */
}

/** \brief Prints a string horizontally centered around (x,y)
 *
 *  "  " in the string is interpreted as linebreak
*/
void Print_Aligned(bool split, int x, int y, int width, const char * string, int align)
{
	int h;
	int w;
	int off;
	char tmp_utf8[5000]; // FIXME: Check this limit
	
	ConvertToUTF8(string, tmp_utf8, sizeof(tmp_utf8)/sizeof(char));
	std::vector<std::string> lines = TextWrapString(tmp_utf8, width, split);
	std::vector<std::string>::iterator iter;
	for (iter = lines.begin() ; iter != lines.end() ; iter++)
	{
		TTF_SizeUTF8(font, iter->c_str(), &w, &h);
		switch (align)
		{
			case 0: off = 0; break;
			case 2: off = -w; break;
			default: off = -w / 2; break;
		}
		TextPrintUTF8(x + off, y, iter->c_str());
		y += h;
	}
}

void PrintC(bool split, int x, int y, const char * string, ...)
{
	va_list marker;
	va_start( marker, string );     /* Initialize variable arguments. */

	char tmp[1000];
	vsnprintf((char*)tmp, 1000, string, marker);

	va_end( marker );              /* Reset variable arguments.      */

	static bool print = true; // avoid flickering!
	if (print) {
		std::cerr << "Warning: don't know window width for message:\n" << tmp << "\n";
		for (unsigned int i=0; i<strlen(tmp); ++i)
			if (!std::isspace(tmp[i]))
				print = false;
	}
	Print_Aligned(split, x, y, 2*std::min(x, SCREEN_W-x), tmp, 1);
}

#else

bool TextInit(const char* base)
{
	SDLPango_Init();
	context = SDLPango_CreateContext_GivenFontDesc("sans-serif bold 12");
	SDLPango_SetDefaultColor(context, MATRIX_TRANSPARENT_BACK_WHITE_LETTER);
	SDLPango_SetMinimumSize(context, SCREEN_W, 0);
	return true;
}

void TextFree()
{
	SDLPango_FreeContext(context);
}

int TextWidth(const std::string &text_utf8)
{
	return SDLPangoTextWidth(text_utf8);
}

int TextHeight(const std::string &text_utf8, int width)
{
	return SDLPangoTextHeight(text_utf8, width);
}

/// Prints a left aligned string (a single line) beginning at (x,y)
// TODO: Check that the maximal text width is already set
void Print(int x, int y, const char * string, ...)
{
	va_list marker;
	va_start( marker, string );     /* Initialize variable arguments. */

	char tmp[1000], tmp_utf8[5000]; // FIXME: Check this limit
	vsprintf((char*)tmp, string, marker);

	ConvertToUTF8(tmp, tmp_utf8, sizeof(tmp_utf8)/sizeof(char));
	Print_Pango(x, y, tmp_utf8);

	va_end( marker );              /* Reset variable arguments.      */
}

/// Prints a string right aligned so that it ends at (x,y)
// TODO: Check that the maximal text width is already set
void PrintR(int x, int y, const char * string, ...)
{
	va_list marker;
	va_start( marker, string );     /* Initialize variable arguments. */

	char tmp[1000], tmp_utf8[5000]; // FIXME: Check this limit
	vsprintf((char*)tmp, string, marker);

	ConvertToUTF8(tmp, tmp_utf8, sizeof(tmp_utf8)/sizeof(char));
	Print_Pango(x-SDLPangoTextWidth(tmp_utf8), y, tmp_utf8);

	va_end( marker );              /* Reset variable arguments.      */
}

/** \brief Prints a string horizontally centered around (x,y)
 *
 *  "  " in the string is interpreted as linebreak
*/
void Print_Aligned(bool split, int x, int y, int width, const char * string, int align)
{
	char tmp_utf8[5000]; // FIXME: Check this limit

	ConvertToUTF8(string, tmp_utf8, sizeof(tmp_utf8)/sizeof(char));

	std::string msg(tmp_utf8);
	while (split && msg.find("  ") != std::string::npos)
		msg.replace(msg.find("  "), 2, "\n");

	Print_Pango_Aligned(x, y, width, msg, align);
}

void PrintC(bool split, int x, int y, const char * string, ...)
{
	va_list marker;
	va_start( marker, string );     /* Initialize variable arguments. */

	char tmp[1000]; // FIXME: Check this limit
	vsprintf((char*)tmp, string, marker);

	va_end( marker );              /* Reset variable arguments.      */

	static bool print = true; // avoid flickering!
	if (print) {
		std::cerr << "Warning: don't know window width for message:\n" << tmp << "\n";
		for (unsigned int i=0; i<strlen(tmp); ++i)
			if (!std::isspace(tmp[i]))
				print = false;
	}
	Print_Aligned(split, x, y, 2*std::min(x, SCREEN_W-x), tmp, 1);
}

#endif

void ConvertToUTF8(const std::string &text_locally_encoded, char *text_utf8, size_t text_utf8_length)
{
#ifdef USE_GETTEXT
	// Is this portable?
	size_t text_length = text_locally_encoded.length()+1;
	errno = 0;
	static const char *locale_enc = gettext_init.GetEncoding();
	iconv_t cd = iconv_open("UTF-8", locale_enc);
	char *in_buf = const_cast<char *>(&text_locally_encoded[0]);
	char *out_buf = &text_utf8[0];
	iconv(cd, &in_buf, &text_length, &out_buf, &text_utf8_length);
	iconv_close(cd);
	if (errno != 0)
		std::cerr << "An error occurred recoding " << text_locally_encoded << " to UTF8" << std::endl;
#else
	strcpy (text_utf8, text_locally_encoded.c_str ());
#endif
}
