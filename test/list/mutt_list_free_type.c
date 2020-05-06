/**
 * @file
 * Test code for mutt_list_free_type()
 *
 * @authors
 * Copyright (C) 2019 Richard Russon <rich@flatcap.org>
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

#define TEST_NO_MAIN
#include "config.h"
#include "acutest.h"
#include "mutt/lib.h"
#include "common.h"

void test_list_free(void **ptr)
{
  if (!ptr || !*ptr)
    return;

  FREE(ptr);
}

void test_mutt_list_free_type(void)
{
  // void mutt_list_free_type(struct ListHead *h, list_free_t fn);

  {
    list_free_t fn = test_list_free;
    mutt_list_free_type(NULL, fn);
    TEST_CHECK_(1, "mutt_list_free_type(NULL, fn)");
  }

  {
    struct ListHead listhead = { 0 };
    mutt_list_free_type(&listhead, NULL);
    TEST_CHECK_(1, "mutt_list_free_type(&listhead, NULL)");
  }

  {
    struct ListHead lh = STAILQ_HEAD_INITIALIZER(lh);
    mutt_list_free_type(&lh, test_list_free);
    TEST_CHECK_(1, "mutt_list_free_type(&lh)");
  }

  {
    static const char *names[] = { "Amy", "Beth", "Cathy", "Denise", NULL };
    struct ListHead lh = test_list_create(names, true);
    mutt_list_free_type(&lh, test_list_free);
    TEST_CHECK_(1, "mutt_list_free_type(&lh)");
  }
}
