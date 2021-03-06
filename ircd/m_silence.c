/*
 * IRC - Internet Relay Chat, ircd/m_silence.c
 * Copyright (C) 1990 Jarkko Oikarinen and
 *                    University of Oulu, Computing Center
 *
 * See file AUTHORS in IRC package for additional names of
 * the programmers.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

/*
 * m_functions execute protocol messages on this server:
 *
 *    cptr    is always NON-NULL, pointing to a *LOCAL* client
 *            structure (with an open socket connected!). This
 *            identifies the physical socket where the message
 *            originated (or which caused the m_function to be
 *            executed--some m_functions may call others...).
 *
 *    sptr    is the source of the message, defined by the
 *            prefix part of the message if present. If not
 *            or prefix not found, then sptr==cptr.
 *
 *            (!IsServer(cptr)) => (cptr == sptr), because
 *            prefixes are taken *only* from servers...
 *
 *            (IsServer(cptr))
 *                    (sptr == cptr) => the message didn't
 *                    have the prefix.
 *
 *                    (sptr != cptr && IsServer(sptr) means
 *                    the prefix specified servername. (?)
 *
 *                    (sptr != cptr && !IsServer(sptr) means
 *                    that message originated from a remote
 *                    user (not local).
 *
 *            combining
 *
 *            (!IsServer(sptr)) means that, sptr can safely
 *            taken as defining the target structure of the
 *            message in this server.
 *
 *    *Always* true (if 'parse' and others are working correct):
 *
 *    1)      sptr->from == cptr  (note: cptr->from == cptr)
 *
 *    2)      MyConnect(sptr) <=> sptr == cptr (e.g. sptr
 *            *cannot* be a local connection, unless it's
 *            actually cptr!). [MyConnect(x) should probably
 *            be defined as (x == x->from) --msa ]
 *
 *    parc    number of variable parameter strings (if zero,
 *            parv is allowed to be NULL)
 *
 *    parv    a NULL terminated list of parameter pointers,
 *
 *                    parv[0], sender (prefix string), if not present
 *                            this points to an empty string.
 *                    parv[1]...parv[parc-1]
 *                            pointers to additional parameters
 *                    parv[parc] == NULL, *always*
 *
 *            note:   it is guaranteed that parv[0]..parv[parc-1] are all
 *                    non-NULL pointers.
 */
#include "config.h"

#include "channel.h"
#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "list.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_user.h"
#include "s_debug.h"
#include "send.h"
#include "ircd_struct.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <stdlib.h>
#include <string.h>

/*
 * m_silence - generic message handler
 *
 *   parv[0] = sender prefix
 * From local client:
 *   parv[1] = mask (NULL sends the list)
 * From remote client:
 *   parv[1] = Numeric nick that must be silenced
 *   parv[2] = mask
 *
 * XXX - ugh 
 */
int m_silence(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct SLink*  lp;
  struct Client* acptr;
  char           c,d;
  char*          cp;
  int            exempt = 0;

  assert(0 != cptr);
  assert(cptr == sptr);

  acptr = sptr;

  if (parc < 2 || EmptyString(parv[1]) || (acptr = FindUser(parv[1]))) {
    if (cli_user(acptr) && ((acptr == sptr) || IsChannelService(acptr))) {
      for (lp = cli_user(acptr)->silence; lp; lp = lp->next) {
        send_reply(sptr, RPL_SILELIST, cli_name(acptr),
                   (lp->flags & SILENCE_EXEMPT) ? "~" : "", lp->value.cp);
      }
    }
    send_reply(sptr, RPL_ENDOFSILELIST, cli_name(acptr));
    return 0;
  }
  cp = parv[1];
  c = *cp;
  if (c == '-' || c == '+')
    cp++;
  else if (!(strchr(cp, '@') || strchr(cp, '.') || strchr(cp, '!') || strchr(cp, '*'))) {
    return send_reply(sptr, ERR_NOSUCHNICK, parv[1]);
  }
  else
    c = '+';

  d = *cp;
  if (d == '~') {
    exempt = 1;
    cp++;
  }

  cp = pretty_mask(cp);
  Debug((DEBUG_DEBUG, "test: %s %d", cp, exempt));
  if ((c == '-' && !del_silence(sptr, cp, exempt)) || (c != '-' && !add_silence(sptr, cp, exempt))) {
    sendcmdto_one(sptr, CMD_SILENCE, sptr, "%c%s%s", c, (d == '~') ? "~" : "", cp);
    sendcmdto_serv_butone(sptr, CMD_SILENCE, cptr, "%C %s%s%s", cptr, (c == '-') ? "-" : "+", (d == '~') ? "~" : "", cp);
  }
  return 0;
}

/*
 * ms_silence - server message handler
 *
 *   parv[0] = sender prefix
 * From local client:
 *   parv[1] = mask (NULL sends the list)
 * From remote client:
 *   parv[1] = Numeric nick that must be silenced
 *   parv[2] = mask
 */
int ms_silence(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client* acptr;
  char* mask;
  int exempt = 0;

  if (parc < 3 || EmptyString(parv[2]))
    return need_more_params(sptr, "SILENCE");

  acptr = findNUser(parv[1]);

  if (!acptr)
    return 0; /* Ignore silences for a user that has quit */

  mask = parv[2];
  if (*mask == '-') {
    mask++;
    if (*mask == '~') {
      exempt = 1;
      mask++;
    }
    del_silence(acptr, mask, exempt);
  } else {
    mask++;
    if (*mask == '~') {
      exempt = 1;
      mask++;
    }
    add_silence(acptr, mask, exempt);
  }

  if (MyUser(acptr))
    sendcmdto_one(acptr, CMD_SILENCE, acptr, "%s", parv[2]);

  sendcmdto_serv_butone(sptr, CMD_SILENCE, cptr, "%C %s", acptr, parv[2]);

  return 0;
}
