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

/**
 * \addtogroup lisys System
 * @{
 * \addtogroup lisysDir Directory
 * @{
 */

#ifndef __SYSTEM_DIRECTORY_H__
#define __SYSTEM_DIRECTORY_H__

typedef int (*lisysDirFilter)(const char* dir, const char* name);
typedef int (*lisysDirSorter)(const char** name0, const char** name1);
typedef struct _lisysDir lisysDir;
struct _lisysDir;

#ifdef __cplusplus
extern "C" {
#endif

lisysDir*
lisys_dir_open (const char* path);

void
lisys_dir_free (lisysDir* self);

int
lisys_dir_scan (lisysDir* self);

int
lisys_dir_get_count (const lisysDir* self);

const char*
lisys_dir_get_name (const lisysDir* self,
                    int             i);

char*
lisys_dir_get_path (const lisysDir* self,
                    int             i);

void
lisys_dir_set_filter (lisysDir*      self,
                      lisysDirFilter filter);

void
lisys_dir_set_sorter (lisysDir*      self,
                      lisysDirSorter sorter);

int
LISYS_DIR_FILTER_FILES (const char* dir,
                        const char* name);

int
LISYS_DIR_FILTER_DIRS (const char* dir,
                       const char* name);

int
LISYS_DIR_FILTER_HIDDEN (const char* dir,
                         const char* name);

int
LISYS_DIR_FILTER_VISIBLE (const char* dir,
                          const char* name);

int
LISYS_DIR_SORTER_ALPHA (const char** name0,
                        const char** name1);

#ifdef __cplusplus
}
#endif

#endif

/** @} */
/** @} */

