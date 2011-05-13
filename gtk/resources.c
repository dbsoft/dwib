/* XPMs */
#include "FILE.xpm"
#include "FOLDER.xpm"
#include "LINK.xpm"
#include "about.xpm"
#include "admin.xpm"
#include "change.xpm"
#include "connect.xpm"
#include "disconnect.xpm"
#include "exit.xpm"
#include "flush.xpm"
#include "handyftp.xpm"
#include "help.xpm"
#include "logo.xpm"
#include "newtab.xpm"
#include "preferences.xpm"
#include "queue.xpm"
#include "remtab.xpm"
#include "save.xpm"
#include "unqueue.xpm"
#include "unsave.xpm"

/* Associated IDs */
#include "handyftp.h"

#define RESOURCE_MAX 20

long _resource_id[RESOURCE_MAX] = {
	FILEICON,
	FOLDERICON,
	LINKICON,
	IDM_ABOUT,
	ADMIN,
	PB_CHANGE,
	CONNECT,
	DISCONNECT,
	IDM_EXIT,
	FLUSHQ,
	MAIN_FRAME,
	IDM_GENERALHELP,
	LOGO,
	NEWTAB,
	IDM_PREFERENCES,
	ADDTOQ,
	REMOVETAB,
	SAVETITLE,
	REMOVEFROMQ,
	UNSAVETITLE
};

char *_resource_data[RESOURCE_MAX] = {
	(char *)FILE_xpm,
	(char *)FOLDER_xpm,
	(char *)LINK_xpm,
	(char *)about_xpm,
	(char *)admin_xpm,
	(char *)change_xpm,
	(char *)connect_xpm,
	(char *)disconnect_xpm,
	(char *)exit_xpm,
	(char *)flush_xpm,
	(char *)handyftp_xpm,
	(char *)help_xpm,
	(char *)logo_xpm,
	(char *)newtab_xpm,
	(char *)preferences_xpm,
	(char *)queue_xpm,
	(char *)remtab_xpm,
	(char *)save_xpm,
	(char *)unqueue_xpm,
	(char *)unsave_xpm
};

typedef struct _resource_struct {
	long resource_max, resource_id;
	char **resource_data;
} DWResources;

DWResources _resources = { RESOURCE_MAX, (long)_resource_id, (char **)_resource_data };
