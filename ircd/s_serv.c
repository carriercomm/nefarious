/*
 * IRC - Internet Relay Chat, ircd/s_serv.c (formerly ircd/s_msg.c)
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
#include "config.h"

#include "s_serv.h"
#include "IPcheck.h"
#include "channel.h"
#include "client.h"
#include "gline.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_alloc.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "ircd_snprintf.h"
#include "ircd_struct.h"
#include "ircd_crypt.h"
#include "jupe.h"
#include "list.h"
#include "map.h"
#include "mark.h"
#include "match.h"
#include "msg.h"
#include "msgq.h"
#include "numeric.h"
#include "numnicks.h"
#include "parse.h"
#include "querycmds.h"
#include "s_bsd.h"
#include "s_conf.h"
#include "s_debug.h"
#include "s_misc.h"
#include "s_user.h"
#include "send.h"
#include "shun.h"
#include "ircd_struct.h"
#include "support.h"
#include "sys.h"
#include "userload.h"
#include "zline.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned int max_connection_count = 0;
unsigned int max_client_count = 0;

int exit_new_server(struct Client *cptr, struct Client *sptr, const char *host,
                    time_t timestamp, const char *pattern, ...)
{
  struct VarData vd;
  int retval = 0;

  vd.vd_format = pattern;
  va_start(vd.vd_args, pattern);

  if (!IsServer(sptr))
    retval = vexit_client_msg(cptr, cptr, &me, pattern, vd.vd_args);
  else
    sendcmdto_one(&me, CMD_SQUIT, cptr, "%s %Tu :%v", host, timestamp, &vd);

  va_end(vd.vd_args);

  return retval;
}

int a_kills_b_too(struct Client *a, struct Client *b)
{
  for (; b != a && b != &me; b = cli_serv(b)->up);
  return (a == b ? 1 : 0);
}

/*
 * server_estab
 *
 * May only be called after a SERVER was received from cptr,
 * and thus make_server was called, and serv->prot set. --Run
 */
int server_estab(struct Client *cptr, struct ConfItem *aconf)
{
  struct Client* acptr = 0;
  const char*    inpath;
  int            i;
  struct dnsblexempts *dnsblexempts;
  char *privs;

  assert(0 != cptr);
  assert(0 != cli_local(cptr));

  inpath = cli_name(cptr);

  if (IsUnknown(cptr)) {
    if (aconf->passwd[0])
      sendrawto_one(cptr, MSG_PASS " :%s", aconf->passwd);
    /*
     *  Pass my info to the new server
     */
    sendrawto_one(cptr, MSG_SERVER " %s 1 %Tu %Tu J%s %s%s +%s :%s",
		  cli_name(&me), cli_serv(&me)->timestamp,
		  cli_serv(cptr)->timestamp, MAJOR_PROTOCOL, NumServCap(&me),
		  feature_bool(FEAT_HUB) ? "h" : "",
		  *(cli_info(&me)) ? cli_info(&me) : "IRCers United");
  }

  det_confs_butmask(cptr, CONF_SERVER);

  if (!IsHandshake(cptr))
    hAddClient(cptr);
  SetServer(cptr);
  cli_handler(cptr) = SERVER_HANDLER;
  Count_unknownbecomesserver(UserStats);

  release_dns_reply(cptr);

  SetBurst(cptr);

/*    nextping = CurrentTime; */

  /*
   * NOTE: check for acptr->user == cptr->serv->user is necessary to insure
   * that we got the same one... bleah
   */
  if (cli_serv(cptr)->user && *(cli_serv(cptr))->by &&
      (acptr = findNUser(cli_serv(cptr)->by))) {
    if (cli_user(acptr) == cli_serv(cptr)->user) {
      sendcmdto_one(&me, CMD_NOTICE, acptr, "%C :Link with %s established.",
                    acptr, inpath);
    }
    else {
      /*
       * if not the same client, set by to empty string
       */
      acptr = 0;
      *(cli_serv(cptr))->by = '\0';
    }
  }

  sendto_opmask_butone(acptr, SNO_OLDSNO, "Link with %s established.", inpath);
  cli_serv(cptr)->up = &me;
  cli_serv(cptr)->updown = add_dlink(&(cli_serv(&me))->down, cptr);
  map_update(cptr);
  sendto_opmask_butone(0, SNO_NETWORK, "Net junction: %s %s", cli_name(&me),
                       cli_name(cptr));
  SetJunction(cptr);
  /*
   * Old sendto_serv_but_one() call removed because we now
   * need to send different names to different servers
   * (domain name matching) Send new server to other servers.
   */
  for (i = 0; i <= HighestFd; i++)
  {
    if (!(acptr = LocalClientArray[i]) || !IsServer(acptr) ||
        acptr == cptr || IsMe(acptr))
      continue;
    if (!match(cli_name(&me), cli_name(cptr)))
      continue;
    sendcmdto_one(&me, CMD_SERVER, acptr,
		  "%s 2 0 %Tu J%02u %s%s +%s%s :%s", cli_name(cptr),
		  cli_serv(cptr)->timestamp, Protocol(cptr), NumServCap(cptr),
		  IsHub(cptr) ? "h" : "", IsService(cptr) ? "s" : "",
		  cli_info(cptr));
  }

  /* Send these as early as possible so that glined users/juped servers can
   * be removed from the network while the remote server is still chewing
   * our burst.
   */
  gline_burst(cptr);
  zline_burst(cptr);
  shun_burst(cptr);
  jupe_burst(cptr);

  /*
   * Pass on my client information to the new server
   *
   * First, pass only servers (idea is that if the link gets
   * cancelled beacause the server was already there,
   * there are no NICK's to be cancelled...). Of course,
   * if cancellation occurs, all this info is sent anyway,
   * and I guess the link dies when a read is attempted...? --msa
   *
   * Note: Link cancellation to occur at this point means
   * that at least two servers from my fragment are building
   * up connection this other fragment at the same time, it's
   * a race condition, not the normal way of operation...
   */

  for (acptr = &me; acptr; acptr = cli_prev(acptr)) {
    /* acptr->from == acptr for acptr == cptr */
    if (cli_from(acptr) == cptr)
      continue;
    if (IsServer(acptr)) {
      const char* protocol_str;

      if (Protocol(acptr) > 9)
        protocol_str = IsBurst(acptr) ? "J" : "P";
      else
        protocol_str = IsBurst(acptr) ? "J0" : "P0";

      if (0 == match(cli_name(&me), cli_name(acptr)))
        continue;
      sendcmdto_one(cli_serv(acptr)->up, CMD_SERVER, cptr,
		    "%s %d 0 %Tu %s%u %s%s +%s%s :%s", cli_name(acptr),
		    cli_hopcount(acptr) + 1, cli_serv(acptr)->timestamp,
		    protocol_str, Protocol(acptr), NumServCap(acptr),
		    IsHub(acptr) ? "h" : "", IsService(acptr) ? "s" : "",
		    cli_info(acptr));
    }
  }

  for (dnsblexempts = DNSBLExemptList; dnsblexempts; dnsblexempts = dnsblexempts->next)
    sendcmdto_one(&me, CMD_EXEMPT, cptr, "%C +%s %Tu nb", cptr, dnsblexempts->host, dnsblexempts->lastseen);

  for (acptr = &me; acptr; acptr = cli_prev(acptr))
  {
    /* acptr->from == acptr for acptr == cptr */
    if (cli_from(acptr) == cptr)
      continue;
    if (IsUser(acptr))
    {
      struct SLink *lp;
      char xxx_buf[8];
      char *s = umode_str(acptr);
      sendcmdto_one(cli_user(acptr)->server, CMD_NICK, cptr,
		    "%s %d %Tu %s %s %s%s%s%s %s%s :%s",
		    cli_name(acptr), cli_hopcount(acptr) + 1, cli_lastnick(acptr),
		    cli_user(acptr)->realusername, cli_user(acptr)->realhost,
		    *s ? "+" : "", s, *s ? " " : "",
		    inttobase64(xxx_buf, ntohl(cli_ip(acptr).s_addr), 6),
		    NumNick(acptr), cli_info(acptr));
      if (cli_user(acptr)->away)
        sendcmdto_one(acptr, CMD_AWAY, cptr, ":%s", cli_user(acptr)->away);
      if (cli_user(acptr)->swhois)
	sendcmdto_one(cli_user(acptr)->server, CMD_SWHOIS, cptr,
		      "%C :%s", acptr, cli_user(acptr)->swhois);
      if (IsDNSBLMarked(acptr)) /* Burst even if dnsbl is disabled */
      {
        struct SLink*  lph;
        char* dnsblhost = cli_user(acptr)->dnsblhost;
        if(!dnsblhost[0])
            dnsblhost = "notmarked";

        sendcmdto_one(cli_user(acptr)->server, CMD_MARK, cptr, "%s %s ma %s", cli_name(acptr), MARK_DNSBL,
                      dnsblhost);

        for (lph = cli_sdnsbls(acptr); lph; lph = lph->next)
           sendcmdto_one(cli_user(acptr)->server, CMD_MARK, cptr, "%s %s %s", cli_name(acptr),
                         MARK_DNSBL_DATA, lph->value.cp);

      }

      if (cli_version(acptr))
        sendcmdto_one(cli_user(acptr)->server, CMD_MARK, cptr, "%s %s :%s", cli_name(acptr), MARK_CVERSION,
                      cli_version(acptr));

      for (lp = cli_user(acptr)->silence; lp; lp = lp->next)
        sendcmdto_one(cli_user(acptr)->server, CMD_SILENCE, cptr, "%C +%s", acptr, lp->value.cp);

      privs = client_print_privs(acptr);
      if (strlen(privs) > 1)
        sendcmdto_one(cli_user(acptr)->server, CMD_PRIVS, cptr, "%C %s", acptr, privs);
      else
        sendcmdto_one(cli_user(acptr)->server, CMD_PRIVS, cptr, "%C PRIV_NONE", acptr);
    }
  }

  /*
   * Last, send the BURST.
   * (Or for 2.9 servers: pass all channels plus statuses)
   */
  {
    struct Channel *chptr;
    for (chptr = GlobalChannelList; chptr; chptr = chptr->next)
      send_channel_modes(cptr, chptr);
  }
  sendcmdto_one(&me, CMD_END_OF_BURST, cptr, "");
  return 0;
}

/*
 * m_randquote
 * - Ported from Ultimate IRCd
 */
int m_randquote(struct Client *cptr, struct Client *sptr, int parc, char *parv[]) {
  int fd, linenum=0, randnum;
  char line[300];
  char *tmp;
  char qfile[1024];

  ircd_snprintf(0, qfile, sizeof(qfile), "%s/%s", DPATH,
		feature_str(FEAT_QPATH));
  fd = open(qfile, O_RDONLY);

  if (fd == -1)
    return 0;

  dgets(-1, NULL, 0);
  while (dgets(fd, line, sizeof(line)-1) > 0) {
    if ((tmp = (char *)index(line,'\n')))
      *tmp = '\0';
    if ((tmp = (char *)index(line,'\r')))
      *tmp = '\0';
    linenum++;
  }

  /* We have an empty file.. bail */
  if (linenum == 0)
    return 0;

  /* srand(CurrentTime+getpid()+rand() % 9999); */
  srandom(CurrentTime); /* may not be the BEST salt, but its close */

  randnum = (random() % linenum)-1;

  close(fd);
  alarm(3);
  fd = open(qfile, O_RDONLY);
  linenum = 0;

  while (dgets(fd, line, sizeof(line)-1) > 0) {
    if ((tmp = (char *)index(line,'\n')))
      *tmp = '\0';
    if ((tmp = (char *)index(line,'\r')))
      *tmp = '\0';
    linenum++;
    if (linenum==randnum)
      break;
  }

  if (line != NULL)
    sendcmdto_one(&me, CMD_NOTICE, sptr, "%C \2Quote:\2 %s", sptr, line);

  return close(fd);
}
