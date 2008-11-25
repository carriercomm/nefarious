/*
 * IRC - Internet Relay Chat, include/client.h
 * Copyright (C) 1990 Jarkko Oikarinen and
 *                    University of Oulu, Computing Center
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
 */
/** @file
 * @brief Structures and functions for handling local clients.
 * @version $Id$
 */

#ifndef INCLUDED_client_h
#define INCLUDED_client_h
#ifndef INCLUDED_ircd_defs_h
#include "ircd_defs.h"
#endif
#ifndef INCLUDED_dbuf_h
#include "dbuf.h"
#endif
#ifndef INCLUDED_flagset_h
#include "flagset.h"
#endif
#ifndef INCLUDED_msgq_h
#include "msgq.h"
#endif
#ifndef INCLUDED_ircd_events_h
#include "ircd_events.h"
#endif
#ifndef INCLUDED_ircd_handler_h
#include "ircd_handler.h"
#endif
#ifndef INCLUDED_sys_types_h
#include <sys/types.h>          /* time_t, size_t */
#define INCLUDED_sys_types_h
#endif
#ifndef INCLUDED_netinet_in_h
#include <netinet/in.h>         /* in_addr */
#define INCLUDED_netinet_in_h
#endif

struct ConfItem;
struct Listener;
struct ListingArgs;
struct SLink;
struct Server;
struct User;
struct Whowas;
struct DNSReply;
struct hostent;
struct Privs;
struct AuthRequest;
struct LOCInfo;


/*
 * Structures
 *
 * Only put structures here that are being used in a very large number of
 * source files. Other structures go in the header file of there corresponding
 * source file, or in the source file itself (when only used in that file).
 */

/** Operator privileges. */
enum Priv {
  PRIV_CHAN_LIMIT,      /**< no channel limit on oper */
  PRIV_MODE_LCHAN,      /**< oper can mode local chans */
  PRIV_WALK_LCHAN,      /**< oper can walk through local modes */
  PRIV_DEOP_LCHAN,      /**< no deop oper on local chans */
  PRIV_SHOW_INVIS,      /**< show local invisible users */
  PRIV_SHOW_ALL_INVIS,  /**< show all invisible users */
  PRIV_UNLIMIT_QUERY,   /**< unlimit who queries */
  PRIV_KILL,            /**< oper can KILL */
  PRIV_LOCAL_KILL,      /**< oper can local KILL */
  PRIV_REHASH,          /**< oper can REHASH */
  PRIV_REMOTEREHASH,    /**< oper can remote REHASH */
  PRIV_RESTART,         /**< oper can RESTART */
  PRIV_DIE,             /**< oper can DIE */
  PRIV_GLINE,           /**< oper can GLINE */
  PRIV_LOCAL_GLINE,     /**< oper can local ZLINE */
  PRIV_ZLINE,           /**< oper can GLINE */
  PRIV_LOCAL_ZLINE,     /**< oper can local ZLINE */
  PRIV_SHUN,            /**< oper can SHUN */
  PRIV_LOCAL_SHUN,      /**< oper can local SHUN */
  PRIV_JUPE,            /**< oper can JUPE */
  PRIV_LOCAL_JUPE,      /**< oper can local JUPE */
  PRIV_OPMODE,          /**< oper can OP/CLEARMODE */
  PRIV_LOCAL_OPMODE,    /**< oper can local OP/CLEARMODE */
  PRIV_SET,             /**< oper can SET */
  PRIV_WHOX,            /**< oper can use /who x */
  PRIV_BADCHAN,         /**< oper can BADCHAN */
  PRIV_LOCAL_BADCHAN,   /**< oper can local BADCHAN */
  PRIV_SEE_CHAN,        /**< oper can see in secret chans */
  PRIV_PROPAGATE,       /**< propagate oper status */
  PRIV_DISPLAY,         /**< "Is an oper" displayed */
  PRIV_SEE_OPERS,       /**< display hidden opers */
  PRIV_WIDE_GLINE,      /**< oper can set wider G-lines */
  PRIV_WIDE_ZLINE,      /**< oper can set wider Z-lines */
  PRIV_WIDE_SHUN,       /**< oper can set wider Shuns */
  PRIV_LIST_CHAN,       /**< oper can list secret channels */
  PRIV_FORCE_OPMODE,    /**< can hack modes on quarantined channels */
  PRIV_FORCE_LOCAL_OPMODE, /**< can hack modes on quarantined local channels */
  PRIV_CHECK,		/**< oper can use CHECK */
  PRIV_SEE_SECRET_CHAN,	/**< oper can see +s channels in whois */
  PRIV_WHOIS_NOTICE,    /**< oper can set/unset user mode +W */
  PRIV_HIDE_IDLE,       /**< oper can set/unset user mode +I */
  PRIV_XTRAOP,          /**< oper can set/unset user mode +X */
  PRIV_HIDE_CHANNELS,   /**< oper can set/unset user mode +n */
  PRIV_LAST_PRIV        /**< number of privileges */
};

/** Client flags and modes.
 * Note that flags at least FLAG_LOCAL_UMODES but less than
 * FLAG_GLOBAL_UMODES are treated as local modes, and flags at least
 * FLAG_GLOBAL_UMODES (but less than FLAG_LAST_FLAG) are treated as
 * global modes.
 */
enum Flag {
    FLAG_PINGSENT,                  /**< Unreplied ping sent */
    FLAG_DEADSOCKET,                /**< Local socket is dead--Exiting soon */
    FLAG_KILLED,                    /**< Prevents "QUIT" from being sent for this */
    FLAG_BLOCKED,                   /**< socket is in a blocked condition */
    FLAG_CLOSING,                   /**< set when closing to suppress errors */
    FLAG_UPING,                     /**< has active UDP ping request */
    FLAG_CHKACCESS,                 /**< ok to check clients access if set */
    FLAG_HUB,                       /**< server is a hub */
    FLAG_SERVICE,                   /**< server is a service */
    FLAG_LOCAL,                     /**< set for local clients */
    FLAG_GOTID,                     /**< successful ident lookup achieved */
    FLAG_DOID,                      /**< I-lines say must use ident return */
    FLAG_NONL,                      /**< No \n in buffer */
    FLAG_TS8,                       /**< Why do you want to know? */
    FLAG_MAP,                       /**< Show server on the map */
    FLAG_JUNCTION,                  /**< Junction causing the net.burst. */
    FLAG_BURST,                     /**< Server is receiving a net.burst */
    FLAG_BURST_ACK,                 /**< Server is waiting for eob ack */
    FLAG_IPCHECK,                   /**< Added or updated IPregistry data */
    FLAG_LOCOP,                     /**< Local operator -- SRB */
    FLAG_SERVNOTICE,                /**< server notices such as kill */
    FLAG_OPER,                      /**< Operator */
    FLAG_INVISIBLE,                 /**< makes user invisible */
    FLAG_WALLOP,                    /**< send wallops to them */
    FLAG_DEAF,                      /**< Makes user deaf */
    FLAG_CHSERV,                    /**< Disallow KICK or MODE -o on the user;
                                       don't display channels in /whois */
    FLAG_DEBUG,                     /**< send global debug/anti-hack info */
    FLAG_ACCOUNT,                   /**< account name has been set */
    FLAG_HIDDENHOST,                /**< user's host is hidden */
    FLAG_CLOAKHOST,                 /**< style 2 cloaked host */
    FLAG_CLOAKIP,                   /**< style 2 cloaked ip */

    FLAG_SETHOST,                   /**< oper's host is changed */
    FLAG_FAKEHOST,                  /**< user has been assigned a fake host */
    FLAG_ACCOUNTONLY,               /**< hide privmsgs/notices if user is
                                       not authed or opered */
    FLAG_REMOTEOPER,                /**< Remote operator */
    FLAG_BOT,                       /**< Bot */
    FLAG_SSL,                       /**< SSL user */
    FLAG_NOCHAN,                    /**< ASUKA_n: hide channels */
    FLAG_NOIDLE,                    /**< ASUKA_I: hide idle time */
    FLAG_XTRAOP,                    /**< ASUKA_X: oper special powers */
    FLAG_ADMIN,                     /**< Client is an admin */
    FLAG_WHOIS,                     /**< oper sees who /WHOISes him/her */
    FLAG_DNSBL,                     /**< Client in DNSBL */
    FLAG_DNSBLMARKED,               /**< Client in DNSBL Marked */
    FLAG_DNSBLALLOWED,              /**< Client in DNSBL Allowed in */
    FLAG_DNSBLDENIED,               /**< Only set if a client is found on a d X:line */
    FLAG_SPAM,                      /**< Only set if a client has triggered a spam filter */

    FLAG_WEBIRC,                    /**< User registered with WEBIRC command */
    FLAG_WEBIRC_UIDENT,             /**< Use USER method for getting IDENT */

    FLAG_NAMESX,                    /**< Client supports extended NAMES replies */
    FLAG_UHNAMES,                   /**< Client supports extended NAMES replies */
    FLAG_PROPAGATED,                /**< Client has been propagated to other servers */

    FLAG_PRIVDEAF,                  /**< Client is deaf to all private messages */

    FLAG_LAST_FLAG,

    FLAG_LOCAL_UMODES = FLAG_LOCOP, /**< First local mode flag */
    FLAG_GLOBAL_UMODES = FLAG_OPER  /**< First global mode flag */
};

DECLARE_FLAGSET(Privs, PRIV_LAST_PRIV);
DECLARE_FLAGSET(Flags, FLAG_LAST_FLAG);

/** Represents a local connection.
 * This contains a lot of stuff irrelevant to server connections, but
 * those are so rare as to not be worth special-casing.
 */
struct Connection {
  unsigned long       con_magic;      /**< magic number */
  struct Connection*  con_next;       /**< Next connection with queued data */
  struct Connection** con_prev_p;     /**< What points to us */
  struct Client*      con_client;     /**< Client associated with connection */
  unsigned int        con_count;      /**< Amount of data in buffer */
  int                 con_fd;         /**< >= 0, for local clients */
  int                 con_freeflag;   /**< indicates if connection can be freed */
  int                 con_error;      /**< last socket level error for client */
  int                 con_sentalong;  /**< sentalong marker for connection */
  unsigned int        con_snomask;    /**< mask for server messages */
  time_t              con_nextnick;   /**< Next time a nick change is allowed */
  time_t              con_nexttarget; /**< Next time a target change is allowed */
  unsigned int        con_cookie;     /**< Random number the user must PONG */
  struct MsgQ         con_sendQ;      /**< Outgoing message queue--if socket full */
  struct DBuf         con_recvQ;      /**< Hold for data incoming yet to be parsed */
  unsigned int        con_sendM;      /**< Statistics: protocol messages send */
  unsigned int        con_sendK;      /**< Statistics: total k-bytes send */
  unsigned int        con_receiveM;   /**< Statistics: protocol messages received */
  unsigned int        con_receiveK;   /**< Statistics: total k-bytes received */
  unsigned short      con_sendB;      /**< counters to count upto 1-k lots of bytes */
  unsigned short      con_receiveB;   /**< sent and received. */
  struct Listener*    con_listener;   /**< listening client which we accepted
				         from */
  struct SLink*       con_confs;      /**< Configuration record associated */
  HandlerType         con_handler;    /**< message index into command table
				      for parsing */
  struct DNSReply*    con_dns_reply;  /**< DNS reply used during client
					registration */
  struct DNSReply*    con_dnsbl_reply; /**< DNSBL reply used during client
					registration */
  struct ListingArgs* con_listing;
  unsigned int        con_max_sendq;  /**< cached max send queue for client */
  unsigned int        con_ping_freq;  /**< cached ping freq from client conf
					class */
  unsigned short      con_lastsq;     /**< # 2k blocks when sendqueued called last */
  unsigned short      con_port;       /**< and the remote port# too :-) */
  unsigned char       con_targets[MAXTARGETS]; /**< Hash values of current
						  targets */
  char con_sock_ip[SOCKIPLEN + 1];    /**< this is the ip address as a string */
  char con_sockhost[HOSTLEN + 1];     /**< This is the host name from the socket and
				         after which the connection was accepted. */
  char con_passwd[PASSWDLEN + 1];
  char con_buffer[BUFSIZE];           /**< Incoming message buffer; or the error that
                                         caused this clients socket to be `dead' */
  struct Socket       con_socket;     /**< socket descriptor for client */
  struct Timer        con_proc;       /**< process latent messages from client */
  struct Privs        con_privs;     /**< Oper privileges */
  struct AuthRequest* con_auth;       /**< auth request for client */
  struct LOCInfo*     con_loc;        /**< Login-on-connect information */
};

/** Magic constant to identify valid Connection structures. */
#define CONNECTION_MAGIC 0x12f955f3

/** Represents a client anywhere on the network. */
struct Client {	
  unsigned long  cli_magic;     /**< magic number */
  struct Client* cli_next;      /**< link in GlobalClientList */
  struct Client* cli_prev;      /**< link in GlobalClientList */
  struct Client* cli_hnext;     /**< link in hash table bucket or this */
  struct Connection* cli_connect; /**< Connection structure associated with us */
  struct User*   cli_user;      /**< ...defined, if this is a User */
  struct Server* cli_serv;      /**< ...defined, if this is a server */
  struct Whowas* cli_whowas;    /**< Pointer to ww struct to be freed on quit */
  char           cli_yxx[4];    /**< Numeric Nick: YMM if this is a server,
                                   XX0 if this is a user */
  /*
   * XXX - move these to local part for next release
   * (lasttime, since)
   */
  time_t         cli_lasttime;  /**< last time data read from socket */
  time_t         cli_since;     /**< last time we parsed something, flood control */
				
  time_t         cli_firsttime; /**< time client was created */
  time_t         cli_lastnick;  /**< TimeStamp on nick */
  int            cli_marker;    /**< /who processing marker */
  struct Flags   cli_flags;     /**< client flags */
  unsigned int   cli_oflags;    /**< oper flags */
  unsigned int   cli_hopcount;  /**< number of servers to this 0 = local */
  unsigned int   cli_dnsblcount; /**< number of dnsbls left to check */
  struct in_addr cli_ip;        /**< Real ip# NOT defined for remote servers! */
  short          cli_status;    /**< Client type */
  unsigned char  cli_local;     /**< local or remote client */
  char cli_name[HOSTLEN + 1];   /**< Unique name of the client, nick or host */
  char cli_username[USERLEN + 1];    /**< username here now for auth stuff */
  char cli_info[REALLEN + 1];        /**< Free form additional client information */
  char cli_version[VERSIONLEN + 1];  /**< Free form client version information - added by Vadtec 02/26/2008 */
  char cli_webirc[BUFSIZE + 1];      /**< webirc description */
  char cli_dnsbl[BUFSIZE + 1];       /**< dnsbl hostname identifier */
  char cli_dnsbls[BUFSIZE + 1];      /**< all dnsbls matched identifier */
  struct SLink*   cli_sdnsbls;       /**< chain of dnsbl pointer blocks */
  char cli_dnsblformat[BUFSIZE + 1]; /**< dnsbl rejection message */
  int  cli_dnsbllastrank;            /**< last rank we got */
};

/** Magic constant to identify valid Client structures. */
#define CLIENT_MAGIC 0x4ca08286

/** Verify that a client is valid. */
#define cli_verify(cli)		((cli)->cli_magic == CLIENT_MAGIC)
/** Get client's magic number. */
#define cli_magic(cli)		((cli)->cli_magic)
/** Get global next client. */
#define cli_next(cli)		((cli)->cli_next)
/** Get global previous client. */
#define cli_prev(cli)		((cli)->cli_prev)
/** Get next client in hash bucket chain. */
#define cli_hnext(cli)		((cli)->cli_hnext)
/** Get connection associated with client. */
#define cli_connect(cli)	((cli)->cli_connect)
/** Get local client that links us to a cli. */
#define cli_from(cli)		((cli)->cli_connect->con_client)
/** Get User structure for client, if client is a user. */
#define cli_user(cli)		((cli)->cli_user)
/** Get Server structure for client, if client is a server. */
#define cli_serv(cli)		((cli)->cli_serv)
/** Return true if client has uworld privileges. */
#define cli_uworld(cli)         (cli_serv(cli) && (cli_serv(cli)->flags & SFLAG_UWORLD))
/** Get Whowas link for client. */
#define cli_whowas(cli)		((cli)->cli_whowas)
/** Get time we last parsed something from the client. */
#define cli_since(cli)		((cli)->cli_since)
/** Get client numnick. */
#define cli_yxx(cli)		((cli)->cli_yxx)
/** Get time we last read data from the client socket. */
#define cli_lasttime(cli)	((cli)->cli_lasttime)
/** Get time client was created. */
#define cli_firsttime(cli)	((cli)->cli_firsttime)
/** Get time client last changed nickname. */
#define cli_lastnick(cli)	((cli)->cli_lastnick)
/** Get WHO marker for client. */
#define cli_marker(cli)		((cli)->cli_marker)
/** Get flags flagset for client. */
#define cli_flags(cli)		((cli)->cli_flags)
/** Get hop count to client. */
#define cli_hopcount(cli)	((cli)->cli_hopcount)
/** Get client IP address. */
#define cli_ip(cli)		((cli)->cli_ip)
/** Get status bitmask for client. */
#define cli_status(cli)		((cli)->cli_status)
/** Return non-zero if the client is local. */
#define cli_local(cli)		((cli)->cli_local)
/** Get oper privileges for client. */
#define cli_privs(cli)		con_privs(cli_connect(cli))
/** Get client name. */
#define cli_name(cli)		((cli)->cli_name)
/** Get client username (ident). */
#define cli_username(cli)	((cli)->cli_username)
/** Get client version (CTCP version). */
#define cli_version(cli)	((cli)->cli_version)
/** Get client webirc description */
#define cli_webirc(cli)         ((cli)->cli_webirc)
/** Get client realname (information field). */
#define cli_info(cli)		((cli)->cli_info)
/** Get client oper flags. */
#define cli_oflags(cli)		((cli)->cli_oflags)
/** Get highest ranked dnsbl set for client. */
#define cli_dnsbl(cli)		((cli)->cli_dnsbl)
/** Get formatted string of all set dnsbls for client. */
#define cli_dnsbls(cli)		((cli)->cli_dnsbls)
/** Get all dnsbls set for client. */
#define cli_sdnsbls(cli)	((cli)->cli_sdnsbls)
/** Get dnsbl rejection message for client. */
#define cli_dnsblformat(cli)	((cli)->cli_dnsblformat)
/** Get dnsbl count for client. */
#define cli_dnsblcount(cli)	((cli)->cli_dnsblcount)
/** Get the last dnsnbl rank that the client went through. */
#define cli_dnsbllastrank(cli)  ((cli)->cli_dnsbllastrank)

/** Get number of incoming bytes queued for client. */
#define cli_count(cli)		((cli)->cli_connect->con_count)
/** Get file descriptor for sending in client's direction. */
#define cli_fd(cli)		((cli)->cli_connect->con_fd)
/** Get free flags for the client's connection. */
#define cli_freeflag(cli)	((cli)->cli_connect->con_freeflag)
/** Get last error code for the client's connection. */
#define cli_error(cli)		((cli)->cli_connect->con_error)
/** Get server notice mask for the client. */
#define cli_snomask(cli)	((cli)->cli_connect->con_snomask)
/** Get next time a nick change is allowed for the client. */
#define cli_nextnick(cli)	((cli)->cli_connect->con_nextnick)
/** Get next time a target change is allowed for the client. */
#define cli_nexttarget(cli)	((cli)->cli_connect->con_nexttarget)
/** Get required PING/PONG cookie for client. */
#define cli_cookie(cli)		((cli)->cli_connect->con_cookie)
/** Get SendQ for client. */
#define cli_sendQ(cli)		((cli)->cli_connect->con_sendQ)
/** Get RecvQ for client. */
#define cli_recvQ(cli)		((cli)->cli_connect->con_recvQ)
/** Get count of messages sent to client. */
#define cli_sendM(cli)		((cli)->cli_connect->con_sendM)
/** Get number of k-bytes sent to client. */
#define cli_sendK(cli)		((cli)->cli_connect->con_sendK)
/** Get number of messages received from client. */
#define cli_receiveM(cli)	((cli)->cli_connect->con_receiveM)
/** Get number of k-bytes recieved from client. */
#define cli_receiveK(cli)	((cli)->cli_connect->con_receiveK)
/** Get number of bytes (modulo 1024) sent to client. */
#define cli_sendB(cli)		((cli)->cli_connect->con_sendB)
/** Get number of bytes (modulo 1024) received from client. */
#define cli_receiveB(cli)	((cli)->cli_connect->con_receiveB)
/** Get listener that accepted the client's connection. */
#define cli_listener(cli)	((cli)->cli_connect->con_listener)
/** Get list of attached conf lines. */
#define cli_confs(cli)		((cli)->cli_connect->con_confs)
/** Get handler type for client. */
#define cli_handler(cli)	((cli)->cli_connect->con_handler)
/** Get DNS reply for client. */
#define cli_dns_reply(cli)	((cli)->cli_connect->con_dns_reply)
/** Get DNSBL reply for client. */
#define cli_dnsbl_reply(cli)	((cli)->cli_connect->con_dnsbl_reply)
/** Get LIST status for client. */
#define cli_listing(cli)	((cli)->cli_connect->con_listing)
/** Get cached max SendQ for client. */
#define cli_max_sendq(cli)	((cli)->cli_connect->con_max_sendq)
/** Get ping frequency for client. */
#define cli_ping_freq(cli)	((cli)->cli_connect->con_ping_freq)
/** Get lastsq for client's connection. */
#define cli_lastsq(cli)		((cli)->cli_connect->con_lastsq)
/** Get port that the client is connected to */
#define cli_port(cli)		((cli)->cli_connect->con_port)
/** Get the array of current targets for the client.  */
#define cli_targets(cli)	((cli)->cli_connect->con_targets)
/** Get the string form of the client's IP address. */
#define cli_sock_ip(cli)	((cli)->cli_connect->con_sock_ip)
/** Get the resolved hostname for the client. */
#define cli_sockhost(cli)	((cli)->cli_connect->con_sockhost)
/** Get the client's password. */
#define cli_passwd(cli)		((cli)->cli_connect->con_passwd)
/** Get sentalong marker for client. */
#define cli_sentalong(cli)      ((cli)->cli_connect->con_sentalong)
/** Get the unprocessed input buffer for a client's connection.  */
#define cli_buffer(cli)		((cli)->cli_connect->con_buffer)
/** Get the Socket structure for sending to a client. */
#define cli_socket(cli)		((cli)->cli_connect->con_socket)
/** Get Timer for processing waiting messages from the client. */
#define cli_proc(cli)		((cli)->cli_connect->con_proc)
/** Get the oper privilege set for the connection. */
#define con_privs(con)          (&(con)->con_privs)
/** Get auth request for client. */
#define cli_auth(cli)		((cli)->cli_connect->con_auth)
/** Get login on connect request for client. */
#define cli_loc(cli)		((cli)->cli_connect->con_loc)

/** Verify that a connection is valid. */
#define con_verify(con)		((con)->con_magic == CONNECTION_MAGIC)
/** Get connection's magic number. */
#define con_magic(con)		((con)->con_magic)
/** Get global next connection. */
#define con_next(con)		((con)->con_next)
/** Get global previous connection. */
#define con_prev_p(con)		((con)->con_prev_p)
/** Get locally connected client for connection. */
#define con_client(con)		((con)->con_client)
/** Get number of unprocessed data bytes from connection. */
#define con_count(con)		((con)->con_count)
/** Get file descriptor for connection. */
#define con_fd(con)		s_fd(&(con)->con_socket)
/** Get freeable flags for connection. */
#define con_freeflag(con)	((con)->con_freeflag)
/** Get last error code on connection. */
#define con_error(con)		((con)->con_error)
/** Get sentalong marker for connection. */
#define con_sentalong(con)      ((con)->con_sentalong)
/** Get server notice mask for connection. */
#define con_snomask(con)	((con)->con_snomask)
/** Get next nick change time for connection. */
#define con_nextnick(con)	((con)->con_nextnick)
/** Get next new target time for connection. */
#define con_nexttarget(con)	((con)->con_nexttarget)
/** Get PING/PONG confirmation cookie for connection. */
#define con_cookie(con)		((con)->con_cookie)
/** Get SendQ for connection. */
#define con_sendQ(con)		((con)->con_sendQ)
/** Get RecvQ for connection. */
#define con_recvQ(con)		((con)->con_recvQ)
/** Get number of messages sent to connection. */
#define con_sendM(con)		((con)->con_sendM)
/** Get number of k-bytes sent to connection. */
#define con_sendK(con)		((con)->con_sendK)
/** Get number of messages received from connection. */
#define con_receiveM(con)	((con)->con_receiveM)
/** Get number of k-bytes recieved from client. */
#define con_receiveK(con)	((con)->con_receiveK)
/** Get number of bytes (modulo 1024) sent to connection. */
#define con_sendB(con)		((con)->con_sendB)
/** Get number of bytes (modulo 1024) received from connection. */
#define con_receiveB(con)	((con)->con_receiveB)
/** Get listener that accepted the connection. */
#define con_listener(con)	((con)->con_listener)
/** Get list of ConfItems attached to the connection. */
#define con_confs(con)		((con)->con_confs)
/** Get command handler for the connection. */
#define con_handler(con)	((con)->con_handler)
/** Get DNS reply for the connection. */
#define con_dns_reply(con)	((con)->con_dns_reply)
/** Get DNSBL reply for the connection. */
#define con_dnsbl_reply(con)	((con)->con_dnsbl_reply)
/** Get the LIST status for the connection. */
#define con_listing(con)	((con)->con_listing)
/** Get the maximum permitted SendQ size for the connection. */
#define con_max_sendq(con)	((con)->con_max_sendq)
/** Get the ping frequency for the connection. */
#define con_ping_freq(con)	((con)->con_ping_freq)
/** Get the lastsq for the connection. */
#define con_lastsq(con)		((con)->con_lastsq)
/** Get the current targets array for the connection. */
#define con_targets(con)	((con)->con_targets)
/** Get the string-formatted IP address for the connection. */
#define con_sock_ip(con)	((con)->con_sock_ip)
/** Get the resolved hostname for the connection. */
#define con_sockhost(con)	((con)->con_sockhost)
/** Get the port the connection is connected on. */
#define con_port(con)		((con)->con_port)
/** Get the password sent by the remote end of the connection.  */
#define con_passwd(con)		((con)->con_passwd)
/** Get the buffer of unprocessed incoming data from the connection. */
#define con_buffer(con)		((con)->con_buffer)
/** Get the Socket for the connection. */
#define con_socket(con)		((con)->con_socket)
/** Get the Timer for processing more data from the connection. */
#define con_proc(con)		((con)->con_proc)
/** Get the auth request for the connection. */
#define con_auth(con)		((con)->con_auth)
/** Get the login on connect request for the connection. */
#define con_loc(con)		((con)->con_loc)

#define STAT_CONNECTING         0x001 /**< connecting to another server */
#define STAT_HANDSHAKE          0x002 /**< pass - server sent */
#define STAT_ME                 0x004 /**< this server */
#define STAT_UNKNOWN            0x008 /**< unidentified connection */
#define STAT_UNKNOWN_USER       0x010 /**< connection on a client port */
#define STAT_UNKNOWN_SERVER     0x020 /**< connection on a server port */
#define STAT_SERVER             0x040 /**< fully registered server */
#define STAT_USER               0x080 /**< fully registered user */

/*
 * status macros.
 */
/** Return non-zero if the client is registered. */
#define IsRegistered(x)         (cli_status(x) & (STAT_SERVER | STAT_USER))
/** Return non-zero if the client is an outbound connection that is
 * still connecting. */
#define IsConnecting(x)         (cli_status(x) == STAT_CONNECTING)
/** Return non-zero if the client is an outbound connection that has
 * sent our password. */
#define IsHandshake(x)          (cli_status(x) == STAT_HANDSHAKE)
/** Return non-zero if the client is this server. */
#define IsMe(x)                 (cli_status(x) == STAT_ME)
/** Return non-zero if the client has not yet registered. */
#define IsUnknown(x)            (cli_status(x) & \
        (STAT_UNKNOWN | STAT_UNKNOWN_USER | STAT_UNKNOWN_SERVER))
/** Return non-zero if the client is an unregistered connection on a
 * server port. */
#define IsServerPort(x)         (cli_status(x) == STAT_UNKNOWN_SERVER )
/** Return non-zero if the client is an unregistered connection on a
 * user port. */
#define IsUserPort(x)           (cli_status(x) == STAT_UNKNOWN_USER )
/** Return non-zero if the client is a real client connection. */
#define IsClient(x)             (cli_status(x) & \
        (STAT_HANDSHAKE | STAT_ME | STAT_UNKNOWN |\
         STAT_UNKNOWN_USER | STAT_UNKNOWN_SERVER | STAT_SERVER | STAT_USER))
/** Return non-zero if the client ignores flood limits. */
#define IsTrusted(x)            (cli_status(x) & \
        (STAT_CONNECTING | STAT_HANDSHAKE | STAT_ME | STAT_SERVER))
/** Return non-zero if the client is a registered server. */
#define IsServer(x)             (cli_status(x) == STAT_SERVER)
/** Return non-zero if the client is a registered user. */
#define IsUser(x)               (cli_status(x) == STAT_USER)


/** Mark a client with STAT_CONNECTING. */
#define SetConnecting(x)        (cli_status(x) = STAT_CONNECTING)
/** Mark a client with STAT_HANDSHAKE. */
#define SetHandshake(x)         (cli_status(x) = STAT_HANDSHAKE)
/** Mark a client with STAT_SERVER. */
#define SetServer(x)            (cli_status(x) = STAT_SERVER)
/** Mark a client with STAT_ME. */
#define SetMe(x)                (cli_status(x) = STAT_ME)
/** Mark a client with STAT_USER. */
#define SetUser(x)              (cli_status(x) = STAT_USER)

/** Return non-zero if a client is directly connected to me. */
#define MyConnect(x)		(cli_from(x) == (x))
/** Return non-zero if a client is a locally connected user. */
#define MyUser(x)		(MyConnect(x) && IsUser(x))
/** Return non-zero if a client is a locally connected IRC operator. */
#define MyOper(x)		(MyConnect(x) && IsOper(x))
/** Return protocol version used by a server. */
#define Protocol(x)		((cli_serv(x))->prot)

#define PARSE_AS_SERVER(x) (cli_status(x) & \
            (STAT_SERVER | STAT_CONNECTING | STAT_HANDSHAKE))

/*
 * flags macros
 */
/** Set a flag in a client's flags. */
#define SetFlag(cli, flag)  FlagSet(&cli_flags(cli), flag)
/** Clear a flag from a client's flags. */
#define ClrFlag(cli, flag)  FlagClr(&cli_flags(cli), flag)
/** Return non-zero if a flag is set in a client's flags. */
#define HasFlag(cli, flag)  FlagHas(&cli_flags(cli), flag)


/** Return non-zero if the client has access */
#define DoAccess(x)             HasFlag(x, FLAG_CHKACCESS)
/** Return non-zero if the client is an IRC operator (global or local). */
#define IsAnOper(x)             (HasFlag(x, FLAG_OPER) || HasFlag(x, FLAG_LOCOP))
/** Return non-zero if the client's connection is blocked. */
#define IsBlocked(x)            HasFlag(x, FLAG_BLOCKED)
/** Return non-zero if the client's connection is still being burst. */
#define IsBurst(x)              HasFlag(x, FLAG_BURST)
/** Return non-zero if we have received the peer's entire burst but
 * not their EOB ack. */
#define IsBurstAck(x)           HasFlag(x, FLAG_BURST_ACK)
/** Return non-zero if we are still bursting to the client. */
#define IsBurstOrBurstAck(x)    (HasFlag(x, FLAG_BURST) || HasFlag(x, FLAG_BURST_ACK))
/** Return non-zero if the client has set mode +k (channel service). */
#define IsChannelService(x)     HasFlag(x, FLAG_CHSERV)
/** Return non-zero if the client's socket is disconnected. */
#define IsDead(x)               HasFlag(x, FLAG_DEADSOCKET)
/** Return non-zero if the client has set mode +d (deaf). */
#define IsDeaf(x)               HasFlag(x, FLAG_DEAF)
/** Return non-zero if the client has been IP-checked for clones. */
#define IsIPChecked(x)          HasFlag(x, FLAG_IPCHECK)
/** Return non-zero if we have received an ident response for the client. */
#define IsIdented(x)            HasFlag(x, FLAG_GOTID)
/** Return non-zero if the client has set mode +i (invisible). */
#define IsInvisible(x)          HasFlag(x, FLAG_INVISIBLE)
/** Return non-zero if the client caused a net.burst. */
#define IsJunction(x)           HasFlag(x, FLAG_JUNCTION)
/** Return non-zero if the client has set mode +O (local operator). */
#define IsLocOp(x)              HasFlag(x, FLAG_LOCOP)
/** Return non-zero if the client is local. */
#define IsLocal(x)              HasFlag(x, FLAG_LOCAL)
/** Return non-zero if the client has set mode +o (global operator). */
#define IsOper(x)               HasFlag(x, FLAG_OPER)
/** Return non-zero if the client has an active UDP ping request. */
#define IsUPing(x)              HasFlag(x, FLAG_UPING)
/** Return non-zero if the client has no '\n' in its buffer. */
#define NoNewLine(x)            HasFlag(x, FLAG_NONL)
/** Return non-zero if the client has set mode +g (debugging). */
#define SendDebug(x)            HasFlag(x, FLAG_DEBUG)
/** Return non-zero if the client has set mode +s (server notices). */
#define SendServNotice(x)       HasFlag(x, FLAG_SERVNOTICE)
/** Return non-zero if the client has set mode +w (wallops). */
#define SendWallops(x)          HasFlag(x, FLAG_WALLOP)
/** Return non-zero if the client claims to be a hub. */
#define IsHub(x)                HasFlag(x, FLAG_HUB)
/** Return non-zero if the client claims to be a services server. */
#define IsService(x)            HasFlag(x, FLAG_SERVICE)
/** Return non-zero if the client has an account stamp. */
#define IsAccount(x)            HasFlag(x, FLAG_ACCOUNT)
/** Return non-zero if the client has set mode +x (hidden host). */
#define IsHiddenHost(x)		HasFlag(x, FLAG_HIDDENHOST)
/** Return non-zero if the client has their host hidden (account/fakehost set). */
#define HasHiddenHost(x)	(IsHiddenHost(x) && (IsAccount(x) || HasFakeHost(x)))
/** Return non-zero if the client hasa cloaked host set (style 2) */
#define HasCloakHost(x)         HasFlag(x, FLAG_CLOAKHOST)
/** Return non-zero if the client hasa cloaked ip set (style 2) */
#define HasCloakIP(x)           HasFlag(x, FLAG_CLOAKIP)
/** Return non-zero if the client has a sethost active. */
#define IsSetHost(x)		HasFlag(x, FLAG_SETHOST)
/** Return non-zero if the client has a sethost active. */
#define HasSetHost(x)		(IsSetHost(x))
/** Return non-zero if the client has a fakehost. */
#define HasFakeHost(x)		HasFlag(x, FLAG_FAKEHOST)
/** Return non-zero if the client only accepts messages from clients with an account. */
#define IsAccountOnly(x)	HasFlag(x, FLAG_ACCOUNTONLY)
/** Return non-zero if the client is a remote oper. */
#define IsRemoteOper(x)		HasFlag(x, FLAG_REMOTEOPER)
/** Return non-zero if the client has set +B. */
#define IsBot(x)		HasFlag(x, FLAG_BOT)
/** Return non-zero if the client is connected via ssl. */
#define IsSSL(x)		HasFlag(x, FLAG_SSL)
/** Return non-zero if the client has +X set. */
#define IsXtraOp(x)		HasFlag(x, FLAG_XTRAOP)
/** Return non-zero if the client has the channel hiding mode set. */
#define IsNoChan(x)		HasFlag(x, FLAG_NOCHAN)
/** Return non-zero if the client has the hidden idle time mode set. */
#define IsNoIdle(x)		HasFlag(x, FLAG_NOIDLE)
/** Return non-zero if the client has been found on a dnsbl. */
#define IsDNSBL(x)		HasFlag(x, FLAG_DNSBL)
/** Return non-zero if the client has been dnsbl marked. */
#define IsDNSBLMarked(x)	HasFlag(x, FLAG_DNSBLMARKED)
/** Return non-zero if the client has been dnsbl allowed .*/
#define IsDNSBLAllowed(x)	HasFlag(x, FLAG_DNSBLALLOWED)
/** Return non-zero if the client has been dnsbl denied.*/
#define IsDNSBLDenied(x)	HasFlag(x, FLAG_DNSBLDENIED)
/** Return non-zero if the client has +A oper mode set. */
#define IsAdmin(x)		(HasFlag(x, FLAG_ADMIN) && feature_bool(FEAT_OPERFLAGS))
/** Return non-zero if the client has +W oper mode set. */
#define IsWhois(x)		HasFlag(x, FLAG_WHOIS)
/** Return non-zero if the client has operator or server privileges. */
#define IsPrivileged(x)         (IsAnOper(x) || IsServer(x))
/** Return non-zero if the client has triggered a spam filter */
#define IsSpam(x)               HasFlag(x, FLAG_SPAM)
/** Return non-zero if the client is a WEBIRC user. */
#define IsWebIRC(x)             HasFlag(x, FLAG_WEBIRC)
/** Return non-zero if the client is WEBIRC line uses USER method */
#define IsWebIRCUserIdent(x)    HasFlag(x, FLAG_WEBIRC_UIDENT)
/** Return non-zero if the client supports extended NAMES */
#define IsNamesX(x)             HasFlag(x, FLAG_NAMESX)
/** Return non-zero if the client supports extended NAMES */
#define IsUHNames(x)             HasFlag(x, FLAG_UHNAMES)
/** Return non-zero if the client has been propagted to servers */
#define IsPropagated(x)         HasFlag(x, FLAG_PROPAGATED)
/** Return non-zero if the client is private deaf */
#define IsPrivDeaf(x)           HasFlag(x, FLAG_PRIVDEAF)

/** Mark a client as having access. */
#define SetAccess(x)            SetFlag(x, FLAG_CHKACCESS)
/** Mark a client as having an in-progress net.burst. */
#define SetBurst(x)             SetFlag(x, FLAG_BURST)
/** Mark a client as being between EOB and EOB ACK. */
#define SetBurstAck(x)          SetFlag(x, FLAG_BURST_ACK)
/** Mark a client as having mode +k (channel service). */
#define SetChannelService(x)    SetFlag(x, FLAG_CHSERV)
/** Mark a client as having mode +d (deaf). */
#define SetDeaf(x)              SetFlag(x, FLAG_DEAF)
/** Mark a client as having mode +g (debugging). */
#define SetDebug(x)             SetFlag(x, FLAG_DEBUG)
/** Mark a client as having ident looked up. */
#define SetGotId(x)             SetFlag(x, FLAG_GOTID)
/** Mark a client as being IP-checked. */
#define SetIPChecked(x)         SetFlag(x, FLAG_IPCHECK)
/** Mark a client as having mode +i (invisible). */
#define SetInvisible(x)         SetFlag(x, FLAG_INVISIBLE)
/** Mark a client as causing a net.join. */
#define SetJunction(x)          SetFlag(x, FLAG_JUNCTION)
/** Mark a client as having mode +O (local operator). */
#define SetLocOp(x)             SetFlag(x, FLAG_LOCOP)
/** Mark a client as having mode +o (global operator). */
#define SetOper(x)              SetFlag(x, FLAG_OPER)
/** Mark a client as having a pending UDP ping. */
#define SetUPing(x)             SetFlag(x, FLAG_UPING)
/** Mark a client as having mode +w (wallops). */
#define SetWallops(x)           SetFlag(x, FLAG_WALLOP)
/** Mark a client as having mode +s (server notices). */
#define SetServNotice(x)        SetFlag(x, FLAG_SERVNOTICE)
/** Mark a client as being a hub server. */
#define SetHub(x)               SetFlag(x, FLAG_HUB)
/** Mark a client as being a services server. */
#define SetService(x)           SetFlag(x, FLAG_SERVICE)
/** Mark a client as having an account stamp. */
#define SetAccount(x)           SetFlag(x, FLAG_ACCOUNT)
/** Mark a client as having mode +x (hidden host). */
#define SetHiddenHost(x)	SetFlag(x, FLAG_HIDDENHOST)
/** Mark a client as having mode +C (cloaked host). */
#define SetCloakHost(x)         SetFlag(x, FLAG_CLOAKHOST)
/** Mark a client as having mode +c (cloaked ip). */
#define SetCloakIP(x)           SetFlag(x, FLAG_CLOAKIP)
/** Mark a client as having a sethost (S:Lines). */
#define SetSetHost(x)		SetFlag(x, FLAG_SETHOST)
/** Mark a client as having a fakehost. */
#define SetFakeHost(x)		SetFlag(x, FLAG_FAKEHOST)
/** Mark a client as only accepting messages from users with accounts. */
#define SetAccountOnly(x)	SetFlag(x, FLAG_ACCOUNTONLY)
/** Mark a client as being a remote oper. */
#define SetRemoteOper(x)	SetFlag(x, FLAG_REMOTEOPER)
/** Mark a client as having mode +B (bot). */
#define SetBot(x)		SetFlag(x, FLAG_BOT)
/** Mark a client as being connected via SSL. */
#define SetSSL(x)		SetFlag(x, FLAG_SSL)
/** Mark a client as having mode +X (Xtraop- misc extra pivs). */
#define SetXtraOp(x)		SetFlag(x, FLAG_XTRAOP)
/** Mark a client as having the channel hiding mode set. */
#define SetNoChan(x)		SetFlag(x, FLAG_NOCHAN)
/** Mark a client as having the hidden idle time mode set. */
#define SetNoIdle(x)		SetFlag(x, FLAG_NOIDLE)
/** Mark a client as being matched on an X:Line. */
#define SetDNSBL(x)		SetFlag(x, FLAG_DNSBL)
/** Mark a client as being marked via an X:line. */
#define SetDNSBLMarked(x)	SetFlag(x, FLAG_DNSBLMARKED)
/** Mark a client as being allowed via an X:line. */
#define SetDNSBLAllowed(x)	SetFlag(x, FLAG_DNSBLALLOWED)
/** Mark a client as being denied via an X:line. */
#define SetDNSBLDenied(x)	SetFlag(x, FLAG_DNSBLDENIED)
/** Mark a client as being an admin (+A). */
#define SetAdmin(x)		SetFlag(x, FLAG_ADMIN)
/** Mark a client as being oper mode +W set (/whois alerts). */
#define SetWhois(x)		SetFlag(x, FLAG_WHOIS)
/** Mark a client as being a spam source */
#define SetSpam(x)              SetFlag(x, FLAG_SPAM)
/** Mark a client as being a WEBIRC user. */
#define SetWebIRC(x)            SetFlag(x, FLAG_WEBIRC)
/** Mark a WEBIRC client as using USER to send ident .*/
#define SetWebIRCUserIdent(x)   SetFlag(x, FLAG_WEBIRC_UIDENT)
/** Mark a client as supporting extended NAMES. */
#define SetNamesX(x)            SetFlag(x, FLAG_NAMESX)
/** Mark a client as supporting extended NAMES. */
#define SetUHNames(x)            SetFlag(x, FLAG_UHNAMES)
/** Mark a client as having been propagated. */
#define SetPropagated(x)        SetFlag(x, FLAG_PROPAGATED)
/** Mark a client as being private deaf. */
#define SetPrivDeaf(x)          SetFlag(x, FLAG_PRIVDEAF)

/** Clear the client's access flag. */
#define ClearAccess(x)          ClrFlag(x, FLAG_CHKACCESS)
/** Clear the client's net.burst in-progress flag. */
#define ClearBurst(x)           ClrFlag(x, FLAG_BURST)
/** Clear the client's between EOB and EOB ACK flag. */
#define ClearBurstAck(x)        ClrFlag(x, FLAG_BURST_ACK)
/** Remove mode +k (channel service) from the client. */
#define ClearChannelService(x)  ClrFlag(x, FLAG_CHSERV)
/** Remove mode +d (deaf) from the client. */
#define ClearDeaf(x)            ClrFlag(x, FLAG_DEAF)
/** Remove mode +g (debugging) from the client. */
#define ClearDebug(x)           ClrFlag(x, FLAG_DEBUG)
/** Remove the client's IP-checked flag. */
#define ClearIPChecked(x)       ClrFlag(x, FLAG_IPCHECK)
/** Remove mode +i (invisible) from the client. */
#define ClearInvisible(x)       ClrFlag(x, FLAG_INVISIBLE)
/** Remove mode +O (local operator) from the client. */
#define ClearLocOp(x)           ClrFlag(x, FLAG_LOCOP)
/** Remove mode +o (global operator) from the client. */
#define ClearOper(x)            ClrFlag(x, FLAG_OPER)
/** Clear the client's account flag. */
#define ClearAccount(x)         ClrFlag(x, FLAG_ACCOUNT)
/** Clear the client's pending UDP ping flag. */
#define ClearUPing(x)           ClrFlag(x, FLAG_UPING)
/** Remove mode +w (wallops) from the client. */
#define ClearWallops(x)         ClrFlag(x, FLAG_WALLOP)
/** Remove mode +s (server notices) from the client. */
#define ClearServNotice(x)      ClrFlag(x, FLAG_SERVNOTICE)
/** Remove mode +x (hidden host) from the client. */
#define ClearHiddenHost(x)	ClrFlag(x, FLAG_HIDDENHOST)
/** Clear the client's sethost flag. */
#define ClearSetHost(x)		ClrFlag(x, FLAG_SETHOST)
/** Clear the client's fakehost flag. */
#define ClearFakeHost(x)	ClrFlag(x, FLAG_FAKEHOST)
/** Remove mode +R (only accept pms from users with an account) from the client. */
#define ClearAccountOnly(x)	ClrFlag(x, FLAG_ACCOUNTONLY)
/** Clear the client's remote oper flag. */
#define ClearRemoteOper(x)	ClrFlag(x, FLAG_REMOTEOPER)
/** Remove mode +B (bot) flag from the client */
#define ClearBot(x)		ClrFlag(x, FLAG_BOT)
/** Clear the client's SSL flag. */
#define ClearSSL(x)		ClrFlag(x, FLAG_SSL)
/** Remove mode +X (xtra op) from the client. */
#define ClearXtraOp(x)		ClrFlag(x, FLAG_XTRAOP)
/** Remove mode +M (hide channels in whois) from the client. */
#define ClearNoChan(x)		ClrFlag(x, FLAG_NOCHAN)
/** Remove mode +I (hide idle time in whois) from the client. */
#define ClearNoIdle(x)		ClrFlag(x, FLAG_NOIDLE)
/** Remove mode +A (admin) from the client. */
#define ClearAdmin(x)		ClrFlag(x, FLAG_ADMIN)
/** Remove mode +W (whois alers)  from the client. */
#define ClearWhois(x)		ClrFlag(x, FLAG_WHOIS)
/** Client is no longer dnsbl marked. Mark flag removed from client. */
#define ClearDNSBLMarked(x)	ClrFlag(x, FLAG_DNSBLMARKED)
/** Client matching on dnsbl is no longer allowed in. Allow flag removed from client */
#define ClearDNSBLAllowed(x)	ClrFlag(x, FLAG_DNSBLALLOWED)
/** Client is no longer a spam source */
#define ClearSpam(x)            ClrFlag(x, FLAG_SPAM)
/** Client is no longer a WEBIRC user. */
#define ClearWebIRC(x)          ClrFlag(x, FLAG_WEBIRC)
/** Client no longer supports extended names. */
#define ClearNamesX(x)          ClrFlag(x, FLAG_NAMESX)
/** Client no longer supports extended names. */
#define ClearUHNames(x)          ClrFlag(x, FLAG_UHNAMES)
/** Client has no longer been propagated. */
#define ClearPropagated(x)      ClrFlag(x, FLAG_PROPAGATED)
/** Client is no longer private deaf. */
#define ClearPrivDeaf(x)        ClrFlag(x, FLAG_PRIVDEAF)

/** Client can see oper. */
#define SeeOper(sptr, acptr) (IsAnOper(acptr) \
			      && (HasPriv(acptr, PRIV_DISPLAY) \
			      || HasPriv(sptr, PRIV_SEE_OPERS)))

/** Oper flag global (+O). */
#define OFLAG_GLOBAL	0x001
/** Oper flag admin (+A). */
#define OFLAG_ADMIN	0x002
/** Oper flag password is rsa key (+R). */
#define OFLAG_RSA	0x004
/** Oper can OPER remotely (+r). */
#define OFLAG_REMOTE    0x008
/** Oper can set user mode +W. */
#define OFLAG_WHOIS     0x010
/** Oper can set user mode +I. */
#define OFLAG_IDLE      0x020
/** Oper can set user mode +X. */
#define OFLAG_XTRAOP    0x040
/** Oper can set user mode +n. */
#define OFLAG_HIDECHANS 0x080

/** Return non-zero if the client is an global oper. */
#define OIsGlobal(x)		(cli_oflags(x) & OFLAG_GLOBAL)
/** Return non-zero if the client is an admin. */
#define OIsAdmin(x)		(cli_oflags(x) & OFLAG_ADMIN)
/** Return non-zero if the client is using rsa */
#define OIsRSA(x)              (cli_oflags(x) & OFLAG_RSA)
/** Return non-zero if the client is oper'ed remotely */
#define OIsRemote(x)            (cli_oflags(x) & OFLAG_REMOTE)
/** Return non-zero if the client can set user mode +W. */
#define OIsWhois(x)             (cli_oflags(x) & OFLAG_WHOIS)
/** Return non-zero if the client can set user mode +I. */
#define OIsIdle(x)              (cli_oflags(x) & OFLAG_IDLE)
/** Return non-zero if the client can set user mode +X. */
#define OIsXtraop(x)            (cli_oflags(x) & OFLAG_XTRAOP)
/** Return non-zero if the client can set user mode +n. */
#define OIsHideChans(x)         (cli_oflags(x) & OFLAG_HIDECHANS)

/** Mark a client as being an global oper. */
#define OSetGlobal(x)		(cli_oflags(x) |= OFLAG_GLOBAL)
/** Mark a client as being an admin. */
#define OSetAdmin(x)		(cli_oflags(x) |= OFLAG_ADMIN)
/** Mark a client as being rsa */
#define OSetRSA(x)             (cli_oflags(x) |= OFLAG_RSA)
/** Mark a client as being remote */
#define OSetRemote(x)           (cli_oflags(x) |= OFLAG_REMOTE)
/** Mark a client as able to set user mode +W */
#define OSetWhois(x)            (cli_oflags(x) |= OFLAG_WHOIS)
/** Mark a client as able to set user mode +I */
#define OSetIdle(x)             (cli_oflags(x) |= OFLAG_IDLE)
/** Mark a client as able to set user mode +X */
#define OSetXtraop(x)           (cli_oflags(x) |= OFLAG_XTRAOP)
/** Mark a client as able to set user mode +n */
#define OSetHideChans(x)        (cli_oflags(x) |= OFLAG_HIDECHANS)

/** Clear the client's global oper status. */
#define OClearGlobal(x)		(cli_oflags(x) &= ~OFLAG_GLOBAL)
/** Clear the client's admin status. */
#define OClearAdmin(x)		(cli_oflags(x) &= ~OFLAG_ADMIN)
/** Clear the client's rsa status */
#define OClearRSA(x) )          (cli_oflags(x) &= ~OFLAG_RSA)
/** Clear the client's remote status */
#define OClearRemote(x)         (cli_oflags(x) &= ~OFLAG_REMOTE)
/** Clear the client's ability to set user mode +W */
#define OClearWhois(x)          (cli_oflags(x) &= ~OFLAG_WHOIS)
/** Clear the client's ability to set user mode +I */
#define OClearIdle(x)           (cli_oflags(x) &= ~OFLAG_IDLE)
/** Clear the client's ability to set user mode +X */
#define OClearXtraop(x)         (cli_oflags(x) &= ~OFLAG_XTRAOP)
/** Clear the client's ability to set user mode +n */
#define OClearHideChans(x)      (cli_oflags(x) &= ~OFLAG_HIDECHANS)

/*
 * X:Line flags
 */

/** Bitmask X:Line. */
#define DFLAG_BITMASK	0x001
/** Reply X:Line. */
#define DFLAG_REPLY	0x002
/** Client is allowed to connect regardless of results. */
#define DFLAG_ALLOW	0x004
/** Client hostname is marked. */
#define DFLAG_MARK	0x008
/** Dont allow even if allowed */
#define DFLAG_DENY	0x010

/** Client webirc desc is sent */
#define WFLAG_MARK	0x001
#define WFLAG_SIDENT    0x002
#define WFLAG_UIDENT    0x004

/** Exception for K:Lines */
#define EFLAG_KLINE     0x001
/** Exception for /GLINES */
#define EFLAG_GLINE     0x002
/** Exception for /ZLINES */
#define EFLAG_ZLINE     0x004
/** Exception for /SHUNS */
#define EFLAG_SHUN      0x008
/** Exception for Spam Filters */
#define EFLAG_SFILTER   0x010

#define RFFLAG_AUTH     0x001
#define RFFLAG_KILL     0x002
#define RFFLAG_GLINE    0x004
#define RFFLAG_SHUN     0x008
#define RFFLAG_BLOCK    0x010
#define RFFLAG_CALERT   0x020
#define RFFLAG_SALERT   0x040
#define RFFLAG_NOTIFY   0x080
#define RFFLAG_ZLINE    0x100
#define RFFLAG_MARK     0x200
#define RFFLAG_IP       0x400
#define RFFLAG_KICK     0x800
#define RFFLAG_OPS      0x1000
#define RFFLAG_VOICE    0x2000

#define WFFLAG_NOTICE     0x001
#define WFFLAG_CHANNOTICE 0x002
#define WFFLAG_PRIVMSG    0x004
#define WFFLAG_CHANMSG    0x008
#define WFFLAG_AWAY       0x010
#define WFFLAG_TOPIC      0x020
#define WFFLAG_CONNECT    0x040
#define WFFLAG_PART       0x080
#define WFFLAG_QUIT       0x100
#define WFFLAG_DCC        0x200
#define WFFLAG_NICK       0x400

/* free flags */
#define FREEFLAG_SOCKET	0x0001	/**< socket needs to be freed */
#define FREEFLAG_TIMER	0x0002	/**< timer needs to be freed */

/* server notice stuff */

#define SNO_ADD         1       /**< Perform "or" on server notice mask. */
#define SNO_DEL         2       /**< Perform "and ~x" on server notice mask. */
#define SNO_SET         3       /**< Set server notice mask. */
                                /* DON'T CHANGE THESE VALUES ! */
                                /* THE CLIENTS DEPEND ON IT  ! */
#define SNO_OLDSNO      0x1     /**< unsorted old messages */
#define SNO_SERVKILL    0x2     /**< server kills (nick collisions) */
#define SNO_OPERKILL    0x4     /**< oper kills */
#define SNO_HACK2       0x8     /**< desyncs */
#define SNO_HACK3       0x10    /**< temporary desyncs */
#define SNO_UNAUTH      0x20    /**< unauthorized connections */
#define SNO_TCPCOMMON   0x40    /**< common TCP or socket errors */
#define SNO_TOOMANY     0x80    /**< too many connections */
#define SNO_HACK4       0x100   /**< Uworld actions on channels */
#define SNO_GLINE       0x200   /**< glines/zlines/shuns */
#define SNO_NETWORK     0x400   /**< net join/break, etc */
#define SNO_IPMISMATCH  0x800   /**< IP mismatches */
#define SNO_THROTTLE    0x1000  /**< host throttle add/remove notices */
#define SNO_OLDREALOP   0x2000  /**< old oper-only messages */
#define SNO_CONNEXIT    0x4000  /**< client connect/exit (ugh) */
#define SNO_AUTO        0x8000  /**< AUTO G-Lines */
#define SNO_DEBUG       0x10000 /**< debugging messages (DEBUGMODE only) */
#define SNO_NICKCHG     0x20000 /**< nick change notices */

#ifdef DEBUGMODE
# define SNO_ALL        0x3ffff  /**< Bitmask of all valid server
                                  * notice bits. */
#else
# define SNO_ALL        0x2ffff
#endif

/** Server notice bits allowed to normal users. */
#define SNO_USER        (SNO_ALL & ~SNO_OPER)

/** Server notice bits enabled by default for normal users. */
#define SNO_DEFAULT (SNO_NETWORK|SNO_OPERKILL|SNO_GLINE)
/** Server notice bits enabled by default for IRC operators. */
#define SNO_OPERDEFAULT (SNO_DEFAULT|SNO_HACK2|SNO_HACK4|SNO_THROTTLE|SNO_OLDSNO)
/** Server notice bits reserved to IRC operators. */
#define SNO_OPER (SNO_CONNEXIT|SNO_OLDREALOP)
/** Noisy server notice bits that cause other bits to be cleared during connect. */
#define SNO_NOISY (SNO_SERVKILL|SNO_UNAUTH)

/** Test whether a privilege has been granted to a client. */
#define HasPriv(cli, priv)  FlagHas(cli_privs(cli), priv)
/** Grant a privilege to a client. */
#define SetPriv(cli, priv)  FlagSet(cli_privs(cli), priv)
/** Revoke a privilege from a client. */
#define ClrPriv(cli, priv)  FlagClr(cli_privs(cli), priv)

/** Used in setting and unsetting privs. */
#define PRIV_ADD 1
/** Used in setting and unsetting privs. */
#define PRIV_DEL 0

/** IP address viewing options in get_client_name(). */
typedef enum ShowIPType {
  HIDE_IP,
  SHOW_IP,
  MASK_IP
} ShowIPType;

extern const char* get_client_name(const struct Client* sptr, int showip);
extern const char* client_get_default_umode(const struct Client* sptr);
extern int client_get_ping(const struct Client* local_client);
extern void client_drop_sendq(struct Connection* con);
extern void client_add_sendq(struct Connection* con,
			     struct Connection** con_p);
extern void client_set_privs(struct Client *client, struct ConfItem *oper);
extern int client_report_privs(struct Client* to, struct Client* client);
extern char *client_print_privs(struct Client* client);
extern int client_modify_priv_by_name(struct Client *who, char *priv, int what);

extern void DoMD5(unsigned char *mdout, unsigned char *src, unsigned long n);

#endif /* INCLUDED_client_h */
