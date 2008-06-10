/*
 * IRC - Internet Relay Chat, include/supported.h
 * Copyright (C) 1999 Perry Lorier.
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
 *
 */
#ifndef INCLUDED_supported_h
#define INCLUDED_supported_h

#include "channel.h"
#include "ircd_defs.h"

#define infochanmodes "abcehiklmnoprstvzCLMNOQSTZ"
#define clearchanmodes "abcehiklmoprsvzCLMNOQSTZ" /* ditto except for +nt */
#define infochanmodeswithparams "behklov"
#define infousermodes "acdfghiknoswxzBCIRWX"

#endif /* INCLUDED_supported_h */
