/* Dynamic Windows Application: Interface Builder
 * Created by Dynamic Windows Interface Builder
 * Author: Brian Smith
 */

#ifndef DWIB_INT_H
#define DWIB_INT_H

#define DWIB_NAME "Dynamic Windows Interface Builder"

enum DWTYPES
{
    TYPE_WINDOW = 1,
    TYPE_BOX,
    TYPE_TEXT,
    TYPE_ENTRYFIELD,
    TYPE_COMBOBOX,
    TYPE_LISTBOX,
    TYPE_CONTAINER,
    TYPE_TREE,
    TYPE_MLE,
    TYPE_RENDER,
    TYPE_BUTTON,
    TYPE_RANGED,
    TYPE_BITMAP,
    TYPE_NOTEBOOK,
    TYPE_NOTEBOOK_PAGE,
    TYPE_HTML,
    TYPE_CALENDAR,
    TYPE_PADDING,
    TYPE_MENU
} DWTypes;

/* Internal library functions */
int _dwib_get_color(char *color);
xmlNodePtr _dwib_find_child(xmlNodePtr node, char *name);
void _dwib_populate_list(HWND list, xmlNodePtr node, xmlDocPtr doc);
void _dwib_builder_toggle(int val);
HMENUI _dwib_children(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND box, int windowlevel);
HMENUI _dwib_child(xmlDocPtr doc, HWND window, HWND box, int windowlevel, xmlNodePtr p, HMENUI menu, int index);

HWND _dwib_window_create(xmlNodePtr node, xmlDocPtr doc);
void _dwib_padding_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_menu_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HMENUI packbox, HMENUI submenu);
HWND _dwib_listbox_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_bitmap_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_calendar_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_html_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_mle_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_render_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_tree_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_combobox_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_entryfield_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_ranged_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_container_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_text_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_button_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_box_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_notebook_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index);
HWND _dwib_notebook_page_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox);

/* Builder app functions */
void DWSIGNAL properties_window(xmlNodePtr node);
int DWSIGNAL preview_delete(HWND window, void *data);
void add_row(HWND vbox, HWND scrollbox, int count, char *colname, char *coltype, char *colalign, int disable);
void properties_current(void);
int generateNode(char *buf, xmlNodePtr p);
xmlNodePtr findWindow(xmlNodePtr thisnode);
void toolbar_bitmap_buttons_create(void);
void toolbar_text_buttons_create(void);


/* INI File support */
typedef struct _saveconfig {
    char name[20];
    int type;
    void *data;
} SaveConfig;

enum type_list {
    TYPE_NONE = 0,
    TYPE_INT,
    TYPE_ULONG,
    TYPE_FLOAT,
    TYPE_BOOLEAN,
    TYPE_STRING
};

#endif
