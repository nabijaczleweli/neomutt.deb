/**
 * @file
 * Test code for mutt_str_skip_whitespace()
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
#include "acutest.h"
#include "config.h"
#include "mutt/lib.h"

struct SkipTest
{
  const char *str;
  size_t offset;
};

void test_mutt_str_skip_whitespace(void)
{
  // char *mutt_str_skip_whitespace(char *p);

  {
    TEST_CHECK(mutt_str_skip_whitespace(NULL) == NULL);
  }

  // clang-format off
  struct SkipTest skip_tests[] =
  {
    { "",              0 },
    { "apple",         0 },
    { " apple",        1 },
    { "\tapple",       1 },
    { "\rapple",       1 },
    { "\napple",       1 },

    { "\t \t \napple", 5 },
    { "\t \t \n",      5 },
  };
  // clang-format on

  {
    for (size_t i = 0; i < mutt_array_size(skip_tests); i++)
    {
      struct SkipTest *t = &skip_tests[i];
      TEST_CASE_("'%s'", t->str);

      const char *result = mutt_str_skip_whitespace(t->str);
      TEST_CHECK(result == (t->str + t->offset));
    }
  }
}
