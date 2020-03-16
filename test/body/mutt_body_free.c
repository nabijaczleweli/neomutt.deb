/**
 * @file
 * Test code for mutt_body_free()
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
#include "address/lib.h"
#include "email/lib.h"

void test_mutt_body_free(void)
{
  // void mutt_body_free(struct Body **p);

  {
    mutt_body_free(NULL);
    TEST_CHECK_(1, "mutt_body_free(NULL)");
  }

  {
    struct Body *body = NULL;
    mutt_body_free(&body);
    TEST_CHECK_(1, "mutt_body_free(&body)");
  }
}
