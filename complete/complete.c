/**
 * @file
 * String auto-completion routines
 *
 * @authors
 * Copyright (C) 1996-2000,2007 Michael R. Elkins <me@mutt.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @page complete_complete String auto-completion routines
 *
 * String auto-completion routines
 */

#include "config.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "core/lib.h"
#include "data.h"
#include "globals.h"
#include "muttlib.h"
#include "protos.h"
#ifdef USE_IMAP
#include "imap/lib.h"
#endif
#ifdef USE_NNTP
#include "nntp/lib.h"
#endif

/**
 * mutt_complete - Attempt to complete a partial pathname
 * @param cd     Completion Data
 * @param buf    Buffer containing pathname
 * @param buflen Length of buffer
 * @retval  0 Ok
 * @retval -1 No matches
 *
 * Given a partial pathname, fill in as much of the rest of the path as is
 * unique.
 */
int mutt_complete(struct CompletionData *cd, char *buf, size_t buflen)
{
  const char *p = NULL;
  DIR *dir = NULL;
  struct dirent *de = NULL;
  int init = 0;
  size_t len;
  struct Buffer *dirpart = NULL;
  struct Buffer *exp_dirpart = NULL;
  struct Buffer *filepart = NULL;
  struct Buffer *tmp = NULL;
#ifdef USE_IMAP
  struct Buffer *imap_path = NULL;
  int rc;
#endif

  mutt_debug(LL_DEBUG2, "completing %s\n", buf);

#ifdef USE_NNTP
  if (OptNews)
    return nntp_complete(buf, buflen);
#endif

  const char *const c_spool_file = cs_subset_string(NeoMutt->sub, "spool_file");
  const char *const c_folder = cs_subset_string(NeoMutt->sub, "folder");
#ifdef USE_IMAP
  imap_path = mutt_buffer_pool_get();
  /* we can use '/' as a delimiter, imap_complete rewrites it */
  if ((*buf == '=') || (*buf == '+') || (*buf == '!'))
  {
    if (*buf == '!')
      p = NONULL(c_spool_file);
    else
      p = NONULL(c_folder);

    mutt_buffer_concat_path(imap_path, p, buf + 1);
  }
  else
    mutt_buffer_strcpy(imap_path, buf);

  if (imap_path_probe(mutt_buffer_string(imap_path), NULL) == MUTT_IMAP)
  {
    rc = imap_complete(buf, buflen, mutt_buffer_string(imap_path));
    mutt_buffer_pool_release(&imap_path);
    return rc;
  }

  mutt_buffer_pool_release(&imap_path);
#endif

  dirpart = mutt_buffer_pool_get();
  exp_dirpart = mutt_buffer_pool_get();
  filepart = mutt_buffer_pool_get();
  tmp = mutt_buffer_pool_get();

  if ((*buf == '=') || (*buf == '+') || (*buf == '!'))
  {
    mutt_buffer_addch(dirpart, *buf);
    if (*buf == '!')
      mutt_buffer_strcpy(exp_dirpart, NONULL(c_spool_file));
    else
      mutt_buffer_strcpy(exp_dirpart, NONULL(c_folder));
    p = strrchr(buf, '/');
    if (p)
    {
      mutt_buffer_concatn_path(tmp, mutt_buffer_string(exp_dirpart),
                               mutt_buffer_len(exp_dirpart), buf + 1,
                               (size_t) (p - buf - 1));
      mutt_buffer_copy(exp_dirpart, tmp);
      mutt_buffer_substrcpy(dirpart, buf, p + 1);
      mutt_buffer_strcpy(filepart, p + 1);
    }
    else
      mutt_buffer_strcpy(filepart, buf + 1);
    dir = mutt_file_opendir(mutt_buffer_string(exp_dirpart), MUTT_OPENDIR_NONE);
  }
  else
  {
    p = strrchr(buf, '/');
    if (p)
    {
      if (p == buf) /* absolute path */
      {
        p = buf + 1;
        mutt_buffer_strcpy(dirpart, "/");
        mutt_buffer_strcpy(filepart, p);
        dir = mutt_file_opendir(mutt_buffer_string(dirpart), MUTT_OPENDIR_NONE);
      }
      else
      {
        mutt_buffer_substrcpy(dirpart, buf, p);
        mutt_buffer_strcpy(filepart, p + 1);
        mutt_buffer_copy(exp_dirpart, dirpart);
        mutt_buffer_expand_path(exp_dirpart);
        dir = mutt_file_opendir(mutt_buffer_string(exp_dirpart), MUTT_OPENDIR_NONE);
      }
    }
    else
    {
      /* no directory name, so assume current directory. */
      mutt_buffer_strcpy(filepart, buf);
      dir = mutt_file_opendir(".", MUTT_OPENDIR_NONE);
    }
  }

  if (!dir)
  {
    mutt_debug(LL_DEBUG1, "%s: %s (errno %d)\n",
               mutt_buffer_string(exp_dirpart), strerror(errno), errno);
    goto cleanup;
  }

  /* special case to handle when there is no filepart yet.  find the first
   * file/directory which is not "." or ".." */
  len = mutt_buffer_len(filepart);
  if (len == 0)
  {
    while ((de = readdir(dir)))
    {
      if (!mutt_str_equal(".", de->d_name) && !mutt_str_equal("..", de->d_name))
      {
        mutt_buffer_strcpy(filepart, de->d_name);
        init++;
        break;
      }
    }
  }

  while ((de = readdir(dir)))
  {
    if (mutt_strn_equal(de->d_name, mutt_buffer_string(filepart), len))
    {
      if (init)
      {
        char *cp = filepart->data;

        for (int i = 0; (*cp != '\0') && (de->d_name[i] != '\0'); i++, cp++)
        {
          if (*cp != de->d_name[i])
            break;
        }
        *cp = '\0';
        mutt_buffer_fix_dptr(filepart);
      }
      else
      {
        struct stat st = { 0 };

        mutt_buffer_strcpy(filepart, de->d_name);

        /* check to see if it is a directory */
        if (mutt_buffer_is_empty(dirpart))
        {
          mutt_buffer_reset(tmp);
        }
        else
        {
          mutt_buffer_copy(tmp, exp_dirpart);
          mutt_buffer_addch(tmp, '/');
        }
        mutt_buffer_addstr(tmp, mutt_buffer_string(filepart));
        if ((stat(mutt_buffer_string(tmp), &st) != -1) && (st.st_mode & S_IFDIR))
          mutt_buffer_addch(filepart, '/');
        init = 1;
      }
    }
  }
  closedir(dir);

  if (!mutt_buffer_is_empty(dirpart))
  {
    mutt_str_copy(buf, mutt_buffer_string(dirpart), buflen);
    if (!mutt_str_equal("/", mutt_buffer_string(dirpart)) &&
        (mutt_buffer_string(dirpart)[0] != '=') && (mutt_buffer_string(dirpart)[0] != '+'))
    {
      mutt_str_copy(buf + strlen(buf), "/", buflen - strlen(buf));
    }
    mutt_str_copy(buf + strlen(buf), mutt_buffer_string(filepart), buflen - strlen(buf));
  }
  else
    mutt_str_copy(buf, mutt_buffer_string(filepart), buflen);

cleanup:
  mutt_buffer_pool_release(&dirpart);
  mutt_buffer_pool_release(&exp_dirpart);
  mutt_buffer_pool_release(&filepart);
  mutt_buffer_pool_release(&tmp);

  return init ? 0 : -1;
}
