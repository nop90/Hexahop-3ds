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

#include <stdint.h>
#include <SDL_endian.h>

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define SWAP16(X)    (X)
#define SWAP32(X)    (X)
#else
#define SWAP16(X)    SDL_Swap16(X)
#define SWAP32(X)    SDL_Swap32(X)
#endif

struct PackFile1
{
  /* Is it *NOT* save to interpret a byte stream as list of Entries!
   * The alignment could increase the Entry size on some systems without attribute.
   *
   * It works on: i386, amd64, mips o32 ABI, powerPC
   * Maybe it's also compiler dependent ...
   * 
   * See also http://c-faq.com/struct/padding.html,
   * http://c-faq.com/strangeprob/ptralign.html and Debian bug #442854
   * (Need to refer to a C FAQ in a (so called) C++ program, argh ...)
   * */
	class Entry {
		int32_t len;
	public:
		// an array of size 1 (no char* pointer!) is saved after len,
		// accessing name[0] should (but doesn't always) fit the first byte after len
		// See e.g. http://c-faq.com/aryptr/index.html 
		char name[1];

		Entry* GetNext()
		{
			return (Entry*)((char*)name + len);
		}

		void* Data()
		{
			char* pos = name;
			while (*pos!=0)
				pos++;
			return pos+1;
		}

		int DataLen()
		{
			return len - strlen(name) - 1;
		}
	}
#ifdef __GNUC__
   __attribute__ ((__packed__));
	 int static_assert1[sizeof(Entry)==5 ? 0 : -1];
#else
  int static_assert1[sizeof(Entry)<=8 ? 0 : -1];
#endif

	int numfiles;
	Entry** e;
	void* data;

	PackFile1() : numfiles(0), e(0), data(0)
	{}

	Entry* Find(const char* name)
	{
		if (numfiles==0) return 0;
		int a=0, b=numfiles;
		while (b>a)
		{
			const int mid = (a+b)>>1;
			int diff = strcmp(name, e[mid]->name);
			if (diff==0)
				return e[mid];
			else if (diff < 0)
				b=mid;
			else
				a=mid+1;
		}
		return 0;
	}

	void Read(FILE* f)
	{
		if (numfiles || e || data)
			FATAL("Calling Packfile1::Read when already initialised.");

		int32_t size;
		fseek(f, -(int)sizeof(size), SEEK_END);
		int end_offset = ftell(f);
		fread(&size, sizeof(size), 1, f);
		size = SWAP32(size);
		fseek(f, end_offset - size, SEEK_SET);

		data = malloc(size);
		char* data_end = (char*)data + size;
		fread(data, 1, size, f);

		numfiles=0;
		Entry* i = (Entry*)data;
		while ((void*)i < data_end)
		{
			numfiles++;
			int32_t *data_length = (int32_t*)i;
			*data_length = SWAP32(*data_length);
			i = i->GetNext();
		}
		
		e = new Entry* [numfiles]; // CHECKME: where to delete?

		i = (Entry*)data;
		for (int j=0; j<numfiles; j++, i = i->GetNext())
			e[j] = i;
	}

	~PackFile1()
	{
		free(data);
	}

};
