#ifndef __HHOP_TEXT_H__
#define __HHOP_TEXT_H__

#include <string>

bool TextInit(const char* base);
void TextFree();
int TextWidth(const std::string &text_utf8);
int TextHeight(const std::string &text_utf8, int width);
void Print(int x, int y, const char * string, ...);
void PrintR(int x, int y, const char * string, ...);
void Print_Aligned(bool split, int x, int y, int width, const char * string, int align);
void PrintC(bool split, int x, int y, const char * string, ...);
void ConvertToUTF8(const std::string &text_locally_encoded, char *text_utf8, size_t text_utf8_length);

#endif
