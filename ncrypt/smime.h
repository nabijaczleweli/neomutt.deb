/**
 * @file
 * SMIME helper routines
 *
 * @authors
 * Copyright (C) 2001-2002 Oliver Ehli <elmy@acm.org>
 * Copyright (C) 2004 g10 Code GmbH
 * Copyright (C) 2019 Pietro Cerutti <gahr@gahr.ch>
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

#ifndef MUTT_NCRYPT_SMIME_H
#define MUTT_NCRYPT_SMIME_H

#include "config.h"
#include <stdbool.h>
#include <stdio.h>
#include "lib.h"

struct AddressList;
struct Body;
struct Email;
struct Envelope;
struct Message;
struct State;

/**
 * struct SmimeKey - An SIME key
 */
struct SmimeKey
{
  char *email;
  char *hash;
  char *label;
  char *issuer;
  char trust; ///< i=Invalid r=revoked e=expired u=unverified v=verified t=trusted
  KeyFlags flags;
  struct SmimeKey *next;
};

void smime_init(void);
void smime_cleanup(void);

int           smime_class_application_handler(struct Body *m, struct State *s);
struct Body * smime_class_build_smime_entity (struct Body *a, char *certlist);
int           smime_class_decrypt_mime       (FILE *fp_in, FILE **fp_out, struct Body *b, struct Body **cur);
char *        smime_class_find_keys          (const struct AddressList *addrlist, bool oppenc_mode);
void          smime_class_getkeys            (struct Envelope *env);
void          smime_class_invoke_import      (const char *infile, const char *mailbox);
SecurityFlags smime_class_send_menu          (struct Email *e);
struct Body * smime_class_sign_message       (struct Body *a, const struct AddressList *from);
bool          smime_class_valid_passphrase   (void);
int           smime_class_verify_one         (struct Body *sigbdy, struct State *s, const char *tempfile);
int           smime_class_verify_sender      (struct Email *e, struct Message *msg);
void          smime_class_void_passphrase    (void);

#endif /* MUTT_NCRYPT_SMIME_H */
