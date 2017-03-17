/* Lips of Suna
 * Copyright© 2007-2009 Lips of Suna development team.
 *
 * Lips of Suna is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Lips of Suna is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Lips of Suna. If not, see <http://www.gnu.org/licenses/>.
 */
#include <string.h>

/**
 * \addtogroup lisys System
 * @{
 * \addtogroup lisysRelative Relative
 * @{
 */

/**
 * \brief Gets the file name of the calling executable.
 *
 * \return New string or NULL.
 */
char*
lisys_relative_exename ()
{
	return NULL;
}

/**
 * \brief Gets the directory in which the calling executable is.
 *
 * \return New string or NULL.
 */
char*
lisys_relative_exedir ()
{
	const char* tmp = "/3ds/Hexahop/";
	return strdup(tmp);;

}

/** @} */
/** @} */
