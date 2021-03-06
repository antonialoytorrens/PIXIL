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


#include "nxapp.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <ctype.h>

#ifdef NANOX
#include <nano-X.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include <stdlib.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Editor.H>

#include <pixil_config.h>

#include "nxbutton.h"
#include "nxbox.h"
#include "nxoutput.h"
#include "nxwindow.h"

#ifdef CONFIG_SYNC
#include <sync/coder.h>
#include <sync/msg_defs.h>
#include <sync/info_defs.h>
#include <sync/syncerr.h>
#endif

#ifdef CONFIG_COLOSSEUM
extern "C"
{
#include <ipc/colosseum.h>
}
#endif

#ifdef DEBUG
#define DPRINT(str, args...) printf("DEBUG: " str, ## args)
#else
#define DPRINT(args...)
#endif

#define OPTIONS "k:d:m:p:"

NxApp *
    NxApp::_instance =
    0;

Fl_Widget *
    NxApp::g_PasteTarget;
Fl_Widget *
    NxApp::undoTarget;
int
    NxApp::fl_editor_type;
bool NxApp::paste_ok_;

Fl_Color
    NxApp::global_colors[MAX_CLR_ATTRIB];

//extern int noguisearch_flag;

NxApp *
NxApp::Instance()
{
    if (_instance == 0) {
	printf("_instance() is really messed up. Bail!");
	exit(-1);
    }

    return _instance;
}

NxApp::NxApp(char *app)
{

    _instance = this;

    noguisearch_flag = 0;
    saveX = saveY = saveW = saveH = 0;

    // Set fd to err
    fd = -1;

    nextSyncState = -1;
    appSyncState = -1;
    cur_db = -1;
    cur_row = -1;
    cur_table_size = -1;
    cur_table = -1;
    total_rows = -1;
    c_db_struct = NULL;
    agent_ipc = -1;
    app_ipc = -1;

    winTitle = new char[MAX_LENGTH];

    if (app != NULL) {

	strcpy(winTitle, app);

    }

    exitOnSearch = 0;

    // Initialize Object Pointer Array
    for (int i = 0; i < MAX_WIN; i++)
	window[i] = 0;

    // Memory Management
    appName = new char[MAX_LENGTH];

    for (int i = 0; i < MAX_CLIENTS; i++)
	clientList[i] = new char[MAX_LENGTH];

    nextClient = 0;

    cur_size = 0;
    catlist_window = NULL;
    title_running = false;
    keyboard_running = false;
    paste_ok_ = false;
    keyboard_path_ = "/usr/nanox/bin/nxkeyboard";
    keyboard_maps_ = "/usr/nanox/keymaps";
    keyboard_size_ = "mid";

#ifdef CONFIG_PAR
    if (colorsFromPAR() < 0) {
#endif
	global_colors[APP_BG] = FL_WHITE;
	global_colors[APP_FG] = FL_BLACK;
	global_colors[APP_SEL] = FL_BLACK;
	global_colors[BUTTON_FACE] = FL_WHITE;
	global_colors[BUTTON_TEXT] = FL_BLACK;
	global_colors[BUTTON_PUSH] = FL_BLACK;
	global_colors[BUTTON_3D_LITE] = FL_BLACK;
	global_colors[BUTTON_3D_DARK] = FL_BLACK;
	global_colors[HILIGHT] = FL_BLACK;
	global_colors[HILIGHT_LITE] = FL_WHITE;
	global_colors[HILIGHT_DARK] = FL_WHITE;
	global_colors[TITLE_FG] = FL_WHITE;
	global_colors[TITLE_BG] = FL_BLACK;
	global_colors[SCROLL_FACE] = FL_WHITE;
	global_colors[SCROLL_LITE] = FL_BLACK;
	global_colors[SCROLL_DARK] = FL_BLACK;
	global_colors[SCROLL_TRAY] = FL_BLACK;
	global_colors[RADIO_FILL] = FL_RED;

#ifdef CONFIG_PAR
    }
#endif
}

NxApp::~NxApp()
{
    // Memory Management
    delete[]appName;
    appName = NULL;

    for (int i = 0; i < MAX_CLIENTS; i++) {
	delete[]clientList[i];
	clientList[i] = NULL;
    }

    delete[]winTitle;
    winTitle = NULL;

    for (unsigned int idx = 0; idx < v_SyncDB.size(); idx++) {
	sync_db_struct s;

	s = v_SyncDB[idx];

	if (s.file_fields) {
	    delete[]s.file_fields;
	    s.file_fields = NULL;
	}
    }

    if (c_db_struct)
	free(c_db_struct);

}

/* Load up default colors from the PAR database */

/*
<category name="appcolors">
<pref key="appbg" type=COLOR>#FFFFFF</pref>
<pref key="appfg" type=COLOR>#000000</pref>
<pref key="buttonface" type=COLOR>#FFFFFF</pref>
<pref key="buttonpush" type=COLOR>#000000</pref>
<pref key="button3dlite" type=COLOR>#000000</pref>
<pref key="button3ddark" type=COLOR>#000000</pref>
<pref key="hillight" type=COLOR>#000000</pref>
<pref key="hilightlite" type=COLOR>#FFFFFF</pref>
<pref key="hilightdark" type=COLOR>#FFFFFF</pref>
<pref key="titlefg" type=COLOR>#FFFFFF</pref>
<pref key="titlebg" type=COLOR>#000000</pref>
<pref key="scrollface" type=COLOR>#FFFFFF</pref>
<pref key="scrolllite" type=COLOR>#000000</pref>
<pref key="scrolldark" type=COLOR>#000000</pref>
<pref key="scrolltray" type=COLOR>#000000</pref>
<pref key="radiofill" type=COLOR>#FF0000</pref>
</category>
*/

Fl_Color NxApp::GlobalColor(NxApp_Color nc)
{
    if ((nc < 0) || (nc >= MAX_CLR_ATTRIB))
	return FL_WHITE;

    return (global_colors[nc]);
}

Fl_Color NxApp::getGlobalColor(NxApp_Color nc)
{

    if ((nc < 0) || (nc >= MAX_CLR_ATTRIB))
	return FL_WHITE;

    return (global_colors[nc]);

}

Fl_Color NxApp::globalColor(unsigned long c)
{

    int
	r =
	0,
	b =
	0,
	g =
	0;

    r = (c >> 16) & 0xFF;
    g = (c >> 8) & 0xFF;
    b = c & 0xFF;

    Fl_Color
	tmp =
	fl_color_cube(r * FL_NUM_RED / 256, g * FL_NUM_GREEN / 256,
		      b * FL_NUM_BLUE / 256);

    //printf("%d, %d, %d, cc(%d)\n", r, g, b, (int)tmp);

    return tmp;

}

#ifdef CONFIG_PAR

int
NxApp::colorsFromPAR()
{

    db_handle *db;
    unsigned long c = 0;

    /* Dum, dah, dum, dum, dummmmmm! */
    /* PAR to the rescue */

    /* Fire up the PAR database */
    db = db_openDB(db_getDefaultDB(), PAR_DB_MODE_RDONLY);

    if (db < 0) {
	fprintf(stderr,
		"Error - Could not open par database %s\n",
		db_getDefaultDB());
	return -1;
    }

    par_getGlobalColor(db, "appbg", &c);
    global_colors[APP_BG] = globalColor(c);

    par_getGlobalColor(db, "appfg", &c);
    global_colors[APP_FG] = globalColor(c);

    par_getGlobalColor(db, "appsel", &c);
    global_colors[APP_SEL] = globalColor(c);

    par_getGlobalColor(db, "buttonface", &c);
    global_colors[BUTTON_FACE] = globalColor(c);

    par_getGlobalColor(db, "buttontext", &c);
    global_colors[BUTTON_TEXT] = globalColor(c);

    par_getGlobalColor(db, "buttonpush", &c);
    global_colors[BUTTON_PUSH] = globalColor(c);

    par_getGlobalColor(db, "button3dlite", &c);
    global_colors[BUTTON_3D_LITE] = globalColor(c);

    par_getGlobalColor(db, "button3ddark", &c);
    global_colors[BUTTON_3D_DARK] = globalColor(c);

    par_getGlobalColor(db, "hilight", &c);
    global_colors[HILIGHT] = globalColor(c);

    par_getGlobalColor(db, "hilighttext", &c);
    global_colors[HILIGHT_TEXT] = globalColor(c);
    //printf("hilight_text: %d\n", (int)global_colors[HILIGHT_TEXT]);

    par_getGlobalColor(db, "hilightlite", &c);
    global_colors[HILIGHT_LITE] = globalColor(c);

    par_getGlobalColor(db, "hilightdark", &c);
    global_colors[HILIGHT_DARK] = globalColor(c);

    par_getGlobalColor(db, "titlefg", &c);
    global_colors[TITLE_FG] = globalColor(c);

    par_getGlobalColor(db, "titlebg", &c);
    global_colors[TITLE_BG] = globalColor(c);

    par_getGlobalColor(db, "scrollface", &c);
    global_colors[SCROLL_FACE] = globalColor(c);

    par_getGlobalColor(db, "scrolllite", &c);
    global_colors[SCROLL_LITE] = globalColor(c);

    par_getGlobalColor(db, "scrolldark", &c);
    global_colors[SCROLL_DARK] = globalColor(c);

    par_getGlobalColor(db, "scrolltray", &c);
    global_colors[SCROLL_TRAY] = globalColor(c);

    par_getGlobalColor(db, "radiofill", &c);
    global_colors[RADIO_FILL] = globalColor(c);

    par_getGlobalColor(db, "editorbg", &c);
    global_colors[EDITOR_BG] = globalColor(c);

    par_getGlobalColor(db, "editorfg", &c);
    global_colors[EDITOR_FG] = globalColor(c);

    par_getGlobalColor(db, "editorsel", &c);
    global_colors[EDITOR_SEL] = globalColor(c);

    return 1;

}
#endif

/* Error codes from Colosseum */
void
NxApp::IPCError(int err, char *errMsg)
{

#ifndef CONFIG_COLOSSEUM
    strcpy(errMsg, "No IPC server available\n");
#else
    switch (err) {

    case CL_E_NONE:
	strcpy(errMsg, "CL_E_NONE or CL_CLIENT_SUCCESS");
	break;
    case CL_E_NOAPP:
	strcpy(errMsg, "CL_E_NOAPP");
	break;
    case CL_E_BADCMD:
	strcpy(errMsg, "CL_E_BADCMD");
	break;
    case CL_E_TIMEOUT:
	strcpy(errMsg, "CL_E_TIMEOUT");
	break;
    case CL_E_NACTIVE:
	strcpy(errMsg, "CL_E_NACTIVE");
	break;
    case CL_E_APPERR:
	strcpy(errMsg, "CL_E_APPERR");
	break;
    case CL_E_DESTFULL:
	strcpy(errMsg, "CL_E_DESTFULL");
	break;
    case CL_E_NODEST:
	strcpy(errMsg, "CL_E_NODEST");
	break;
    case CL_E_EMPTY:
	strcpy(errMsg, "CL_E_EMPTY");
	break;
    case CL_E_NOSTART:
	strcpy(errMsg, "CL_E_NOSTART");
	break;
    case CL_E_APPEXISTS:
	strcpy(errMsg, "CL_E_APPEXISTS");
	break;
    case CL_E_APPUNKWN:
	strcpy(errMsg, "CL_E_APPUNKWN");
	break;
    case CL_E_APPACTIVE:
	strcpy(errMsg, "CL_E_APPACTIVE");
	break;
    case CL_CLIENT_ERROR:
	strcpy(errMsg, "CL_CLIENT_ERROR");
	break;
    case CL_CLIENT_SOCK_ERROR:
	strcpy(errMsg, "CL_CLIENT_SOCK_ERROR");
	break;
    case CL_CLIENT_NOSRVR:
	strcpy(errMsg, "CL_CLIENT_NOSRVR");
	break;
    case CL_CLIENT_TIMEOUT:
	strcpy(errMsg, "CL_CLIENT_TIMEOUT");
	break;
    case CL_CLIENT_NODATA:
	strcpy(errMsg, "CL_CLIENT_NODATA");
	break;
    case CL_CLIENT_NOCONN:
	strcpy(errMsg, "CL_CLIENT_NOCONN");
	break;
    case CL_CLIENT_INVALID:
	strcpy(errMsg, "CL_CLIENT_INVALID");
	break;
    case CL_CLIENT_CONNECTED:
	strcpy(errMsg, "CL_CLIENT_CONNECTED");
	break;
    case CL_CLIENT_BADNAME:
	strcpy(errMsg, "CL_CLIENT_BADNAME");
	break;
    case CL_CLIENT_NOTFOUND:
	strcpy(errMsg, "CL_CLIENT_NOTFOUND");
	break;
    }
#endif
}

int
NxApp::ExecuteShow()
{

    Fl_Window *w = get_shown_window();

    if (w == NULL)
	return 0;

    while (w->parent() != NULL)
	w = (Fl_Window *) w->parent();

    w->show();
    show_window(get_shown_window());

#ifdef NANOX
    GR_WINDOW_ID wid = fl_xid(w);
    GR_WINDOW_INFO info;
    GrGetWindowInfo(wid, &info);
    GrMapWindow(info.parent);
    GrRaiseWindow(info.parent);
#endif

    return 0;

}

int
NxApp::ExecuteNoGui()
{

    DPRINT("******* noguisearch_flag = 1\n It won't exit!");
    noguisearch_flag = 1;
    DPRINT("******* exitOnSearch = 1!\n");
    exitOnSearch = 1;

    return 0;
}

int
NxApp::Add_Fd(char *_appName, void (*cb) (int, void *), void *o)
{

#ifndef CONFIG_COLOSSEUM
    return -1;
#else
    strcpy(appName, _appName);
    int flags = 0;
    fd = ClRegister((unsigned char *) appName, &flags);

    ClFindApp((unsigned char *) appName);
    //DPRINT("IPC ID is: [%d]\n", ipcId);
    // If you can't register with the Colosseum,
    // then don't try to register with FLNX.

    DPRINT("NxApp::Add_Fd \t COLOSSEUM RETURNED A %d\n", fd);

    if (fd < 0) {
	char errMsg[255];
	IPCError(fd, &errMsg[0]);
	DPRINT("ClRegister() returned %s. Showing anyway!\n", errMsg);

	ExecuteShow();
	return -1;
    }

    Fl::add_fd(fd, cb, NULL);

    switch (flags) {
    case 2:
	DPRINT("EXECUTE NO GUI SEARCH\n");
	ExecuteNoGui();
	break;

    default:
	DPRINT("TRYING to EXECUTE SHOW\n");
	ExecuteShow();
	break;
    }

    _ClientIPCHandler(fd, NULL);

    return fd;
#endif

}

void
NxApp::Remove_Fd(int fd)
{

    if (fd == -1)
	return;

#ifdef CONFIG_COLOSSEUM
    Fl::remove_fd(fd);
    ClClose();
#endif
}

int
NxApp::Find_Fd(char *_appName)
{

#ifndef CONFIG_COLOSSEUM
    return -1;
#else
    int ipcId = ClFindApp((unsigned char *) _appName);

    if (ipcId < 0) {

	char errMsg[255];
	IPCError(ipcId, &errMsg[0]);
	DPRINT("ClFindApp() returned %s.\n", errMsg);

    }

    return ipcId;
#endif
}

int
NxApp::Read_Fd(char *readMsg, int *length)
{

#ifndef CONFIG_COLOSSEUM
    return -1;
#else
    unsigned short src;
    
    int err = ClGetMessage(readMsg, length, &src);
    
    if (err < 0) {
	char errMsg[255];
	IPCError(err, &errMsg[0]);
	DPRINT("ClGetMessage() returned %s.\n", errMsg);
    }

    DPRINT("Read_Fd(): From = %d, readMsg = (%s) and length = (%i)\n", err,
	   readMsg, *length);
    
    return err;
#endif
}

int
NxApp::Write_Fd(int fd, char *writeMsg, int length)
{

#ifndef CONFIG_COLOSSEUM
    return -1;
#else
    int err = ClSendMessage(fd, writeMsg, length);
    DPRINT("Write_Fd(): To = %d, writeMsg = (%s) and length = (%i)\n", fd,
	   writeMsg, length);

    //  if ( err < 0 ) {

    char errMsg[255];
    IPCError(err, &errMsg[0]);

    //  }

    return err;
#endif

}

int
NxApp::StartApp(char *_appName, unsigned char *args, int flags, int timeout)
{
#ifndef CONFIG_COLOSSEUM
    return -1;
#else
    int err = ClStartApp((unsigned char *) _appName, args, flags, timeout);

    //  if ( err < 0 )
    //    {
    char errMsg[255];
    IPCError(err, &errMsg[0]);
    DPRINT("ClStart() returned %s for application %s\n", errMsg, _appName);
    //    }

    return err;
#endif
}

int
NxApp::VerifyClient(char *client)
{

    if (client == NULL)
	return -1;

    for (int i = 0; i < nextClient; i++) {

	if (strstr(clientList[i], client)) {
	    //DPRINT("VerifyClient() successfully validated client.\n");
	    return 1;
	}

    }

    DPRINT("VerifyClient() unsuccesfuly validated client.\n");
    return 0;

}

int
NxApp::GetKey(NxDb * db, char *db_name, int key_field)
{

    int recs = db->NumRecs(db_name);
    int *rec_array = new int[recs];
    int hi_key = 0;
    int temp_key = 0;
    char str_key[16];

    db->Select(db_name, rec_array, recs, true);

    for (int idx = 0; idx < recs; idx++) {
	db->Extract(db_name, rec_array[idx], key_field, str_key);
	temp_key = atoi(str_key);
	if (temp_key > hi_key)
	    hi_key = temp_key;
    }
    return hi_key;
}

#ifdef CONFIG_COLOSSEUM

void
NxApp::_ClientIPCHandler(int fd, void *o)
{
  int ipc_id = 0;
  char *passMsg = new char[MAX_LENGTH];
  
  int length = MAX_LENGTH - 1;
  memset(passMsg, 0, MAX_LENGTH);
  
  while ((ipc_id = NxApp::Instance()->Read_Fd(passMsg, &length)) >= 0) {
    DPRINT("_ClientIPCHandler(): passMsg = %s\n", passMsg);
    NxApp::Instance()->ClientIPCHandler(fd, passMsg, ipc_id);

    length = MAX_LENGTH - 1;
    memset(passMsg, 0, MAX_LENGTH);
  }

  DPRINT("_ClientIPCHandler ipc_id: [%d]\n", ipc_id);
  delete[]passMsg;
  passMsg = NULL; 
}

void
NxApp::ServerIPCHandler(int fd, int ipc_id, void *o)
{

    char *ack_msg = new char[MAX_LENGTH];

    // Explode msg
    char *service = new char[MAX_LENGTH];
    char *msg_cmd = new char[MAX_LENGTH];
    char *data_item = new char[MAX_LENGTH];
    char *sync_msg = new char[MAX_LENGTH];
    char *new_msg = new char[MAX_LENGTH];

    // Reset
    memset(service, 0, MAX_LENGTH);
    memset(msg_cmd, 0, MAX_LENGTH);
    memset(data_item, 0, MAX_LENGTH);
    memset(sync_msg, 0, MAX_LENGTH);
    memset(new_msg, 0, MAX_LENGTH);

    // Copy sent message from void* o into new_msg and sync_msg
    memcpy(new_msg, (char *) o, MAX_LENGTH);
    memcpy(sync_msg, (char *) o, MAX_LENGTH);

    DPRINT("ServerIPCHandler(): sync_msg = %s\n", sync_msg);

    // SERVICE
    char *tmp = strtok(new_msg, TOKEN);
    if (NULL != tmp)
	strcpy(service, tmp);

    // CMD_MSG
    tmp = strtok(NULL, TOKEN);
    if (NULL != tmp)
	strcpy(msg_cmd, tmp);

    // DATA_ITEM
    tmp = strtok(NULL, TOKEN);
    if (NULL != tmp)
	strcpy(data_item, tmp);

    // Memory Management. Cleanup passMsg from ClientIPCHandler
    delete[]new_msg;
    new_msg = NULL;


    DPRINT("ServerIPCHandler(): Expoding Message... %s, %s, %s\n", service,
	   msg_cmd, data_item);

    // If exitApp gets set, then the PIM bails
    // after it has cleaned up all its memory.
    exitApp = 0;

    if (strcmp(msg_cmd, "INITIATE") == 0) {

	// send acknowledgement
	strcpy(ack_msg, appName);
	strcat(ack_msg, "^ACK^INITIATE^");

	strcpy(clientList[nextClient], service);
	nextClient++;

	if (!VerifyClient(service)) {
	    DPRINT("Error INITIATE communication with server.\n");
	    exit(-1);
	}

    } else if (strcmp(msg_cmd, "EXECUTE") == 0) {

	// verify client
	if (!VerifyClient(service))
	    return;

	// send acknowledgement
	strcpy(ack_msg, appName);
	strcat(ack_msg, "^ACK^EXECUTE^");

	int err = 0;		// = Write_Fd(ipc_id, ack_msg, MAX_LENGTH);

	if (err == -1) {
	    // now what?
	}

	do {

	    if (strcmp(data_item, "move_keyboard") == 0) {
#ifdef NANOX
		keyboard_running = false;
		GR_WINDOW_ID wid = check_for_keyboard(GR_ROOT_WINDOW_ID);

		if (keyboard_running) {
		    int x = atoi(strtok(NULL, TOKEN));
		    int y = atoi(strtok(NULL, TOKEN));

		    GrMoveWindow(wid, x, y);

		}
#endif


	    } else if (strcmp(data_item, "resize_win") == 0) {

		int x = atoi(strtok(NULL, TOKEN));
		int y = atoi(strtok(NULL, TOKEN));
		int width = atoi(strtok(NULL, TOKEN));
		int h = atoi(strtok(NULL, TOKEN));

		/*
		   Fl_Window *w = get_shown_window();

		   while ( w->parent() != NULL ) {

		   // Make sure widgets fit within x, y, width, & h
		   int nKids = w->Fl_Group::children();

		   for ( int i=0; i < nKids; i++ ) {

		   Fl_Widget* kid = w->Fl_Group::child(i);

		   // Let's just try the dh
		   if ( kid->y() > h )
		   {
		   //printf("resize(%d, %d, %d, %d)\n", kid->x(), (h-kid->h()), kid->w(), kid->h());
		   kid->resize(kid->x(), h-kid->h(), kid->w(), kid->h());
		   }

		   kid->redraw();

		   }

		   w->damage(FL_DAMAGE_ALL);

		   w = (Fl_Window*)w->parent();
		   }
		 */

		Fl_Window *w = get_shown_window();
		w->resize(x, y, width, h);

		while (w->parent() != NULL)
		    w = (Fl_Window *) w->parent();
#ifdef NANOX
		GR_WINDOW_ID wid = fl_xid(w);
		GR_WINDOW_INFO info;

		GrGetWindowInfo(wid, &info);
		GrMoveWindow(info.parent, x, y);
		GrResizeWindow(info.parent, width, h);
#endif
	    } else if (strcmp(data_item, "noguisearch") == 0) {

		ExecuteNoGui();

	    } else if (strcmp(data_item, "show") == 0) {

		ExecuteShow();

	    } else if (strcmp(data_item, "hide") == 0) {

		Fl_Window *w = get_shown_window();
		while (w->parent() != NULL)
		    w = (Fl_Window *) w->parent();

		if (w->shown()) {
#ifdef NANOX
		    GR_WINDOW_ID wid = fl_xid(w);
		    GR_WINDOW_INFO info;
		    GrGetWindowInfo(wid, &info);
		    GrUnmapWindow(info.parent);
#endif
		}

	    } else if (strcmp(data_item, "search") == 0) {

		printf("date_item\n");
		if (exitOnSearch) {
		    DPRINT("exitApp = 1\n");
		    exitApp = 1;
		}

	    } else if (strcmp(data_item, "datesearch") == 0) {

		printf("datesearch\n");
		if (exitOnSearch) {

		    exitApp = 1;
		}

	    } else if (strcmp(data_item, "exit") == 0) {

		exitApp = 1;

	    }

	} while ((data_item = strtok(NULL, TOKEN)));


    } else if (strcmp(msg_cmd, "REQUEST") == 0) {

	// verify client
	if (!VerifyClient(service))
	    return;

	// send acknowledgement
	strcpy(ack_msg, appName);
	strcat(ack_msg, "^ACK^REQUEST^");

	int length = MAX_LENGTH;
	int err = Write_Fd(ipc_id, ack_msg, strlen(ack_msg));

	if (err == -1) {
	    // now what?
	}

	char *buf = new char[4];
	char *msg = new char[MAX_LENGTH];
	strcpy(msg, appName);
	strcat(msg, "^DATA^");

	if (strcmp(data_item, "all_clients") == 0) {

	    // Send message to everybody.
	    ipc_id = 255;

	    memset(msg, 0, MAX_LENGTH);
	    strcpy(msg, appName);

	    // Tell all listening clients to...
	    //

	    char *data = strtok(NULL, TOKEN);

	    if (strcmp(data, "show") == 0) {
		strcpy(msg, "^EXECUTE^show^");
	    } else if (strcpy(data, "hide") == 0) {
		strcpy(msg, "^EXECUTE^hide^");
	    } else if (strcpy(data, "exit") == 0) {
		strcpy(msg, "^EXECUTE^exit^");
	    } else if (strcpy(data, "initiate") == 0) {
		strcpy(msg, "^INITIATE^0^");
	    } else if (strcmp(data, "terminate") == 0) {
		strcpy(msg, "^TERMINATE^0^");
	    }

	} else if (strcmp(data_item, "default_win_x") == 0) {

	    sprintf(buf, "%d", W_X);
	    strcat(msg, buf);
	    strcat(msg, "^");

	} else if (strcmp(data_item, "default_win_y") == 0) {

	    sprintf(buf, "%d", W_Y);
	    strcat(msg, buf);
	    strcat(msg, "^");

	} else if (strcmp(data_item, "default_win_w") == 0) {

	    sprintf(buf, "%d", W_W);
	    strcat(msg, buf);
	    strcat(msg, "^");

	} else if (strcmp(data_item, "default_win_h") == 0) {

	    sprintf(buf, "%d", W_H);
	    strcat(msg, buf);
	    strcat(msg, "^");

	} else if (strcmp(data_item, "show_win_x") == 0) {

	    Fl_Window *w = get_shown_window();

	    sprintf(buf, "%d", w->x());
	    strcat(msg, buf);
	    strcat(msg, "^");

	} else if (strcmp(data_item, "show_win_y") == 0) {

	    Fl_Window *w = get_shown_window();

	    sprintf(buf, "%d", w->y());
	    strcat(msg, buf);
	    strcat(msg, "^");

	} else if (strcmp(data_item, "show_win_w") == 0) {

	    Fl_Window *w = get_shown_window();
	    while (w->parent() != NULL)
		w = (Fl_Window *) w->parent();

	    sprintf(buf, "%d", w->w());
	    strcat(msg, buf);
	    strcat(msg, "^");

	} else if (strcmp(data_item, "show_win_h") == 0) {

	    Fl_Window *w = get_shown_window();

	    sprintf(buf, "%d", w->h());
	    strcat(msg, buf);
	    strcat(msg, "^");

	} else if (strcmp(data_item, "keyboard_running") == 0) {

#ifdef NANOX
	    DPRINT("keyboard_running %d\n", keyboard_running);
	    keyboard_running = false;
	    check_for_keyboard(GR_ROOT_WINDOW_ID);
	    sprintf(buf, "%d", keyboard_running);
	    strcat(msg, buf);
	    strcat(msg, "^");
#endif

	}

	length = MAX_LENGTH;
	err = Write_Fd(ipc_id, msg, strlen(msg));

	if (err == -1) {
	    // now what?
	}

	delete[]buf;
	delete[]msg;
	buf = msg = NULL;

    } else if ((strcmp(msg_cmd, "TERMINATE") == 0)) {

	// verify client
	if (!VerifyClient(service))
	    return;

	int position = -1;

	// find client in client list
	for (int i = 0; i < nextClient; i++) {

	    if (strcmp(clientList[i], service) == 0) {

		position = i;
		break;

	    }

	}

	// rearrange client list
	if (position != -1) {

	    for (int j = position; j < nextClient; j++) {

		strcpy(clientList[j], "");

		if (j != (nextClient - 1))
		    strcpy(clientList[j], clientList[j + 1]);

	    }

	    nextClient--;

	}

	DPRINT("Connected Clients: \n");
	for (int i = 0; i < nextClient; i++)
	    DPRINT("%d: %s\n", i, clientList[i]);

    }
#ifdef CONFIG_SYNC
    else {
	if (0 != strcmp("nxsync", appName)) {
	    SyncDb(sync_msg);
	}
    }
#endif

    // Memory Management
    delete[]ack_msg;
    delete[]service;
    delete[]msg_cmd;
    delete[]data_item;
    delete[]sync_msg;
    sync_msg = ack_msg = service = msg_cmd = data_item = NULL;

    DPRINT("exitApp = %d\n", exitApp);

    if (exitApp) {
	DPRINT("********* Exiting Address Application.\n");
	//exit(33);
	noguisearch_flag = 0;
	hide_all_windows();
    }

    DPRINT("IPC Server Done.\n");

}
#endif /* CONFIG_COLOSSEUM */

#ifdef CONFIG_SYNC

bool NxApp::ValidMsgId(short int msg_id)
{

    bool
	ret =
	false;

    switch (msg_id) {
    case ERR:
    case ABORT:
    case OK:
    case INFO:
    case BP:
    case EP:
    case STATUS:
    case BS:
    case ES:
    case TS:
    case RD:
    case ET:
    case EOT:
    case FLIP:
    case COMMIT:
	ret = true;
	break;
    default:
	ret = false;
	break;
    }

    return ret;
}

void
NxApp::GetTableSchema(vector < char >&col_type, vector < int >&col_size)
{

    int idx = 0;
    bool added = false;

    //DPRINT("GetTableSchema(): c_db_struct [%p]\n", c_db_struct);
    //DPRINT("sizeof field [%d]\n", sizeof(field));

    for (field * p_field = c_db_struct->pField; p_field->type != 0;
	 p_field = &(c_db_struct->pField[++idx])) {
	//DPRINT("GetTableScehma() idx [%d] p_field->type [%c] p_field->size [%d] p_field [%p]\n", idx, p_field->type, p_field->size, p_field);

	added = false;
	//DPRINT("GetTableSchema(): p_field %p\n", p_field);
	//DPRINT("GetTableSchema(): p_field->type %c\n", p_field->type);

	for (int jdx = 0; jdx < c_db_struct->size; jdx++) {
	    //DPRINT("GetTableSchema() field [%d]\n", c_db_struct->file_fields[jdx]);
	    if (idx == c_db_struct->file_fields[jdx]) {
		//DPRINT("GetTableSchema () file field [%d]\n", idx);
		col_type.push_back('t');
		col_size.push_back(p_field->size);
		added = true;
		break;
	    }
	}

	if (added == false) {
	    if (p_field->type != 'c') {
		//DPRINT("GetTableSchema() push_back [1]\n");
		col_size.push_back(1);
	    } else {
		//DPRINT("GetTableScehma() push_back [%d]\n", p_field->size);
		col_size.push_back(p_field->size);
	    }
	    //DPRINT("GetTableSchema() push_back [%c]\n", p_field->type);
	    col_type.push_back(p_field->type);
	}
    }
}

void
NxApp::GetRowData(int &flags, vector < string > &data, string & key)
{
    MsgCoder coder;

    // need to handle fields that have a file name
    // these field numbers are stored in v_SyncDB as the
    // file_fields member in the sync_db_struct
    char f_str[16];

    if (NULL != c_db_struct) {

	DPRINT("!!!!! cur_row [%d] is [%d]!!!!!\n", cur_row,
	       rows[cur_row - 1]);
	c_db_struct->db->GetFlags(c_db_struct->str_dbName, rows[cur_row - 1],
				  flags);
	DPRINT("!!!!!! flags [%d] db_Name [%s]!!!!!!!!\n", flags,
	       c_db_struct->str_dbName.c_str());
	if (NxDb::DELETED == flags) {
	    char *ret_buf = new char[MAXRECSIZ];

	    c_db_struct->db->ExtractDeleted(c_db_struct->str_dbName,
					    rows[cur_row - 1], 0, ret_buf);
	    key = string(ret_buf);
	    DPRINT("!!!!!! GetRowData key [%s] !!!!!!!!!!\n", key.c_str());

	    delete[]ret_buf;
	    ret_buf = 0;

	    cur_row++;

	    return;
	}

	int f_size = 1;
	int idx = 0;
	bool got_data = false;
	char *file_buf = 0;
	FILE *file = 0;
	char *file_name = 0;

	for (field * p_field = c_db_struct->pField; p_field->type != 0;
	     p_field = &(c_db_struct->pField[++idx])) {

	    got_data = false;

	    if (0 < c_db_struct->size) {
		for (int jdx = 0; jdx < c_db_struct->size; jdx++) {
		    if (idx == c_db_struct->file_fields[jdx]) {
			// get data from the file
			//DPRINT("Getting file contents\n");
			f_size = p_field->size;
			got_data = true;

			file_name = new char[f_size];
			struct stat stat_buf;
			bool err = false;

			c_db_struct->db->Extract(c_db_struct->str_dbName,
						 rows[cur_row - 1], idx,
						 file_name);
			fprintf(stderr, "got filename [%s]\n", file_name);
			if (stat(file_name, &stat_buf) != 0) {
			    err = true;
			}
			int file_size = 0;

			if (false == err) {
			    if (stat_buf.st_size > 2048)
				file_size = 2048;
			    else
				file_size = stat_buf.st_size;

			    file_buf = new char[file_size];
			    file = fopen(file_name, "r");

			    if (NULL == file) {
				err = true;
			    }

			    if (false == err) {
				int f_pos = 0;
				char c;

				memset(file_buf, 0, file_size);

				while ((c = getc(file)) != EOF) {
				    if (f_pos >= file_size)
					break;
				    file_buf[f_pos] = c;
				    //DPRINT("f_pos [%d] c[%c]\n", f_pos, c);
				    f_pos++;
				}
				fclose(file);
			    }
			}

			sprintf(f_str, "%d", idx);
			//DPRINT("pushing [%s] on data\n", f_str);
			data.push_back(f_str);
			//DPRINT("Going to push file contents *file_buf[%p]\n", file_buf);
			//DPRINT("File buf [%s]\n", file_buf);
			if (0 == file_size || err) {
			    //DPRINT("pushed blank\n");
			    data.push_back("");
			} else {
			    //DPRINT("pushed [%s]\n", file_buf);
			    data.push_back(string(file_buf));
			}

			if (file_buf) {
			    delete[]file_buf;
			    file_buf = 0;
			}
			if (file_name) {
			    delete[]file_name;
			    file_name = 0;
			}

		    }
		}
	    }
	    if (false == got_data) {
		switch (p_field->type) {
		case 'c':
		case 'n':
		    f_size = p_field->size;
		    break;
		case 'd':
		case 'i':
		    f_size = p_field->size * sizeof(short);
		    break;
		case 'l':
		    f_size = p_field->size * sizeof(long);
		    break;
		}

		char *ret_buf = new char[f_size];

		sprintf(f_str, "%d", idx);
		fprintf(stderr, "pushing [%s] on data\n", f_str);
		data.push_back(f_str);
		c_db_struct->db->Extract(c_db_struct->str_dbName,
					 rows[cur_row - 1], idx, ret_buf);
		if (0 == idx) {	// this is the key field
		    key = ret_buf;
		    //DPRINT("!!!Setting the key!!! [%s]\n", key.c_str());
		}
		//DPRINT("!!Got [%s] for ret_buf\n", ret_buf);
		fprintf(stderr, "pushing [%s] on data\n", ret_buf);
		data.push_back(ret_buf);

		delete[]ret_buf;
		ret_buf = 0;
	    }
	}
    }

    else {
	fprintf(stderr, "c_db_struct is NULL\n");
    }

    cur_row++;
}

bool NxApp::SaveRowData(const vector < string > &vmessages)
{

    int
	vdx =
	1;
    sync_db_struct
	db_;
    char
	str_value[16];
    int
	size =
	vmessages.
	size();
    int
	status =
	atoi(vmessages[vdx++].c_str());
    string
	key =
	vmessages[2];

    if (size < 3) {
	DPRINT("Failure on size size[%d] cur_table_size[%d]\n", size,
	       cur_table_size);
	return false;
    }
    if (cur_table_size != (size - 3) / 2 && status != NxDb::DELETED) {
	DPRINT("Failure on size size[%d] cur_table_size[%d]\n", size,
	       cur_table_size);
	return false;
    }
    // find the cur_table
    for (unsigned int idx = 0; idx < v_SyncDB.size(); idx++) {

	db_ = v_SyncDB[idx];

	if (cur_table == db_.table_num) {
	    break;
	}
    }

    if (NULL != db_.db) {
	NxDb *
	    db =
	    db_.
	    db;
	if (0 < cur_table_size) {
	    if (status == NxDb::DELETED) {	// delete
		int
		    data[1];

		int
		    rows =
		    db->
		    Select(db_.str_dbName, (char *) key.c_str(), 0, data,
			   1);
		if (0 < rows) {
		    // delete the file if their is one with this record
		    char
			file_name[255];
		    bool
			file_data =
			false;

		    for (int idx = 0; idx < cur_table_size; idx++) {
			for (int jdx = 0; jdx < db_.size; jdx++) {
			    file_data = false;
			    if (idx == db_.file_fields[jdx]) {
				file_data = true;
			    }
			    if (true == file_data) {
				memset(file_name, 0, sizeof(file_name));
				db->Extract(db_.str_dbName, data[0], idx,
					    file_name);
				if (strlen(file_name) > 0) {
				    unlink(file_name);
				}
			    }
			}
		    }
		    db->EraseRec(db_.str_dbName, data[0]);
		}
		return true;
	    }
	    DPRINT("Key [%s]\n", key.c_str());
	    char *
		rec =
		new char[MAXRECSIZ];
	    memset(rec, 0, MAXRECSIZ);

	    field *
		pField =
		db_.
		pField;

	    short
		short_num =
		0;
	    int
		num =
		0;
	    bool
		file_data =
		false;

	    //DPRINT("cur table size [%d]\n", cur_table_size);
	    for (int idx = 0; idx < cur_table_size; idx++) {

		file_data = false;

		if (&pField[idx] == NULL) {
		    //DPRINT("returing false on pField\n");
		    return false;
		}
		if (vdx > size) {
		    //DPRINT("returning false on size\n");
		    return false;
		}
		if (atoi(vmessages[++vdx].c_str()) != idx) {
		    delete[]rec;
		    rec = 0;
		    //DPRINT("returning false on col num != idx vdx[%d] idx[%d]\n", vdx, idx);
		    return false;
		}
		for (int jdx = 0; jdx < db_.size; jdx++) {
		    if (idx == db_.file_fields[jdx]) {
			//DPRINT("set file_data to true idx [%d]\n", idx);
			file_data = true;
		    }
		}
		if (file_data == true) {
		    //DPRINT("Field Type [%c]\n", pField[idx].type);
		    if (pField[idx].type == 'c') {
			char *
			    path =
			    db->
			    GetPath();
			char *
			    new_app_name =
			    0;
			char
			    new_name[4];
			char *
			    file_name =
			    new char[pField[idx].size];

			memset(file_name, 0, sizeof(pField[idx].size));
			memset(new_name, 0, sizeof(new_name));
			if (appName != NULL) {
			    new_app_name = strdown(appName, strlen(appName));
			    strncpy(new_name, new_app_name, 3);
			    strcpy(file_name, tempnam(path, new_name));
			} else {
			    strcpy(file_name, tempnam(path, "pim"));
			}

			vdx++;
			if (vmessages[vdx] != "") {
			    //DPRINT("copy [%s] to rec\n", file_name);
			    strcpy(&rec[pField[idx].offset], file_name);

			    //DPRINT("Opening file [%s]\n", file_name);
			    FILE *
				file =
				fopen(file_name, "w+");
			    if (NULL == file) {
				//DPRINT("Unable to open file [%s] file_name\n", file_name);
			    } else {
				//DPRINT("Write [%s] to [%s] vdx[%d]\n", vmessages[vdx].c_str(), file_name, vdx);
				size_t
				    byte_w =
				    fwrite(vmessages[vdx].c_str(), 1,
					   vmessages[vdx].size(), file);
				if (byte_w != vmessages[vdx].size()); {
				    //DPRINT("Unable to write [%s] to file [%s]\n", vmessages[vdx].c_str(),
				    //                              file_name);
				}
				//DPRINT("closing file\n");
				fclose(file);
				delete[]file_name;
				file_name = 0;
			    }
			}
		    } else {
			DPRINT
			    ("Returning false on wrong field type for file\n");
			return false;
		    }
		} else {
		    switch (pField[idx].type) {
		    case 'i':
			DPRINT("case 'i'\n");
			strcpy(str_value, vmessages[++vdx].c_str());
			//DPRINT("case 'i' str_value[%s]\n", str_value);
			short_num = atoi(str_value);
			//DPRINT("case 'i' short_num [%d]\n", short_num);
			put16(&rec[pField[idx].offset], short_num);
			//DPRINT("put 16\n");
			break;
		    case 'l':
			//DPRINT("case 'l'\n");
			strcpy(str_value, vmessages[++vdx].c_str());
			//DPRINT("case 'l' str_value[%s]\n", str_value);
			num = atoi(str_value);
			//DPRINT("case 'l' num [%d]\n", num);
			put32(&rec[pField[idx].offset], num);
			//DPRINT("put 32\n");
			break;
		    case 'c':
			//DPRINT("case 'c'\n");
			//DPRINT("case 'c' value[%s] idx[%d] vdx[%d]\n", vmessages[vdx + 1].c_str(),
			//idx, vdx);
			//DPRINT("case 'c' doing strcpy\n");
			strcpy(&rec[pField[idx].offset],
			       vmessages[++vdx].c_str());
			//DPRINT("case 'c' after strcpy\n");
			break;
		    default:
			delete[]rec;
			rec = 0;
			//DPRINT("returning false on default\n");
			return false;
			break;
		    }
		}
	    }
	    if (status == NxDb::NONE) {
		DPRINT("Got a NONE status\n");
	    } else if (status == NxDb::CHANGED) {
		int
		    data[1];

		data[0] = -1;

		int
		    rows =
		    db->
		    Select(db_.str_dbName, (char *) key.c_str(), 0, data,
			   1,
			   true);

		DPRINT("rows [%d] data[0] [%d]\n", rows, data[0]);
		if (rows > 0) {
		    DPRINT("Doing an edit!\n");
		    db->Edit(db_.str_dbName, data[0], rec);
		    db->SetFlags(db_.str_dbName, data[0], NxDb::NONE);
		}
		delete[]rec;
		rec = 0;
	    } else if (status == NxDb::NEW) {
		DPRINT("Going to insert\n");
		// save the record and reset the flag to none
		int
		    data[1];

		data[0] = -1;

		int
		    rows =
		    db->
		    Select(db_.str_dbName, (char *) key.c_str(), 0, data,
			   1);

		DPRINT("rows [%d] data[0] [%d]\n", rows, data[0]);
		if (rows > 0) {	// should not happen?
		    return true;
		}

		int
		    recno = -
		    1;
		db->Insert(db_.str_dbName, rec, recno);
		delete[]rec;
		rec = 0;
		if (-1 == recno) {
		    //DPRINT("returnig false on insert\n");
		    return false;
		}
		DPRINT("Inserted on recno [%d]\n", recno);
		db->SetFlags(db_.str_dbName, recno, NxDb::NONE);
	    } else {
		DPRINT("Got an Unknown status. Bail Out!!!\n");
		return false;
	    }
	}
    }
    return true;
}

bool NxApp::CheckTableSchema(const vector < string > &vmessages)
{

    // find the pointer to the pField from the cur_table num
    field *
	pField =
	NULL;

    for (unsigned int idx = 0; idx < v_SyncDB.size(); idx++) {
	sync_db_struct
	    db_;

	db_ = v_SyncDB[idx];

	if (cur_table == db_.table_num) {
	    //DPRINT("Checking db [%d]\n", cur_table);
	    pField = db_.pField;
	}
    }

    if (pField != NULL) {

	int
	    num_cols =
	    0;
	int
	    idx =
	    0;
	int
	    vdx =
	    3;
	string
	    num_col_str =
	    vmessages[2];

	cur_table_size = num_cols = atoi(num_col_str.c_str());
	//DPRINT("num_cols [%d]\n", num_cols);

	for (idx = 0; idx < num_cols; idx++, vdx++) {

	    if ("i" == vmessages[vdx]) {
		if (pField[idx].type != 'i') {
		    //DPRINT("failed on i idx[%d]\n", idx);
		    return false;
		}
	    } else if ("l" == vmessages[vdx]) {
		if (pField[idx].type != 'l') {
		    //DPRINT("failed on l idx[%d]\n", idx);
		    return false;
		}
	    } else if ("c" == vmessages[vdx] || "t" == vmessages[vdx]) {
		if (pField[idx].type != 'c') {
		    //DPRINT("failed on c or t idx[%d]\n", idx);
		    return false;
		}
	    }
	    // check the size
	    if ("t" != vmessages[vdx]) {
		string
		    size_str =
		    vmessages[vdx + num_cols];
		int
		    size =
		    atoi(size_str.c_str());

		if (pField[idx].size != size) {
		    //DPRINT("failed on size check idx[%d]\n", idx);
		    return false;
		}
	    }
	}

	if (num_cols == idx)
	    return true;
    } else
	cur_table_size = 0;

    return false;

}

void
NxApp::UpdateFlags()
{
    int data[1024];
    int jdx;

    for (unsigned int idx = 0; idx < v_SyncDB.size(); idx++) {
	sync_db_struct db_ = v_SyncDB[idx];
	NxDb *db = db_.db;

	int rows = db->Select(db_.str_dbName, data, 1024, false,
			      NxDb::CHANGED | NxDb::NEW);

	for (jdx = 0; jdx < rows; jdx++) {
	    db->SetFlags(db_.str_dbName, data[jdx], NxDb::NONE);
	}

	rows = db->Select(db_.str_dbName, data, 1024, false, NxDb::DELETED);

	for (jdx = 0; jdx < rows; jdx++) {
	    db->EraseRec(db_.str_dbName, data[jdx]);
	}
    }
}

void
NxApp::DoError(int err, int ipc)
{

    MsgCoder coder;
    string msg = coder.Err(err);

    DPRINT("Doing Error [%d]\n", err);
    Write_Fd(ipc, (char *) msg.c_str(), msg.length());
    nextSyncState = -1;
    appSyncState = -1;
    cur_db = -1;
    cur_row = -1;
    cur_table_size = -1;
    cur_table = -1;
    total_rows = -1;
    c_db_struct = NULL;
    if (exitOnSearch) {
	//DPRINT("exitApp = 1\n");
	exitApp = 1;
    }

}

void
NxApp::SyncDb(char *new_msg)
{

    printf("=========================================\n");
    printf("MSG:  [%s]\n", new_msg);

    // Connect to Synchronization Application through Colosseum
    ////////////////////////////////////////////////////////////

    short msg_id;		// CODE of the message
    MsgCoder coder;		// Class to encode/decode message
    string msg(new_msg);	// convert char* to string
    string next_msg;		// Message to send in reponse to msg

    /* Start the sync angent (if it hasn't already been started */

    if ((agent_ipc = Find_Fd("syncagent")) < 0) {
	if (agent_ipc == CL_CLIENT_NOTFOUND)
	    agent_ipc = StartApp("syncagent");
    }

    app_ipc = Find_Fd("nxsync");

    printf("DEBUG:  AGENT = %d, APP = %d\n", agent_ipc, app_ipc);

    // Test for valid ipc returned from Colosseum.
    // If bad ipc, send error and return.
    //////////////////////////////////////////////
    if (0 >= agent_ipc || 0 >= app_ipc) {
	printf("Error - Unable to open agent [%d] or app [%d]\n",
	       agent_ipc, app_ipc);

	if (app_ipc > 0) {
	    DoError(NO_AGENT, app_ipc);
	}
    }
    // Decode the new message
    /////////////////////////
    coder.vmessages.clear();
    coder.DecodeMsg(msg);

    // Get the message id & validate it
    ////////////////////////////////////
    msg_id = atoi((char *) coder.vmessages[0].c_str());

    if (!ValidMsgId(msg_id)) {
	printf("Error - Invalid id %d\n", msg_id);
	printf("=========================================\n");
	return;
    }

    if (-1 == total_rows) {	//either on first or going to next table

	//DPRINT("SyncDb(): v_SyncDB.size = %d\n", v_SyncDB.size());

	if (0 == v_SyncDB.size()) {
	    // no databases go to commit state
	    // and write COMMIT msg
	    nextSyncState = COMMIT;
	    DoError(NO_DB_REG, app_ipc);
	    printf("=========================================");
	    return;
	} else {
	    cur_db++;
	    if (cur_db < (int) v_SyncDB.size()) {
		sync_db_struct db_struct = v_SyncDB[cur_db];
		DPRINT("v_SyncDB table num [%d]\n",
		       v_SyncDB[cur_db].table_num);
		if (NULL == c_db_struct)
		    c_db_struct =
			(sync_db_struct *) malloc(sizeof(sync_db_struct));
		memcpy(c_db_struct, &db_struct, sizeof(sync_db_struct));

		DPRINT("SyncDB(): Init c_db_struct [%p]\n", c_db_struct);

		total_rows = 0;
		cur_row = 1;
	    }
	}
    }

    if (ERR == msg_id) {
	if (DT_BUSY == atoi((char *) coder.vmessages[1].c_str())) {
	    DoError(DT_BUSY, app_ipc);
	    printf("=========================================");
	    return;
	}
	if (CLOSE_CONN == atoi((char *) coder.vmessages[1].c_str())) {
	    DoError(CLOSE_CONN, app_ipc);
	    printf("=========================================");
	    return;
	}
	if (AGENT_NS == atoi((char *) coder.vmessages[1].c_str())) {
	    DoError(AGENT_NS, app_ipc);
	    printf("=========================================");
	    return;
	}
    }
    if (BS == msg_id || ES == msg_id || STATUS == msg_id || ABORT == msg_id) {	// these come from app

	// This switch statement covers the state machine logic for the state
	// between the Synchronization Application and this PIM Application.
	//////////////////////////////////////////////////////////////////////

	printf("*** Synchronization Application State Machine Started.\n");

	if (msg_id == ABORT) {
	    nextSyncState = -1;
	    appSyncState = -1;
	    cur_db = -1;
	    cur_row = -1;
	    cur_table_size = -1;
	    cur_table = -1;
	    total_rows = -1;
	    c_db_struct = NULL;
	    msg = coder.Err(USER_ABORT);
	    Write_Fd(agent_ipc, (char *) msg.c_str(), msg.length());
	    printf("=========================================");
	    return;
	}

	switch (appSyncState) {

	    // Received a BS (Begin Sync) from Sync Application.
	    // Begin the Sync process by sending a BP (Begin PIM)
	    // to the Synchronization Agent. The Synchronization agent
	    // will pass it on to the Desktop. The Desktop should return
	    // an OK message back.
	    /////////////////////////////////////////////////////////////
	case -1:{

		char *p_msg = new char[CL_MAX_MSG_LEN];

		delete[]p_msg;
		p_msg = 0;

		if (BS == msg_id) {
		    DPRINT("Sending BP Message to DT...\n");
		    next_msg = coder.BeginPimSync(appName);
		    nextSyncState = BP;
		    printf("-----OUTGOING:  [%s] [%d]\n", next_msg.c_str(),
			   next_msg.length());

		    Write_Fd(agent_ipc, (char *) next_msg.c_str(),
			     next_msg.length());
		    next_msg = coder.Ok();
		    appSyncState = OK;
		    DPRINT("BS set appSyncState OK\n");
		} else {
		    appSyncState = ERR;
		    DoError(EXP_BS, app_ipc);
		    printf("=========================================");
		    return;
		}
		break;

	    }
	    //case INFO:
	case OK:
				/***********
				if(STATUS == msg_id) {
					appSyncState = INFO;
						// sync app needs to know current status from here
						next_msg = coder.Info(0, "0");
					}
					else {
						next_msg = coder.Info(0, "100");
					}
				}
				********/
				/*********
				if(ES == msg_id) {
					appSyncState = -1;
					nextSyncState = -1;
					next_msg = coder.Ok();
					if ( exitOnSearch ) {
	 					//DPRINT("exitApp = 1\n");
	  				exitApp = 1;
					}
				}
				else {
					DoError(ES, app_ipc);
					return;
				}
				else {
					if(INFO == appSyncState) {
						DoError(EXP_INFO, app_ipc);
						return;
					}
					if(OK == appSyncState) {
						DoError(EXP_OK, app_ipc);
						return;
					}
					appSyncState = ERR;
				}
				*********/
	    break;
      /*******
			case ES:
				if(STATUS == appSyncState) {
					next_msg = coder.Ok();
					appSyncState = -1;

				}
				else {
					DoError(EXP_ES, app_ipc);
					return;
				}
				break;
			******/
	default:
	    DoError(UNEXP_ERROR, app_ipc);
	    printf("=========================================");
	    return;
	    break;
	}

	printf("&&&&& APP IPC = [%s] &&&&&\n", next_msg.c_str());

	//DPRINT("Sending [%s]\n", next_msg.c_str());
	Write_Fd(app_ipc, (char *) next_msg.c_str(), next_msg.length());

    } else {

	// This switch statement covers the state machine logic for the state
	// of the Desktop and the PIM Application.
	//////////////////////////////////////////////////////////////////////

	printf("*** Desktop State Machine Started...\n");

	switch (nextSyncState) {

	    // 100  ERR     Error Message
	    //////////////////////////////
	case ERR:
	    //next_msg = coder.Err(UNEXP_ERROR);
	    printf("=========================================");
	    return;

	    break;

	    // OK   200     Okay Message
	    /////////////////////////////
	case OK:
	    if (RD == msg_id) {
		if (SaveRowData(coder.vmessages)) {
		    next_msg = coder.Ok();
		    nextSyncState = OK;
		} else {
		    nextSyncState = ERR;
		    DoError(SAVE_RD, agent_ipc);
		    printf("=========================================");
		    return;
		}
	    } else if (ET == msg_id) {
		next_msg = coder.Ok();
		nextSyncState = OK;
	    } else if (EOT == msg_id) {
		cur_table = -1;
		next_msg = coder.Ok();
		nextSyncState = OK;
	    } else if (FLIP == msg_id) {
		next_msg = coder.Commit();
		nextSyncState = COMMIT;
	    } else if (TS == msg_id) {
		//DPRINT("!!!!!GOT 600 TS!!!!!!!!");
		string t_num = coder.vmessages[1];

		cur_table = atoi(t_num.c_str());

		if (CheckTableSchema(coder.vmessages)) {
		    nextSyncState = INFO;
		    next_msg = coder.Info(0);
		} else {
		    nextSyncState = ERR;
		    DoError(BAD_TS, agent_ipc);
		    printf("=========================================");
		    return;
		}
	    } else if (ERR == msg_id) {
		DPRINT("BP expecting ok\n");
		DPRINT("OK got an ERR\n");
	    }
	    break;

	    // 250  INFO    Information Message
	    ////////////////////////////////////
	case INFO:
	    if (RD == msg_id) {
		// got the first row data set up to save data
		if (SaveRowData(coder.vmessages)) {
		    next_msg = coder.Ok();
		    nextSyncState = OK;
		} else {
		    nextSyncState = ERR;
		    DoError(SAVE_RD, agent_ipc);
		    printf("=========================================");
		    return;
		}
	    } else if (ET == msg_id) {
		next_msg = coder.Ok();
		nextSyncState = OK;
	    } else {
		nextSyncState = ERR;
		DoError(EXP_RD, agent_ipc);
		printf("=========================================");
		return;
	    }
	    break;

	    // 300  BP      Begin PIM Message
	    //////////////////////////////////
	case BP:{


		if (OK == msg_id) {

		    DPRINT("ServerIPCHandler(): Got OK response.\n");

		    vector < char >col_type;
		    vector < int >col_size;

		    GetTableSchema(col_type, col_size);
		    nextSyncState = TS;
		    next_msg = coder.TableSchema(c_db_struct->table_num,
						 col_type, col_size);
		    // Write TS message to sync agent
		} else {
		    DPRINT("BP expecting ok\n");
		    nextSyncState = ERR;
		    DoError(EXP_OK, agent_ipc);
		    printf("=========================================");
		    return;
		}
		break;
	    }

	    // 350  EP      End PIM Message
	    ////////////////////////////////
	case EP:

	    // If you get an EP and recv. no OK
	    // What should we do?
	    ////////////////////////////////////
	    //if(OK == msg_id) {
	    //nextSyncState = OK;
	    //next_msg = coder.Ok();

	    DPRINT("In EP STATE\n");
					/*****
					nextSyncState = -1;
					appSyncState = -1;
					cur_db = -1;
					cur_row = -1;
					cur_table_size = -1;
					cur_table = -1;
					total_rows = -1;
					c_db_struct = NULL;
	
					// we are done so send 100 info to app	
					next_msg = coder.Info(0, "100");
					Write_Fd(app_ipc, (char *)next_msg.c_str(), next_msg.length());
					********/
	    printf("=========================================");
	    return;

	    //}
	    //else {
	    //      nextSyncState = ERR;
	    //      next_msg = coder.Err(EXP_OK);
	    //}

	    break;

	    // 600  TS      Table Schema Message
	    /////////////////////////////////////
	case TS:{
		if (INFO == msg_id) {
		    int flags = -1;
		    int i_key = 0;
		    vector < string > data;
		    string key;
		    int sync_type = atoi(coder.vmessages[1].c_str());
		    NxDb *db = c_db_struct->db;

		    if (sync_type == MERGE) {
			total_rows =
			    db->Select(c_db_struct->str_dbName, rows, 1,
				       false,
				       NxDb::CHANGED | NxDb::DELETED | NxDb::
				       NEW);
		    } else if (sync_type == PDAOVR) {
			total_rows =
			    db->Select(c_db_struct->str_dbName, rows, 1,
				       true);
		    } else if (sync_type == DTOVR) {
			total_rows = 0;
		    } else {
			total_rows = 0;
		    }
		    if (0 == total_rows) {
			nextSyncState = ET;
			next_msg = coder.EndTable();
			total_rows = -1;
			cur_row = -1;
		    } else {
			nextSyncState = RD;
			DPRINT("TS total_rows [%d]\n", total_rows);
			GetRowData(flags, data, key);
			i_key = atoi(key.c_str());
			DPRINT("!!!!!!!!! i_key [%d] !!!!!!!!\n", i_key);
			next_msg = coder.RowData(flags, i_key, data);
		    }
		} else {
		    nextSyncState = ERR;
		    DoError(EXP_INFO, agent_ipc);
		    printf("=========================================");
		    return;
		}
		break;
	    }

	    // 800  ET      End Table Message
	    //////////////////////////////////
	case ET:{
		if (OK == msg_id) {
		    if (cur_db == (int) v_SyncDB.size()) {	// send EOT
			next_msg = coder.EndOfTables();
			nextSyncState = EOT;
			cur_db = -1;
			total_rows = -1;
		    } else {	// send next Table Schema
			nextSyncState = TS;
			vector < char >col_type;
			vector < int >col_size;

			GetTableSchema(col_type, col_size);
			next_msg = coder.TableSchema(c_db_struct->table_num,
						     col_type, col_size);
		    }
		} else {
		    nextSyncState = ERR;
		    DoError(EXP_OK, agent_ipc);
		    printf("=========================================");
		    return;
		}
		break;
	    }

	    // 850  EOT     End of Tables Message
	    //////////////////////////////////////
	case EOT:
	    if (OK == msg_id) {
		nextSyncState = FLIP;
		next_msg = coder.Flip();
	    } else {
		nextSyncState = ERR;
		DoError(EXP_OK, agent_ipc);
		printf("=========================================");
		return;
	    }
	    break;

	    // 700  RD      Row Data Message
	    /////////////////////////////////
	case RD:{
		if (OK == msg_id) {	// send ET
		    if ((cur_row - 1) == total_rows) {
			DPRINT("Sending end table\n");
			if (cur_db == (int) v_SyncDB.size()) {
			    cur_db = -1;
			}
			cur_row = -1;
			total_rows = -1;
			next_msg = coder.EndTable();
			nextSyncState = ET;
		    } else {	// send the next row
			int flags = -1;
			int i_key = 0;
			vector < string > data;
			string key;

			nextSyncState = RD;
			DPRINT("Getting row data cur_row [%d]\n", cur_row);
			GetRowData(flags, data, key);
			i_key = atoi(key.c_str());
			DPRINT("!!!!!! key [%d]!!!!!!\n", i_key);
			next_msg = coder.RowData(flags, i_key, data);
		    }
		} else {
		    nextSyncState = ERR;
		    DoError(EXP_OK, agent_ipc);
		    printf("=========================================");
		    return;
		}
		break;
	    }

	    // 900  FLIP    Flip Direction Message
	    ///////////////////////////////////////
	case FLIP:{
		if (TS == msg_id) {
		    // got TS set cur table
		    string t_num = coder.vmessages[1];
		    cur_table = atoi(t_num.c_str());
		    nextSyncState = INFO;
		    next_msg = coder.Info(0);
		    if (CheckTableSchema(coder.vmessages)) {
			nextSyncState = INFO;
			next_msg = coder.Info(0);
		    } else {
			nextSyncState = ERR;
			DoError(BAD_TS, agent_ipc);
			printf("=========================================");
			return;
		    }
		} else {
		    nextSyncState = ERR;
		    DoError(EXP_TS, agent_ipc);
		    printf("=========================================");
		    return;
		}
		break;
	    }

	    // 950  COMMIT  Commit Changes Message
	    ///////////////////////////////////////
	case COMMIT:
	    if (OK == msg_id) {
		nextSyncState = EP;
		UpdateFlags();
		Refresh();

		nextSyncState = -1;
		appSyncState = -1;
		cur_db = -1;
		cur_row = -1;
		cur_table_size = -1;
		cur_table = -1;
		total_rows = -1;
		c_db_struct = NULL;

		// we are done so send 100 info to app  
		next_msg = coder.Info(0, "100");
		Write_Fd(app_ipc, (char *) next_msg.c_str(),
			 next_msg.length());

		next_msg = coder.EndPimSync();
		Write_Fd(agent_ipc, (char *) next_msg.c_str(),
			 next_msg.length());
		exitApp = 1;
		printf("=========================================");
		return;
	    } else {
		nextSyncState = ERR;
		DoError(EXP_OK, agent_ipc);
		printf("=========================================");
		return;
	    }
	    break;

	default:
	    // error on sync state
	    DoError(UNEXP_ERROR, agent_ipc);
	    printf("=========================================");
	    return;
	    break;
	}

	if (appSyncState != -1)
	    Write_Fd(agent_ipc, (char *) next_msg.c_str(), next_msg.length());

    }

}
#endif

void
NxApp::resize_notify(int X, int Y, int W, int H)
{
    //printf("\t+++ NxApp::resize_notify()\n");

#ifdef NANOX
    for (int i = 0; i < cur_size; i++) {
	if ((window[i]->x() != X) || (window[i]->y() != Y) ||
	    (window[i]->w() != W) || (window[i]->h() != H)) {
	    window[i]->resize_notify(X, Y, W, H);
	}
    }
#endif

    //printf("\t+++ done.\n");
}

void
NxApp::hide_all_windows()
{
    //printf("hide_all_windows()\n");
    for (int i = 0; i < cur_size; i++) {
	//fprintf(stderr, "hide_all_windows:w[%p]\n", window[i]);
	window[i]->hide();
    }
}

void
NxApp::show_window(Fl_Window * w, int type, Fl_Window * w_target)
{

    //fprintf(stderr, "show_window:w[%p] w_target[%p]\n", w, w_target);
    if (w == 0)
	w = window[0];

    bool activated = false;
    for (int i = 0; i < cur_size; i++) {

	// Hide all windows except w and DEACTIVATE
	if (window[i] != w) {	// This is not the window to show()
	    if (type == DEACTIVATE) {
		w_target->deactivate();
	    } else {
		window[i]->hide();
	    }
	} else {
	    if (false == activated) {
		w->activate();
		w->show();
		set_shown_window(w);
		activated = true;
	    }
	}
    }

}

void
NxApp::add_window(Fl_Window * w)
{

    if ((cur_size < MAX_WIN) && (window[cur_size + 1] == 0)) {

	for (int i = 0; i < cur_size; i++) {
	    if (window[i] == w)
		return;
	}

	window[cur_size] = w;
	cur_size++;
    } else {
	//cerr << "Maximum Window Limit Reached: " << MAX_WIN << ". Exiting Application.\n";
	hide_all_windows();
    }

}

void
NxApp::set_catlist_window(Fl_Window * w)
{

    for (int idx = 0; idx < MAX_WIN; idx++) {
	if (window[idx] == catlist_window && catlist_window != NULL) {
	    window[idx] = w;
	    catlist_window = w;
	    return;
	}
    }
    catlist_window = w;
    add_window(catlist_window);
}

Fl_Window *
NxApp::get_catlist_window()
{
    return catlist_window;
}


//
// Cut and Paste code
//

void
NxApp::copy_callback(Fl_Widget * fl, void *o)
{
    printf("copy_callback\n");
    Fl_Widget *w = Fl::selection_owner();

    if (fl_editor_type && dynamic_cast < Fl_Editor * >(w)) {

	//    printf("Fl_Editor->Copy() w = %p\n", w);
	((Fl_Editor *) w)->Copy();

    } else {

	if (!fl_editor_type && dynamic_cast < Fl_Input * >(w)) {
	    ((Fl_Input_ *) w)->copy();
	}

    }
    paste_ok_ = true;
}

void
NxApp::cut_callback(Fl_Widget * fl, void *o)
{
    Fl_Widget *w = Fl::selection_owner();

    undoTarget = w;

    if (fl_editor_type && dynamic_cast < Fl_Editor * >(w)) {
	((Fl_Editor *) w)->Cut();

    } else {

	if (!fl_editor_type && dynamic_cast < Fl_Input * >(w)) {
	    ((Fl_Input *) w)->cut();
	}

    }
    paste_ok_ = true;
}

void
NxApp::pasteTarget_callback(Fl_Widget * fl, void *o)
{
    //  printf("pasteTarget_callback() Fl::selection_owner = %p\n", fl);
    Fl::selection_owner(fl);
    g_PasteTarget = fl;
    int temp = (int) o;
    fl_editor_type = temp;
}


void
NxApp::paste_callback(Fl_Widget * fl, void *o)
{

    //  printf("NxApp::paste_callback() start.\n");

    if (g_PasteTarget)
	undoTarget = g_PasteTarget;

    if (paste_ok_) {

	if (g_PasteTarget && !fl_editor_type) {
	    Fl::paste(*(Fl_Input *) g_PasteTarget);
	} else {
	    //      printf("\telse !Fl_Input\n\t\tg_PasteTarget=%p, fl_editor_type=%d\n", g_PasteTarget, fl_editor_type);
	    if (g_PasteTarget && fl_editor_type) {
		//      Fl::paste(*(Fl_Editor*)g_PasteTarget);
		//      printf("\t\tFl_Editor's g_PasteTarget->Paste()\n");
		((Fl_Editor *) g_PasteTarget)->Paste();
	    }

	}

    }
    //  printf("NxApp::paste_callback() end.\n");

}

void
NxApp::undo_callback(Fl_Widget * fl, void *o)
{
    if ((undoTarget)) {
	((Fl_Input_ *) undoTarget)->undo();
    }
}

void
NxApp::set_about(about about_this)
{
    memcpy(&about_app, &about_this, sizeof(about));
}

void
NxApp::hide_about_cb(Fl_Widget * fl, void *o)
{
    NxApp *this_app = (NxApp *) o;
    Fl_Window *about_win = this_app->get_about_window();

    about_win->hide();

    this_app->get_shown_window()->activate();

}

void
NxApp::show_about(Fl_Widget * fl, void *o)
{
    int x = 10;
    int y = W_H / 3;
    int w = W_W - 20;
    int h = 114;
    about about_app = NxApp::Instance()->get_about();
    char tempBuf[100];

    NxPimPopWindow *win = new NxPimPopWindow("About",
					     NxApp::Instance()->
					     NxApp::getGlobalColor(APP_FG),
					     x, y, w, h);
    NxApp::Instance()->set_about_window(win->GetWindowPtr());
    {
	NxBox *o = new NxBox(0, 0, w, (h));
	o->color(NxApp::Instance()->getGlobalColor(APP_BG));
	o->box(FL_BORDER_BOX);
	win->add((Fl_Widget *) o);
    }
    {
	NxOutput *o = new NxOutput(2, 2, (w - 4), 15, "");
	o->box(FL_BORDER_BOX);
	o->color(NxApp::Instance()->getGlobalColor(TITLE_BG));
	o->textcolor(NxApp::Instance()->getGlobalColor(TITLE_FG));
	o->value(about_app.title);
	win->add((Fl_Widget *) o);
    }
    {
	NxButton *o =
	    new NxButton(BUTTON_X, h - 24, BUTTON_WIDTH, BUTTON_HEIGHT,
			 "Close");
	o->callback(hide_about_cb, NxApp::Instance());
	win->add((Fl_Widget *) o);
    }
    {
	sprintf(tempBuf, "%s", about_app.copyright);
	Fl_Output *o =
	    new Fl_Output(BUTTON_X, 20, w - BUTTON_X - 5, BUTTON_HEIGHT, "");
	o->color(NxApp::Instance()->getGlobalColor(APP_BG));
	o->box(FL_FLAT_BOX);
	o->value(tempBuf);
	NxApp::Instance()->def_font(o);
	win->add((Fl_Widget *) o);
    }
  /******
	{
    sprintf(tempBuf, "Author: %s", about_app.author);
    Fl_Output *o = new Fl_Output(BUTTON_X, 35, w - BUTTON_X - 15, BUTTON_HEIGHT, "");
    o->box(FL_FLAT_BOX);
    o->color(FL_GRAY);
    o->value(tempBuf);
  }
	*******/
    {
	//    sprintf(tempBuf, "%s", about_app.date);       
	sprintf(tempBuf, "10/30/2001");
	Fl_Output *o =
	    new Fl_Output(BUTTON_X, 35, w - BUTTON_X - 15, BUTTON_HEIGHT, "");
	o->box(FL_FLAT_BOX);
	o->color(NxApp::Instance()->getGlobalColor(APP_BG));
	o->value(tempBuf);
	NxApp::Instance()->def_font(o);
	win->add((Fl_Widget *) o);
    }
    {
	//    sprintf(tempBuf, "Version %s", about_app.version);
	sprintf(tempBuf, "Version 1.1");
	Fl_Output *o =
	    new Fl_Output(BUTTON_X, 50, w - BUTTON_X - 15, BUTTON_HEIGHT, "");
	o->box(FL_FLAT_BOX);
	o->color(NxApp::Instance()->getGlobalColor(APP_BG));
	o->value(tempBuf);
	NxApp::Instance()->def_font(o);
	win->add((Fl_Widget *) o);
    }


    Fl_Window **win_list;
    win_list = NxApp::Instance()->get_window_list();
    for (int i = 0; i < NxApp::Instance()->get_cur_size(); i++) {
	if (win_list[i]->shown()) {
	    NxApp::Instance()->set_shown_window(win_list[i]);
	    break;
	}
    }
    for (int jdx = 0; jdx < NxApp::Instance()->get_cur_size(); jdx++) {
	if (NxApp::Instance()->get_shown_window() == win_list[jdx]) {
	    win_list[jdx]->deactivate();
	} else {
	    win_list[jdx]->hide();
	}
    }
    Fl_Widget *par_window =
	(Fl_Widget *) NxApp::Instance()->get_shown_window()->parent();
    ((Fl_Window *) par_window)->add(NxApp::Instance()->get_about_window());
    win->GetWindowPtr()->show();
}

void
NxApp::keyboard_callback(Fl_Widget *, void *o)
{

    // FIXME
    // eventully the app should be moved or resized so 
    // the keyboard will fit at the bottom of the screen?????

    NxApp::Instance()->launch_keyboard(0, 0, 0, 0);

}

void
NxApp::set_keyboard(int argc, char *argv[])
{

    int c;

    while ((c = getopt(argc, argv, OPTIONS)) != -1) {
	switch (c) {
	case 'k':
	    keyboard_path_ = optarg;
	    break;
	case 'd':
	    keyboard_maps_ = optarg;
	    break;
	case 'm':
	    keyboard_size_ = optarg;
	    break;
	default:
	    break;
	}
    }
}

#ifdef NANOX
GR_WINDOW_ID NxApp::check_for_keyboard(GR_WINDOW_ID wid)
{

    GR_WM_PROPERTIES
	child_props;
    GR_WM_PROPERTIES
	sib_props;
    GR_WINDOW_INFO
	info;

    GrGetWindowInfo(wid, &info);

    if (info.child) {
	GrGetWMProperties(info.child, &child_props);

	if (child_props.title) {
	    if (0 == strcmp("Popup Keyboard", (char *) child_props.title) ||
		0 == strcmp("Soft Keyboard", (char *) child_props.title)) {
		keyboard_running = true;
		GrMapWindow(wid);
		GrRaiseWindow(wid);
		return info.wid;
	    }
	}

	check_for_keyboard(info.child);
    }

    if (info.sibling) {
	GrGetWMProperties(info.sibling, &sib_props);
	check_for_keyboard(info.sibling);
    }

    return 0;
}
#endif /* NANO_X */

int
NxApp::launch_keyboard(int x, int y, int w, int h)
{
#ifdef NANOX
    pid_t childpid;

    keyboard_running = false;
    check_for_keyboard(GR_ROOT_WINDOW_ID);

    if (!keyboard_running) {
	if ((childpid = fork()) == -1) {
	    perror("NxApp::launch_keyboard (FORK FAILED)");
	    return (-1);
	} else if (childpid == 0) {
	    int ret =
		execlp(keyboard_path_.c_str(), keyboard_path_.c_str(), "-d",
		       keyboard_maps_.c_str(), "-m", keyboard_size_.c_str(),
		       NULL);
	    if (ret == -1) {
		perror("NxApp::launch_keyboard (EXEC FAILED)");
		return (-1);
	    }
	}
    }
#endif
    return 0;
}

char *
NxApp::strdown(const char *str1, int size)
{
    char *str2 = new char[size + 1];
    memset(str2, 0, size);

    char c;

    for (int idx = 0; idx < size + 1; idx++) {
	c = str1[idx];
	str2[idx] = (char) tolower(c);
    }

    return str2;
}

char *
NxApp::strup(const char *str1, int size)
{

    char *str2 = new char[size + 1];
    memset(str2, 0, size);

    char c;

    for (int idx = 0; idx < size + 1; idx++) {
	c = str1[idx];
	str2[idx] = (char) toupper(c);
    }

    return str2;
}

int
NxApp::GetDateString(char *buf, const struct tm *tm, int size, int format)
{
    char day[4];
    char *new_buf = NULL;
    int ret = 0;

    strftime(day, 3, "%d", tm);

    if ('0' == day[0]) {
	day[0] = day[1];
	day[1] = '\0';
    }

    ret += strftime(buf, size - 1, "%b", tm);
    strcat(buf, " ");
    strcat(buf, day);

    new_buf = (char *) calloc(size, sizeof(char));
    if (NULL == new_buf)
	return errno;

    strcpy(new_buf, buf);

    ret += strftime(buf, size - 1, "%Y", tm);

    strcat(new_buf, ", ");
    strcat(new_buf, buf);

    strcpy(buf, new_buf);
    free(new_buf);

    if (0 == ret)
	return ret;

    ret += 3;
    return ret;
}

bool NxApp::searchFile(const char *str, const char *file)
{
    char
	block[SMALL_BLK];
    char
	big_block[BIG_BLK];
    size_t
	bytes_read;
    char *
	needle =
	strup(str, strlen(str));
    long
	pos =
	0;
    long
	tmp_pos =
	0;

    if (strlen(str) == 0 || 0 == strcmp(str, "")) {
	delete[]needle;
	return false;
    }

    FILE *
	fp =
	fopen(file, "r");
    if (!fp) {
	delete[]needle;
	return false;
    }

    memset(block, 0, sizeof(block));
    memset(big_block, 0, sizeof(big_block));

    while ((bytes_read = fread(block, 1, SMALL_BLK - 1, fp)) > 0) {
	pos += bytes_read;
	tmp_pos = pos;
	char *
	    haystack =
	    strup(block, bytes_read);

	if (strstr(haystack, needle)) {
	    delete[]needle;
	    delete[]haystack;
	    needle = 0;
	    haystack = 0;
	    fclose(fp);
	    return true;
	}

	if (pos > (SMALL_BLK - 1))
	    memmove(big_block, big_block + SMALL_BLK - 1, SMALL_BLK - 1);
	memcpy(big_block + SMALL_BLK - 1, haystack, bytes_read);
	if (strstr(big_block, needle)) {
	    delete[]needle;
	    delete[]haystack;
	    needle = 0;
	    haystack = 0;
	    fclose(fp);
	    return true;
	}
	delete[]haystack;
	haystack = 0;
	memset(block, 0, sizeof(block));
    }
    delete[]needle;
    needle = 0;
    fclose(fp);
    return false;
}

void
NxApp::DefaultFont(Fl_Widget * widget)
{
    widget->labelfont(DEFAULT_LABEL_FONT);
    widget->labelsize(DEFAULT_LABEL_SIZE);
}

void
NxApp::def_font(Fl_Widget * widget)
{
    widget->labelfont(DEFAULT_LABEL_FONT);
    widget->labelsize(DEFAULT_LABEL_SIZE);
}

void
NxApp::def_font(Fl_Input * input)
{
    input->labelfont(DEFAULT_LABEL_FONT);
    input->labelsize(DEFAULT_LABEL_SIZE);
    input->textfont(DEFAULT_TEXT_FONT);
    input->textsize(DEFAULT_TEXT_SIZE);

}

void
NxApp::def_font(Fl_Output * output)
{
    output->labelfont(DEFAULT_LABEL_FONT);
    output->labelsize(DEFAULT_LABEL_SIZE);
    output->textfont(DEFAULT_TEXT_FONT);
    output->textsize(DEFAULT_TEXT_SIZE);
}

void
NxApp::big_font(Fl_Output * output)
{
    output->textfont(DEFAULT_BIG_FONT);
    output->textsize(DEFAULT_BIG_SIZE);
}

void
NxApp::def_font(Fl_Menu_Bar * mb)
{
    mb->labelfont(DEFAULT_LABEL_FONT);
    mb->labelsize(DEFAULT_LABEL_SIZE);
    mb->textfont(DEFAULT_TEXT_FONT);
    mb->textsize(DEFAULT_TEXT_SIZE);
}

void
NxApp::def_font(Fl_Menu_Button * mb)
{
    mb->labelfont(DEFAULT_LABEL_FONT);
    mb->labelsize(DEFAULT_LABEL_SIZE);
    mb->textfont(DEFAULT_TEXT_FONT);
    mb->textsize(DEFAULT_TEXT_SIZE);
}

void
NxApp::def_font(Fl_Hold_Browser * hb)
{
    hb->labelfont(DEFAULT_LABEL_FONT);
    hb->labelsize(DEFAULT_LABEL_SIZE);
    hb->textfont(DEFAULT_TEXT_FONT);
    hb->textsize(DEFAULT_TEXT_SIZE);
}

void
NxApp::DefaultFont(Fl_Toggle_Tree * tree)
{
    tree->textfont((Fl_Font) DEFAULT_LABEL_FONT);
    tree->textsize(DEFAULT_LABEL_SIZE);
}

void
NxApp::def_font(Fl_Toggle_Tree * tree)
{
    tree->textfont((Fl_Font) DEFAULT_LABEL_FONT);
    tree->textsize(DEFAULT_LABEL_SIZE);
}

void
NxApp::DefaultFont(void)
{
    fl_font(DEFAULT_TEXT_FONT, DEFAULT_TEXT_SIZE);
}

void
NxApp::def_font()
{
    fl_font(DEFAULT_TEXT_FONT, DEFAULT_TEXT_SIZE);
}

void
NxApp::def_small_font()
{
#ifdef NEW
  char dir[128];
  getDirectory("fontdir", dir, sizeof(dir));
  
  strcat(fontdir, "/share/fonts/pda3x6.fnt");
  printf("TRYING TO LOAD THE FONT %s\n", dir);
  fl_font(dir, DEFAULT_SMALL_SIZE);
#endif

#ifdef NANOX
  fl_font(DEFAULT_SMALL_FONT, DEFAULT_SMALL_SIZE);
#else
  fl_font(DEFAULT_TEXT_FONT, DEFAULT_SMALL_SIZE);
#endif
}

/* I don't know where else this should go */

#ifdef CONFIG_PAR
static db_handle *
openPar()
{
    db_handle *db = db_openDB(db_getDefaultDB(), PAR_DB_MODE_RDONLY);
    return db;
}

static db_handle *par_db = openPar();

int
getGblParInt(char *cat, char *pref)
{
    int s;
    par_getGlobalPref(par_db, cat, pref, PAR_INT, &s, sizeof(int));
    return s;
}

void
getGblParStr(char *cat, char *pref, char *text, int len)
{
    par_getGlobalPref(par_db, cat, pref, PAR_TEXT, text, len);
}

void 
getDirectory(char *dir, char *text, int size) {
  par_getScreentopDir(par_db, dir, text, size);
}

#endif
