/*
 * IRC - Internet Relay Chat, ircd/gline.c
 * Copyright (C) 1990 Jarkko Oikarinen and
 *                    University of Oulu, Finland
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
#include "config.h"

#include "gline.h"
#include "channel.h"
#include "client.h"
#include "ircd.h"
#include "ircd_alloc.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_snprintf.h"
#include "ircd_string.h"
#include "match.h"
#include "numeric.h"
#include "s_bsd.h"
#include "s_debug.h"
#include "s_misc.h"
#include "s_stats.h"
#include "send.h"
#include "struct.h"
#include "support.h"
#include "msg.h"
#include "numnicks.h"
#include "numeric.h"
#include "sys.h"    /* FALSE bleah */
#include "whocmds.h"
#include "hash.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h> /* for inet_ntoa */

#define CHECK_APPROVED	   0	/* Mask is acceptable */
#define CHECK_OVERRIDABLE  1	/* Mask is acceptable, but not by default */
#define CHECK_REJECTED	   2	/* Mask is totally unacceptable */

#define MASK_WILD_0	0x01	/* Wildcards in the last position */
#define MASK_WILD_1	0x02	/* Wildcards in the next-to-last position */

#define MASK_WILD_MASK	0x03	/* Mask out the positional wildcards */

#define MASK_WILDS	0x10	/* Mask contains wildcards */
#define MASK_IP		0x20	/* Mask is an IP address */
#define MASK_HALT	0x40	/* Finished processing mask */

struct Gline* GlobalGlineList  = 0;
struct Gline* BadChanGlineList = 0;

static void
#ifdef NICKGLINES
canon_userhost(char *userhost, char **nick_p, char **user_p, char **host_p, char *def_user)
#else
canon_userhost(char *userhost, char **user_p, char **host_p, char *def_user)
#endif
{
#ifdef NICKGLINES
  char *tmp, *s;
#else
  char *tmp;
#endif

  if (*userhost == '$') {
    *user_p = userhost;
    *host_p = NULL;
    return;
  }

#ifdef NICKGLINES
  if ((tmp = strchr(userhost, '!'))) {
    *nick_p = userhost;
#else
  if (!(tmp = strchr(userhost, '@'))) {
    *user_p = def_user;
    *host_p = userhost;
  } else {
    *user_p = userhost;
#endif
    *(tmp++) = '\0';
#ifdef NICKGLINES
  } else {
    *nick_p = def_user;
    tmp = userhost;
  }

  if (!(s = strchr(tmp, '@'))) {
    *user_p = def_user;
#endif
    *host_p = tmp;
#ifdef NICKGLINES
 } else {
    *user_p = tmp;
    *(s++) = '\0';
    *host_p = s;
#endif
  }
}

#ifdef NICKGLINES
static struct Gline *
make_gline(char *nick, char *user, char *host, char *reason, time_t expire,
	   time_t lastmod, unsigned int flags)
#else
static struct Gline *
make_gline(char *user, char *host, char *reason, time_t expire, time_t lastmod,
	   unsigned int flags)
#endif
{
  struct Gline *gline, *sgline, *after = 0;

#ifdef NICKGLINES
  if (!(flags & GLINE_BADCHAN) && !(flags & GLINE_REALNAME)) {
	  /* search for overlapping glines first, skipping badchans and
	   * special $ glines.
 	   */
#else
  if (!(flags & GLINE_BADCHAN)) { 
	  /* search for overlapping glines first, skipping badchans 
	   */
#endif
    for (gline = GlobalGlineList; gline; gline = sgline) {
      sgline = gline->gl_next;

      if (gline->gl_expire <= CurrentTime)
	gline_free(gline);
      else if (((gline->gl_flags & GLINE_LOCAL) != (flags & GLINE_LOCAL)) ||
	       (gline->gl_host && !host) || (!gline->gl_host && host))
	continue;
#ifdef NICKGLINES
      else if (!mmatch(gline->gl_nick, nick) && /* gline contains new mask */
	       !mmatch(gline->gl_user, user) &&
	       (gline->gl_host == NULL || !mmatch(gline->gl_host, host))) {
#else
      else if (!mmatch(gline->gl_user, user) /* gline contains new mask */
	       && (gline->gl_host == NULL || !mmatch(gline->gl_host, host))) {
#endif
	if (expire <= gline->gl_expire) /* will expire before wider gline */
	  return 0;
	else
	  after = gline; /* stick new gline after this one */
#ifdef NICKGLINES
      } else if (!mmatch(nick, gline->gl_nick) && /* new mask contains gline */
		 !mmatch(user, gline->gl_user) &&
		 (gline->gl_host==NULL || !mmatch(host, gline->gl_host)) 
#else
      } else if (!mmatch(user, gline->gl_user) /* new mask contains gline */
		 && (gline->gl_host==NULL || !mmatch(host, gline->gl_host)) 
#endif
		 && gline->gl_expire <= expire) /* old expires before new */

	gline_free(gline); /* save some memory */
    }
  }

#ifdef NICKGLINES
  if ((flags & GLINE_REALNAME)) {
    for (gline = GlobalGlineList; gline; gline = sgline) {
      sgline = gline->gl_next;

      if (gline->gl_expire <= CurrentTime)
        gline_free(gline);
      else if (((gline->gl_flags & GLINE_LOCAL) != (flags & GLINE_LOCAL)) ||
               (gline->gl_host && !host) || (!gline->gl_host && host))
        continue;
      else if (!mmatch(gline->gl_user, user) &&
               (gline->gl_host == NULL || !mmatch(gline->gl_host, host))) {

        if (expire <= gline->gl_expire) /* will expire before wider gline */
          return 0;
        else
          after = gline; /* stick new gline after this one */
      } else if (!mmatch(user, gline->gl_user) &&
                 (gline->gl_host==NULL || !mmatch(host, gline->gl_host))
                 && gline->gl_expire <= expire) /* old expires before new */

        gline_free(gline); /* save some memory */
    }
  }
#endif

  gline = (struct Gline *)MyMalloc(sizeof(struct Gline)); /* alloc memory */
  assert(0 != gline);

  DupString(gline->gl_reason, reason); /* initialize gline... */
  gline->gl_expire = expire;
  gline->gl_lastmod = lastmod;
  gline->gl_flags = flags & GLINE_MASK;

  if (flags & GLINE_BADCHAN) { /* set a BADCHAN gline */
    DupString(gline->gl_user, user); /* first, remember channel */
#ifdef NICKGLINES
    gline->gl_nick = 0;
#endif
    gline->gl_host = 0;

    gline->gl_next = BadChanGlineList; /* then link it into list */
    gline->gl_prev_p = &BadChanGlineList;
    if (BadChanGlineList)
      BadChanGlineList->gl_prev_p = &gline->gl_next;
    BadChanGlineList = gline;
  } else {
#ifdef NICKGLINES
    if (flags & GLINE_REALNAME)
      gline->gl_nick = 0;
    else
      DupString(gline->gl_nick, nick); /* remember them... */

    DupString(gline->gl_user, user);
#else
    DupString(gline->gl_user, user); /* remember them... */
#endif

    if (*user!='$')
    	DupString(gline->gl_host, host);
    else
	gline->gl_host = 0;

    if (*user!='$' && check_if_ipmask(host)) { /* mark if it's an IP mask */
      int class;
      char ipname[16];
      int ad[4] = { 0 };
      int bits2 = 0;
      char *ch;
      int seenwild;
      int badmask=0;
      
      /* Sanity check for dodgy IP masks 
       * Any mask featuring a digit after a wildcard will 
       * not behave as expected. */
      for (seenwild=0,ch=host;*ch;ch++) {
        if (*ch=='*' || *ch=='?') 
          seenwild=1;
        if (IsDigit(*ch) && seenwild) {
          badmask=1;
          break;
        }
      }

      if (badmask) {
        /* It's bad - let's make it match 0.0.0.0/32 */
        gline->bits=32;
        gline->ipnum.s_addr=0;
      } else {

        class = sscanf(host,"%d.%d.%d.%d/%d",
                       &ad[0],&ad[1],&ad[2],&ad[3], &bits2);
        if (class!=5) {
          gline->bits=class*8;
        }
        else {
          gline->bits=bits2;
        }
        ircd_snprintf(0, ipname, sizeof(ipname), "%d.%d.%d.%d", ad[0], ad[1],
                      ad[2], ad[3]);
        gline->ipnum.s_addr = inet_addr(ipname);
      }
      Debug((DEBUG_DEBUG,"IP gline: %08x/%i",gline->ipnum.s_addr,gline->bits));
      gline->gl_flags |= GLINE_IPMASK;
    }

    if (after) {
      gline->gl_next = after->gl_next;
      gline->gl_prev_p = &after->gl_next;
      if (after->gl_next)
	after->gl_next->gl_prev_p = &gline->gl_next;
      after->gl_next = gline;
    } else {
      gline->gl_next = GlobalGlineList; /* then link it into list */
      gline->gl_prev_p = &GlobalGlineList;
      if (GlobalGlineList)
	GlobalGlineList->gl_prev_p = &gline->gl_next;
      GlobalGlineList = gline;
    }
  }

  return gline;
}

static int
do_badchanneled(struct Channel *chptr, struct Gline *gline) {
  struct Membership *member,*nmember;
  for (member=chptr->members;member;member=nmember) {
    nmember=member->next_member;
    if (!MyUser(member->user) || IsZombie(member) || IsAnOper(member->user))
      continue;
    sendcmdto_serv_butone(&me, CMD_KICK, NULL, "%H %C :%s (%s)", chptr,
			  member->user, feature_str(FEAT_BADCHAN_REASON),
			  gline->gl_reason);
    sendcmdto_channel_butserv_butone(&me, CMD_KICK, chptr, NULL,
			  "%H %C :%s (%s)", chptr, member->user,
			  feature_str(FEAT_BADCHAN_REASON), 
			  gline->gl_reason);
    make_zombie(member, member->user, &me, &me, chptr);
    return 1;
  }
  return 0;
}

static int
do_mangle_gline(struct Client* cptr, struct Client* acptr,
		struct Client* sptr, const char* orig_reason)
{
  char reason[BUFSIZE];
  char* endanglebracket;
  char* space;

  if (!feature_bool(FEAT_HIS_GLINE))
    return exit_client_msg(cptr, acptr, &me, "G-lined (%s)", orig_reason);

  endanglebracket = strchr(orig_reason, '>');
  space = strchr(orig_reason, ' ');

  if (IsService(sptr))
  {
    if (orig_reason[0] == '<' && endanglebracket && endanglebracket < space)
    {
      strcpy(reason, "G-lined by ");
      strncat(reason, orig_reason + 1, endanglebracket - orig_reason - 1);
    } else {
      ircd_snprintf(0, reason, sizeof(reason), "G-lined (%s)",
		    orig_reason);
    }
  } else {
    ircd_snprintf(0, reason, sizeof(reason), "G-lined (<%s> %s)",
		  sptr->cli_name, orig_reason);
  }
  return exit_client_msg(cptr, acptr, &me, reason);
}

static int
do_gline(struct Client *cptr, struct Client *sptr, struct Gline *gline)
{
  struct Client *acptr;
  int fd, retval = 0, tval;

  if (!GlineIsActive(gline)) /* no action taken on inactive glines */
    return 0;

  for (fd = HighestFd; fd >= 0; --fd) {
    /*
     * get the users!
     */
    if ((acptr = LocalClientArray[fd])) {
      if (!cli_user(acptr))
        continue;

      if (gline->gl_flags & GLINE_REALNAME) { /* Realname Gline */
	Debug((DEBUG_DEBUG,"Realname Gline: %s %s",(cli_info(acptr)),
					gline->gl_user+2));
        if (match(gline->gl_user+2, cli_info(acptr)) != 0)
            continue;
        Debug((DEBUG_DEBUG,"Matched!"));

      } else if (gline->gl_flags & GLINE_BADCHAN) { /* Badchan Gline */
        struct Channel *chptr,*nchptr;
	if (string_has_wildcards(gline->gl_user)) {
	  for(chptr=GlobalChannelList;chptr;chptr=nchptr) {
	    nchptr=chptr->next;
	    if (match(gline->gl_user, chptr->chname))
	      continue;
	    retval = do_badchanneled(chptr, gline);
	  }
	} else { 
	  if ((chptr=FindChannel(gline->gl_user))) { 
	    retval = do_badchanneled(chptr, gline);
	  }
	}
	continue;
      } else { /* Host/IP gline */
#ifdef NICKGLINES
              if (cli_name(acptr) && 
                  match (gline->gl_nick, cli_name(acptr)) != 0)
                       continue;
#endif

	      if (cli_user(acptr)->username && 
			      match (gline->gl_user, (cli_user(acptr))->realusername) != 0)
		      continue;

	      if (GlineIsIpMask(gline)) {
		      Debug((DEBUG_DEBUG,"IP gline: %08x %08x/%i",(cli_ip(cptr)).s_addr,
                 gline->ipnum.s_addr,gline->bits));
		      if (((cli_ip(acptr)).s_addr & NETMASK(gline->bits)) 
              != gline->ipnum.s_addr)
			      continue;
	      }
	      else {
		      if (match(gline->gl_host, cli_sockhost(acptr)) != 0)
			      continue;
	      }
      } /* of Host/IP Gline */

      /* ok, here's one that got G-lined */
      send_reply(acptr, SND_EXPLICIT | ERR_YOUREBANNEDCREEP, ":%s",
      	   gline->gl_reason);

      /* let the ops know about it */
      sendto_opmask_butone(0, SNO_GLINE, "G-line active for %s",
      		     get_client_name(acptr, TRUE));

      /* and get rid of him */
      if ((tval = do_mangle_gline(cptr, acptr, sptr, gline->gl_reason)))
        retval = tval; /* retain killed status */
    }
  }
  return retval;
}

/*
 * This routine implements the mask checking applied to local
 * G-lines.  Basically, host masks must have a minimum of two non-wild
 * domain fields, and IP masks must have a minimum of 16 bits.  If the
 * mask has even one wild-card, OVERRIDABLE is returned, assuming the
 * other check doesn't fail.
 */
static int
gline_checkmask(char *mask)
{
  unsigned int flags = MASK_IP;
  unsigned int dots = 0;
  unsigned int ipmask = 0;

  for (; *mask; mask++) { /* go through given mask */
    if (*mask == '.') { /* it's a separator; advance positional wilds */
      flags = (flags & ~MASK_WILD_MASK) | ((flags << 1) & MASK_WILD_MASK);
      dots++;

      if ((flags & (MASK_IP | MASK_WILDS)) == MASK_IP)
	ipmask += 8; /* It's an IP with no wilds, count bits */
    } else if (*mask == '*' || *mask == '?')
      flags |= MASK_WILD_0 | MASK_WILDS; /* found a wildcard */
    else if (*mask == '/') { /* n.n.n.n/n notation; parse bit specifier */
      mask++;
      ipmask = strtoul(mask, &mask, 10);

      if (*mask || dots != 3 || ipmask > 32 || /* sanity-check to date */
	  (flags & (MASK_WILDS | MASK_IP)) != MASK_IP)
	return CHECK_REJECTED; /* how strange... */

      if (ipmask < 32) /* it's a masked address; mark wilds */
	flags |= MASK_WILDS;

      flags |= MASK_HALT; /* Halt the ipmask calculation */

      break; /* get out of the loop */
    } else if (!IsDigit(*mask)) {
      flags &= ~MASK_IP; /* not an IP anymore! */
      ipmask = 0;
    }
  }

  /* Sanity-check quads */
  if (dots > 3 || (!(flags & MASK_WILDS) && dots < 3)) {
    flags &= ~MASK_IP;
    ipmask = 0;
  }

  /* update bit count if necessary */
  if ((flags & (MASK_IP | MASK_WILDS | MASK_HALT)) == MASK_IP)
    ipmask += 8;

  /* Check to see that it's not too wide of a mask */
  if (flags & MASK_WILDS &&
      ((!(flags & MASK_IP) && (dots < 2 || flags & MASK_WILD_MASK)) ||
       (flags & MASK_IP && ipmask < 16)))
    return CHECK_REJECTED; /* to wide, reject */

  /* Ok, it's approved; require override if it has wildcards, though */
  return flags & MASK_WILDS ? CHECK_OVERRIDABLE : CHECK_APPROVED;
}

int
gline_propagate(struct Client *cptr, struct Client *sptr, struct Gline *gline)
{
  if (GlineIsLocal(gline) || (IsUser(sptr) && !gline->gl_lastmod))
    return 0;

  if (gline->gl_lastmod)
#ifdef NICKGLINES
    sendcmdto_serv_butone(sptr, CMD_GLINE, cptr, "* %c%s%s%s%s%s %Tu %Tu :%s",
			  GlineIsRemActive(gline) ? '+' : '-',
			  (GlineIsRealName(gline) || GlineIsBadChan(gline)) ? "" : gline->gl_nick,
 			  (GlineIsRealName(gline) || GlineIsBadChan(gline)) ? "" : "!",
			  gline->gl_user,
#else
    sendcmdto_serv_butone(sptr, CMD_GLINE, cptr, "* %c%s%s%s %Tu %Tu :%s",
			  GlineIsRemActive(gline) ? '+' : '-', gline->gl_user,
#endif
			  gline->gl_host ? "@" : "",
			  gline->gl_host ? gline->gl_host : "",
			  gline->gl_expire - CurrentTime, gline->gl_lastmod,
			  gline->gl_reason);
  else
    sendcmdto_serv_butone(sptr, CMD_GLINE, cptr,
#ifdef NICKGLINES
			  (GlineIsRemActive(gline) ?
			   "* +%s%s%s%s%s %Tu :%s" : "* -%s%s%s%s%s"),
			  (GlineIsRealName(gline) || GlineIsBadChan(gline)) ? "" : gline->gl_nick,
			  (GlineIsRealName(gline) || GlineIsBadChan(gline)) ? "" : "!",
#else
			  (GlineIsRemActive(gline) ?
			   "* +%s%s%s %Tu :%s" : "* -%s%s%s"),
#endif
			  gline->gl_user, 
			  gline->gl_host ? "@" : "",
			  gline->gl_host ? gline->gl_host : "",
			  gline->gl_expire - CurrentTime, gline->gl_reason);

  return 0;
}

int 
gline_add(struct Client *cptr, struct Client *sptr, char *userhost,
	  char *reason, time_t expire, time_t lastmod, unsigned int flags)
{
  struct Gline *agline;
#ifdef NICKGLINES
  char uhmask[NICKLEN + USERLEN + HOSTLEN + 3];
  char *nick, *user, *host;
#else
  char uhmask[USERLEN + HOSTLEN + 2];
  char *user, *host;
#endif
  int tmp;

  assert(0 != userhost);
  assert(0 != reason);

  /* NO_OLD_GLINE allows *@#channel to work correctly */
  if (*userhost == '#' || *userhost == '&'
# ifndef NO_OLD_GLINE
      || ((userhost[2] == '#' || userhost[2] == '&') && (userhost[1] == '@'))
# endif /* OLD_GLINE */
      ) {
    if ((flags & GLINE_LOCAL) && !HasPriv(sptr, PRIV_LOCAL_BADCHAN))
      return send_reply(sptr, ERR_NOPRIVILEGES);

    flags |= GLINE_BADCHAN;
# ifndef NO_OLD_GLINE
    if ((userhost[2] == '#' || userhost[2] == '&') && (userhost[1] == '@'))
      user = userhost + 2;
    else
# endif /* OLD_GLINE */
      user = userhost;
    host = 0;
  } else if (*userhost == '$'
# ifndef NO_OLD_GLINE
   || userhost[2] == '$'
# endif /* OLD_GLINE */
  ) {
    switch (*userhost == '$' ? userhost[1] : userhost[3]) {
      case 'R':
        flags |= GLINE_REALNAME;
        break;
      default:
        /* uh, what to do here? */
        /* The answer, my dear Watson, is we throw a protocol_violation()
           -- hikari */
        return protocol_violation(sptr, "%s has sent an incorrectly formatted gline",
				  cli_name(sptr));
        break;
    }
     user = (*userhost =='$' ? userhost : userhost+2);
     host = 0;
  } else {
#ifdef NICKGLINES
    canon_userhost(userhost, &nick, &user, &host, "*");
    if (sizeof(uhmask) <
	ircd_snprintf(0, uhmask, sizeof(uhmask), "%s!%s@%s", nick, user, host))
#else
    canon_userhost(userhost, &user, &host, "*");
    if (sizeof(uhmask) <
	ircd_snprintf(0, uhmask, sizeof(uhmask), "%s@%s", user, host))
#endif
      return send_reply(sptr, ERR_LONGMASK);
    else if (MyUser(sptr) || (IsUser(sptr) && flags & GLINE_LOCAL)) {
      switch (gline_checkmask(host)) {
      case CHECK_OVERRIDABLE: /* oper overrided restriction */
	if (flags & GLINE_OPERFORCE)
	  break;
	/*FALLTHROUGH*/
      case CHECK_REJECTED:
	return send_reply(sptr, ERR_MASKTOOWIDE, uhmask);
	break;
      }

      if ((tmp = count_users(uhmask)) >=
	  feature_int(FEAT_GLINEMAXUSERCOUNT) && !(flags & GLINE_OPERFORCE))
	return send_reply(sptr, ERR_TOOMANYUSERS, tmp);
    }
  }

  /*
   * You cannot set a negative (or zero) expire time, nor can you set an
   * expiration time for greater than GLINE_MAX_EXPIRE.
   */
  if (!(flags & GLINE_FORCE) && (expire <= 0 || expire > GLINE_MAX_EXPIRE)) {
    if (!IsServer(sptr) && MyConnect(sptr))
      send_reply(sptr, ERR_BADEXPIRE, expire);
    return 0;
  }

  expire += CurrentTime; /* convert from lifetime to timestamp */

  /* Inform ops... */
  sendto_opmask_butone(0, ircd_strncmp(reason, "AUTO", 4) ? SNO_GLINE :
#ifdef NICKGLINES
		       SNO_AUTO, "%s adding %s %s for %s%s%s%s%s, expiring at "
#else
		       SNO_AUTO, "%s adding %s %s for %s%s%s, expiring at "
#endif
		       "%Tu: %s",
		       feature_bool(FEAT_HIS_SNOTICES) || IsServer(sptr) ?
		       cli_name(sptr) : cli_name((cli_user(sptr))->server),
		       flags & GLINE_LOCAL ? "local" : "global",
#ifdef NICKGLINES
		       flags & GLINE_BADCHAN ? "BADCHAN" : "GLINE", 
		       flags & (GLINE_BADCHAN|GLINE_REALNAME) ? "" : nick,
		       flags & (GLINE_BADCHAN|GLINE_REALNAME) ? "" : "!",
			   user,
#else
		       flags & GLINE_BADCHAN ? "BADCHAN" : "GLINE", user,
#endif
		       flags & (GLINE_BADCHAN|GLINE_REALNAME) ? "" : "@",
		       flags & (GLINE_BADCHAN|GLINE_REALNAME) ? "" : host,
		       expire + TSoffset, reason);

  /* and log it */
  log_write(LS_GLINE, L_INFO, LOG_NOSNOTICE,
#ifdef NICKGLINES
	    "%#C adding %s %s for %s%s%s%s%s, expiring at %Tu: %s", sptr,
	    flags & GLINE_LOCAL ? "local" : "global",
	    flags & GLINE_BADCHAN ? "BADCHAN" : "GLINE",
	    flags & (GLINE_BADCHAN|GLINE_REALNAME) ? "" : nick,
	    flags & (GLINE_BADCHAN|GLINE_REALNAME) ? "" : "!",
            user,
	    flags & (GLINE_BADCHAN|GLINE_REALNAME) ? "" : "@",
            flags & (GLINE_BADCHAN|GLINE_REALNAME) ? "" : host,
	    expire + TSoffset, reason);

  /* make the gline */
  agline = make_gline(nick, user, host, reason, expire, lastmod, flags);
#else
	    "%#C adding %s %s for %s, expiring at %Tu: %s", sptr,
	    flags & GLINE_LOCAL ? "local" : "global",
	    flags & GLINE_BADCHAN ? "BADCHAN" : "GLINE", userhost,
	    expire + TSoffset, reason);

  /* make the gline */
  agline = make_gline(user, host, reason, expire, lastmod, flags);
#endif

  if (!agline) /* if it overlapped, silently return */
    return 0;

  gline_propagate(cptr, sptr, agline);

  return do_gline(cptr, sptr, agline); /* knock off users if necessary */
}

int
gline_activate(struct Client *cptr, struct Client *sptr, struct Gline *gline,
	       time_t lastmod, unsigned int flags)
{
  unsigned int saveflags = 0;

  assert(0 != gline);

  saveflags = gline->gl_flags;

  if (flags & GLINE_LOCAL)
    gline->gl_flags &= ~GLINE_LDEACT;
  else {
    gline->gl_flags |= GLINE_ACTIVE;

    if (gline->gl_lastmod) {
      if (gline->gl_lastmod >= lastmod) /* force lastmod to increase */
	gline->gl_lastmod++;
      else
	gline->gl_lastmod = lastmod;
    }
  }

  if ((saveflags & GLINE_ACTMASK) == GLINE_ACTIVE)
    return 0; /* was active to begin with */

  /* Inform ops and log it */
#ifdef NICKGLINES
  sendto_opmask_butone(0, SNO_GLINE, "%s activating global %s for %s%s%s%s%s, "
#else
  sendto_opmask_butone(0, SNO_GLINE, "%s activating global %s for %s%s%s, "
#endif
		       "expiring at %Tu: %s",
		       feature_bool(FEAT_HIS_SNOTICES) || IsServer(sptr) ?
		       cli_name(sptr) : cli_name((cli_user(sptr))->server),
		       GlineIsBadChan(gline) ? "BADCHAN" : "GLINE",
#ifdef NICKGLINES
		       (GlineIsBadChan(gline) || GlineIsRealName(gline)) ? "" : gline->gl_nick,
		       (GlineIsBadChan(gline) || GlineIsRealName(gline)) ? "" : "!",
#endif
		       gline->gl_user, gline->gl_host ? "@" : "",
		       gline->gl_host ? gline->gl_host : "",
		       gline->gl_expire + TSoffset, gline->gl_reason);

  log_write(LS_GLINE, L_INFO, LOG_NOSNOTICE,
#ifdef NICKGLINES
	    "%#C activating global %s for %s%s%s%s%s, expiring at %Tu: %s", sptr,
	    GlineIsBadChan(gline) ? "BADCHAN" : "GLINE",
	    (GlineIsBadChan(gline) || GlineIsRealName(gline)) ? "" : gline->gl_nick,
	    (GlineIsBadChan(gline) || GlineIsRealName(gline)) ? "" : "!",
	    gline->gl_user,
#else
	    "%#C activating global %s for %s%s%s, expiring at %Tu: %s", sptr,
	    GlineIsBadChan(gline) ? "BADCHAN" : "GLINE", gline->gl_user,
#endif
	    gline->gl_host ? "@" : "",
	    gline->gl_host ? gline->gl_host : "",
	    gline->gl_expire + TSoffset, gline->gl_reason);

  if (!(flags & GLINE_LOCAL)) /* don't propagate local changes */
    gline_propagate(cptr, sptr, gline);

  return GlineIsBadChan(gline) ? 0 : do_gline(cptr, sptr, gline);
}

int
gline_deactivate(struct Client *cptr, struct Client *sptr, struct Gline *gline,
		 time_t lastmod, unsigned int flags)
{
  unsigned int saveflags = 0;
  char *msg;

  assert(0 != gline);

  saveflags = gline->gl_flags;

  if (GlineIsLocal(gline))
    msg = "removing local";
  else if (!gline->gl_lastmod && !(flags & GLINE_LOCAL)) {
    msg = "removing global";
    gline->gl_flags &= ~GLINE_ACTIVE; /* propagate a -<mask> */
  } else {
    msg = "deactivating global";

    if (flags & GLINE_LOCAL)
      gline->gl_flags |= GLINE_LDEACT;
    else {
      gline->gl_flags &= ~GLINE_ACTIVE;

      if (gline->gl_lastmod) {
	if (gline->gl_lastmod >= lastmod)
	  gline->gl_lastmod++;
	else
	  gline->gl_lastmod = lastmod;
      }
    }

    if ((saveflags & GLINE_ACTMASK) != GLINE_ACTIVE)
      return 0; /* was inactive to begin with */
  }

  /* Inform ops and log it */
#ifdef NICKGLINES
  sendto_opmask_butone(0, SNO_GLINE, "%s %s %s for %s%s%s%s%s, expiring at %Tu: "
#else
  sendto_opmask_butone(0, SNO_GLINE, "%s %s %s for %s%s%s, expiring at %Tu: "
#endif
		       "%s",
		       feature_bool(FEAT_HIS_SNOTICES) || IsServer(sptr) ?
		       cli_name(sptr) : cli_name((cli_user(sptr))->server),
		       msg, GlineIsBadChan(gline) ? "BADCHAN" : "GLINE",
#ifdef NICKGLINES
		       (GlineIsBadChan(gline) || GlineIsRealName(gline)) ? "" : gline->gl_nick,
		       (GlineIsBadChan(gline) || GlineIsRealName(gline)) ? "" : "!",
#endif
		       gline->gl_user, gline->gl_host ? "@" : "",
		       gline->gl_host ? gline->gl_host : "",
		       gline->gl_expire + TSoffset, gline->gl_reason);

  log_write(LS_GLINE, L_INFO, LOG_NOSNOTICE,
#ifdef NICKGLINES
	    "%#C %s %s for %s%s%s%s%s, expiring at %Tu: %s", sptr, msg,
	    GlineIsBadChan(gline) ? "BADCHAN" : "GLINE",
	    (GlineIsBadChan(gline) || GlineIsRealName(gline)) ? "" : gline->gl_nick,
	    (GlineIsBadChan(gline) || GlineIsRealName(gline)) ? "" : "!",
	    gline->gl_user,
#else
	    "%#C %s %s for %s%s%s, expiring at %Tu: %s", sptr, msg,
	    GlineIsBadChan(gline) ? "BADCHAN" : "GLINE", gline->gl_user,
#endif
	    gline->gl_host ? "@" : "",
	    gline->gl_host ? gline->gl_host : "",
	    gline->gl_expire + TSoffset, gline->gl_reason);

  if (!(flags & GLINE_LOCAL)) /* don't propagate local changes */
    gline_propagate(cptr, sptr, gline);

  /* if it's a local gline or a Uworld gline (and not locally deactivated).. */
  if (GlineIsLocal(gline) || (!gline->gl_lastmod && !(flags & GLINE_LOCAL)))
    gline_free(gline); /* get rid of it */

  return 0;
}

struct Gline *
gline_find(char *userhost, unsigned int flags)
{
  struct Gline *gline;
  struct Gline *sgline;
#ifdef NICKGLINES
  char *nick, *user, *host, *t_uh;
#else
  char *user, *host, *t_uh;
#endif

  if (flags & (GLINE_BADCHAN | GLINE_ANY)) {
    for (gline = BadChanGlineList; gline; gline = sgline) {
      sgline = gline->gl_next;

      if (gline->gl_expire <= CurrentTime)
	gline_free(gline);
      else if ((flags & GLINE_GLOBAL && gline->gl_flags & GLINE_LOCAL) ||
	       (flags & GLINE_LASTMOD && !gline->gl_lastmod))
	continue;
      else if ((flags & GLINE_EXACT ? ircd_strcmp(gline->gl_user, userhost) :
		match(gline->gl_user, userhost)) == 0)
	return gline;
    }
  }

  if ((flags & (GLINE_BADCHAN | GLINE_ANY)) == GLINE_BADCHAN ||
      *userhost == '#' || *userhost == '&'
#ifndef NO_OLD_GLINE
      || userhost[2] == '#' || userhost[2] == '&'
#endif /* NO_OLD_GLINE */
      )
    return 0;

  DupString(t_uh, userhost);
#ifdef NICKGLINES
  canon_userhost(t_uh, &nick, &user, &host, 0);
#else
  canon_userhost(t_uh, &user, &host, 0);
#endif

  if(BadPtr(user))
    return 0;

  for (gline = GlobalGlineList; gline; gline = sgline) {
    sgline = gline->gl_next;

    if (gline->gl_expire <= CurrentTime)
      gline_free(gline);
    else if ((flags & GLINE_GLOBAL && gline->gl_flags & GLINE_LOCAL) ||
	     (flags & GLINE_LASTMOD && !gline->gl_lastmod))
      continue;
    else if (flags & GLINE_EXACT) {
#ifdef NICKGLINES
      if ((gline->gl_nick == NULL) && (gline->gl_host == NULL)) {
         if (((gline->gl_host && host && ircd_strcmp(gline->gl_host,host) == 0) ||
                 (!gline->gl_host && !host)) &&
                 ((!user && ircd_strcmp(gline->gl_user, "*") == 0) ||
                 (user && ircd_strcmp(gline->gl_user, user) == 0)))
           break;
     } else {
         if (((gline->gl_host && host && ircd_strcmp(gline->gl_host,host) == 0) ||
                 (!gline->gl_host && !host)) &&
		 ((!user && ircd_strcmp(gline->gl_user, "*") == 0) ||
		 (user && ircd_strcmp(gline->gl_user, user) == 0)) &&
		 ((!nick && ircd_strcmp(gline->gl_nick, "*") == 0) ||
		 (nick && ircd_strcmp(gline->gl_nick, nick) == 0)))
  	   break;
      }
#else
      if (((gline->gl_host && host && ircd_strcmp(gline->gl_host,host) == 0)
	 ||(!gline->gl_host && !host)) &&
	  ((!user && ircd_strcmp(gline->gl_user, "*") == 0) ||
	   ircd_strcmp(gline->gl_user, user) == 0))
	break;
#endif
    } else {
#ifdef NICKGLINES
      if ((nick == NULL) && (host == NULL)) {
        if (((gline->gl_host && host && ircd_strcmp(gline->gl_host,host) == 0)
  	   ||(!gline->gl_host && !host)) &&
		 ((!user && ircd_strcmp(gline->gl_user, "*") == 0) ||
		 (user && match(gline->gl_user, user) == 0)) &&
		 ((!nick && ircd_strcmp(gline->gl_nick, "*") == 0) ||
		 (nick && (match(gline->gl_nick, nick) == 0))))
           break;
      } else {
        if (((gline->gl_host && host && ircd_strcmp(gline->gl_host,host) == 0) ||
           (!gline->gl_host && !host)) &&
           ((!user && ircd_strcmp(gline->gl_user, "*") == 0) ||
           (user && match(gline->gl_user, user) == 0)))
          break;
      }
#else
      if (((gline->gl_host && host && ircd_strcmp(gline->gl_host,host) == 0)
	 ||(!gline->gl_host && !host)) &&
	  ((!user && ircd_strcmp(gline->gl_user, "*") == 0) ||
	   match(gline->gl_user, user) == 0))
      break;
#endif
    }
  }

  if (!BadPtr(t_uh))
    MyFree(t_uh);

  return gline;
}

struct Gline *
gline_lookup(struct Client *cptr, unsigned int flags)
{
  struct Gline *gline;
  struct Gline *sgline;

  for (gline = GlobalGlineList; gline; gline = sgline) {
    sgline = gline->gl_next;

    if (gline->gl_expire <= CurrentTime) {
      gline_free(gline);
      continue;
    }
    
    if ((flags & GLINE_GLOBAL && gline->gl_flags & GLINE_LOCAL) ||
	     (flags & GLINE_LASTMOD && !gline->gl_lastmod))
      continue;

    if (GlineIsRealName(gline)) {
       Debug((DEBUG_DEBUG,"realname gline: '%s' '%s'",gline->gl_user,cli_info(cptr)));
      if (match(gline->gl_user+2, cli_info(cptr)) != 0)
	continue;

      return gline;
    }
    else {
#ifdef NICKGLINES
      if (match(gline->gl_nick, cli_name(cptr)) != 0)
        continue;
#endif

      if (match(gline->gl_user, (cli_user(cptr))->realusername) != 0)
        continue;
    	 
      if (GlineIsIpMask(gline)) {
        Debug((DEBUG_DEBUG,"IP gline: %08x %08x/%i",(cli_ip(cptr)).s_addr,gline->ipnum.s_addr,gline->bits));
        if (((cli_ip(cptr)).s_addr & NETMASK(gline->bits)) != gline->ipnum.s_addr)
          continue;
      }    
      else {
        if (match(gline->gl_host, (cli_user(cptr))->realhost) != 0) 
          continue;

#ifdef NICKGLINES
        if (match(gline->gl_nick, cli_name(cptr)) != 0)
          continue;
#endif
      }
    }
    return gline;
  }
  /*
   * No Glines matched
   */
  return 0;
}

void
gline_free(struct Gline *gline)
{
  assert(0 != gline);

  *gline->gl_prev_p = gline->gl_next; /* squeeze this gline out */
  if (gline->gl_next)
    gline->gl_next->gl_prev_p = gline->gl_prev_p;

#ifdef NICKGLINES
  if (gline->gl_nick)
    MyFree(gline->gl_nick);
#endif
  MyFree(gline->gl_user); /* free up the memory */
  if (gline->gl_host)
    MyFree(gline->gl_host);
  MyFree(gline->gl_reason);
  MyFree(gline);
}

void
gline_burst(struct Client *cptr)
{
  struct Gline *gline;
  struct Gline *sgline;

  for (gline = GlobalGlineList; gline; gline = sgline) { /* all glines */
    sgline = gline->gl_next;

    if (gline->gl_expire <= CurrentTime) /* expire any that need expiring */
      gline_free(gline);
    else if (!GlineIsLocal(gline) && gline->gl_lastmod)
#ifdef NICKGLINES
      sendcmdto_one(&me, CMD_GLINE, cptr, "* %c%s%s%s%s%s %Tu %Tu :%s",
		    GlineIsRemActive(gline) ? '+' : '-',
                    gline->gl_nick ? gline->gl_nick : "",
                    GlineIsRealName(gline) ? "" : "!",
                    gline->gl_user,
		    (gline->gl_host && !GlineIsRealName(gline)) ? "@" : "",
#else
      sendcmdto_one(&me, CMD_GLINE, cptr, "* %c%s%s%s %Tu %Tu :%s",
		    GlineIsRemActive(gline) ? '+' : '-', gline->gl_user,
		    gline->gl_host ? "@" : "",
#endif
		    gline->gl_host ? gline->gl_host : "", 
		    gline->gl_expire - CurrentTime, gline->gl_lastmod, 
		    gline->gl_reason);
  }

  for (gline = BadChanGlineList; gline; gline = sgline) { /* all glines */
    sgline = gline->gl_next;

    if (gline->gl_expire <= CurrentTime) /* expire any that need expiring */
      gline_free(gline);
    else if (!GlineIsLocal(gline) && gline->gl_lastmod)
      sendcmdto_one(&me, CMD_GLINE, cptr, "* %c%s %Tu %Tu :%s",
		    GlineIsRemActive(gline) ? '+' : '-', gline->gl_user,
		    gline->gl_expire - CurrentTime, gline->gl_lastmod,
		    gline->gl_reason);
  }
}

int
gline_resend(struct Client *cptr, struct Gline *gline)
{
  if (GlineIsLocal(gline) || !gline->gl_lastmod)
    return 0;

#ifdef NICKGLINES
  sendcmdto_one(&me, CMD_GLINE, cptr, "* %c%s%s%s%s%s %Tu %Tu :%s",
		GlineIsRemActive(gline) ? '+' : '-', 
		GlineIsBadChan(gline) ? "" : gline->gl_nick,
		GlineIsBadChan(gline) ? "" : "!",
		gline->gl_user,
#else
  sendcmdto_one(&me, CMD_GLINE, cptr, "* %c%s%s%s %Tu %Tu :%s",
		GlineIsRemActive(gline) ? '+' : '-', gline->gl_user,
#endif
		gline->gl_host ? "@" : "",
		gline->gl_host ? gline->gl_host : "",
		gline->gl_expire - CurrentTime, gline->gl_lastmod,
		gline->gl_reason);

  return 0;
}

int
gline_list(struct Client *sptr, char *userhost)
{
  struct Gline *gline;
  struct Gline *sgline;

  if (userhost) {
    if (!(gline = gline_find(userhost, GLINE_ANY))) /* no such gline */
      return send_reply(sptr, ERR_NOSUCHGLINE, userhost);

    /* send gline information along */
#ifdef NICKGLINES
    send_reply(sptr, RPL_GLIST, 
	       (GlineIsBadChan(gline) || (GlineIsRealName(gline)) ? "" : gline->gl_nick),
	       (GlineIsBadChan(gline) || (GlineIsRealName(gline)) ? "" : "!"),
	       gline->gl_user,
               (GlineIsBadChan(gline) || (GlineIsRealName(gline)) ? "" : "@"),
#else
    send_reply(sptr, RPL_GLIST, gline->gl_user,
	       GlineIsBadChan(gline) ? "" : "@",
#endif
	       gline->gl_host ? gline->gl_host : "",
	       gline->gl_expire + TSoffset,
	       GlineIsLocal(gline) ? cli_name(&me) : "*",
	       GlineIsActive(gline) ? '+' : '-', gline->gl_reason);
  } else {
    for (gline = GlobalGlineList; gline; gline = sgline) {
      sgline = gline->gl_next;

      if (gline->gl_expire <= CurrentTime)
	gline_free(gline);
      else
#ifdef NICKGLINES
        send_reply(sptr, RPL_GLIST,
                   GlineIsRealName(gline) ? "" : gline->gl_nick,
                   GlineIsRealName(gline) ? "" : "!",
                   gline->gl_user,
                   gline->gl_host ? "@" : "",
#else
	send_reply(sptr, RPL_GLIST, gline->gl_user, 
		   gline->gl_host ? "@" : "", 
#endif
		   gline->gl_host ? gline->gl_host : "",
		   gline->gl_expire + TSoffset,
		   GlineIsLocal(gline) ? cli_name(&me) : "*",
		   GlineIsActive(gline) ? '+' : '-', gline->gl_reason);
    }

    for (gline = BadChanGlineList; gline; gline = sgline) {
      sgline = gline->gl_next;

      if (gline->gl_expire <= CurrentTime)
	gline_free(gline);
      else
#ifdef NICKGLINES
        send_reply(sptr, RPL_GLIST, "", "", gline->gl_user, "", "",
                   gline->gl_expire + TSoffset,
#else
	send_reply(sptr, RPL_GLIST, gline->gl_user, "", "",
		   gline->gl_expire + TSoffset,
#endif
		   GlineIsLocal(gline) ? cli_name(&me) : "*",
		   GlineIsActive(gline) ? '+' : '-', gline->gl_reason);
    }
  }

  /* end of gline information */
  return send_reply(sptr, RPL_ENDOFGLIST);
}

void
gline_stats(struct Client *sptr, struct StatDesc *sd, int stat, char *param)
{
  struct Gline *gline;
  struct Gline *sgline;

  for (gline = GlobalGlineList; gline; gline = sgline) {
    sgline = gline->gl_next;

    if (gline->gl_expire <= CurrentTime)
      gline_free(gline);
#ifdef NICKGLINES
    else {
      send_reply(sptr, RPL_STATSGLINE, 'G',
                 GlineIsRealName(gline) ? "" : gline->gl_nick,
                 GlineIsRealName(gline) ? "" : "!",
                 gline->gl_user,
	         gline->gl_host ? "@" : "",
	         gline->gl_host ? gline->gl_host : "",
	         gline->gl_expire + TSoffset, gline->gl_reason);
    }
  }

  for (gline = BadChanGlineList; gline; gline = sgline) {
    sgline = gline->gl_next;

    if (gline->gl_expire <= CurrentTime)
      gline_free(gline);
    else {
      send_reply(sptr, RPL_STATSGLINE, 'G', "", "", "", "",
                 gline->gl_user,
                 gline->gl_expire + TSoffset, gline->gl_reason);
    }
#else
    else
      send_reply(sptr, RPL_STATSGLINE, 'G', gline->gl_user, 
		 gline->gl_host ? "@" : "",
		 gline->gl_host ? gline->gl_host : "",
		 gline->gl_expire + TSoffset, gline->gl_reason);
#endif
  }
}

int
gline_memory_count(size_t *gl_size)
{
  struct Gline *gline;
  unsigned int gl = 0;

  for (gline = GlobalGlineList; gline; gline = gline->gl_next) {
    gl++;
    *gl_size += sizeof(struct Gline);
#ifdef NICKGLINES
    *gl_size += gline->gl_nick ? (strlen(gline->gl_nick) + 1) : 0;
#endif
    *gl_size += gline->gl_user ? (strlen(gline->gl_user) + 1) : 0;
    *gl_size += gline->gl_host ? (strlen(gline->gl_host) + 1) : 0;
    *gl_size += gline->gl_reason ? (strlen(gline->gl_reason) + 1) : 0;
  }
  return gl;
}

#ifdef NICKGLINES
struct Gline *
IsNickGlined(struct Client *cptr, char *nick)
{
  struct Gline *gline;
  struct Gline *sgline;

  for (gline = GlobalGlineList; gline; gline = sgline) {
    sgline = gline->gl_next;

    if (gline->gl_expire <= CurrentTime) {
      gline_free(gline);
      continue;
    }
    
    if (!ircd_strcmp(gline->gl_nick, "*"))	/* skip glines w. wildcarded nick */
      continue;

    if (match(gline->gl_nick, nick) != 0)
      continue;

    if (match(gline->gl_user, (cli_user(cptr))->username) != 0)
      continue;
    	 
    if (GlineIsIpMask(gline)) {
      Debug((DEBUG_DEBUG,"IP gline: %08x %08x/%i",(cli_ip(cptr)).s_addr,gline->ipnum.s_addr,gline->bits));
      if (((cli_ip(cptr)).s_addr & NETMASK(gline->bits)) != gline->ipnum.s_addr)
        continue;
    }
    else {
      if (match(gline->gl_host, (cli_user(cptr))->realhost) != 0) 
        continue;
    }
    return gline;
  }
  /*
   * No Glines matched
   */
  return 0;
}
#endif

