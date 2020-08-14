/**
 * @file
 * Dialog Windows
 *
 * @authors
 * Copyright (C) 2020 Richard Russon <rich@flatcap.org>
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

#ifndef MUTT_GUI_DIALOG_H
#define MUTT_GUI_DIALOG_H

#include "mutt_window.h"

struct Menu;

struct MuttWindow *dialog_create_simple_index(struct Menu *menu, enum WindowType type);
void               dialog_destroy_simple_index(struct MuttWindow **ptr);
struct MuttWindow *dialog_find(struct MuttWindow *win);
void               dialog_pop(void);
void               dialog_push(struct MuttWindow *dlg);

#endif /* MUTT_GUI_DIALOG_H */
