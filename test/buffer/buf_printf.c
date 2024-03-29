/**
 * @file
 * Test code for buf_printf()
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
#include <string.h>
#include "mutt/lib.h"
#include "test_common.h"

void test_buf_printf(void)
{
  // int buf_printf(struct Buffer *buf, const char *fmt, ...);

  {
    TEST_CHECK(buf_printf(NULL, "apple") != 0);
  }

  {
    struct Buffer buf = buf_make(0);
    TEST_CHECK(buf_printf(&buf, NULL) != 0);
  }

  TEST_CASE("printf to an empty Buffer");

  {
    TEST_CASE("Varargs");
    const char *str = "apple";
    const char *result = "app 1234567 3.1416";
    struct Buffer buf = buf_make(0);
    TEST_CHECK(buf_printf(&buf, "%.3s %d %3.4f", str, 1234567, 3.141592654) == 18);
    TEST_CHECK_STR_EQ(buf_string(&buf), result);
    buf_dealloc(&buf);
  }

  TEST_CASE("printf to a non-empty Buffer");

  {
    TEST_CASE("Varargs");
    const char *str = "apple";
    const char *result = "app 1234567 3.1416";
    struct Buffer buf = buf_make(0);
    buf_addstr(&buf, "test");
    TEST_CHECK(buf_printf(&buf, "%.3s %d %3.4f", str, 1234567, 3.141592654) == 18);
    TEST_CHECK_STR_EQ(buf_string(&buf), result);
    buf_dealloc(&buf);
  }
}
