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

//--------------------------------------------------------------//
// Group for the center of the main window.  This group will    //
// contain four groups only one of which will be visible at any //
// given time.                                                  //
//--------------------------------------------------------------//
#ifndef INFOGROUP_H_

#define INFOGROUP_H_

#ifdef WIN32
#pragma warning(disable:4786)
#define inline __inline
#endif /*  */

#include <FL/Fl_Group.H>
#include "AddressBook.h"
#include "Messages.h"
#include "Notes.h"
#include "Scheduler.h"
#include "ToDoList.h"
class InfoGroup:public Fl_Group
{
  public:InfoGroup(int nX,	// Constructor
	      int nY, int nWidth, int nHeight);
     ~InfoGroup();		// Destructor
    int Message(PixilDTMessage nMessage,	// Process a message from the parent widget
		int nInfo);
    void ShowInfo(PixilDTMessage nMessage);	// Show a requested set of information
    void Refresh();
  private:  AddressBook * m_pAddressBook;
    // Address Book info
    Notes *m_pNotes;		// Notes info
    PixilDTMessage m_nCurrentPage;	// Last selected page
    Scheduler *m_pScheduler;	// Scheduler info
    ToDoList *m_pToDoList;	// ToDo List info
};


#endif /*  */
