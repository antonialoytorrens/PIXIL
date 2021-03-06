/*                                                                       
 * Copyright (c) 2003 Century Software, Inc.   All Rights Reserved.     
 *                                                                       
 * This file is part of the PIXIL Operating Environment                 
 *                                                                       
 * The use, copying and distribution of this file is governed by one    
 * of two licenses, the PIXIL Commercial License, or the GNU General    
 * Public License, version 2.                                           
 *                                                                       
 * Licensees holding a valid PIXIL Commercial License may use this file 
 * in accordance with the PIXIL Commercial License Agreement provided   
 * with the Software. Others are governed under the terms of the GNU   
 * General Public License version 2.                                    
 *                                                                       
 * This file may be distributed and/or modified under the terms of the  
 * GNU General Public License version 2 as published by the Free        
 * Software Foundation and appearing in the file LICENSE.GPL included   
 * in the packaging of this file.                                      
 *                                                                       
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING  
 * THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A            
 * PARTICULAR PURPOSE.                                                  
 *                                                                       
 * RESTRICTED RIGHTS LEGEND                                             
 *                                                                     
 * Use, duplication, or disclosure by the government is subject to      
 * restriction as set forth in paragraph (b)(3)(b) of the Rights in     
 * Technical Data and Computer Software clause in DAR 7-104.9(a).       
 *                                                                      
 * See http://www.pixil.org/gpl/ for GPL licensing       
 * information.                                                         
 *                                                                      
 * See http://www.pixil.org/license.html or              
 * email cetsales@centurysoftware.com for information about the PIXIL   
 * Commercial License Agreement, or if any conditions of this licensing 
 * are not clear to you.                                                
 */



/* System header files */
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


/* Local header files */
#include <pixlib/pixlib.h>

/* Typedef, macros, enum/struct/union definitions */


/* Global scope variables */


/* File scope variables */
unsigned char f_maxval = -1;	/* Maximum backlight value */


/* Static function prototypes */


/*******************************************************************************\
**
**	Static function definitions
**
\*******************************************************************************/


/*******************************************************************************\
**
**	Externally callable function definitions 
**
\*******************************************************************************/

/*******************************************************************************\
**
**	Function:	int pix_bl_ctrl()
**	Desc:		Controls the backlight settings on the iPAQ
**	Accepts:	int pwr = Power settings (0 = off, 1 = on)
**				int level = Brightness level (0 - f_maxval) will be clamped
**	Returns:	int; 0 on success, -1 on error (with errno being set)
**
\*******************************************************************************/
int
pix_bl_ctrl(int pwr, int level)
{
    int retval = 0;		/* Default to success */

    return (retval);
}				/* end of pix_bl_ctrl() */

/*******************************************************************************\
**
**	Function:	int pix_bl_getmxval()
**	Desc:		Returns the maximum backlite value supported by hardware for the
**				given platform (default is 255)
**	Accepts:	Nothing (void)
**	Returns:	int; >= 0 on success, -1 on error
**
\*******************************************************************************/
int
pix_bl_getmxval(void)
{
    return (f_maxval);
}				/* end of bl_getmxval() */
