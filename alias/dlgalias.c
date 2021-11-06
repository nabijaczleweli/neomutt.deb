/**
 * @file
 * Address book
 *
 * @authors
 * Copyright (C) 1996-2000,2007 Michael R. Elkins <me@mutt.org>
 * Copyright (C) 2019 Pietro Cerutti <gahr@gahr.ch>
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

/**
 * @page alias_dlgalias Address Book Dialog
 *
 * The Address Book Dialog allows the user to select, add or delete aliases.
 *
 * This is a @ref gui_simple
 *
 * @note New aliases will be saved to `$alias_file`.
 *       Deleted aliases are deleted from memory only.
 *
 * ## Windows
 *
 * | Name                | Type         | See Also           |
 * | :------------------ | :----------- | :----------------- |
 * | Address Book Dialog | WT_DLG_ALIAS | dlg_select_alias() |
 *
 * **Parent**
 * - @ref gui_dialog
 *
 * **Children**
 * - See: @ref gui_simple
 *
 * ## Data
 * - #Menu
 * - #Menu::mdata
 * - #AliasMenuData
 *
 * The @ref gui_simple holds a Menu.  The Address Book Dialog stores
 * its data (#AliasMenuData) in Menu::mdata.
 *
 * ## Events
 *
 * Once constructed, it is controlled by the following events:
 *
 * | Event Type            | Handler                    |
 * | :-------------------- | :------------------------- |
 * | #NT_ALIAS             | alias_alias_observer()     |
 * | #NT_CONFIG            | alias_config_observer()    |
 * | #NT_WINDOW            | alias_window_observer()    |
 * | MuttWindow::recalc()  | alias_recalc()             |
 *
 * The Address Book Dialog doesn't have any specific colours, so it doesn't
 * need to support #NT_COLOR.
 *
 * MuttWindow::recalc() is handled to support custom sorting.
 *
 * Some other events are handled by the @ref gui_simple.
 */

#include "config.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mutt/lib.h"
#include "address/lib.h"
#include "config/lib.h"
#include "core/lib.h"
#include "gui/lib.h"
#include "lib.h"
#include "menu/lib.h"
#include "pattern/lib.h"
#include "question/lib.h"
#include "alias.h"
#include "format_flags.h"
#include "gui.h"
#include "muttlib.h"
#include "opcodes.h"

/// Help Bar for the Alias dialog (address book)
static const struct Mapping AliasHelp[] = {
  // clang-format off
  { N_("Exit"),     OP_EXIT },
  { N_("Del"),      OP_DELETE },
  { N_("Undel"),    OP_UNDELETE },
  { N_("Sort"),     OP_SORT },
  { N_("Rev-Sort"), OP_SORT_REVERSE },
  { N_("Select"),   OP_GENERIC_SELECT_ENTRY },
  { N_("Help"),     OP_HELP },
  { NULL, 0 },
  // clang-format on
};

/**
 * alias_format_str - Format a string for the alias list - Implements ::format_t - @ingroup expando_api
 *
 * | Expando | Description
 * |:--------|:--------------------------------------------------------
 * | \%a     | Alias name
 * | \%c     | Comments
 * | \%f     | Flags - currently, a 'd' for an alias marked for deletion
 * | \%n     | Index number
 * | \%r     | Address which alias expands to
 * | \%t     | Character which indicates if the alias is tagged for inclusion
 */
static const char *alias_format_str(char *buf, size_t buflen, size_t col, int cols,
                                    char op, const char *src, const char *prec,
                                    const char *if_str, const char *else_str,
                                    intptr_t data, MuttFormatFlags flags)
{
  char fmt[128], addr[1024];
  struct AliasView *av = (struct AliasView *) data;
  struct Alias *alias = av->alias;

  switch (op)
  {
    case 'a':
      mutt_format_s(buf, buflen, prec, alias->name);
      break;
    case 'c':
      mutt_format_s(buf, buflen, prec, alias->comment);
      break;
    case 'f':
      snprintf(fmt, sizeof(fmt), "%%%ss", prec);
      snprintf(buf, buflen, fmt, av->is_deleted ? "D" : " ");
      break;
    case 'n':
      snprintf(fmt, sizeof(fmt), "%%%sd", prec);
      snprintf(buf, buflen, fmt, av->num + 1);
      break;
    case 'r':
      addr[0] = '\0';
      mutt_addrlist_write(&alias->addr, addr, sizeof(addr), true);
      mutt_format_s(buf, buflen, prec, addr);
      break;
    case 't':
      buf[0] = av->is_tagged ? '*' : ' ';
      buf[1] = '\0';
      break;
  }

  return src;
}

/**
 * alias_make_entry - Format a menu item for the alias list - Implements Menu::make_entry() - @ingroup menu_make_entry
 */
static void alias_make_entry(struct Menu *menu, char *buf, size_t buflen, int line)
{
  const struct AliasMenuData *mdata = menu->mdata;
  const struct AliasViewArray *ava = &mdata->ava;
  const struct AliasView *av = ARRAY_GET(ava, line);

  const char *const alias_format = cs_subset_string(mdata->sub, "alias_format");

  mutt_expando_format(buf, buflen, 0, menu->win->state.cols, NONULL(alias_format),
                      alias_format_str, (intptr_t) av, MUTT_FORMAT_ARROWCURSOR);
}

/**
 * alias_tag - Tag some aliases - Implements Menu::tag() - @ingroup menu_tag
 */
static int alias_tag(struct Menu *menu, int sel, int act)
{
  const struct AliasMenuData *mdata = menu->mdata;
  const struct AliasViewArray *ava = &mdata->ava;
  struct AliasView *av = ARRAY_GET(ava, sel);

  bool ot = av->is_tagged;

  av->is_tagged = ((act >= 0) ? act : !av->is_tagged);

  return av->is_tagged - ot;
}

/**
 * alias_alias_observer - Notification that an Alias has changed - Implements ::observer_t - @ingroup observer_api
 */
static int alias_alias_observer(struct NotifyCallback *nc)
{
  if ((nc->event_type != NT_ALIAS) || !nc->global_data || !nc->event_data)
    return -1;

  struct EventAlias *ev_a = nc->event_data;
  struct Menu *menu = nc->global_data;
  struct AliasMenuData *mdata = menu->mdata;
  struct Alias *alias = ev_a->alias;

  if (nc->event_subtype == NT_ALIAS_ADD)
  {
    alias_array_alias_add(&mdata->ava, alias);

    if (alias_array_count_visible(&mdata->ava) != ARRAY_SIZE(&mdata->ava))
    {
      mutt_pattern_alias_func(NULL, mdata, menu);
    }
  }
  else if (nc->event_subtype == NT_ALIAS_DELETE)
  {
    alias_array_alias_delete(&mdata->ava, alias);

    int vcount = alias_array_count_visible(&mdata->ava);
    int index = menu_get_index(menu);
    if ((index > (vcount - 1)) && (index > 0))
      menu_set_index(menu, index - 1);
  }

  alias_array_sort(&mdata->ava, mdata->sub);

  menu->max = alias_array_count_visible(&mdata->ava);
  menu_queue_redraw(menu, MENU_REDRAW_FULL);
  mutt_debug(LL_DEBUG5, "alias done, request WA_RECALC, MENU_REDRAW_FULL\n");

  return 0;
}

/**
 * alias_window_observer - Notification that a Window has changed - Implements ::observer_t - @ingroup observer_api
 *
 * This function is triggered by changes to the windows.
 *
 * - Delete (this window): clean up the resources held by the Help Bar
 */
int alias_window_observer(struct NotifyCallback *nc)
{
  if ((nc->event_type != NT_WINDOW) || !nc->global_data || !nc->event_data)
    return -1;

  if (nc->event_subtype != NT_WINDOW_DELETE)
    return 0;

  struct MuttWindow *win_menu = nc->global_data;
  struct EventWindow *ev_w = nc->event_data;
  if (ev_w->win != win_menu)
    return 0;

  struct Menu *menu = win_menu->wdata;

  notify_observer_remove(NeoMutt->notify, alias_alias_observer, menu);
  notify_observer_remove(NeoMutt->notify, alias_config_observer, menu);
  notify_observer_remove(win_menu->notify, alias_window_observer, win_menu);

  mutt_debug(LL_DEBUG5, "window delete done\n");
  return 0;
}

/**
 * alias_dialog_new - Create an Alias Selection Dialog
 * @param mdata Menu data holding Aliases
 * @retval ptr New Dialog
 */
struct MuttWindow *alias_dialog_new(struct AliasMenuData *mdata)
{
  struct MuttWindow *dlg = simple_dialog_new(MENU_ALIAS, WT_DLG_ALIAS, AliasHelp);

  struct Menu *menu = dlg->wdata;
  menu->make_entry = alias_make_entry;
  menu->custom_search = true;
  menu->tag = alias_tag;
  menu->max = alias_array_count_visible(&mdata->ava);
  menu->mdata = mdata;

  struct MuttWindow *win_menu = menu->win;

  // Override the Simple Dialog's recalc()
  win_menu->recalc = alias_recalc;

  struct MuttWindow *sbar = window_find_child(dlg, WT_STATUS_BAR);

  alias_set_title(sbar, _("Aliases"), mdata->str);

  // NT_COLOR is handled by the SimpleDialog
  notify_observer_add(NeoMutt->notify, NT_ALIAS, alias_alias_observer, menu);
  notify_observer_add(NeoMutt->notify, NT_CONFIG, alias_config_observer, menu);
  notify_observer_add(win_menu->notify, NT_WINDOW, alias_window_observer, win_menu);

  return dlg;
}

/**
 * dlg_select_alias - Display a menu of Aliases
 * @param buf    Buffer for expanded aliases
 * @param buflen Length of buffer
 * @param mdata  Menu data holding Aliases
 */
static void dlg_select_alias(char *buf, size_t buflen, struct AliasMenuData *mdata)
{
  if (ARRAY_EMPTY(&mdata->ava))
  {
    mutt_warning(_("You have no aliases"));
    return;
  }

  struct MuttWindow *dlg = alias_dialog_new(mdata);
  struct Menu *menu = dlg->wdata;
  struct MuttWindow *sbar = window_find_child(dlg, WT_STATUS_BAR);

  int t = -1;
  bool done = false;

  alias_array_sort(&mdata->ava, mdata->sub);

  struct AliasView *avp = NULL;
  ARRAY_FOREACH(avp, &mdata->ava)
  {
    avp->num = ARRAY_FOREACH_IDX;
  }

  while (!done)
  {
    int op = menu_loop(menu);
    switch (op)
    {
      case OP_DELETE:
      case OP_UNDELETE:
        if (menu->tagprefix)
        {
          ARRAY_FOREACH(avp, &mdata->ava)
          {
            if (avp->is_tagged)
              avp->is_deleted = (op == OP_DELETE);
          }
          menu_queue_redraw(menu, MENU_REDRAW_INDEX);
        }
        else
        {
          int index = menu_get_index(menu);
          ARRAY_GET(&mdata->ava, index)->is_deleted = (op == OP_DELETE);
          menu_queue_redraw(menu, MENU_REDRAW_CURRENT);
          const bool c_resolve = cs_subset_bool(NeoMutt->sub, "resolve");
          if (c_resolve && (index < (menu->max - 1)))
          {
            menu_set_index(menu, index + 1);
            menu_queue_redraw(menu, MENU_REDRAW_INDEX);
          }
        }
        break;
      case OP_SORT:
      case OP_SORT_REVERSE:
      {
        int sort = cs_subset_sort(mdata->sub, "sort_alias");
        bool resort = true;
        bool reverse = (op == OP_SORT_REVERSE);

        switch (mutt_multi_choice(
            reverse ?
                /* L10N: The highlighted letters must match the "Sort" options */
                _("Rev-Sort (a)lias, a(d)dress or (u)nsorted?") :
                /* L10N: The highlighted letters must match the "Rev-Sort" options */
                _("Sort (a)lias, a(d)dress or (u)nsorted?"),
            /* L10N: These must match the highlighted letters from "Sort" and "Rev-Sort" */
            _("adu")))
        {
          case -1: /* abort */
            resort = false;
            break;

          case 1: /* (a)lias */
            sort = SORT_ALIAS;
            break;

          case 2: /* a(d)dress */
            sort = SORT_ADDRESS;
            break;

          case 3: /* (u)nsorted */
            sort = SORT_ORDER;
            break;
        }

        if (resort)
        {
          sort |= reverse ? SORT_REVERSE : 0;

          // This will trigger a WA_RECALC
          cs_subset_str_native_set(mdata->sub, "sort_alias", sort, NULL);
        }

        break;
      }
      case OP_SEARCH_REVERSE:
      case OP_SEARCH_NEXT:
      case OP_SEARCH_OPPOSITE:
      case OP_SEARCH:
      {
        int index = mutt_search_alias_command(menu, menu_get_index(menu), op);
        if (index == -1)
          break;

        menu_set_index(menu, index);
        break;
      }

      case OP_MAIN_LIMIT:
      {
        int rc = mutt_pattern_alias_func(_("Limit to addresses matching: "), mdata, menu);
        if (rc == 0)
        {
          alias_set_title(sbar, _("Aliases"), mdata->str);
          menu_queue_redraw(menu, MENU_REDRAW_FULL);
          window_redraw(NULL);
        }

        break;
      }

      case OP_GENERIC_SELECT_ENTRY:
        t = menu_get_index(menu);
        if (t >= ARRAY_SIZE(&mdata->ava))
          t = -1;
        done = true;
        break;

      case OP_EXIT:
        done = true;
        break;
    }
  }

  ARRAY_FOREACH(avp, &mdata->ava)
  {
    if (avp->is_tagged)
    {
      mutt_addrlist_write(&avp->alias->addr, buf, buflen, true);
      t = -1;
    }
  }

  if (t != -1)
  {
    mutt_addrlist_write(&ARRAY_GET(&mdata->ava, t)->alias->addr, buf, buflen, true);
  }

  simple_dialog_free(&dlg);
}

/**
 * alias_complete - Alias completion routine
 * @param buf    Partial Alias to complete
 * @param buflen Length of buffer
 * @param sub    Config items
 * @retval 1 Success
 * @retval 0 Error
 *
 * Given a partial alias, this routine attempts to fill in the alias
 * from the alias list as much as possible. if given empty search string
 * or found nothing, present all aliases
 */
int alias_complete(char *buf, size_t buflen, struct ConfigSubset *sub)
{
  struct Alias *np = NULL;
  char bestname[8192] = { 0 };

  struct AliasMenuData mdata = { NULL, ARRAY_HEAD_INITIALIZER, sub };
  mdata.str = mutt_str_dup(buf);

  if (buf[0] != '\0')
  {
    TAILQ_FOREACH(np, &Aliases, entries)
    {
      if (np->name && mutt_strn_equal(np->name, buf, strlen(buf)))
      {
        if (bestname[0] == '\0') /* init */
        {
          mutt_str_copy(bestname, np->name,
                        MIN(mutt_str_len(np->name) + 1, sizeof(bestname)));
        }
        else
        {
          int i;
          for (i = 0; np->name[i] && (np->name[i] == bestname[i]); i++)
            ; // do nothing

          bestname[i] = '\0';
        }
      }
    }

    if (bestname[0] != '\0')
    {
      /* fake the pattern for menu title */
      char *mtitle = NULL;
      mutt_str_asprintf(&mtitle, "~f ^%s", buf);
      FREE(&mdata.str);
      mdata.str = mtitle;

      if (!mutt_str_equal(bestname, buf))
      {
        /* we are adding something to the completion */
        mutt_str_copy(buf, bestname, mutt_str_len(bestname) + 1);
        FREE(&mdata.str);
        return 1;
      }

      /* build alias list and show it */
      TAILQ_FOREACH(np, &Aliases, entries)
      {
        int aasize = alias_array_alias_add(&mdata.ava, np);

        struct AliasView *av = ARRAY_GET(&mdata.ava, aasize - 1);

        if (np->name && !mutt_strn_equal(np->name, buf, strlen(buf)))
        {
          av->is_visible = false;
        }
      }
    }
  }

  if (ARRAY_EMPTY(&mdata.ava))
  {
    TAILQ_FOREACH(np, &Aliases, entries)
    {
      alias_array_alias_add(&mdata.ava, np);
    }

    mutt_pattern_alias_func(NULL, &mdata, NULL);
  }

  alias_array_sort(&mdata.ava, mdata.sub);

  bestname[0] = '\0';
  dlg_select_alias(bestname, sizeof(bestname), &mdata);
  if (bestname[0] != '\0')
    mutt_str_copy(buf, bestname, buflen);

  struct AliasView *avp = NULL;
  ARRAY_FOREACH(avp, &mdata.ava)
  {
    if (!avp->is_deleted)
      continue;

    TAILQ_REMOVE(&Aliases, avp->alias, entries);
    alias_free(&avp->alias);
  }

  ARRAY_FREE(&mdata.ava);
  FREE(&mdata.str);

  return 0;
}
