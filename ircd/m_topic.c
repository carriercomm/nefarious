/*
 * IRC - Internet Relay Chat, ircd/m_topic.c
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

#include "channel.h"
#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_features.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "send.h"

#include <assert.h>
#include <stdlib.h>

static void do_settopic(struct Client *sptr, struct Client *cptr, 
		        struct Channel *chptr, char *topic, time_t ts)
{
   int newtopic;
   struct Client *from;

   if (feature_bool(FEAT_HIS_BANWHO) && IsServer(sptr)) {
      from = &me;
   }
   else {
      from = sptr;
   }

   /* Note if this is just a refresh of an old topic, and don't
    * send it to all the clients to save bandwidth.  We still send
    * it to other servers as they may have split and lost the topic.
    */
   newtopic=ircd_strncmp(chptr->topic,topic,TOPICLEN)!=0;
   /* setting a topic */
   ircd_strncpy(chptr->topic, topic, TOPICLEN);
   ircd_strncpy(chptr->topic_nick, cli_name(from), NICKLEN);
   chptr->topic_time = ts ? ts : CurrentTime;
   /* Fixed in 2.10.11: Don't propergate local topics */
   if (!IsLocalChannel(chptr->chname))
     sendcmdto_serv_butone(sptr, CMD_TOPIC, cptr, "%H %Tu :%s", chptr,
                           chptr->topic_time, chptr->topic);
   if (newtopic)
      sendcmdto_channel_butserv_butone(from, CMD_TOPIC, chptr, NULL,
                                       "%H :%s", chptr, chptr->topic);
      /* if this is the same topic as before we send it to the person that
       * set it (so they knew it went through ok), but don't bother sending
       * it to everyone else on the channel to save bandwidth
       */
    else if (MyUser(sptr))
      sendcmdto_one(sptr, CMD_TOPIC, sptr, "%H :%s", chptr, chptr->topic);   	
}

/*
 * m_topic - generic message handler
 *
 * parv[0]        = sender prefix
 * parv[1]        = channel
 * parv[parc - 1] = topic (if parc > 2)
 */
int m_topic(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Channel *chptr;
  char *topic = 0, *name, *p = 0;
  struct Membership *member;
  
  if (parc < 2)
    return need_more_params(sptr, "TOPIC");

  if (parc > 2)
    topic = parv[parc - 1];

  for (; (name = ircd_strtok(&p, parv[1], ",")); parv[1] = 0)
  {
    chptr = 0;
    /* Does the channel exist */
    if (!IsChannelName(name) || !(chptr = FindChannel(name)))
    {
    	send_reply(sptr, ERR_NOSUCHCHANNEL, name);
    	continue;
    }
    member=find_channel_member(sptr, chptr);
    /* Trying to check a topic outside a secret channel */
    if ((topic || SecretChannel(chptr)) && !member)
    {
      send_reply(sptr, ERR_NOTONCHANNEL, chptr->chname);
      continue;
    }

    if (!topic)                 /* only asking for topic */
    {
      if (chptr->topic[0] == '\0')
	send_reply(sptr, RPL_NOTOPIC, chptr->chname);
      else
      {
	send_reply(sptr, RPL_TOPIC, chptr->chname, chptr->topic);
	send_reply(sptr, RPL_TOPICWHOTIME, chptr->chname, chptr->topic_nick,
		   chptr->topic_time);
      }
    }
    else
    {
      /* if +t and not @'d, return an error and ignore the topic */
      /* Note that "member" must be non-NULL here due to the checks above */
      if ((chptr->mode.mode & MODE_TOPICLIMIT) && !IsChanOp(member) &&
	  !IsChannelService(member))
      {
        send_reply(sptr, ERR_CHANOPRIVSNEEDED, chptr->chname);
        return 0;
      }
      /* If a delayed join member is changing the topic, reveal them */
      if (member && IsDelayedJoin(member)) {
        ClearDelayedJoin(member);
        sendcmdto_channel_butserv_butone(member->user, CMD_JOIN, member->channel,
                                         member->user, ":%H", member->channel);
      }
      do_settopic(sptr,cptr,chptr,topic,0);
    }
  }
  return 0;
}

/*
 * ms_topic - server message handler
 *
 * parv[0]        = sender prefix
 * parv[1]        = channel
 * parv[parc - 2] = timestamp (optional)
 * parv[parc - 1] = topic
 */
int ms_topic(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Channel *chptr;
  struct Membership *member;
  char *topic = 0, *name, *p = 0;
  time_t ts = 0;

  if (parc < 3)
    return need_more_params(sptr, "TOPIC");

  topic = parv[parc - 1];

  for (; (name = ircd_strtok(&p, parv[1], ",")); parv[1] = 0)
  {
    chptr = 0;
    /* Does the channel exist */
    if (!IsChannelName(name) || !(chptr = FindChannel(name)))
    {
    	send_reply(sptr, ERR_NOSUCHCHANNEL, name);
    	continue;
    }

    /* Ignore requests for local channel topics from remote servers */
    if (IsLocalChannel(name) && !MyUser(sptr))
    {
      protocol_violation(sptr, "Topic request");
      continue;
    }

    if (parc > 3 && (ts = atoi(parv[parc - 2])) && chptr->topic_time > ts)
      continue;
    
    /* If it's being set by a delayed join member, send the join */
    if ((member=find_channel_member(sptr,chptr)) && IsDelayedJoin(member)) {
      ClearDelayedJoin(member);
      sendcmdto_channel_butserv_butone(member->user, CMD_JOIN, member->channel,
                                         member->user, ":%H", member->channel);
    }

    do_settopic(sptr,cptr,chptr,topic,ts);
  }
  return 0;
}
