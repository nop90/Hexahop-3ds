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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "system-directory.h"

#ifdef LI_ENABLE_ERROR
#include "system-error.h"
#define error_memory() lisys_error_set (ENOMEM, NULL)
#define error_open(fmt, file) lisys_error_set (EIO, fmt, file)
#else
#define error_memory()
#define error_open(fmt, file)
#endif

struct _lisysDir
{
	void* dir;
	char* path;
	struct
	{
		lisysDirFilter filter;
		lisysDirSorter sorter;
	} calls;
	struct
	{
		int count;
		char** array;
	} names;
};

static char*
private_concat_paths (const char* a,
                      const char* b);

/*****************************************************************************/

/**
 * \brief Opens a directory.
 *
 * \param path Path to directory.
 * \return Directory or NULL.
 */
lisysDir*
lisys_dir_open (const char* path)
{
	lisysDir* self;

	/* Allocate self. */
	self = calloc (1, sizeof (lisysDir));
	if (self == NULL)
	{
		error_memory ();
		return NULL;
	}

	/* Store path. */
	self->path = strdup (path);
	if (path == NULL)
	{
		error_memory ();
		free (self);
		return NULL;
	}

	/* Open the directory. */
	self->dir = opendir (path);
	if (self->dir == NULL)
	{
		error_open ("cannot open directory `%s'", path);
		free (self->path);
		free (self);
		return NULL;
	}

	return self;
}

/**
 * \brief Frees a directory.
 *
 * \param self Directory.
 */
void
lisys_dir_free (lisysDir* self)
{
	int i;

	closedir (self->dir);
	for (i = 0 ; i < self->names.count ; i++)
		free (self->names.array[i]);
	free (self->names.array);
	free (self->path);
	free (self);
}

/**
 * \brief Scans a directory.
 *
 * Scans the directory for files and filters and sorts them according to the
 * current filter and sorter rules. The results are stored to the names array.
 *
 * \param self Directory.
 * \return Nonzero on success.
 */
int
lisys_dir_scan (lisysDir* self)
{
	int i;
	int num = 0;
	int cap = 16;
	char** tmp;
	char** list;
	struct dirent* ent;

	/* Allocate the name list. */
	list = malloc (cap * sizeof (char*));
	if (list == NULL)
	{
		error_memory ();
		return 0;
	}

	/* Read names. */
	rewinddir (self->dir);
	while ((ent = readdir (self->dir)))
	{
		/* Filter. */
		if (self->calls.filter != NULL)
		{
			if (!self->calls.filter (self->path, ent->d_name))
				continue;
		}

		/* Resize the list. */
		if (num == cap)
		{
			tmp = realloc (list, (cap << 1) * sizeof (char*));
			if (tmp == NULL)
			{
				error_memory ();
				goto error;
			}
			cap <<= 1;
			list = tmp;
		}

		/* Append to the list. */
		list[num] = strdup (ent->d_name);
		if (list[num] == NULL)
		{
			error_memory ();
			goto error;
		}
		num++;
	}

	/* Shrink. */
	if (num != 0)
	{
		tmp = realloc (list, num * sizeof (char*));
		if (tmp != NULL)
			list = tmp;
	}
	else
	{
		free (list);
		list = NULL;
	}

	/* Sort names. */
	if (self->calls.sorter != NULL)
		qsort (list, num, sizeof (char*), (int(*)(const void*, const void*)) self->calls.sorter);

	/* Replace old names. */
	for (i = 0 ; i < self->names.count ; i++)
		free (self->names.array[i]);
	free (self->names.array);
	self->names.count = num;
	self->names.array = list;

	return 1;

error:
	for (i = 0 ; i < num ; i++)
		free (list[i]);
	free (list);
	return 0;
}

/**
 * \brief Gets the number of files in the directory.
 * 
 * \param self Directory.
 * \return Number of files.
 */
int
lisys_dir_get_count (const lisysDir* self)
{
	return self->names.count;
}

/**
 * \brief Gets the name of one of the files in the name array.
 *
 * \param self Directory.
 * \param i Index in the name array.
 * \return String owned by the directory.
 */
const char*
lisys_dir_get_name (const lisysDir* self,
                    int             i)
{
	assert (i >= 0);
	assert (i < self->names.count);

	return self->names.array[i];
}

/**
 * \brief Gets the path to one of the files in the name array.
 *
 * \param self Directory.
 * \param i Index in the name array.
 * \return New string or NULL.
 */
char*
lisys_dir_get_path (const lisysDir* self,
                    int             i)
{
	assert (i >= 0);
	assert (i < self->names.count);

	return private_concat_paths (self->path, self->names.array[i]);
}

/**
 * \brief Set the current filter rule.
 *
 * \param self Directory.
 * \param filter Fiulter function or NULL.
 */
void
lisys_dir_set_filter (lisysDir*      self,
                      lisysDirFilter filter)
{
	self->calls.filter = filter;
}

/**
 * \brief Set the current sorter rule.
 *
 * \param self Directory.
 * \param sorter Sorter function or NULL.
 */
void
lisys_dir_set_sorter (lisysDir*      self,
                      lisysDirSorter sorter)
{
	self->calls.sorter = sorter;
}

/*****************************************************************************/

/**
 * \brief Lets everything except directories through.
 */
int
LISYS_DIR_FILTER_FILES (const char* dir,
                        const char* name)
{
	char* path;
	struct stat st;

	path = private_concat_paths (dir, name);
	if (path == NULL)
		return 1;
	if (stat (path, &st) < 0)
	{
		free (path);
		return 0;
	}
	if (!S_ISDIR (st.st_mode))
	{
		free (path);
		return 1;
	}

	free (path);
	return 0;
}

/**
 * \brief Only lets directories through.
 */
int
LISYS_DIR_FILTER_DIRS (const char* dir,
                       const char* name)
{
	char* path;
	struct stat st;

	path = private_concat_paths (dir, name);
	if (path == NULL)
		return 1;
	if (stat (path, &st) < 0)
	{
		free (path);
		return 0;
	}
	if (S_ISDIR (st.st_mode))
	{
		free (path);
		return 1;
	}

	free (path);
	return 0;
}

/**
 * \brief Only lets hidden files through.
 */
int
LISYS_DIR_FILTER_HIDDEN (const char* dir,
                         const char* name)
{
	if (name[0] == '.')
		return 1;
	return 0;
}

/**
 * \brief Only lets non-hidden files through.
 */
int
LISYS_DIR_FILTER_VISIBLE (const char* dir,
                          const char* name)
{
	if (name[0] == '.')
		return 0;
	return 1;
}

/**
 * \brief Sorts entries alphabetically.
 */
int
LISYS_DIR_SORTER_ALPHA (const char** name0,
                        const char** name1)
{
	return strcmp (*name0, *name1);
}

/*****************************************************************************/

static char*
private_concat_paths (const char* a,
                      const char* b)
{
	int len0;
	int len1;
	char* ret;

	len0 = strlen (a);
	len1 = strlen (b);
	ret = malloc (len0 + len1 + 2);
	if (ret == NULL)
	{
		error_memory ();
		return NULL;
	}
	strcpy (ret, a);
	if (len0 && a[len0] != '/')
		ret[len0++] = '/';
	strcpy (ret + len0, b);
	ret[len0 + len1] = '\0';

	return ret;
}

/** @} */
/** @} */
