/* Dynamic Windows Application: Interface Builder
 * Created by Dynamic Windows Interface Builder
 * Author: Brian Smith
 */

#include <dw.h>
#include <libxml/tree.h>
#include <string.h>
#include "resources.h"
#include "dwib_int.h"
#include "dwib.h"

HWND hwndToolbar, hwndProperties, hwndPreview = 0;
xmlDocPtr DWDoc;
xmlNodePtr DWCurrNode = NULL, DWClipNode = NULL;
HICN hIcons[20];

#ifdef MSVC
#define snprintf _snprintf
#endif

char *Classes[] =
{
    "Window",
    "Box",
    "Text",
    "Entryfield",
    "Combobox",
    "Listbox",
    "Container",
    "Tree",
    "MLE",
    "Render",
    "Button",
    "Ranged",
    "Bitmap",
    "Notebook",
    "NotebookPage",
    "HTML",
    "Calendar",
    "Padding",
    "Menu", 
    NULL
};

/* Returns TRUE if the node is a valid class */
int is_valid(xmlNodePtr node)
{
    int x = 0;
    
    while(Classes[x] && strcmp(Classes[x], (char *)node->name))
    {
        x++;
    }
    if(Classes[x])
        return x+1;
    return FALSE;
}

/* Returns TRUE if a packable class is selected */
int is_packable(xmlNodePtr node, int message)
{
    if(strcmp((char *)node->name, "Window") == 0 ||
       strcmp((char *)node->name, "Box") == 0 ||
       strcmp((char *)node->name, "NotebookPage") == 0)
    {
        return TRUE;
    }
    if(message)
    {
        dw_messagebox(DWIB_NAME, DW_MB_OK | DW_MB_ERROR, "Selected widget needs to be packable (window, box, splitbar, notebook page)");
    }
    return FALSE;
}

/* Gets the contents of the list and puts it into the XML tree...
 * replacing any previous contents of the list.
 */
void saveList(xmlNodePtr node, HWND vbox)
{
    HWND list = (HWND)dw_window_get_data(vbox, "list");
    xmlNodePtr this = _dwib_find_child(node, "List");
    
    if(node && list)
    {
        int x, count = dw_listbox_count(list);
        char buf[100];
        
        if(this)
        {
            xmlUnlinkNode(this);
            xmlFreeNode(this);
        }
        this = xmlNewTextChild(node, NULL, (xmlChar *)"List", (xmlChar *)"");
        
        for(x=0;x<count;x++)
        {
            dw_listbox_get_text(list, x, buf, 100);
            xmlNewTextChild(this, NULL, (xmlChar *)"Item", (xmlChar *)buf);
        }
    }
}

/* Gets the contents of the list and puts it into the XML tree...
 * replacing any previous contents of the list.
 */
void save_columns(xmlNodePtr node, HWND vbox)
{
    int x, count = DW_POINTER_TO_INT(dw_window_get_data(vbox, "colcount"));
    xmlNodePtr this = _dwib_find_child(node, "Columns");
    
    if(node)
    {
        char buf[50];
        
        if(this)
        {
            xmlUnlinkNode(this);
            xmlFreeNode(this);
        }
        
        this = xmlNewTextChild(node, NULL, (xmlChar *)"Columns", (xmlChar *)"");
        
        count++;
        
        for(x=0;x<count;x++)
        {
            HWND entry, combo1, combo2;
            
            snprintf(buf, 50, "entryfield%d", x);
            entry = (HWND)dw_window_get_data(vbox, buf);
            snprintf(buf, 50, "coltype%d", x);
            combo1 = (HWND)dw_window_get_data(vbox, buf);
            snprintf(buf, 50, "colalign%d", x);
            combo2 = (HWND)dw_window_get_data(vbox, buf);
            
            if(entry && combo1 && combo2)
            {
                char *colname = dw_window_get_text(entry);
                char *coltype = dw_window_get_text(combo1);
                char *colalign = dw_window_get_text(combo2);
                
                if(colname && *colname && coltype && *coltype && colalign && *colalign)
                {
                    xmlNodePtr thisNode = xmlNewTextChild(this, NULL, (xmlChar *)"Item", (xmlChar *)colname);
                    xmlSetProp(thisNode, (xmlChar *)"ColType", (xmlChar *)coltype);
                    xmlSetProp(thisNode, (xmlChar *)"ColAlign", (xmlChar *)colalign);
                }
                if(colname)
                    dw_free(colname);
                if(coltype)
                    dw_free(coltype);
                if(colalign)
                    dw_free(colalign);
            }
        }
    }
}

/* Checks the values on the properties and updates
 * the XML node data.
 */
void updateNode(xmlNodePtr node, HWND vbox, char *name, int toggle)
{
    HWND item = (HWND)dw_window_get_data(vbox, name);
    char *val = "0";
    
    if(!item)
        return;
    
    if(toggle && dw_checkbox_get(item))
    {
        val = "1";
    }
    
    if((toggle || (val = dw_window_get_text(item))))
    {
        xmlNodePtr this = _dwib_find_child(node, name);
        
        if(!this)
            this = xmlNewTextChild(node, NULL, (xmlChar *)name, (xmlChar *)val);
        else
            xmlNodeSetContent(this, (xmlChar *)val);
        if(!toggle)
            dw_free(val);
    }
}

/* Save the properties for general items */
void save_item(xmlNodePtr node, HWND vbox)
{
    updateNode(node, vbox, "dataname", FALSE);
    updateNode(node, vbox, "width", FALSE);
    updateNode(node, vbox, "height", FALSE);
    updateNode(node, vbox, "hexpand", TRUE);
    updateNode(node, vbox, "vexpand", TRUE);
    updateNode(node, vbox, "padding", FALSE);
    updateNode(node, vbox, "enabled", TRUE);
    updateNode(node, vbox, "fcolor", FALSE);
    updateNode(node, vbox, "bcolor", FALSE);
    updateNode(node, vbox, "font", FALSE);
#ifdef __OS2__
    updateNode(node, vbox, "os2font", FALSE);
#elif defined(__MAC__)    
    updateNode(node, vbox, "macfont", FALSE);
#elif defined(__WIN32__)    
    updateNode(node, vbox, "winfont", FALSE);
#elif defined(__UNIX__)    
    updateNode(node, vbox, "unixfont", FALSE);
#endif
}

/* Updates the XML tree with current settings */
void save_properties(void)
{
    int which = DW_POINTER_TO_INT(dw_window_get_data(hwndProperties, "type"));
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    xmlNodePtr node;
    
    if(!vbox || !which)
        return;
        
    if(!(node = dw_window_get_data(vbox, "node")))
        return;
    
    switch(which)
    {
        case TYPE_WINDOW:
            updateNode(node, vbox, "title", FALSE);
            updateNode(node, vbox, "width", FALSE);
            updateNode(node, vbox, "height", FALSE);
            updateNode(node, vbox, "x", FALSE);
            updateNode(node, vbox, "y", FALSE);
            updateNode(node, vbox, "center", TRUE);
            updateNode(node, vbox, "bordersize", FALSE);
            updateNode(node, vbox, "close", TRUE);
            updateNode(node, vbox, "minimize", TRUE);
            updateNode(node, vbox, "maximize", TRUE);
            updateNode(node, vbox, "hide", TRUE);
            updateNode(node, vbox, "titlebar", TRUE);
            updateNode(node, vbox, "resize", TRUE);
            updateNode(node, vbox, "dialog", TRUE);
            updateNode(node, vbox, "border", TRUE);
            updateNode(node, vbox, "sysmenu", TRUE);
            updateNode(node, vbox, "tasklist", TRUE);
            updateNode(node, vbox, "orientation", FALSE);
            break;
        case TYPE_BOX:
            updateNode(node, vbox, "subtype", FALSE);
            save_item(node, vbox);
            updateNode(node, vbox, "orientation", FALSE);
            updateNode(node, vbox, "title", FALSE);
            updateNode(node, vbox, "splitper", FALSE);
            break;
        case TYPE_TEXT:
            updateNode(node, vbox, "subtype", FALSE);
            save_item(node, vbox);
            updateNode(node, vbox, "label", FALSE);
            updateNode(node, vbox, "alignment", FALSE);
            updateNode(node, vbox, "valignment", FALSE);
            break;
        case TYPE_ENTRYFIELD:
            updateNode(node, vbox, "subtype", FALSE);
            save_item(node, vbox);
            updateNode(node, vbox, "deftext", FALSE);
            updateNode(node, vbox, "limit", FALSE);
            break;
        case TYPE_COMBOBOX:
            save_item(node, vbox);
            updateNode(node, vbox, "deftext", FALSE);
            saveList(node, vbox);
            break;
        case TYPE_LISTBOX:
            save_item(node, vbox);
            updateNode(node, vbox, "multi", TRUE);
            saveList(node, vbox);
            break;
        case TYPE_CONTAINER:
            updateNode(node, vbox, "subtype", FALSE);
            save_item(node, vbox);
            updateNode(node, vbox, "multi", TRUE);
            save_columns(node, vbox);
            break;
        case TYPE_TREE:
            save_item(node, vbox);
            break;
        case TYPE_MLE:
            save_item(node, vbox);
            updateNode(node, vbox, "deftext", FALSE);
            break;
        case TYPE_RENDER:
            save_item(node, vbox);
            break;
        case TYPE_BUTTON:
            updateNode(node, vbox, "subtype", FALSE);
            save_item(node, vbox);
            updateNode(node, vbox, "checked", TRUE);
            updateNode(node, vbox, "setting", FALSE);
            updateNode(node, vbox, "bubblehelp", FALSE);
            updateNode(node, vbox, "borderless", TRUE);
            break;
        case TYPE_RANGED:
            updateNode(node, vbox, "subtype", FALSE);
            save_item(node, vbox);
            updateNode(node, vbox, "position", FALSE);
            updateNode(node, vbox, "upper", FALSE);
            updateNode(node, vbox, "lower", FALSE);
            break;
        case TYPE_BITMAP:
            save_item(node, vbox);
            updateNode(node, vbox, "setting", FALSE);
            break;
        case TYPE_NOTEBOOK:
            save_item(node, vbox);
            updateNode(node, vbox, "position", FALSE);
            break;
        case TYPE_NOTEBOOK_PAGE:
            updateNode(node, vbox, "title", FALSE);
            updateNode(node, vbox, "pagetext", FALSE);
            updateNode(node, vbox, "statustext", FALSE);
            updateNode(node, vbox, "orientation", FALSE);
            break;
        case TYPE_HTML:
            save_item(node, vbox);
            updateNode(node, vbox, "URL", FALSE);
            break;
        case TYPE_CALENDAR:
            save_item(node, vbox);
            break;
        case TYPE_PADDING:
            updateNode(node, vbox, "width", FALSE);
            updateNode(node, vbox, "height", FALSE);
            updateNode(node, vbox, "hexpand", TRUE);
            updateNode(node, vbox, "vexpand", TRUE);
            break;
        case TYPE_MENU:
            updateNode(node, vbox, "title", FALSE);
            updateNode(node, vbox, "dataname", FALSE);
            updateNode(node, vbox, "checkable", TRUE);
            updateNode(node, vbox, "checked", TRUE);
            updateNode(node, vbox, "enabled", TRUE);
            break;
    }
    if(tree)
    {
        char buf[200];
        /* Create the title for the node */
        int index = generateNode(buf, node);
        
        if(index && node->_private)
        {
            dw_tree_item_change(tree, (HTREEITEM)node->_private, buf, hIcons[index]);
        }
    }
}

#define PROPERTIES_HEIGHT 22
#define PROPERTIES_WIDTH 124
#define BUTTON_ICON_WIDTH 40
#define BUTTON_ICON_HEIGHT 30

char *defvalstr = "", *defvalint = "-1", *defvaltrue = "1", *defvalzero = "0";

extern char *Colors[];

/* Populate the properties dialog with nothing */
void properties_none(int refresh)
{
    HWND item, vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    item = dw_text_new("No item selected", 0);
    dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
    
    if(refresh)
        dw_window_redraw(hwndProperties);
}

/* Create a color picker dialog to set the field */
int DWSIGNAL color_clicked(HWND window, void *data)
{
    HWND item = (HWND)data;
    char *oldcolor = dw_window_get_text(item);
    unsigned long color = DW_CLR_WHITE;
    
    if(oldcolor)
    {
        color = _dwib_get_color(oldcolor);
        dw_free(oldcolor);
    }
    color = dw_color_choose(color == DW_CLR_DEFAULT ? DW_CLR_WHITE : color);
    
    if(color != DW_CLR_DEFAULT)
    {
        char buf[50];
        
        snprintf(buf, 50, "#%06x", (int)((DW_RED_VALUE(color) << 16) |
                 (DW_GREEN_VALUE(color) << 8) | (DW_BLUE_VALUE(color))));
        dw_window_set_text(item, buf);
    }
    return FALSE;
}

/* Create a color picker dialog to set the field */
int DWSIGNAL font_clicked(HWND window, void *data)
{
    HWND item = (HWND)data;
    char *oldfont = dw_window_get_text(item);
    char *font = dw_font_choose(oldfont);
    
    if(oldfont)
        dw_free(oldfont);
    if(font)
    {
        dw_window_set_text(item, font);
        dw_free(font);
    }
    return FALSE;
}

/* Populate the properties window with generic item fields */
void properties_item(xmlNodePtr node, HWND scrollbox, int box)
{
    HWND item, tmp, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    int x;
#ifdef __OS2__
    char *sysfont = "os2font";
    char *sysfonttext = "OS/2 Font:";
#elif defined(__MAC__)
    char *sysfont = "macfont";
    char *sysfonttext = "Mac Font:";
#elif defined(__WIN32__)
    char *sysfont = "winfont";
    char *sysfonttext = "Windows Font:";
#elif defined(__UNIX__)
    char *sysfont = "macfont";
    char *sysfonttext = "Unix Font:";
#else
    char *sysfont = NULL;
    char *sysfonttext = NULL;
#endif
    
    /* Data name*/
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Data name", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    if((this = _dwib_find_child(node, "dataname")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "dataname", (void *)item);
    if(box)
    {
        /* Required size */
        hbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
        item = dw_text_new("Size (width, height)", 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
        dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
        val = defvaltrue;
        if((this = _dwib_find_child(node, "width")))
        {
            if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
                val = thisval;
        }
        item = dw_spinbutton_new(val, 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        dw_spinbutton_set_limits(item, 2000, 0);
        dw_window_set_data(vbox, "width", (void *)item);
        val = defvaltrue;
        if((this = _dwib_find_child(node, "height")))
        {
            if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
                val = thisval;
        }
        item = dw_spinbutton_new(val, 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        dw_spinbutton_set_limits(item, 2000, 0);
        dw_window_set_data(vbox, "height", (void *)item);
        /* Expandable */
        hbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
        item = dw_text_new("Expand", 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
        dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
        val = defvaltrue;
        if((this = _dwib_find_child(node, "hexpand")))
        {
            if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
                val = thisval;
        }
        item = dw_checkbox_new("Horizontally", 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        dw_checkbox_set(item, atoi(val));
        dw_window_set_data(vbox, "hexpand", (void *)item);
        hbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
        dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
        val = defvaltrue;
        if((this = _dwib_find_child(node, "vexpand")))
        {
            if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
                val = thisval;
        }
        item = dw_checkbox_new("Vertically", 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        dw_checkbox_set(item, atoi(val));
        dw_window_set_data(vbox, "vexpand", (void *)item);
        /* Padding */
        hbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
        item = dw_text_new("Padding", 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
        dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
        val = defvalzero;
        if((this = _dwib_find_child(node, "padding")))
        {
            if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
                val = thisval;
        }
        item = dw_spinbutton_new(val, 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
        dw_spinbutton_set_limits(item, 2000, 0);
        dw_window_set_data(vbox, "padding", (void *)item);
        /* Enabled */
        hbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
        item = dw_text_new("Enabled", 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
        dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
        item = dw_checkbox_new("", 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        val = defvaltrue;
        if((this = _dwib_find_child(node, "enabled")))
        {
            if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
                val = thisval;
        }
        dw_checkbox_set(item, atoi(val));
        dw_window_set_data(vbox, "enabled", (void *)item);
    }
    /* Foreground Color */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Foreground Color", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = "Default";
    if((this = _dwib_find_child(node, "fcolor")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    tmp = item = dw_combobox_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - PROPERTIES_HEIGHT, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Default");
    for(x=0;x<16;x++)
    {
        dw_listbox_append(item, Colors[x]);
    }
    if((x = _dwib_get_color(val)) != DW_CLR_DEFAULT && x < 16 && x > -1)
    {
        dw_listbox_select(item, x + 1, TRUE);
    }
    dw_window_set_data(vbox, "fcolor", (void *)item);    
    item = dw_bitmapbutton_new("Color chooser", ICON_COLOR);
    dw_box_pack_start(hbox, item, PROPERTIES_HEIGHT, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(color_clicked), tmp);
    /* Background Color */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Background Color", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = "Default";
    if((this = _dwib_find_child(node, "bcolor")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    tmp = item = dw_combobox_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - PROPERTIES_HEIGHT, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Default");
    for(x=0;x<16;x++)
    {
        dw_listbox_append(item, Colors[x]);
    }
    if((x = _dwib_get_color(val)) != DW_CLR_DEFAULT && x < 16 && x > -1)
    {
        dw_listbox_select(item, x + 1, TRUE);
    }
    dw_window_set_data(vbox, "bcolor", (void *)item);    
    item = dw_bitmapbutton_new("Color chooser", ICON_COLOR);
    dw_box_pack_start(hbox, item, PROPERTIES_HEIGHT, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(color_clicked), tmp);
    /* Font */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Font", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = "Default";
    if((this = _dwib_find_child(node, "font")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    tmp = item = dw_combobox_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - PROPERTIES_HEIGHT, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Default");
    dw_window_set_data(vbox, "font", (void *)item);    
    item = dw_bitmapbutton_new("Font chooser", ICON_FONT);
    dw_box_pack_start(hbox, item, PROPERTIES_HEIGHT, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(font_clicked), tmp);
    if(sysfont)
    {
        /* System Font */
        hbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
        item = dw_text_new(sysfonttext, 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
        dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
        if((this = _dwib_find_child(node, sysfont)))
        {
            if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
                val = thisval;
        }
        tmp = item = dw_combobox_new(val, 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - PROPERTIES_HEIGHT, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        dw_listbox_append(item, "Default");
        dw_window_set_data(vbox, sysfont, (void *)item);    
        item = dw_bitmapbutton_new("Font chooser", ICON_FONT);
        dw_box_pack_start(hbox, item, PROPERTIES_HEIGHT, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
        dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(font_clicked), (void *)tmp);
    }
}

/* Create a new text definition */
int DWSIGNAL text_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Text", (xmlChar *)subtype);
    }
    
    
    if(!thisNode)
        return FALSE;
    
    snprintf(buf, 200, "Text - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_TEXT], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a text */
void DWSIGNAL properties_text(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Text Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    /* Sub-type */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Sub-type", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("None", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "None");
    dw_listbox_append(item, "Status");
    val = defvalstr;
    if((this = _dwib_find_child(node, "subtype")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
        {
            val = thisval;
            if(strcmp(val, "Status") == 0)
                val = "1";
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "subtype", (void *)item);    
    
    properties_item(node, scrollbox, TRUE);
    
    /* Text */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Label", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "label")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "label", (void *)item);
    /* Alignment */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Alignment", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Left", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Left");
    dw_listbox_append(item, "Center");
    dw_listbox_append(item, "Right");
    val = defvalstr;
    if((this = _dwib_find_child(node, "alignment")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
        {
            val = thisval;
            if(strcmp(val, "Center") == 0)
                val = "1";
            else if(strcmp(val, "Right") == 0)
                val = "2";
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "alignment", (void *)item);    
    
    /* Vertical alignment */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Vertical alignment", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Top", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Top");
    dw_listbox_append(item, "Center");
    dw_listbox_append(item, "Bottom");
    val = defvalstr;
    if((this = _dwib_find_child(node, "valignment")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
        {
            val = thisval;
            if(strcmp(val, "Center") == 0)
                val = "1";
            else if(strcmp(val, "Bottom") == 0)
                val = "2";
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "valignment", (void *)item);  
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(text_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}
   
/* Create a new entryfield definition */
int DWSIGNAL entryfield_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Entryfield", (xmlChar *)subtype);
    }
    
    
    if(!thisNode)
        return FALSE;
    
    snprintf(buf, 200, "Entryfield - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_ENTRYFIELD], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a entryfield */
void DWSIGNAL properties_entryfield(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Entryfield Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    /* Sub-type */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Sub-type", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("None", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "None");
    dw_listbox_append(item, "Password");
    val = defvalstr;
    if((this = _dwib_find_child(node, "subtype")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
        {
            val = thisval;
            if(strcmp(val, "Password") == 0)
                val = "1";
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "subtype", (void *)item);    
    
    properties_item(node, scrollbox, TRUE);
    
    /* Default Text */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Default text", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "deftext")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "deftext", (void *)item);
    /* Limit */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Limit", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalint;
    if((this = _dwib_find_child(node, "limit")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "limit", (void *)item);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(entryfield_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new combobox definition */
int DWSIGNAL combobox_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Combobox", (xmlChar *)"");
    }
        
    if(!thisNode)
        return FALSE;
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(thisNode, NULL, (xmlChar *)"List", (xmlChar *)"");
    
    treeitem = dw_tree_insert(tree, "Combobox", hIcons[TYPE_COMBOBOX], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Add to the list */
int DWSIGNAL add_clicked(HWND window, void *data)
{
    HWND vbox = (HWND)data;
    HWND list = (HWND)dw_window_get_data(vbox, "list");
    HWND entry = (HWND)dw_window_get_data(vbox, "list_entry");
    
    if(vbox && entry && list)
    {
        char *text = dw_window_get_text(entry);
        
        if(text)
        {
            if(*text)
            {
                dw_listbox_append(list, text);
                dw_window_set_text(entry, "");
            }
            dw_free(text);
        }
    }
    return FALSE;
}

/* Remove from the list */
int DWSIGNAL rem_clicked(HWND window, void *data)
{
    HWND vbox = (HWND)data;
    HWND list = (HWND)dw_window_get_data(vbox, "list");
    
    if(vbox && list)
    {
        int selected = dw_listbox_selected(list);
        
        if(selected != DW_LIT_NONE)
        {
            dw_listbox_delete(list, selected);
        }
    }
    return FALSE;
}

/* Populate the properties window for a combobox */
void DWSIGNAL properties_combobox(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Combobox Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    properties_item(node, scrollbox, TRUE);
    
    /* Default Text */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Default text", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "deftext")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "deftext", (void *)item);
    /* List */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, TRUE, 0);
    item = dw_text_new("List", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_listbox_new(0, FALSE);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, 200, TRUE, TRUE, 0);
    dw_window_set_data(vbox, "list", (void *)item);
    if((this = _dwib_find_child(node, "List")))
    {
        _dwib_populate_list(item, this, DWDoc);
    }
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_entryfield_new("", 0);
    dw_window_set_data(vbox, "list_entry", (void *)item);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_button_new("+", 0);
    dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(add_clicked), (void *)vbox);
    item = dw_button_new("-", 0);
    dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(rem_clicked), (void *)vbox);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(combobox_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new listbox definition */
int DWSIGNAL listbox_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Listbox", (xmlChar *)"");
    }
    
    if(!thisNode)
        return FALSE;
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(thisNode, NULL, (xmlChar *)"List", (xmlChar *)"");
    
    treeitem = dw_tree_insert(tree, "Listbox", hIcons[TYPE_LISTBOX], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a listbox */
void DWSIGNAL properties_listbox(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *val = defvalzero, *thisval;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Listbox Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    properties_item(node, scrollbox, TRUE);
    
    /* Multiple select */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Selection", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    if((this = _dwib_find_child(node, "multi")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_checkbox_new("Multiple", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "multi", (void *)item);
    
    /* List */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, TRUE, 0);
    item = dw_text_new("List", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_listbox_new(0, FALSE);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, 200, TRUE, TRUE, 0);
    if((this = _dwib_find_child(node, "List")))
    {
        _dwib_populate_list(item, this, DWDoc);
    }
    dw_window_set_data(vbox, "list", (void *)item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_entryfield_new("", 0);
    dw_window_set_data(vbox, "list_entry", (void *)item);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_button_new("+", 0);
    dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(add_clicked), (void *)vbox);
    item = dw_button_new("-", 0);
    dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(rem_clicked), (void *)vbox);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(listbox_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

char *ColTypes[] =
{
    "String",
    "Icon",
    "Number",
    "Date",
    "Time",
    NULL
};

/* When the + is clicked... 
 * add a new row and disable the previous rows +
 */
int DWSIGNAL add_row_clicked(HWND window, void *data)
{
    HWND button, scrollbox = (HWND)data;
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    int count = DW_POINTER_TO_INT(dw_window_get_data(vbox, "colcount"));
    char buf[50];
    
    snprintf(buf, 50, "addbutton%d", count);
    button = (HWND)dw_window_get_data(vbox, buf);
    dw_window_disable(button);
    
    count++;
    add_row(vbox, scrollbox, count, "", "", "", FALSE);
    dw_window_set_data(vbox, "colcount", DW_INT_TO_POINTER(count));
    
    dw_window_redraw(hwndProperties);
    return FALSE;
}

/* Add a single row */
void add_row(HWND vbox, HWND scrollbox, int count, char *colname, char *coltype, char *colalign, int disable)
{
    HWND item, hbox = dw_box_new(DW_HORZ, 0);
    char buf[50];
    int x = 0, which = 0;
    
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_entryfield_new(colname, 0);
    snprintf(buf, 50, "entryfield%d", count);
    dw_window_set_data(vbox, buf, (void *)item);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - BUTTON_ICON_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_combobox_new(coltype, 0);
    snprintf(buf, 50, "coltype%d", count);
    dw_window_set_data(vbox, buf, (void *)item);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH / 2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    while(ColTypes[x])
    {
        dw_listbox_append(item, ColTypes[x]);
        if(strcmp(ColTypes[x], coltype) == 0)
            which = x;
        x++;
    }
    dw_listbox_select(item, which, TRUE);
    item = dw_combobox_new(coltype, 0);
    snprintf(buf, 50, "colalign%d", count);
    dw_window_set_data(vbox, buf, (void *)item);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH / 2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Left");
    dw_listbox_append(item, "Center");
    dw_listbox_append(item, "Right");
    which = 0;
    if(strcmp(colalign, "Center") == 0)
        which = 1;
    else if(strcmp(colalign, "Right") == 0)
        which = 2;
    dw_listbox_select(item, which, TRUE);
    item = dw_button_new("+", 0);
    snprintf(buf, 50, "addbutton%d", count);
    dw_window_set_data(vbox, buf, (void *)item);
    dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(add_row_clicked), (void *)scrollbox);
    if(disable)
        dw_window_disable(item);
}

/* Add rows for columns from the XML tree */
void populate_columns(HWND vbox, HWND scrollbox, xmlNodePtr node)
{
    xmlNodePtr p;
    int count = 0;
    
    if(node)
    {
        for(p=node->children;p;p = p->next)
        {
            if(strcmp((char *)p->name, "Item") == 0)
            {
                char *thisval;
                
                if((thisval = (char *)xmlNodeListGetString(DWDoc, p->children, 1)))
                {
                    char *coltype = (char *)xmlGetProp(p, (xmlChar *)"ColType");
                    char *colalign = (char *)xmlGetProp(p, (xmlChar *)"ColAlign");
                    add_row(vbox, scrollbox, count, thisval, coltype ? coltype : "", colalign ? colalign : "", TRUE);
                    count++;
                }
            }
        }
    }
    add_row(vbox, scrollbox, count, "", "", "", FALSE);
    dw_window_set_data(vbox, "colcount", DW_INT_TO_POINTER(count));
}

/* Create a new container definition */
int DWSIGNAL container_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Container", (xmlChar *)subtype);
    }
    
    if(!thisNode)
        return FALSE;
    
    snprintf(buf, 200, "Container - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_CONTAINER], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a container */
void DWSIGNAL properties_container(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Container Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    /* Sub-type */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Sub-type", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("None", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "None");
    dw_listbox_append(item, "Filesystem");
    val = defvalzero;
    if((this = _dwib_find_child(node, "subtype")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
        {
            val = thisval;
            if(strcmp(val, "Filesystem") == 0)
                val = "1";
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "subtype", (void *)item);    
    
    properties_item(node, scrollbox, TRUE);
    
    /* Multiple select */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Selection", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvaltrue;
    if((this = _dwib_find_child(node, "multi")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_checkbox_new("Multiple", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "multi", (void *)item);
    item = dw_text_new("Column names, types and alignment", 0);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    this = _dwib_find_child(node, "Columns");
    populate_columns(vbox, scrollbox, this);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(container_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new tree definition */
int DWSIGNAL tree_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Tree", (xmlChar *)"");
    }
    
    if(!thisNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "Tree", hIcons[TYPE_TREE], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a tree */
void DWSIGNAL properties_tree(xmlNodePtr node)
{
    HWND item, scrollbox, vbox = dw_window_get_data(hwndProperties, "box");
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Tree Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    properties_item(node, scrollbox, TRUE);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(tree_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new MLE definition */
int DWSIGNAL mle_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"MLE", (xmlChar *)"");
    }
    
    if(!thisNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "MLE", hIcons[TYPE_MLE], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a MLE */
void DWSIGNAL properties_mle(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("MLE Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    properties_item(node, scrollbox, TRUE);
    
    /* Default Text */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Default text", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "deftext")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "deftext", (void *)item);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(mle_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new render definition */
int DWSIGNAL render_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Render", (xmlChar *)"");
    }
    
    if(!thisNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "Render", hIcons[TYPE_RENDER], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a render */
void DWSIGNAL properties_render(xmlNodePtr node)
{
    HWND item, scrollbox, vbox = dw_window_get_data(hwndProperties, "box");
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Render Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    properties_item(node, scrollbox, TRUE);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(render_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new button definition */
int DWSIGNAL button_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Button", (xmlChar *)subtype);
    }
    
    if(!thisNode)
        return FALSE;
    
    snprintf(buf, 200, "Button - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_BUTTON], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a button */
void DWSIGNAL properties_button(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Button Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    /* Sub-type */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Sub-type", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("None", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "None");
    dw_listbox_append(item, "Bitmap");
    dw_listbox_append(item, "Check");
    dw_listbox_append(item, "Radio");
    val = defvalstr;
    if((this = _dwib_find_child(node, "subtype")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
        {
            val = thisval;
            if(strcmp(val, "Bitmap") == 0)
                val = "1";
            else if(strcmp(val, "Check") == 0)
                val = "2";
            else if(strcmp(val, "Radio") == 0)
                val = "3";
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "subtype", (void *)item);    
    
    properties_item(node, scrollbox, TRUE);
    
    /* State */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("State", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalzero;
    if((this = _dwib_find_child(node, "checked")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_checkbox_new("Checked", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "checked", (void *)item);
    /* Setting text */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Text/ID/Filename", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "setting")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "setting", (void *)item);
    /* Bitmap button help */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Bitmap Tooltip", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "bubblehelp")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "bubblehelp", (void *)item);
    /* Border */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Border", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalzero;
    if((this = _dwib_find_child(node, "borderless")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_checkbox_new("Borderless", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "borderless", (void *)item);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(button_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new ranged definition */
int DWSIGNAL ranged_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Ranged", (xmlChar *)subtype);
    }
    
    if(!thisNode)
        return FALSE;
    
    snprintf(buf, 200, "Ranged - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_RANGED], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a ranged */
void DWSIGNAL properties_ranged(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Ranged Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    /* Sub-type */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Sub-type", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Percent", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Percent");
    dw_listbox_append(item, "Slider");
    dw_listbox_append(item, "Scrollbar");
    dw_listbox_append(item, "Spinbutton");
    val = defvalstr;
    if((this = _dwib_find_child(node, "subtype")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
        {
            val = thisval;
            if(strcmp(val, "Slider") == 0)
                val = "1";
            else if(strcmp(val, "Scrollbar") == 0)
                val = "2";
            else if(strcmp(val, "Spinbutton") == 0)
                val = "3";
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "subtype", (void *)item);    
    
    properties_item(node, scrollbox, TRUE);
    
    /* Position */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Position", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalzero;
    if((this = _dwib_find_child(node, "position")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_spinbutton_set_limits(item, 65536, 0);
    dw_window_set_data(vbox, "position", (void *)item);
    /* Ranger - Upper */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Range", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = "100";
    if((this = _dwib_find_child(node, "upper")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_spinbutton_set_limits(item, 65536, -65536);
    dw_window_set_data(vbox, "upper", (void *)item);
    /* Ranger - Lower */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Range", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalzero;
    if((this = _dwib_find_child(node, "lower")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_spinbutton_set_limits(item, 65536, -65536);
    dw_window_set_data(vbox, "lower", (void *)item);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(ranged_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new bitmap definition */
int DWSIGNAL bitmap_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Bitmap", (xmlChar *)"");
    }
    
    if(!thisNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "Bitmap", hIcons[TYPE_BITMAP], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a bitmap */
void DWSIGNAL properties_bitmap(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Bitmap Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    properties_item(node, scrollbox, TRUE);
    
    /* Default Text */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Res ID/Filename", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "setting")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "setting", (void *)item);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(bitmap_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new HTML definition */
int DWSIGNAL html_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"HTML", (xmlChar *)"");
    }
    
    if(!thisNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "HTML", hIcons[TYPE_HTML], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a HTML */
void DWSIGNAL properties_html(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("HTML Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    properties_item(node, scrollbox, TRUE);
    
    /* URL */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("URL", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "URL")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "URL", (void *)item);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(html_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new notebook definition */
int DWSIGNAL notebook_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Notebook", (xmlChar *)"");
    }
    
    if(!thisNode)
        return FALSE;
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(thisNode, NULL, (xmlChar *)"Children", (xmlChar *)"");
    
    treeitem = dw_tree_insert(tree, "Notebook", hIcons[TYPE_NOTEBOOK], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a notebook */
void DWSIGNAL properties_notebook(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Notebook Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    properties_item(node, scrollbox, TRUE);
    
    /* Tab Position */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Tab Position", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Top", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Top");
    dw_listbox_append(item, "Bottom");
    val = defvalstr;
    if((this = _dwib_find_child(node, "position")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
        {
            val = thisval;
            if(strcmp(val, "Bottom") == 0)
                val = "1";
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "position", (void *)item);    
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(notebook_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new notebook page definition */
int DWSIGNAL notebook_page_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    char buf[200], *title = dw_window_get_text((HWND)dw_window_get_data(vbox, "title"));
    xmlNodePtr thisNode = NULL;
    
    if(strcmp((char *)DWCurrNode->name, "Notebook") == 0 && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"NotebookPage", (xmlChar *)"");
    }
    
    if(!thisNode)
        return FALSE;
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(thisNode, NULL, (xmlChar *)"Children", (xmlChar *)"");
    
    snprintf(buf, 200, "Page - (%s)", title ? title : "");
    
    if(title)
        dw_free(title);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_NOTEBOOK_PAGE], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a notebook page */
void DWSIGNAL properties_notebook_page(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Notebook Page Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    /* Title */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Page Title", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "title")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "title", (void *)item);
    
    properties_item(node, scrollbox, FALSE);
    
    /* Status Text */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Status Text (OS/2)", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "statustext")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "statustext", (void *)item);
    /* Orientation */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Orientation", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Horizontal", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Horizontal");
    dw_listbox_append(item, "Vertical");
    val = defvalstr;
    if((this = _dwib_find_child(node, "orientation")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
        {
            val = thisval;
            if(strcmp(val, "Vertical") == 0)
                val = "1";
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "orientation", (void *)item);    
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(notebook_page_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new calendar definition */
int DWSIGNAL calendar_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE) && parentNode)
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Calendar", (xmlChar *)"");
    }
    
    if(!thisNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "Calendar", hIcons[TYPE_CALENDAR], (HTREEITEM)DWCurrNode->_private, thisNode);
    thisNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)thisNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a calendar */
void DWSIGNAL properties_calendar(xmlNodePtr node)
{
    HWND item, scrollbox, vbox = dw_window_get_data(hwndProperties, "box");
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Calendar Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    properties_item(node, scrollbox, TRUE);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(calendar_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new box definition */
int DWSIGNAL box_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr boxNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE))
    {
        boxNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Box", (xmlChar *)subtype);
    }

    
    if(!boxNode)
        return FALSE;
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(boxNode, NULL, (xmlChar *)"Children", (xmlChar *)"");
    
    snprintf(buf, 200, "Box - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_BOX], (HTREEITEM)DWCurrNode->_private, boxNode);
    boxNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)boxNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a box */
void DWSIGNAL properties_box(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Box Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    /* Sub-type */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Sub-type", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("None", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "None");
    dw_listbox_append(item, "Group");
    dw_listbox_append(item, "Scroll");
    dw_listbox_append(item, "Splitbar");
    val = defvalstr;
    if((this = _dwib_find_child(node, "subtype")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
        {
            val = thisval;
            if(strcmp(val, "Group") == 0)
                val = "1";
            if(strcmp(val, "Scroll") == 0)
                val = "2";
            if(strcmp(val, "Splitbar") == 0)
                val = "3";
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "subtype", (void *)item);    
    
    properties_item(node, scrollbox, TRUE);
    
    /* Orientation */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Orientation", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Horizontal", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Horizontal");
    dw_listbox_append(item, "Vertical");
    val = defvalstr;
    if((this = _dwib_find_child(node, "orientation")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
        {
            val = thisval;
            if(strcmp(val, "Vertical") == 0)
                val = "1";
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "orientation", (void *)item);    
    /* Group title */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Group title", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "title")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "title", (void *)item);
    /* Split % */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Splitbar %", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = "50";
    if((this = _dwib_find_child(node, "splitper")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_spinbutton_set_limits(item, 100, 0);
    dw_window_set_data(vbox, "splitper", (void *)item);

    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(box_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}    

/* Create a new padding definition */
int DWSIGNAL padding_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    xmlNodePtr boxNode = NULL;
    
    if(is_packable(DWCurrNode, TRUE))
    {
        boxNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Padding", (xmlChar *)"");
    }
    
    if(!boxNode)
        return FALSE;
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(boxNode, NULL, (xmlChar *)"Children", (xmlChar *)"");
    
    treeitem = dw_tree_insert(tree, "Padding", hIcons[TYPE_PADDING], (HTREEITEM)DWCurrNode->_private, boxNode);
    boxNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)boxNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for padding */
void DWSIGNAL properties_padding(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Padding", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);

    /* Required size */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Size (width, height)", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvaltrue;
    if((this = _dwib_find_child(node, "width")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, 0);
    dw_window_set_data(vbox, "width", (void *)item);
    val = defvaltrue;
    if((this = _dwib_find_child(node, "height")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, 0);
    dw_window_set_data(vbox, "height", (void *)item);
    /* Expandable */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Expand", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvaltrue;
    if((this = _dwib_find_child(node, "hexpand")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_checkbox_new("Horizontally", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "hexpand", (void *)item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    val = defvaltrue;
    if((this = _dwib_find_child(node, "vexpand")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_checkbox_new("Vertically", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "vexpand", (void *)item);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(padding_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new menu definition */
int DWSIGNAL menu_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    xmlNodePtr boxNode = NULL;
    char buf[200], *title = dw_window_get_text((HWND)dw_window_get_data(vbox, "title"));
    
    /* Menus can only be added to the window... or other menus */
    if(strcmp((char *)DWCurrNode->name, "Window") == 0 ||
       strcmp((char *)DWCurrNode->name, "Menu") == 0)
    {
        boxNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Menu", (xmlChar *)"");
    }
    
    if(!boxNode)
        return FALSE;
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(boxNode, NULL, (xmlChar *)"Children", (xmlChar *)"");
    
    snprintf(buf, 200, "Menu - (%s)", title ? title : "");
    
    if(title)
        dw_free(title);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_MENU], (HTREEITEM)DWCurrNode->_private, boxNode);
    boxNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", (void *)boxNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Make sure the two checkboxes have valid settings */
int DWSIGNAL toggle_clicked(HWND window, void *data)
{
    HWND vbox = (HWND)data;
    HWND checkable = (HWND)dw_window_get_data(vbox, "checkable");
    HWND checked = (HWND)dw_window_get_data(vbox, "checked");
    int state = dw_checkbox_get(window);
    
    /* If the menu isnt' checkable... it can't default to checked */
    if(window == checkable)
    {
        if(!state)
            dw_checkbox_set(checked, state);
    }
    /* If the menu is checked... it has to be checkable */
    else if(window == checked)
    {
        if(state)
            dw_checkbox_set(checkable, state);
    }
    return FALSE;
}

/* Populate the properties menubar for a menu */
void DWSIGNAL properties_menu(xmlNodePtr node)
{
    HWND item, checkable, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Menu Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    /* Create the actual properties - Title */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Title", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    if((this = _dwib_find_child(node, "title")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "title", (void *)item);
    /* Data name*/
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Data name", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "dataname")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "dataname", (void *)item);
    /* Style */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Style", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "checkable")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    checkable = item = dw_checkbox_new("Checkable", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "checkable", (void *)item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    val = defvalstr;
    if((this = _dwib_find_child(node, "checked")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_checkbox_new("Checked", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "checked", (void *)item);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toggle_clicked), (void *)vbox);
    dw_signal_connect(checkable, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toggle_clicked), (void *)vbox);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    val = defvaltrue;
    if((this = _dwib_find_child(node, "enabled")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_checkbox_new("Enabled", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "enabled", (void *)item);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(menu_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new window definition */
int DWSIGNAL window_create(HWND window, void *data)
{
    xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
    xmlNodePtr windowNode = xmlNewTextChild(rootNode, NULL, (xmlChar *)"Window", (xmlChar *)"");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    char buf[200], *title = dw_window_get_text((HWND)dw_window_get_data(vbox, "title"));
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(windowNode, NULL, (xmlChar *)"Children", (xmlChar *)"");
    
    snprintf(buf, 200, "Window - (%s)", title ? title : "");
    
    if(title)
        dw_free(title);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_WINDOW], 0, windowNode);
    windowNode->_private = (void *)treeitem;
    
    dw_window_set_data(vbox, "node", (void *)windowNode);
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a window */
void DWSIGNAL properties_window(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", (void *)vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Top-level Window", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    /* Create the actual properties - Title */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Title", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    if((this = _dwib_find_child(node, "title")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "title", (void *)item);
    /* Size */ 
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Size (width, height)", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = "100";
    if((this = _dwib_find_child(node, "width")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "width", (void *)item);
    val = "100";
    if((this = _dwib_find_child(node, "height")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "height", (void *)item);
    /* Positon */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Position (x, y)", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalint;
    if((this = _dwib_find_child(node, "x")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "x", (void *)item);
    val = defvalint;
    if((this = _dwib_find_child(node, "y")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "y", (void *)item);
    /* Center */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Center", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_checkbox_new("", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvalzero;
    if((this = _dwib_find_child(node, "center")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "center", (void *)item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Border", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalint;
    if((this = _dwib_find_child(node, "bordersize")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "bordersize", (void *)item);
    /* Buttons */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Buttons", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_checkbox_new("Close", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvaltrue;
    if((this = _dwib_find_child(node, "close")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "close", (void *)item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    item = dw_checkbox_new("Minimize", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvaltrue;
    if((this = _dwib_find_child(node, "minimize")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "minimize", (void *)item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    item = dw_checkbox_new("Maximize", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvaltrue;
    if((this = _dwib_find_child(node, "maximize")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "maximize", (void *)item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    item = dw_checkbox_new("Hide", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvalstr;
    if((this = _dwib_find_child(node, "hide")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "hide", (void *)item);
    /* Style */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Style", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_checkbox_new("Titlebar", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvaltrue;
    if((this = _dwib_find_child(node, "titlebar")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "titlebar", (void *)item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    item = dw_checkbox_new("Resize", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvalstr;
    if((this = _dwib_find_child(node, "resize")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "resize", (void *)item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    item = dw_checkbox_new("Dialog", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvalstr;
    if((this = _dwib_find_child(node, "dialog")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "dialog", (void *)item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    item = dw_checkbox_new("Border", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvalstr;
    if((this = _dwib_find_child(node, "border")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "border", (void *)item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    item = dw_checkbox_new("System Menu", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvaltrue;
    if((this = _dwib_find_child(node, "sysmenu")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "sysmenu", (void *)item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    item = dw_checkbox_new("Task List", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvaltrue;
    if((this = _dwib_find_child(node, "tasklist")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "tasklist", (void *)item);
    /* Orientation */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Orientation", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Horizontal", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Horizontal");
    dw_listbox_append(item, "Vertical");
    val = defvalstr;
    if((this = _dwib_find_child(node, "orientation")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
        {
            val = thisval;
            if(strcmp(val, "Vertical") == 0)
                val = "1";
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "orientation", (void *)item);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(window_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Handle saving the current layout */
int DWSIGNAL save_clicked(HWND button, void *data)
{
    char *filename = dw_file_browse("Save interface", ".", "xml", DW_FILE_SAVE);
    
    if(filename)
    {
        /* Make sure the XML tree is up-to-date */
        save_properties();
        
        /* Enable indenting in the output */
        xmlIndentTreeOutput = 1;
        
        xmlSaveFormatFile(filename, DWDoc, 1);
        dw_free(filename);
    }
    return FALSE;
}

/* Count the number of children */
int count_children(xmlNodePtr node)
{
    xmlNodePtr p = _dwib_find_child(node, "Children");
    int count = 0;
    
    if(!p)
        return count;
    
    for(p=p->children;p;p = p->next)
    {
        if(is_valid(p))
            count++;
    }
    return count;
}

HTREEITEM _tree_insert(HWND handle, HTREEITEM item, char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
    if(item)
        return dw_tree_insert_after(handle, item, title, icon, parent, itemdata);
    return dw_tree_insert(handle, title, icon, parent, itemdata);
}


int generateNode(char *buf, xmlNodePtr p)
{
    xmlNodePtr this;
    char *val = NULL, buf2[100];
    int which = is_valid(p);
    
    /* Shorten "NotebookPage" to just "Page" */
    if(which == TYPE_NOTEBOOK_PAGE)
        strcpy(buf, "Page");
    else
        strcpy(buf, (char *)p->name);
    
    /* If there is a dataname include it in brackets [] */
    if((this = _dwib_find_child(p, "dataname")) &&
       (val = (char *)xmlNodeListGetString(DWDoc, this->children, 1)) && *val)
    {
        snprintf(buf2, 100, " [%s]", val ? val : "");
        strcat(buf, buf2);
    }
    
    /* If there is a title or label include it in quotes "" */
    this = _dwib_find_child(p, "title");
    if(!this)
        this = _dwib_find_child(p, "label");        
    
    if(this && (val = (char *)xmlNodeListGetString(DWDoc, this->children, 1)) && *val)
    {
        snprintf(buf2, 100, " \"%s\"", val ? val : "");
        strcat(buf, buf2);
    }
    
    switch(which)
    {
        case TYPE_BOX:
        case TYPE_BUTTON:
        case TYPE_TEXT:
        case TYPE_CONTAINER:
        case TYPE_RANGED:
        case TYPE_ENTRYFIELD:
        {
            this = _dwib_find_child(p, "subtype");
            val = NULL;
            
            if(this)
                val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
            
            snprintf(buf2, 100, " (%s)", val ? val : "None");
            strcat(buf, buf2);
        }
            break;
    }
    return which;
}

/* Parse the children if packable widgets... boxes, notebook pages, etc */
void handleChildren(xmlNodePtr node, HWND tree, xmlNodePtr thisnode, xmlNodePtr afternode)
{
    char buf[200], *val;
    HTREEITEM treeitem, after = (thisnode && afternode) ? afternode->_private : 0, parent = (HTREEITEM)node->_private;
    xmlNodePtr p;
    
    if(strcmp((char *)node->name, "Children") == 0)
    {
        p = node;
        parent = node->parent ? node->parent->_private : 0;
    }
    else
    {
        p = _dwib_find_child(node, "Children");
    }
    
    if(!p)
        return;
    
    for(p=p->children;p;p = p->next)
    {
        val = NULL;
        
        if(!thisnode || p == thisnode)
        {
            /* Create the title for the node */
            int index = generateNode(buf, p);
            
            if(index)
            {
                /* Delete the old node if we are recreating */
                if(p == thisnode && p->_private)
                    dw_tree_item_delete(tree, (HTREEITEM)p->_private);
                
                /* Create the new node */
                treeitem = _tree_insert(tree, after, buf, hIcons[index], parent, p);
                p->_private = (void *)treeitem;
                dw_tree_item_expand(tree, parent);
                
                /* For the special cases with children... */
                switch(index)
                {
                    case TYPE_BOX:
                    case TYPE_NOTEBOOK:
                    case TYPE_NOTEBOOK_PAGE:
                    case TYPE_MENU:
                    {
                        handleChildren(p, tree, NULL, NULL);
                    }
                        break;
                }
            }
        }
    }
}

/* Clears and reloads the tree data from XML */
void reloadTree(void)
{
    xmlNodePtr p, rootNode = xmlDocGetRootElement(DWDoc);
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    HTREEITEM treeitem;
    
    dw_tree_clear(tree);
    
    if(!rootNode)
        return;
    
    for(p=rootNode->children;p;p = p->next)
    {
        if(strcmp((char *)p->name, "Window") == 0)
        {
            char buf[200];
            
            generateNode(buf, p);
            treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_WINDOW], 0, p);
            p->_private = (void *)treeitem;
            
            handleChildren(p, tree, NULL, NULL);
        }
    }
}

/* Handle starting a new layout */
int DWSIGNAL new_clicked(HWND button, void *data)
{
    if(dw_messagebox(DWIB_NAME, DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to lose the current layout?"))
    {
        HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
        HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");

        /* Remove the current tree */
        dw_tree_clear(tree);
        
        /* Remove the properties */
        dw_window_destroy(vbox);
        properties_none(TRUE);
        
        /* Free the existing doc */
        xmlFreeDoc(DWDoc);
        
        /* Create a new empty XML document */
        DWDoc = xmlNewDoc((xmlChar *)"1.0");
        DWCurrNode = xmlNewNode(NULL, (xmlChar *)"DynamicWindows");
        xmlDocSetRootElement(DWDoc, DWCurrNode);
    }
    return FALSE;
}


/* Handle loading a new layout */
int DWSIGNAL open_clicked(HWND button, void *data)
{
    if(dw_messagebox(DWIB_NAME, DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to lose the current layout?"))
    {
        char *filename = dw_file_browse("Open interface", ".", "xml", DW_FILE_OPEN);
        
        if(filename)
        {
            xmlDocPtr doc = xmlParseFile(filename);
            
            if(doc)
            {
                xmlNodePtr node = xmlDocGetRootElement(DWDoc);
                
                if(node)
                {
                    DWDoc = doc; 
                    DWCurrNode = node;
                    reloadTree();
                }
            }
            dw_free(filename);
        }
    }
    return FALSE;
}

/* Reset the preview window handle when deleted */
int DWSIGNAL preview_delete(HWND window, void *data)
{
    dw_window_destroy(hwndPreview);
    hwndPreview = 0;
    return FALSE;
}

/* Handle loading a new layout */
int DWSIGNAL refresh_clicked(HWND button, void *data)
{
    xmlNodePtr node = data;
    
    while(node)
    {
        if(strcmp((char *)node->name, "Window") == 0)
        {
            xmlNodePtr this = _dwib_find_child(node, "title");
            
            if(this)
            {
                char *val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
                
                if(val)
                {
                    /* Make sure the XML tree is up-to-date */
                    save_properties();
                    
                    if(hwndPreview)
                        dw_window_destroy(hwndPreview);
                    hwndPreview = dwib_load((DWIB)DWDoc, val);
                    
                    dw_signal_connect(hwndPreview, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(preview_delete), NULL);
                    
                    if(hwndPreview)
                        dwib_show(hwndPreview);
                    else
                        dw_messagebox(DWIB_NAME, DW_MB_OK | DW_MB_ERROR, "Failed to load window definition.");
                }
                else
                    dw_messagebox(DWIB_NAME, DW_MB_OK | DW_MB_ERROR, "Could not find a window title to load.");
            }
            return FALSE;
        }
        node=node->parent;
    }
    return FALSE;
}

/* Handle deleting a node layout */
int DWSIGNAL delete_clicked(HWND button, void *data)
{
    xmlNodePtr node = data;
    
    if(!node)
        return FALSE;
    
    if(strcmp((char *)node->name, "DynamicWindows") == 0)
    {
        dw_messagebox(DWIB_NAME, DW_MB_OK, "No node selected.");
        return FALSE;
    }
    if(dw_messagebox(DWIB_NAME, DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to remove the current node (%s)?", 
                     node && node->name ? (char *)node->name : ""))
    {
        HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
        HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
        
        /* Remove the properties */
        dw_window_destroy(vbox);
        properties_none(TRUE);
        
        DWCurrNode = xmlDocGetRootElement(DWDoc);
        xmlUnlinkNode(node);
        dw_tree_item_delete(tree, (HTREEITEM)node->_private);
        xmlFreeNode(node);
    }
    return FALSE;
}

/* Handle cutting a node layout */
int DWSIGNAL cut_clicked(HWND button, void *data)
{
    xmlNodePtr node = data;
    
    if(!node)
        return FALSE;
    
    if(strcmp((char *)node->name, "DynamicWindows") == 0)
    {
        dw_messagebox(DWIB_NAME, DW_MB_OK, "No node selected.");
        return FALSE;
    }
    if(dw_messagebox(DWIB_NAME, DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to cut the current node (%s) to the clipboard?", 
                     node && node->name ? (char *)node->name : ""))
    {
        HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
        HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
        
        /* Remove the properties */
        dw_window_destroy(vbox);
        properties_none(TRUE);
        
        DWCurrNode = xmlDocGetRootElement(DWDoc);
        xmlUnlinkNode(node);
        dw_tree_item_delete(tree, (HTREEITEM)node->_private);
        if(DWClipNode)
            xmlFreeNode(DWClipNode);
        DWClipNode = node;
    }
    return FALSE;
}

/* Handle copying a node layout */
int DWSIGNAL copy_clicked(HWND button, void *data)
{
    xmlNodePtr node = data;
    
    if(!node)
        return FALSE;
    
    if(strcmp((char *)node->name, "DynamicWindows") == 0)
    {
        dw_messagebox(DWIB_NAME, DW_MB_OK, "No node selected.");
        return FALSE;
    }
    if(DWClipNode)
        xmlFreeNode(DWClipNode);
    DWClipNode = xmlCopyNode(node, 1);
    return FALSE;
}

/* Finds the previous valid sibling node */
xmlNodePtr getPrevNode(xmlNodePtr node)
{
    xmlNodePtr p = node->parent;
    xmlNodePtr last = NULL;
    
    for(p=p->children;p;p = p->next)
    {
        if(p == node)
            return last;
        else if(is_valid(p))
            last = p;
    }
    return NULL;
}

/* Finds the next valid sibling node */
xmlNodePtr getNextNode(xmlNodePtr node)
{
    xmlNodePtr p = node->parent;
    int found = FALSE;
    
    for(p=p->children;p;p = p->next)
    {
        if(p == node)
            found = TRUE;
        else if(found && is_valid(p))
            return p;
    }
    return NULL;
}

/* Handle paste a node layout */
int DWSIGNAL paste_clicked(HWND button, void *data)
{
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    xmlNodePtr node = data;
    
    if(!node || !DWClipNode)
        return FALSE;
    
     /* Check for special case of splitbar */
    if(strcmp((char *)node->name, "Box") == 0)
    {
        xmlNodePtr this;
        
        if((this = _dwib_find_child(node, "subtype")))
        {
            char *thisval;
            
            if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            {
                if(strcmp(thisval, "Splitbar") == 0)
                {
                    int count = count_children(node);
                    
                    if(count > 1)
                    {
                        dw_messagebox(DWIB_NAME, DW_MB_OK, "Splitbars can't have more than two children packed.");
                        return FALSE;
                    }
                }
            }
        }
    }
    
    if(strcmp((char *)node->name, "DynamicWindows") == 0)
    {
        dw_messagebox(DWIB_NAME, DW_MB_OK, "No node selected.");
        return FALSE;
    }
    else if(strcmp((char *)node->name, "Window") != 0 && strcmp((char *)DWClipNode->name, "Menu") == 0)
    {
        dw_messagebox(DWIB_NAME, DW_MB_OK, "The menu on the clipboard needs to be pasted into a top-level window.");
        return FALSE;
    }
    else if(strcmp((char *)node->name, "Notebook") != 0 && strcmp((char *)DWClipNode->name, "NotebookPage") == 0)
    {
        dw_messagebox(DWIB_NAME, DW_MB_OK, "The notebook page on the clipboard needs to be pasted into a notebook widget.");
        return FALSE;
    }
    else if(is_packable(node, FALSE))
    {
        xmlNodePtr this = _dwib_find_child(node, "Children");
        
        if(this)
        {
            xmlNodePtr copy = xmlCopyNode(DWClipNode, 1);
        
            xmlAddChild(this, copy);
        
            handleChildren(node, tree, copy, getPrevNode(copy));

            dw_tree_item_select(tree, (HTREEITEM)node->_private);
        }
    }
    return FALSE;
}

/* One of the buttons on the toolbar was clicked */
int DWSIGNAL toolbar_clicked(HWND button, void *data)
{
    int which = DW_POINTER_TO_INT(data);
    
    if(!data)
    {
        return FALSE;
    }
    
    /* Save existing data... if any... here */
    save_properties();
    
    /* Check for special case of splitbar */
    if(strcmp((char *)DWCurrNode->name, "Box") == 0)
    {
        xmlNodePtr this;
        
        if((this = _dwib_find_child(DWCurrNode, "subtype")))
        {
            char *thisval;
            
            if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            {
                if(strcmp(thisval, "Splitbar") == 0)
                {
                    int count = count_children(DWCurrNode);
                    
                    if(count > 1)
                    {
                        dw_messagebox(DWIB_NAME, DW_MB_ERROR | DW_MB_OK, "Splitbars can't have more than two children packed.");
                        return FALSE;
                    }
                }
            }
        }
    }
    
    if(which == TYPE_WINDOW)
    {
        properties_window(NULL);
    }
    else if(which == TYPE_NOTEBOOK_PAGE)
    {
        if(strcmp((char *)DWCurrNode->name, "Notebook") == 0)
        {
            properties_notebook_page(NULL);
        }
        else
        {
            dw_messagebox(DWIB_NAME, DW_MB_OK | DW_MB_ERROR, "Selected widget needs to be a notebook.");
        }
    }
    else if(which == TYPE_MENU)
    {
        if(strcmp((char *)DWCurrNode->name, "Window") == 0 ||
           strcmp((char *)DWCurrNode->name, "Menu") == 0)
        {
            properties_menu(NULL);
        }
        else
        {
            dw_messagebox(DWIB_NAME, DW_MB_OK | DW_MB_ERROR, "Selected widget needs to be a window or menu.");
        }
    }
    else if(is_packable(DWCurrNode, TRUE))
    {
        switch(which)
        {
            case TYPE_BOX:
                properties_box(NULL);
                break;
            case TYPE_TEXT:
                properties_text(NULL);
                break;
            case TYPE_ENTRYFIELD:
                properties_entryfield(NULL);
                break;
            case TYPE_COMBOBOX:
                properties_combobox(NULL);
                break;
            case TYPE_LISTBOX:
                properties_listbox(NULL);
                break;
            case TYPE_CONTAINER:
                properties_container(NULL);
                break;
            case TYPE_TREE:
                properties_tree(NULL);
                break;
            case TYPE_MLE:
                properties_mle(NULL);
                break;
            case TYPE_RENDER:
                properties_render(NULL);
                break;
            case TYPE_BUTTON:
                properties_button(NULL);
                break;
            case TYPE_RANGED:
                properties_ranged(NULL);
                break;
            case TYPE_BITMAP:
                properties_bitmap(NULL);
                break;
            case TYPE_NOTEBOOK:
                properties_notebook(NULL);
                break;
            case TYPE_HTML:
                properties_html(NULL);
                break;
            case TYPE_CALENDAR:
                properties_calendar(NULL);
                break;
            case TYPE_PADDING:
                properties_padding(NULL);
                break;
            default:
                return FALSE;
        }
    }
    dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(which));
    return FALSE;
}

/* Show the appropriate properties for the currently selected node */
void properties_current(void)
{
    if(DWCurrNode && DWCurrNode->name)
    {
        HWND vbox;
        
        if(strcmp((char *)DWCurrNode->name, "Window") == 0)
        {
            properties_window(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_WINDOW);
        }
        else if(strcmp((char *)DWCurrNode->name, "Box") == 0)
        {
            properties_box(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_BOX);
        }
        else if(strcmp((char *)DWCurrNode->name, "Text") == 0)
        {
            properties_text(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_TEXT);
        }
        else if(strcmp((char *)DWCurrNode->name, "Entryfield") == 0)
        {
            properties_entryfield(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_ENTRYFIELD);
        }
        else if(strcmp((char *)DWCurrNode->name, "Combobox") == 0)
        {
            properties_combobox(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_COMBOBOX);
        }
        else if(strcmp((char *)DWCurrNode->name, "Listbox") == 0)
        {
            properties_listbox(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_LISTBOX);
        }
        else if(strcmp((char *)DWCurrNode->name, "Container") == 0)
        {
            properties_container(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_CONTAINER);
        }
        else if(strcmp((char *)DWCurrNode->name, "Tree") == 0)
        {
            properties_tree(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_TREE);
        }
        else if(strcmp((char *)DWCurrNode->name, "MLE") == 0)
        {
            properties_mle(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_MLE);
        }
        else if(strcmp((char *)DWCurrNode->name, "Render") == 0)
        {
            properties_render(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_RENDER);
        }
        else if(strcmp((char *)DWCurrNode->name, "Button") == 0)
        {
            properties_button(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_BUTTON);
        }
        else if(strcmp((char *)DWCurrNode->name, "Ranged") == 0)
        {
            properties_ranged(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_RANGED);
        }
        else if(strcmp((char *)DWCurrNode->name, "Bitmap") == 0)
        {
            properties_bitmap(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_BITMAP);
        }
        else if(strcmp((char *)DWCurrNode->name, "Notebook") == 0)
        {
            properties_notebook(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_NOTEBOOK);
        }
        else if(strcmp((char *)DWCurrNode->name, "NotebookPage") == 0)
        {
            properties_notebook_page(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_NOTEBOOK_PAGE);
        }
        else if(strcmp((char *)DWCurrNode->name, "HTML") == 0)
        {
            properties_html(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_HTML);
        }
        else if(strcmp((char *)DWCurrNode->name, "Calendar") == 0)
        {
            properties_calendar(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_CALENDAR);
        }
        else if(strcmp((char *)DWCurrNode->name, "Padding") == 0)
        {
            properties_padding(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_PADDING);
        }
        else if(strcmp((char *)DWCurrNode->name, "Menu") == 0)
        {
            properties_menu(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", (void *)TYPE_MENU);
        }
        else 
            return;
        
         if((vbox = (HWND)dw_window_get_data(hwndProperties, "box")))
            dw_window_set_data(vbox, "node", (void *)DWCurrNode);
    }
}

/* Handle loading a new item when selectng the tree */
int DWSIGNAL tree_select(HWND window, HTREEITEM item, char *text, void *data, void *itemdata)
{
    /* Save existing data... if any... here */
    save_properties();
    
    DWCurrNode = itemdata;
    
    properties_current();
    
    return FALSE;
}

/* Handles raising the properties inspector when the toolbar gets focus */
int DWSIGNAL toolbar_focus(HWND toolbar, void *data)
{
    dw_window_raise(hwndProperties);
    return FALSE;
}

/* Closing the toolbar window */
int DWSIGNAL toolbar_delete(HWND hwnd, void *data)
{
    if(dw_messagebox(DWIB_NAME, DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to exit Interface Builder?"))
    {
        dw_window_destroy(hwndProperties);
        dw_window_destroy(hwndToolbar);
        xmlFreeDoc(DWDoc);
        dw_exit(0);
	}
	return TRUE;
}

/* Handle moving a node up */
int DWSIGNAL up_clicked(HWND button, void *data)
{
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    xmlNodePtr node = data, prevNode;
    
    if(!node)
        return FALSE;
    
    if((prevNode = getPrevNode(data)))
    {
        xmlAddPrevSibling(prevNode, node);
        if(node->parent)
            handleChildren(node->parent, tree, prevNode, node);
        dw_tree_item_select(tree, (HTREEITEM)node->_private);
    }
    return FALSE;
}

/* Handle moving a node down */
int DWSIGNAL down_clicked(HWND button, void *data)
{
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree");
    xmlNodePtr node = data, nextNode;
    
    if(!node)
        return FALSE;
    
    if((nextNode = getNextNode(node)))
    {
        xmlAddNextSibling(nextNode, node);
        if(node->parent)
            handleChildren(node->parent, tree, node, nextNode);
        dw_tree_item_select(tree, (HTREEITEM)node->_private);
    }
    return FALSE;
}

/* Pop up a tree context menu */
int DWSIGNAL tree_context(HWND window, char *text, int x, int y, void *data, void *itemdata)
{
    HMENUI menu = dw_menu_new(0);
    int menuid = 1050;
    HWND item = dw_menu_append_item(menu, "~Up", menuid, 0, TRUE, FALSE, DW_NOMENU);
    menuid++;
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(up_clicked), itemdata);
    item = dw_menu_append_item(menu, "~Down", menuid, 0, TRUE, FALSE, DW_NOMENU);
    menuid++;
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(down_clicked), itemdata);
    item = dw_menu_append_item(menu, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
    item = dw_menu_append_item(menu, "C~opy", menuid, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(copy_clicked), itemdata);
    menuid++;
    item = dw_menu_append_item(menu, "C~ut", menuid, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(cut_clicked), itemdata);
    menuid++;
    item = dw_menu_append_item(menu, "D~elete", menuid, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(delete_clicked), itemdata);
    menuid++;
    if(DWClipNode)
    {
        item = dw_menu_append_item(menu, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
        item = dw_menu_append_item(menu, "~Paste", menuid, 0, TRUE, FALSE, DW_NOMENU);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(paste_clicked), itemdata);
    }
    menuid++;
    item = dw_menu_append_item(menu, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
    item = dw_menu_append_item(menu, "~Refresh", menuid, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(refresh_clicked), itemdata);
    
    dw_menu_popup(&menu, hwndToolbar, x, y);
    return FALSE;
}

#define TOOLBAR_WIDTH   100
#define TOOLBAR_HEIGHT  30

void dwib_init(void)
{
    HWND vbox, hbox, item;
    HMENUI menu, submenu;
    int x;
    
    /* Enable builder mode */
    _dwib_builder_toggle(TRUE);
    
    hIcons[0] = (HICN)0;
    for(x=1;x<20;x++)
    {
        hIcons[x] = dw_icon_load(0, x + 100);
    }
    
    /* Create a new empty XML document */
    DWDoc = xmlNewDoc((xmlChar *)"1.0");
    DWCurrNode = xmlNewNode(NULL, (xmlChar *)"DynamicWindows");
    xmlDocSetRootElement(DWDoc, DWCurrNode);
    
    hwndToolbar = dw_window_new(DW_DESKTOP, DWIB_NAME, 
                                DW_FCF_TITLEBAR | DW_FCF_MINMAX | DW_FCF_SYSMENU | DW_FCF_TASKLIST | DW_FCF_SIZEBORDER);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(hwndToolbar, hbox, 1, 1, TRUE, TRUE, 0);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hbox, vbox, 0, 0, FALSE, FALSE, 0);
    item = dw_button_new("Window", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_WINDOW);
    item = dw_button_new("Box", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_BOX);
    item = dw_button_new("Text", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_TEXT);
    item = dw_button_new("Entryfield", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_ENTRYFIELD);
    item = dw_button_new("Combobox", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_COMBOBOX);
    item = dw_button_new("Listbox", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_LISTBOX);
    item = dw_button_new("Container", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_CONTAINER);
    item = dw_button_new("Tree", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_TREE);
    item = dw_button_new("MLE", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_MLE);
    item = dw_button_new("Render", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_RENDER);
    item = dw_button_new("Button", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_BUTTON);
    item = dw_button_new("Ranged", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_RANGED);
    item = dw_button_new("Bitmap", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_BITMAP);
    item = dw_button_new("Notebook", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_NOTEBOOK);
    item = dw_button_new("NB Page", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_NOTEBOOK_PAGE);
    item = dw_button_new("HTML", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_HTML);
    item = dw_button_new("Calendar", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_CALENDAR);
    item = dw_button_new("Padding", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_PADDING);
    item = dw_button_new("Menu", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_MENU);
    dw_box_pack_start(vbox, 0, 1, 1, TRUE, TRUE, 0);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hbox, vbox, 0, 0, TRUE, TRUE, 0);
    item = dw_tree_new(0);
    dw_box_pack_start(vbox, item, 1, 1, TRUE, TRUE, 0);
    dw_signal_connect(item, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(tree_select), NULL);
    dw_signal_connect(item, DW_SIGNAL_ITEM_CONTEXT, DW_SIGNAL_FUNC(tree_context), NULL);
    dw_window_set_data(hwndToolbar, "tree", (void *)item);
    
    x=1000;
    menu = dw_menubar_new(hwndToolbar);
    submenu = dw_menu_new(0);
    item = dw_menu_append_item(submenu, "~New", x, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(new_clicked), NULL);
    item = dw_menu_append_item(submenu, DW_MENU_SEPARATOR, x, 0, TRUE, FALSE, DW_NOMENU);
    x++;
    item = dw_menu_append_item(submenu, "~Open", x, 0, TRUE, FALSE, DW_NOMENU);
    x++;
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(open_clicked), NULL);
    item = dw_menu_append_item(submenu, "~Save", x, 0, TRUE, FALSE, DW_NOMENU);
    x++;
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(save_clicked), NULL);
    item = dw_menu_append_item(submenu, DW_MENU_SEPARATOR, x, 0, TRUE, FALSE, DW_NOMENU);
    x++;
    item = dw_menu_append_item(submenu, "~Exit", x, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_delete), NULL);
    x++;
    item = dw_menu_append_item(menu, "~File", x, 0, TRUE, FALSE, submenu);
    
    dw_signal_connect(hwndToolbar, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(toolbar_delete), NULL);
    dw_window_set_icon(hwndToolbar, DW_RESOURCE(ICON_APP));
    dw_window_set_pos_size(hwndToolbar, 20, 20, 600, 650);
    
    hwndProperties = dw_window_new(DW_DESKTOP, "Properties Inspector", DW_FCF_TITLEBAR | DW_FCF_SIZEBORDER);
    properties_none(FALSE);
    dw_signal_connect(hwndToolbar, DW_SIGNAL_SET_FOCUS, DW_SIGNAL_FUNC(toolbar_focus), NULL);
    dw_window_set_pos_size(hwndProperties, 650, 20, 300, 550);
    dw_window_show(hwndProperties);
    dw_window_show(hwndToolbar);
}

/* The main entry point.  Notice we don't use WinMain() on Windows */
int main(int argc, char *argv[])
{
    dw_init(TRUE, argc, argv);
    
    dwib_init();
    
    dw_main();

    return 0;
}

