/*
 * IRC - Internet Relay Chat, ircd/m_pass.c
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

#include "client.h"
#include "ircd_alloc.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "send.h"
#include "ircd_struct.h"
#include "s_user.h"
#include "msg.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

/* XXX */
#include "s_debug.h"

/*
 * mr_pass - registration message handler
 */
int mr_pass(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  const char* password;
  char* loc = NULL;
  char* locargv[3] = {NULL, NULL, NULL};

  assert(0 != cptr);
  assert(cptr == sptr);
  assert(!IsRegistered(sptr));

  /* TODO: For protocol negotiation */
#if 0
  if (ircd_strcmp(password,"PROT")==0) {
      /* Do something here */
  }
#endif

  if (parc == 2 && parv[1][0] == ':') {
    /*
     * All hail code duplication! 
     *
     * Here we emulate parse_client, for the benefit of buggy clients.
     * If there's only one arg that starts with a literal ':', we
     * split it again. Thus, a valid line might be:
     *     PASS ::X username :pass phrase
     * or
     *     PASS ::I-line-pass X username :pass phrase
     * or
     *     PASS ::I-Line-pass /X/username/passphrase
     *     PASS ::/X/username/passphrase
     *     PASS ::/username/passphrase
     *     PASS ::/passphrase   ??pull username from user/ident string??
     *
     * after a while we'll remove the non '/' version
     * when noone is using it anymore, and clean
     * this function up significantly.
     */
     char* s = &parv[1][1];
     
     parc = 1;
     for (;;) {
       while (*s == ' ')
         *s++ = 0;
       if (*s == 0)
         break;
       if (*s == ':') {
         parv[parc++] = s + 1;
         break;
       }
       parv[parc++] = s;
       if (parc >= MAXPARA)
         break;
       while (*s != ' ' && *s)
         s++;
     }
     parv[parc] = NULL;
  }

  password = parc > 1 ? parv[1] : 0;
  if (!EmptyString(password))
    ircd_strncpy(cli_passwd(cptr), password, PASSWDLEN);

  if (feature_bool(FEAT_LOGIN_ON_CONNECT) && !cli_loc(cptr)) {
    /* Check for leading '/' to indicate new-fangled LOC syntax */
    if (parc > 1 && parv[1][0] == '/')
      loc = parv[1]+1;
    else if (parc > 2 && parv[2][0] == '/')
      loc = parv[2]+1;
    if (loc && *loc) { /* Split loc up into locargv[0 through 2] */
      int locargc = 0;
      locargv[locargc++] = loc;
      for ( ; *loc; loc++) {
        if (*loc == '/') {
          *loc = 0;
          locargv[locargc++] = loc + 1;
          if (locargc > 2)
            break;
        }
      }
      if (locargc == 2) {
        /* Missing service nick, fill in default and shift arguments up */
        locargv[2] = locargv[1];
        locargv[1] = locargv[0];
        locargv[0] = (char *)feature_str(FEAT_LOC_DEFAULT_SERVICE);
      }
    }
    else if (parc > 3) { /* Old style for backward compatability: */
      locargv[0] = parv[parc - 3];
      locargv[1] = parv[parc - 2];
      locargv[2] = parv[parc - 1];
    }
    if (locargv[0] && *locargv[0] && locargv[1] && *locargv[1]) {
      cli_loc(cptr) = MyMalloc(sizeof(struct LOCInfo));
      memset(cli_loc(cptr), 0, sizeof(struct LOCInfo));

      cli_loc(cptr)->cookie = 0;

      ircd_strncpy(cli_loc(cptr)->service, locargv[0], NICKLEN);
      ircd_strncpy(cli_loc(cptr)->account, locargv[1], ACCOUNTLEN);

      if (locargv[2] && *locargv[2])
        ircd_strncpy(cli_loc(cptr)->password, locargv[2], ACCPASSWDLEN);
      else
        ircd_strncpy(cli_loc(cptr)->password, "", ACCPASSWDLEN);
    }
    
    /* handle retries */
    if ((cli_name(cptr))[0] && cli_cookie(cptr) == COOKIE_VERIFIED)
      return register_user(cptr, cptr, cli_name(cptr), cli_user(cptr)->username);
  }
  return 0;
}
