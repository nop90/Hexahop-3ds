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

#ifndef _I18N_H
#define _I18N_H

#ifdef USE_GETTEXT

#include <libintl.h>
#define _(String) gettext (String)

class GetTextInit
{
	protected:
		static bool IsInit;
	public:
		GetTextInit();
		const char *GetEncoding() const;
};

static GetTextInit gettext_init;

#else

#define _(String) String
#define ngettext(a,b,n) ((n)?(b):(a))

#endif

#endif
