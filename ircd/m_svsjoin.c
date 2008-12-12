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
#include "gline.h"
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

static int bouncedtimes = 0;
/*
 * Helper function to find last 0 in a comma-separated list of
 * channel names.
 */
static char *
last0(char *chanlist)
{
  char *p;

  for (p = chanlist; p[0]; p++) /* find last "JOIN 0" */
    if (p[0] == '0' && (p[1] == ',' || p[1] == '\0' || !IsChannelChar(p[1]))) {
      chanlist = p; /* we'll start parsing here */

      if (!p[1]) /* hit the end */
        break;

      p++;
    } else {
      while (p[0] != ',' && p[0] != '\0') /* skip past channel name */
        p++;

      if (!p[0]) /* hit the end */
        break;
    }

  return chanlist;
}

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

int do_svsjoin(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  struct Channel *chptr = NULL;
  struct JoinBuf join;
  struct JoinBuf create;
  struct Gline *gline;
  unsigned int flags = 0;
  int automodes = 0;
  char* bjoin[2];
  char *p = 0;
  char *chanlist;
  char *name;

#define RET { bouncedtimes--; continue; }

  if (parc < 2 || *parv[1] == '\0')
    return need_more_params(sptr, "JOIN");

  joinbuf_init(&join, sptr, cptr, JOINBUF_TYPE_JOIN, 0, 0);
  joinbuf_init(&create, sptr, cptr, JOINBUF_TYPE_CREATE, 0, TStime());

  chanlist = last0(parv[1]); /* find last "JOIN 0" */

  for (name = ircd_strtok(&p, chanlist, ","); name;
       name = ircd_strtok(&p, 0, ",")) {
    clean_channelname(name);

    if (join0(&join, cptr, sptr, name)) /* did client do a JOIN 0? */
      continue;

    bouncedtimes++;
    /* don't use 'return x;' but 'RET' from here ;p */

    if (bouncedtimes > feature_int(FEAT_MAX_BOUNCE))
    {
      sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :*** Couldn't join %s ! - Link setting was too bouncy", sptr, name);
      RET
    }

    /* bad channel name */
    if ((!IsChannelName(name)) || (HasCntrl(name))) {
      send_reply(sptr, ERR_NOSUCHCHANNEL, name);
      continue;
    }

    /* BADCHANed channel */
    if ((gline = gline_find(name, GLINE_BADCHAN)) &&
        GlineIsActive(gline) && !IsAnOper(sptr)) {
      send_reply(sptr, ERR_BADCHANNAME, name, gline->gl_reason);
      continue;
    }

    if ((chptr = FindChannel(name))) {
      if (find_member_link(chptr, sptr))
        continue; /* already on channel */

      flags = CHFL_DEOPPED;
    }
    else
      flags = CHFL_CHANOP;

    /* disallow creating local channels */
    if (IsLocalChannel(name) && !chptr) {
      if (IsAnOper(sptr)) {
        if (!feature_bool(FEAT_OPER_LOCAL_CHANNELS)) {
          send_reply(sptr, ERR_NOSUCHCHANNEL, name);
          continue;
        }
      } else {
        if (!feature_bool(FEAT_LOCAL_CHANNELS)) {
          send_reply(sptr, ERR_NOSUCHCHANNEL, name);
          continue;
        }
      }
    }

    if (chptr) {
      if (chptr->mode.redirect && (*chptr->mode.redirect != '\0')) {
        send_reply(sptr, ERR_LINKSET, sptr->cli_name, chptr->chname, chptr->mode.redirect);
        bjoin[0] = cli_name(sptr);
        bjoin[1] = chptr->mode.redirect;
        do_join(cptr, sptr, 2, bjoin);
        continue;
      }

      if (check_target_limit(sptr, chptr, chptr->chname, 0))
        continue; /* exceeded target limit */

      joinbuf_join(&join, chptr, flags);
    } else if (!(chptr = get_channel(sptr, name, CGT_CREATE))) {
      continue; /* couldn't get channel */
    } else {
      joinbuf_join(&create, chptr, flags);
      if (feature_bool(FEAT_AUTOCHANMODES) &&
          feature_str(FEAT_AUTOCHANMODES_LIST) &&
          !IsLocalChannel(chptr->chname) &&
          strlen(feature_str(FEAT_AUTOCHANMODES_LIST)) > 0) {
        SetAutoChanModes(chptr);
        automodes = 1;
      }
    }

    del_invite(sptr, chptr);

    if (chptr->topic[0]) {
      send_reply(sptr, RPL_TOPIC, chptr->chname, chptr->topic);
      send_reply(sptr, RPL_TOPICWHOTIME, chptr->chname, chptr->topic_nick,
                 chptr->topic_time);
    }

    do_names(sptr, chptr, NAMES_ALL|NAMES_EON); /* send /names list */
/*
 *    if (((chptr->mode.mode & MODE_ACCONLY) && !IsAccount(sptr)) ||
 *      ((chptr->mode.mode & MODE_MODERATED) &&
 *      (flex == 1) && feature_bool(FEAT_FLEXABLEKEYS))) {
 *      sendcmdto_one(&me, CMD_MODE, sptr, "%H +v %C", chptr, sptr);
 *      sendcmdto_serv_butone(&me, CMD_MODE, sptr, "%H +v %C", chptr, sptr);
 *      sendcmdto_channel_butserv_butone(&me, CMD_MODE, chptr, cptr, 0,
 *                                     "%H +v %C", chptr, sptr);
 *    }
 */
  }

  joinbuf_flush(&join); /* must be first, if there's a JOIN 0 */
  joinbuf_flush(&create);

  if (automodes && chptr)
    sendcmdto_serv_butone(&me, CMD_MODE, sptr,
                          "%H +%s", chptr, feature_str(FEAT_AUTOCHANMODES_LIST));

  return 0;
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

  /* this could be done with hunt_server_cmd but its a bucket of shit */

  if(!(acptr = findNUser(parv[1]))) {
    if (!(acptr = FindUser(parv[1]))) {
      return 0; /* Ignore SVSNICK for a user that has quit */
    }
  }

  if (0 == ircd_strcmp(cli_name(acptr->cli_user->server), cli_name(&me))) {
    char* join[2];

    if (parc < 3) {
      protocol_violation(sptr, "Too few arguments for SVSJOIN");
      return need_more_params(sptr, "SVSJOIN");
    }

    join[0] = cli_name(acptr);
    join[1] = parv[2];

    do_svsjoin(acptr, acptr, 2, join);
  }

  sendcmdto_serv_butone(sptr, CMD_SVSJOIN, cptr, "%s%s %s", acptr->cli_user->server->cli_yxx, acptr->cli_yxx, parv[2]);
  return 0;
}
