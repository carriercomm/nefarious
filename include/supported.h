/*
 * IRC - Internet Relay Chat, include/supported.h
 * Copyright (C) 1999 Perry Lorier.
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
 *
 * Description: This file has the featureset that ircu announces on connecting
 *              a client.  It's in this .h because it's likely to be appended
 *              to frequently and s_user.h is included by basically everyone.
 */
#ifndef INCLUDED_supported_h
#define INCLUDED_supported_h

#include "channel.h"
#include "ircd_defs.h"

/* 
 * 'Features' supported by this ircd
 */
#define FEATURES1 \
                "WHOX"\
                " WALLCHOPS"\
		" WALLVOICES"\
                " USERIP"\
                " CPRIVMSG"\
                " CNOTICE"\
                " SILENCE=%i" \
                " MODES=%i" \
                " MAXCHANNELS=%i" \
                " MAXBANS=%i" \
                " MAXEXCEPTS=%i " \
                " NICKLEN=%i" \
                " MAXNICKLEN=%i"

#define FEATURES2 "TOPICLEN=%i" \
                " AWAYLEN=%i" \
                " KICKLEN=%i" \
		" CHANTYPES=%s" \
                " PREFIX=%s" \
                " CHANMODES=%s" \
                " CASEMAPPING=%s" \
                " NETWORK=%s"

#define FEATURESVALUES1 feature_int(FEAT_MAXSILES), MAXMODEPARAMS, \
			feature_int(FEAT_MAXCHANNELSPERUSER), \
			feature_int(FEAT_MAXBANS), \
			feature_int(FEAT_MAXEXCEPTS), \
			feature_int(FEAT_NICKLEN), NICKLEN

#define FEATURESVALUES2 TOPICLEN, AWAYLEN, TOPICLEN, \
			feature_bool(FEAT_LOCAL_CHANNELS) ? "#&" : "#", \
			"(ohv)@%+", "b,e,k,l,cimnprstzCLMNOQST", \
			"rfc1459", feature_str(FEAT_NETWORK)

#define infochanmodes "bcehiklmnoprstvzCLMNOQST"
#define clearchanmodes "bcehiklmoprsvzCLMNOQST" /* ditto except for +nt */
#define infochanmodeswithparams "behklov"
#define infousermodes "dfghiknoswxBIRX"

#endif /* INCLUDED_supported_h */
