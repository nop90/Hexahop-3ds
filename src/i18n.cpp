// (c) 2007 Miriam Ruiz <little_miry@yahoo.es>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.

#ifdef USE_GETTEXT

#include "i18n.h"

#include <libintl.h>
#include <locale.h>
#include <langinfo.h>

GetTextInit::GetTextInit()
{
	if  (!IsInit)
	{
		IsInit = true;
		setlocale (LC_MESSAGES, "");
		setlocale (LC_CTYPE, "");
		setlocale (LC_COLLATE, "");
		textdomain ("hex-a-hop");
		bindtextdomain ("hex-a-hop", NULL);
	}
}

bool GetTextInit::IsInit = false;

const char *GetTextInit::GetEncoding() const
{
	// See http://svn.xiph.org/trunk/vorbis-tools/intl/localcharset.c
	// if nl_langinfo isn't found
	char *locale_enc = nl_langinfo(CODESET);
	return locale_enc;
}

#endif
