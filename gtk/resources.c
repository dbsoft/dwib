#ifndef __MAC__
#include <gtk/gtk.h>

/* XPMs */
#include "Interface Builder.xpm"

#include "Window.xpm"
#include "Box.xpm"
#include "Text.xpm"
#include "Entryfield.xpm"
#include "Combobox.xpm"
#include "Listbox.xpm"
#include "Container.xpm"
#include "Tree.xpm"
#include "MLE.xpm"
#include "Render.xpm"
#include "Button.xpm"
#include "Ranged.xpm"
#include "Bitmap.xpm"
#include "Notebook.xpm"
#include "NotebookPage.xpm"
#include "HTML.xpm"
#include "Calendar.xpm"
#include "Padding.xpm"
#include "Menu.xpm"
#include "Placehold.xpm"
#if GTK_MAJOR_VERSION < 3
#include "Font.xpm"
#include "Color.xpm"
#endif

#if GTK_MAJOR_VERSION > 2
/* GdkPixbufs Inline */
#include "Font.h"
#include "Color.h"
#endif

/* Associated IDs */
#include "resources.h"

#define RESOURCE_MAX 23

long _resource_id[RESOURCE_MAX] = {
ICON_APP,
ICON_WINDOW,
ICON_BOX,
ICON_TEXT,
ICON_ENTRYFIELD,
ICON_COMBOBOX,
ICON_LISTBOX,
ICON_CONTAINER,
ICON_TREE,
ICON_MLE,
ICON_RENDER,
ICON_BUTTON,
ICON_RANGED,
ICON_BITMAP,
ICON_NOTEBOOK,
ICON_NOTEBOOK_PAGE,
ICON_HTML,
ICON_CALENDAR,
ICON_PADDING,
ICON_MENU,
ICON_FONT,
ICON_COLOR,
BITMAP_PLACEHOLD
};

char *_resource_data[RESOURCE_MAX] = {
(char *)Interface_Builder_xpm,
(char *)Window_xpm,
(char *)Box_xpm,
(char *)Text_xpm,
(char *)Entryfield_xpm,
(char *)Combobox_xpm,
(char *)Listbox_xpm,
(char *)Container_xpm,
(char *)Tree_xpm,
(char *)MLE_xpm,
(char *)Render_xpm,
(char *)Button_xpm,
(char *)Ranged_xpm,
(char *)Bitmap_xpm,
(char *)Notebook_xpm,
(char *)NotebookPage_xpm,
(char *)HTML_xpm,
(char *)Calendar_xpm,
(char *)Padding_xpm,
(char *)Menu_xpm,
#if GTK_MAJOR_VERSION > 2
(char *)Font,
(char *)Color,
#else
(char *)Font_xpm,
(char *)Color_xpm,
#endif
(char *)Placehold_xpm
};

typedef struct _resource_struct {
	long resource_max, resource_id;
	char **resource_data;
} DWResources;

DWResources _resources = { RESOURCE_MAX, (long)_resource_id, (char **)_resource_data };
#endif
