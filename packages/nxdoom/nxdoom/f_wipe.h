// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: f_wipe.h,v 1.2 2003/09/08 22:34:27 jasonk Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//	Mission start screen wipe/melt, special effects.
//	
//-----------------------------------------------------------------------------


#ifndef __F_WIPE_H__
#define __F_WIPE_H__

//
//                       SCREEN WIPE PACKAGE
//

enum
{
    // simple gradual pixel change for 8-bit only
    wipe_ColorXForm,
    
    // weird screen melt
    wipe_Melt,	

    wipe_NUMWIPES
};

int
wipe_StartScreen
( int		x,
  int		y,
  int		width,
  int		height );


int
wipe_EndScreen
( int		x,
  int		y,
  int		width,
  int		height );


int
wipe_ScreenWipe
( int		wipeno,
  int		x,
  int		y,
  int		width,
  int		height,
  int		ticks );

#endif
//-----------------------------------------------------------------------------
//
// $Log: f_wipe.h,v $
// Revision 1.2  2003/09/08 22:34:27  jasonk
// Updated files because this fucker won't build for no fucking good reason.
//
// Revision 1.1.1.1  2003/09/04 21:08:12  jasonk
// Initial import
//
// Revision 1.1  2000/12/08 21:07:53  jeffw
// nxdoom initial entry -- No nxdoom/Makefile so it won't build automatically
//
//
//-----------------------------------------------------------------------------
