/*
 * IRC - Internet Relay Chat, ircd/m_svsjoin.c
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
#include "handlers.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_chattr.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_debug.h"
#include "s_user.h"
#include "send.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>


/*
 * Helper function to perform a JOIN 0 if needed; returns 0 if channel
 * name is not 0, else removes user from all channels and returns 1.
 */
static int
join0(struct JoinBuf *join, struct Client *cptr, struct Client *sptr,
      char *chan)
{
  struct Membership *member;
  struct JoinBuf part;

  /* is it a JOIN 0? */
  if (chan[0] != '0' || chan[1] != '\0')
    return 0;

  joinbuf_join(join, 0, 0); /* join special channel 0 */

  /* leave all channels */
  joinbuf_init(&part, sptr, cptr, JOINBUF_TYPE_PARTALL,
               "Left all channels", 0);

  while ((member = cli_user(sptr)->channel))
    joinbuf_join(&part, member->channel,
                 IsZombie(member) ? CHFL_ZOMBIE : 0);

  joinbuf_flush(&part);

  return 1;
}

/*
 * ms_svsjoin - server message handler
 *
 * parv[0] = sender prefix
 * parv[1] = numeric of client to act on
 * parv[2] = channel to force client to join
 */
int ms_svsjoin(struct Client* cptr, struct Client* sptr, int parc, char* parv[]) {
  struct Client *acptr = NULL;
  struct Channel *chptr = NULL;
  struct JoinBuf join;
  struct JoinBuf create;
  unsigned int flags = 0;
  char *name;
  int automodes = 0;

  /* this could be done with hunt_server_cmd but its a bucket of shit */

  if(!(acptr = findNUser(parv[1])))
    return 0;

  if (0 == ircd_strcmp(cli_name(acptr->cli_user->server), cli_name(&me))) {

    if (parc < 3) {
      protocol_violation(sptr, "Too few arguments for SVSJOIN");
      return need_more_params(sptr, "SVSJOIN");
    }

    if (IsChannelService(acptr))
      return 0;

    if ((chptr = FindChannel(parv[2]))) {
      flags = CHFL_DEOPPED;
      if (find_member_link(chptr, acptr))
        return 0;
    } else
      flags = CHFL_CHANOP;

    joinbuf_init(&join, acptr, acptr, JOINBUF_TYPE_JOIN, 0, 0);  
    joinbuf_init(&create, acptr, acptr, JOINBUF_TYPE_CREATE, 0, TStime());

    if (join0(&join, acptr, acptr, parv[2])) { /* did client do a JOIN 0? */
      joinbuf_flush(&join); /* must be first, if there's a JOIN 0 */
      joinbuf_flush(&create);
      return 0;
    }

    chptr = get_channel(acptr, parv[2],
		        (!FindChannel(parv[2])) ? CGT_CREATE :
		        CGT_NO_CREATE);

    name = chptr->chname;
    clean_channelname(name);

    /* bad channel name */
    if ((!IsChannelName(name)) || (HasCntrl(name)))
      return 0;

    if (chptr)
      joinbuf_join(&join, chptr, flags);
    else {
      if (!MyUser(acptr)) {
        sendcmdto_serv_butone(sptr, CMD_SVSJOIN, cptr, "%s%s %s", acptr->cli_user->server->cli_yxx, acptr->cli_yxx, parv[2]);
        return 0;
      }
      return 0;
      joinbuf_join(&create, chptr, flags);
      if (feature_bool(FEAT_AUTOCHANMODES) &&
          feature_str(FEAT_AUTOCHANMODES_LIST) &&
          !IsLocalChannel(chptr->chname) &&
          strlen(feature_str(FEAT_AUTOCHANMODES_LIST)) > 0) {
        SetAutoChanModes(chptr);
        automodes = 1;
      }
    }

    if (chptr->topic[0]) {
      send_reply(acptr, RPL_TOPIC, chptr->chname, chptr->topic);
      send_reply(acptr, RPL_TOPICWHOTIME, chptr->chname,
  	         chptr->topic_nick, chptr->topic_time);
    }

    do_names(acptr, chptr, NAMES_ALL|NAMES_EON); /* send /names list */

    joinbuf_flush(&join); /* must be first, if there's a JOIN 0 */  
    joinbuf_flush(&create);

    if (automodes && chptr)
      sendcmdto_serv_butone(&me, CMD_MODE, sptr,
                            "%H +%s", chptr, feature_str(FEAT_AUTOCHANMODES_LIST));
  }

  sendcmdto_serv_butone(sptr, CMD_SVSJOIN, cptr, "%s%s %s", acptr->cli_user->server->cli_yxx, acptr->cli_yxx, parv[2]);
  return 0;
}
