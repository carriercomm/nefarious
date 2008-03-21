#ifndef INCLUDED_features_h
#define INCLUDED_features_h
/*
 * IRC - Internet Relay Chat, include/features.h
 * Copyright (C) 2000 Kevin L. Mitchell <klmitch@mit.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
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

struct Client;
struct StatDesc;

extern struct Client his;

enum Feature {
  /* Misc. features */
  FEAT_LOG,
  FEAT_DOMAINNAME,
  FEAT_RELIABLE_CLOCK,
  FEAT_BUFFERPOOL,
  FEAT_HAS_FERGUSON_FLUSHER,
  FEAT_CLIENT_FLOOD,
  FEAT_SERVER_PORT,
  FEAT_NODEFAULTMOTD,
  FEAT_MOTD_BANNER,
  FEAT_PROVIDER,
  FEAT_KILL_IPMISMATCH,
  FEAT_IDLE_FROM_MSG,
  FEAT_HUB,
  FEAT_WALLOPS_OPER_ONLY,
  FEAT_NODNS,
  FEAT_RANDOM_SEED,
  FEAT_DEFAULT_LIST_PARAM,
  FEAT_NICKNAMEHISTORYLENGTH,
  FEAT_HOST_HIDING,
  FEAT_HIDDEN_HOST,
  FEAT_HIDDEN_IP,
  FEAT_CONNEXIT_NOTICES,

  /* features that probably should not be touched */
  FEAT_KILLCHASETIMELIMIT,
  FEAT_MAXCHANNELSPERUSER,
  FEAT_NICKLEN,
  FEAT_CHANNELLEN,
  FEAT_AVBANLEN,
  FEAT_MAXBANS,
  FEAT_MAXSILES,
  FEAT_MAXWATCHS,
  FEAT_HANGONGOODLINK,
  FEAT_HANGONRETRYDELAY,
  FEAT_CONNECTTIMEOUT,
  FEAT_TIMESEC,
  FEAT_MAXIMUM_LINKS,
  FEAT_PINGFREQUENCY,
  FEAT_CONNECTFREQUENCY,
  FEAT_DEFAULTMAXSENDQLENGTH,
  FEAT_GLINEMAXUSERCOUNT,
  FEAT_SOCKSENDBUF,
  FEAT_SOCKRECVBUF,
  FEAT_IPCHECK_CLONE_LIMIT,
  FEAT_IPCHECK_CLONE_PERIOD,
  FEAT_IPCHECK_CLONE_DELAY,

  /* Some misc. default paths */
  FEAT_MPATH,
  FEAT_RPATH,
  FEAT_PPATH,

  /* Networking features */
  FEAT_VIRTUAL_HOST,
  FEAT_TOS_SERVER,
  FEAT_TOS_CLIENT,
  FEAT_POLLS_PER_LOOP,
  FEAT_IRCD_RES_RETRIES,
  FEAT_IRCD_RES_TIMEOUT,
  FEAT_AUTH_TIMEOUT,

  /* features that affect all operators */
  FEAT_CRYPT_OPER_PASSWORD,
  FEAT_OPER_NO_CHAN_LIMIT,
  FEAT_OPER_MODE_LCHAN,
  FEAT_OPER_WALK_THROUGH_LMODES,
  FEAT_NO_OPER_DEOP_LCHAN,
  FEAT_SHOW_INVISIBLE_USERS,
  FEAT_SHOW_ALL_INVISIBLE_USERS,
  FEAT_UNLIMIT_OPER_QUERY,
  FEAT_LOCAL_KILL_ONLY,
  FEAT_CONFIG_OPERCMDS,

  /* features that affect global opers on this server */
  FEAT_OPER_KILL,
  FEAT_OPER_REHASH,
  FEAT_OPER_RESTART,
  FEAT_OPER_DIE,
  FEAT_OPER_GLINE,
  FEAT_OPER_LGLINE,
  FEAT_OPER_JUPE,
  FEAT_OPER_LJUPE,
  FEAT_OPER_OPMODE,
  FEAT_OPER_LOPMODE,
  FEAT_OPER_FORCE_OPMODE,
  FEAT_OPER_FORCE_LOPMODE,
  FEAT_OPER_BADCHAN,
  FEAT_OPER_LBADCHAN,
  FEAT_OPER_SET,
  FEAT_OPERS_SEE_IN_SECRET_CHANNELS,
  FEAT_OPER_WIDE_GLINE,

  /* features that affect local opers on this server */
  FEAT_LOCOP_KILL,
  FEAT_LOCOP_REHASH,
  FEAT_LOCOP_RESTART,
  FEAT_LOCOP_DIE,
  FEAT_LOCOP_LGLINE,
  FEAT_LOCOP_LJUPE,
  FEAT_LOCOP_LOPMODE,
  FEAT_LOCOP_FORCE_LOPMODE,
  FEAT_LOCOP_LBADCHAN,
  FEAT_LOCOP_SET,
  FEAT_LOCOP_SEE_IN_SECRET_CHANNELS,
  FEAT_LOCOP_WIDE_GLINE,

  /* HEAD_IN_SAND Features */
  FEAT_HIS_SNOTICES,
  FEAT_HIS_SNOTICES_OPER_ONLY,
  FEAT_HIS_DESYNCS,
  FEAT_HIS_DEBUG_OPER_ONLY,
  FEAT_HIS_WALLOPS,
  FEAT_HIS_MAP,
  FEAT_HIS_LINKS,
  FEAT_HIS_TRACE,
  FEAT_HIS_STATS_l,
  FEAT_HIS_STATS_c,
  FEAT_HIS_STATS_g,
  FEAT_HIS_STATS_h,
  FEAT_HIS_STATS_k,
  FEAT_HIS_STATS_f,
  FEAT_HIS_STATS_i,
  FEAT_HIS_STATS_j,
  FEAT_HIS_STATS_M,
  FEAT_HIS_STATS_m,
  FEAT_HIS_STATS_o,
  FEAT_HIS_STATS_p,
  FEAT_HIS_STATS_q,
  FEAT_HIS_STATS_r,
  FEAT_HIS_STATS_d,
  FEAT_HIS_STATS_e,
  FEAT_HIS_STATS_t,
  FEAT_HIS_STATS_T,
  FEAT_HIS_STATS_u,
  FEAT_HIS_STATS_U,
  FEAT_HIS_STATS_v,
  FEAT_HIS_STATS_w,
  FEAT_HIS_STATS_x,
  FEAT_HIS_STATS_y,
  FEAT_HIS_STATS_z,
  FEAT_HIS_WHOIS_SERVERNAME,
  FEAT_HIS_WHOIS_IDLETIME,
  FEAT_HIS_WHOIS_LOCALCHAN,
  FEAT_HIS_WHO_SERVERNAME,
  FEAT_HIS_WHO_HOPCOUNT,
  FEAT_HIS_BANWHO,
  FEAT_HIS_KILLWHO,
  FEAT_HIS_REWRITE,
  FEAT_HIS_REMOTE,
  FEAT_HIS_NETSPLIT,
  FEAT_HIS_SERVERNAME,
  FEAT_HIS_SERVERINFO,
  FEAT_HIS_URLSERVERS,

  /* Misc. random stuff */
  FEAT_NETWORK,
  FEAT_URL_CLIENTS,

  /* Nefarious features */
  FEAT_NEFARIOUS,
  FEAT_OMPATH,
  FEAT_QPATH,
  FEAT_EPATH,
  FEAT_TPATH,
  FEAT_HOST_HIDING_STYLE,
  FEAT_HOST_HIDING_PREFIX,
  FEAT_HOST_HIDING_KEY1,
  FEAT_HOST_HIDING_KEY2,
  FEAT_HOST_HIDING_KEY3,
  FEAT_OPERHOST_HIDING,
  FEAT_HIDDEN_OPERHOST,
  FEAT_TOPIC_BURST,
  FEAT_REMOTE_OPER,
  FEAT_REMOTE_MOTD,
  FEAT_BOT_CLASS,
  FEAT_LOCAL_CHANNELS,
  FEAT_OPER_LOCAL_CHANNELS,
  FEAT_OPER_XTRAOP,
  FEAT_XTRAOP_CLASS,
  FEAT_OPER_HIDECHANS,
  FEAT_OPER_HIDEIDLE,
  FEAT_CHECK,
  FEAT_CHECK_EXTENDED,
  FEAT_OPER_SINGLELETTERNICK,
  FEAT_SETHOST,
  FEAT_SETHOST_FREEFORM,
  FEAT_SETHOST_USER,
  FEAT_SETHOST_AUTO,
  FEAT_FAKEHOST,
  FEAT_DEFAULT_FAKEHOST,
  FEAT_HIS_STATS_B,
  FEAT_HIS_STATS_b,
  FEAT_HIS_STATS_R,
  FEAT_HIS_STATS_s,
  FEAT_HIS_GLINE,
  FEAT_HIS_USERGLINE,
  FEAT_HIS_USERIP,
  FEAT_AUTOJOIN_USER,
  FEAT_AUTOJOIN_USER_CHANNEL,
  FEAT_AUTOJOIN_USER_NOTICE,
  FEAT_AUTOJOIN_USER_NOTICE_VALUE,
  FEAT_AUTOJOIN_OPER,
  FEAT_AUTOJOIN_OPER_CHANNEL,
  FEAT_AUTOJOIN_OPER_NOTICE,
  FEAT_AUTOJOIN_OPER_NOTICE_VALUE,
  FEAT_AUTOJOIN_ADMIN,
  FEAT_AUTOJOIN_ADMIN_CHANNEL,
  FEAT_AUTOJOIN_ADMIN_NOTICE,
  FEAT_AUTOJOIN_ADMIN_NOTICE_VALUE,
  FEAT_QUOTES,
  FEAT_POLICY_NOTICE,
  FEAT_BADCHAN_REASON,
  FEAT_RULES,
  FEAT_OPERMOTD,
  FEAT_GEO_LOCATION,
  FEAT_DEFAULT_UMODE,
  FEAT_HOST_IN_TOPIC,
  FEAT_TIME_IN_TIMEOUT,
  FEAT_HALFOPS,
  FEAT_EXCEPTS,
  FEAT_BREAK_P10,
  FEAT_AVEXCEPTLEN,
  FEAT_MAXEXCEPTS,
  FEAT_HIS_EXCEPTWHO,
  FEAT_TARGET_LIMITING,
  FEAT_BADUSER_URL,
  FEAT_STATS_C_IPS,
  FEAT_HIS_IRCOPS,
  FEAT_HIS_IRCOPS_SERVERS,
  FEAT_HIS_MAP_SCRAMBLED,
  FEAT_HIS_LINKS_SCRAMBLED,
  FEAT_HIS_SCRAMBLED_CACHE_TIME,
  FEAT_FLEXABLEKEYS,
  FEAT_NOTHROTTLE,
  FEAT_CREATE_CHAN_OPER_ONLY,
  FEAT_MAX_CHECK_OUTPUT,
  FEAT_RESTARTPASS,
  FEAT_DIEPASS,
  FEAT_DNSBL_CHECKS,
  FEAT_DNSBL_EXEMPT_CLASS,
  FEAT_HIS_STATS_X,
  FEAT_ANNOUNCE_INVITES,
  FEAT_OPERFLAGS,
  FEAT_WHOIS_OPER,
  FEAT_WHOIS_ADMIN,
  FEAT_WHOIS_SERVICE,
  FEAT_AUTOCHANMODES,
  FEAT_AUTOCHANMODES_LIST,
  FEAT_LOGIN_ON_CONNECT,
  FEAT_EXTENDED_ACCOUNTS,
  FEAT_LOC_DEFAULT_SERVICE,
  FEAT_DNSBL_LOC_EXEMPT,
  FEAT_DNSBL_LOC_EXEMPT_N_ONE,
  FEAT_DNSBL_LOC_EXEMPT_N_TWO,
  FEAT_DNSBL_WALLOPS_ONLY,
  FEAT_DNSBL_MARK_FAKEHOST,
  FEAT_OPER_WHOIS_SECRET,
  FEAT_AUTOINVISIBLE,
  FEAT_SWHOIS,
  FEAT_CHMODE_a,
  FEAT_CHMODE_c,
  FEAT_CHMODE_z,
  FEAT_CHMODE_C,
  FEAT_CHMODE_L,
  FEAT_CHMODE_M,
  FEAT_CHMODE_N,
  FEAT_CHMODE_O,
  FEAT_CHMODE_Q,
  FEAT_CHMODE_S,
  FEAT_CHMODE_T,
  FEAT_CHMODE_Z,
  FEAT_LUSERS_AUTHED,
  FEAT_OPER_WHOIS_PARANOIA,
  FEAT_SHUNMAXUSERCOUNT,
  FEAT_OPER_SHUN,
  FEAT_OPER_LSHUN,
  FEAT_OPER_WIDE_SHUN,
  FEAT_LOCOP_LSHUN,
  FEAT_LOCOP_WIDE_SHUN,
  FEAT_HIS_SHUN,
  FEAT_HIS_USERSHUN,
  FEAT_HIS_STATS_S,
  FEAT_HIS_SHUN_REASON,
  FEAT_ERR_OPERONLYCHAN,
  FEAT_EXEMPT_EXPIRE,
  FEAT_HIS_HIDEWHO,
  FEAT_STRICTUSERNAME,
  FEAT_SET_ACTIVE_ON_CREATE,
  FEAT_DEF_ALIST_LIMIT,
  FEAT_ALIST_SEND_FREQ,
  FEAT_ALIST_SEND_DIFF,
  FEAT_ZLINEMAXUSERCOUNT,
  FEAT_OPER_ZLINE,
  FEAT_OPER_LZLINE,
  FEAT_OPER_WIDE_ZLINE,
  FEAT_LOCOP_LZLINE,
  FEAT_LOCOP_WIDE_ZLINE,
  FEAT_HIS_ZLINE,
  FEAT_HIS_USERZLINE,
  FEAT_HIS_STATS_Z,
  FEAT_HIS_ZLINE_REASON,
  FEAT_NICK_DELAY,

  /* Added as part of WEBIRC support */
  FEAT_HIS_STATS_W,
  FEAT_WEBIRC_SPOOFIDENT,
  FEAT_WEBIRC_FAKEIDENT,
  FEAT_WEBIRC_USERIDENT,

  FEAT_CTCP_VERSIONING, /* added by Vadtec 02/25/2008 */
  FEAT_CTCP_VERSIONING_KILL, /* added by Vadtec 02/27/2008 */
  FEAT_CTCP_VERSIONING_CHAN, /* added by Vadtec 02/27/2008 */
  FEAT_CTCP_VERSIONING_CHANNAME, /* added by Vadtec 02/27/2008 */
  FEAT_CTCP_VERSIONING_USEMSG, /* added by Vadtec 02/28/2008 */
  FEAT_CTCP_VERSIONING_NOTICE,

  /* Really special features (tm) */
  FEAT_NETWORK_REHASH,
  FEAT_NETWORK_RESTART,
  FEAT_NETWORK_DIE,

  FEAT_LAST_F
};

extern void feature_init(void);

extern int feature_set(struct Client* from, const char* const* fields,
		       int count);
extern int feature_reset(struct Client* from, const char* const* fields,
			 int count);
extern int feature_get(struct Client* from, const char* const* fields,
		       int count);

extern void feature_unmark(void);
extern void feature_mark(void);

extern void feature_report(struct Client* to, struct StatDesc* sd, int stat,
			   char* param);

extern int feature_int(enum Feature feat);
extern int feature_bool(enum Feature feat);
extern const char *feature_str(enum Feature feat);

#endif /* INCLUDED_features_h */
