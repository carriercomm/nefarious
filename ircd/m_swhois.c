/*
 * IRC - Internet Relay Chat, ircd/m_swhois.c
 * Copyright (C) 1990 Jarkko Oikarinen and
 *                    University of Oulu, Computing Center
 *
 * See file AUTHORS in IRC package for additional names of
 * the programmers.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
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

#include "client.h"
#include "ircd.h"
#include "ircd_alloc.h"
#include "ircd_features.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_user.h"
#include "send.h"

#include <assert.h>
#include <string.h>

/*
 * user_set_swhois - set user swhois state
 * returns 1 if client has swhois or swhois is changing.
 * returns 0 if client's swhois is being removed.
 * NOTE: this function may modify user and message, so they
 * must be mutable.
 */
static int user_set_swhois(struct User* user, char* message)
{
  char* swhois;
  assert(0 != user);

  swhois = user->swhois;

  if (EmptyString(message)) {
    /*
     * Marking as not swhois
     */
    if (swhois) {
      MyFree(swhois);
      user->swhois = 0;
    }
  }
  else {
    /*
     * Marking as having a swhois
     */
    unsigned int len = strlen(message);

    if (len > AWAYLEN) {
      message[AWAYLEN] = '\0';
      len = AWAYLEN;
    }
    if (swhois)
      swhois = (char*) MyRealloc(swhois, len + 1);
    else
      swhois = (char*) MyMalloc(len + 1);
    assert(0 != swhois);

    user->swhois = swhois;
    strcpy(swhois, message);
  }
  return (user->swhois != 0);
}

/*
 * ms_swhois - server message handler
 *
 * parv[0] = sender prefix
 * parv[1] = numeric of client to act on
 * parv[2] = swhois message
 */
int ms_swhois(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr;
  char* swhois = parv[2];

  assert(0 != cptr);
  assert(0 != sptr);

  if (!feature_bool(FEAT_SWHOIS))
    return 0;

  if (parc < 3) {
    protocol_violation(sptr, "Too few arguments for SWHOIS");
    return need_more_params(sptr, "SWHOIS");
  }

  if (!(acptr = findNUser(parv[1])))
    return 0;

  if (IsChannelService(acptr))
    return 0;

  if (user_set_swhois(cli_user(acptr), swhois))
    sendcmdto_serv_butone(acptr, CMD_SWHOIS, acptr, ":%s", swhois);
  else
    sendcmdto_serv_butone(acptr, CMD_SWHOIS, acptr, "");
  return 0;
}
