/* Dynamic Windows Application: Interface Builder
 * Created by Dynamic Windows Interface Builder
 * Author: Brian Smith
 */

#include <dw.h>
#include <libxml/tree.h>
#include <string.h>
#include "resources.h"
#include "dwib.h"

HWND hwndToolbar, hwndProperties;
xmlDocPtr DWDoc;
xmlNodePtr DWCurrNode = NULL;

/* Returns a child of node with the specified name.
 * Returns NULL on failure.
 */
xmlNodePtr findChildName(xmlNodePtr node, char *name)
{
    xmlNodePtr p = node ? node->children : NULL;
    
    for(;p; p = p->next)
    {
        if(p->name && strcmp(name, (char *)p->name) == 0)
        {
            return p;
        }
    }
    return NULL;
}

/* Returns TRUE if a packable class is selected */
int is_packable(int message)
{
    if(strcmp((char *)DWCurrNode->name, "Window") == 0 ||
       strcmp((char *)DWCurrNode->name, "Box") == 0 ||
       strcmp((char *)DWCurrNode->name, "Notebook Page") == 0)
    {
        return TRUE;
    }
    if(message)
    {
        dw_messagebox(DWIB_NAME, DW_MB_OK | DW_MB_ERROR, "Selected widget needs to be packable (window, box, splitbar, notebook page)");
    }
    return FALSE;
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
        xmlNodePtr this = findChildName(node, name);
        
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
    updateNode(node, vbox, "hexpand", FALSE);
    updateNode(node, vbox, "vexpand", FALSE);
    updateNode(node, vbox, "padding", FALSE);
    updateNode(node, vbox, "enabled", TRUE);
    updateNode(node, vbox, "fcolor", FALSE);
    updateNode(node, vbox, "bcolor", FALSE);
    updateNode(node, vbox, "font", FALSE);
}

/* Updates the XML tree with current settings */
void save_properties(void)
{
    int which = (int)dw_window_get_data(hwndProperties, "type");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
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
            updateNode(node, vbox, "bordersize", FALSE);
            updateNode(node, vbox, "close", TRUE);
            updateNode(node, vbox, "minimize", TRUE);
            updateNode(node, vbox, "maximize", TRUE);
            updateNode(node, vbox, "hide", TRUE);
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
            break;
        case TYPE_LISTBOX:
            save_item(node, vbox);
            break;
        case TYPE_CONTAINER:
            save_item(node, vbox);
            break;
        case TYPE_TREE:
            save_item(node, vbox);
            break;
        case TYPE_MLE:
            save_item(node, vbox);
            break;
        case TYPE_RENDER:
            save_item(node, vbox);
            break;
        case TYPE_BUTTON:
            save_item(node, vbox);
            break;
        case TYPE_RANGED:
            save_item(node, vbox);
            break;
        case TYPE_SPINBUTTON:
            save_item(node, vbox);
            break;
        case TYPE_BITMAP:
            save_item(node, vbox);
            break;
        case TYPE_NOTEBOOK:
            save_item(node, vbox);
            break;
        case TYPE_NOTEBOOK_PAGE:
            break;
        case TYPE_HTML:
            save_item(node, vbox);
            break;
        case TYPE_CALENDAR:
            save_item(node, vbox);
            break;
    }
}

#define PROPERTIES_HEIGHT 22
#define PROPERTIES_WIDTH 120

char *defvalstr = "", *defvalint = "-1", *defvaltrue = "1", *defvalzero = "0";

char *Colors[] =
{
    "Black",
    "Dark Red",
    "Dark Green",
    "Brown",
    "Dark Blue",
    "Dark Pink",
    "Dark Cyan",
    "Pale Gray",
    "Dark Gray",
    "Red",
    "Green",
    "Yellow",
    "Blue",
    "Pink",
    "Cyan",
    "White"
};

/* Populate the properties window with generic item fields */
void properties_item(xmlNodePtr node, HWND scrollbox, int box)
{
    HWND item, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *val = defvalstr;
    xmlNodePtr this;
    int x;
    
    /* Data name*/
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Data name", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    if((this = findChildName(node, "dataname")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_entryfield_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "dataname", item);
    if(!box)
    {
        /* Required size */
        hbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
        item = dw_text_new("Size (width, height)", 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
        val = defvaltrue;
        if((this = findChildName(node, "width")))
        {
            val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
        }
        item = dw_spinbutton_new(val, 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        dw_spinbutton_set_limits(item, 2000, 0);
        dw_window_set_data(vbox, "width", item);
        val = defvaltrue;
        if((this = findChildName(node, "height")))
        {
            val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
        }
        item = dw_spinbutton_new(val, 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        dw_spinbutton_set_limits(item, 2000, 0);
        dw_window_set_data(vbox, "height", item);
    }
    /* Expandable */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Expand", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvaltrue;
    if((this = findChildName(node, "hexpand")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_checkbox_new("Horizontally", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "hexpand", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvaltrue;
    if((this = findChildName(node, "vexpand")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_checkbox_new("Vertically", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "vexpand", item);
    /* Padding */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Padding", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalzero;
    if((this = findChildName(node, "padding")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, 0);
    dw_window_set_data(vbox, "padding", item);
    /* Enabled */
    if(!box)
    {
        hbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
        item = dw_text_new("Enabled", 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
        item = dw_checkbox_new("", 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        val = defvaltrue;
        if((this = findChildName(node, "enabled")))
        {
            val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
        }
        dw_checkbox_set(item, atoi(val));
        dw_window_set_data(vbox, "enabled", item);
    }
    /* Foreground Color */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Foreground Color", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Default", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Default");
    for(x=0;x<16;x++)
    {
        dw_listbox_append(item, Colors[x]);
    }
    val = defvalstr;
    if((this = findChildName(node, "fcolor")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "fcolor", item);    
    /* Background Color */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Background Color", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Default", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Default");
    for(x=0;x<16;x++)
    {
        dw_listbox_append(item, Colors[x]);
    }
    val = defvalstr;
    if((this = findChildName(node, "bcolor")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "bcolor", item);    
    /* Font */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Font", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Default", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Default");
    dw_window_set_data(vbox, "font", item);    
}

/* Create a new text definition */
int DWSIGNAL text_create(HWND window, void *data)
{
    xmlNodePtr parentNode = findChildName(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree"), treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr textNode = NULL;
    
    if(is_packable(TRUE) && parentNode)
    {
        textNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Text", (xmlChar *)subtype);
    }
    
    
    if(!textNode)
        return FALSE;
    
    snprintf(buf, 200, "Text - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, 0, (HTREEITEM)DWCurrNode->_private, textNode);
    textNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", textNode);
    
    save_properties();
    
    properties_window(DWCurrNode);
    
    return FALSE;
}

/* Populate the properties window for a text */
void DWSIGNAL properties_text(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", vbox);
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
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("None", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "None");
    dw_listbox_append(item, "Status");
    val = defvalstr;
    if((this = findChildName(node, "subtype")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "subtype", item);    
    
    properties_item(node, scrollbox, TRUE);
    
    /* Text */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Label", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = findChildName(node, "label")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "label", item);
    /* Alignment */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Alignment", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Left", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Left");
    dw_listbox_append(item, "Center");
    dw_listbox_append(item, "Right");
    val = defvalstr;
    if((this = findChildName(node, "alignment")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "alignment", item);    
    
    /* Vertical alignment */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Vertical alignment", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Top", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Top");
    dw_listbox_append(item, "Center");
    dw_listbox_append(item, "Bottom");
    val = defvalstr;
    if((this = findChildName(node, "valignment")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "valignment", item);  
    
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
    xmlNodePtr parentNode = findChildName(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree"), treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr textNode = NULL;
    
    if(is_packable(TRUE) && parentNode)
    {
        textNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Entryfield", (xmlChar *)subtype);
    }
    
    
    if(!textNode)
        return FALSE;
    
    snprintf(buf, 200, "Entryfield - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, 0, (HTREEITEM)DWCurrNode->_private, textNode);
    textNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", textNode);
    
    save_properties();
    
    properties_text(DWCurrNode);
    
    return FALSE;
}

/* Populate the properties window for a entryfield */
void DWSIGNAL properties_entryfield(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", vbox);
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
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("None", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "None");
    dw_listbox_append(item, "Password");
    val = defvalstr;
    if((this = findChildName(node, "subtype")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "subtype", item);    
    
    properties_item(node, scrollbox, TRUE);
    
    /* Default Text */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Default text", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = findChildName(node, "deftext")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "deftext", item);
    /* Limit */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Limit", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalint;
    if((this = findChildName(node, "limit")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "limit", item);
    
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
    xmlNodePtr parentNode = findChildName(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree"), treeitem;
    xmlNodePtr textNode = NULL;
    
    if(is_packable(TRUE) && parentNode)
    {
        textNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Combobox", (xmlChar *)"");
    }
        
    if(!textNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "Combobox", 0, (HTREEITEM)DWCurrNode->_private, textNode);
    textNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", textNode);
    
    save_properties();
    
    properties_text(DWCurrNode);
    
    return FALSE;
}

/* Populate the properties window for a combobox */
void DWSIGNAL properties_combobox(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", vbox);
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
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = findChildName(node, "deftext")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "deftext", item);
    /* List */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("List", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_listbox_new(0, FALSE);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, 200, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "list", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_button_new("+", 0);
    dw_box_pack_start(hbox, item, 40, 30, FALSE, FALSE, 0);
    item = dw_button_new("-", 0);
    dw_box_pack_start(hbox, item, 40, 30, FALSE, FALSE, 0);
    
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
    xmlNodePtr parentNode = findChildName(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree"), treeitem;
    xmlNodePtr textNode = NULL;
    
    if(is_packable(TRUE) && parentNode)
    {
        textNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Listbox", (xmlChar *)"");
    }
    
    if(!textNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "Listbox", 0, (HTREEITEM)DWCurrNode->_private, textNode);
    textNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", textNode);
    
    save_properties();
    
    properties_text(DWCurrNode);
    
    return FALSE;
}

/* Populate the properties window for a listbox */
void DWSIGNAL properties_listbox(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    item = dw_text_new("Listbox Widget", 0);
    dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    
    properties_item(node, scrollbox, TRUE);
    
    /* List */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("List", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_listbox_new(0, FALSE);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, 200, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "list", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_button_new("+", 0);
    dw_box_pack_start(hbox, item, 40, 30, FALSE, FALSE, 0);
    item = dw_button_new("-", 0);
    dw_box_pack_start(hbox, item, 40, 30, FALSE, FALSE, 0);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(listbox_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}

/* Create a new container definition */
int DWSIGNAL container_create(HWND window, void *data)
{
    xmlNodePtr parentNode = findChildName(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree"), treeitem;
    xmlNodePtr textNode = NULL;
    
    if(is_packable(TRUE) && parentNode)
    {
        textNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Container", (xmlChar *)"");
    }
    
    if(!textNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "Tree", 0, (HTREEITEM)DWCurrNode->_private, textNode);
    textNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", textNode);
    
    save_properties();
    
    properties_text(DWCurrNode);
    
    return FALSE;
}

/* Populate the properties window for a container */
void DWSIGNAL properties_container(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", vbox);
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
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("None", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "None");
    dw_listbox_append(item, "Filesystem");
    val = defvalstr;
    if((this = findChildName(node, "subtype")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "subtype", item);    
    
    properties_item(node, scrollbox, TRUE);
    
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
    xmlNodePtr parentNode = findChildName(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree"), treeitem;
    xmlNodePtr textNode = NULL;
    
    if(is_packable(TRUE) && parentNode)
    {
        textNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Tree", (xmlChar *)"");
    }
    
    if(!textNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "Tree", 0, (HTREEITEM)DWCurrNode->_private, textNode);
    textNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", textNode);
    
    save_properties();
    
    properties_text(DWCurrNode);
    
    return FALSE;
}

/* Populate the properties window for a tree */
void DWSIGNAL properties_tree(xmlNodePtr node)
{
    HWND item, scrollbox, vbox = dw_window_get_data(hwndProperties, "box");
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", vbox);
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
    xmlNodePtr parentNode = findChildName(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree"), treeitem;
    xmlNodePtr textNode = NULL;
    
    if(is_packable(TRUE) && parentNode)
    {
        textNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"MLE", (xmlChar *)"");
    }
    
    if(!textNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "MLE", 0, (HTREEITEM)DWCurrNode->_private, textNode);
    textNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", textNode);
    
    save_properties();
    
    properties_text(DWCurrNode);
    
    return FALSE;
}

/* Populate the properties window for a MLE */
void DWSIGNAL properties_mle(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", vbox);
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
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = findChildName(node, "deftext")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_entryfield_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "deftext", item);
    
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
    xmlNodePtr parentNode = findChildName(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree"), treeitem;
    xmlNodePtr textNode = NULL;
    
    if(is_packable(TRUE) && parentNode)
    {
        textNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Render", (xmlChar *)"");
    }
    
    if(!textNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "Render", 0, (HTREEITEM)DWCurrNode->_private, textNode);
    textNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", textNode);
    
    save_properties();
    
    properties_text(DWCurrNode);
    
    return FALSE;
}

/* Populate the properties window for a render */
void DWSIGNAL properties_render(xmlNodePtr node)
{
    HWND item, scrollbox, vbox = dw_window_get_data(hwndProperties, "box");
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", vbox);
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

/* Create a new box definition */
int DWSIGNAL box_create(HWND window, void *data)
{
    xmlNodePtr parentNode = findChildName(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree"), treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr boxNode = NULL;
    
    if(is_packable(TRUE))
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
    
    treeitem = dw_tree_insert(tree, buf, 0, (HTREEITEM)DWCurrNode->_private, boxNode);
    boxNode->_private = (void *)treeitem;
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", boxNode);
    
    save_properties();
    
    properties_window(DWCurrNode);
    
    return FALSE;
}

/* Populate the properties window for a box */
void DWSIGNAL properties_box(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", vbox);
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
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("None", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "None");
    dw_listbox_append(item, "Group");
    dw_listbox_append(item, "Scroll");
    dw_listbox_append(item, "Splitbar");
    val = defvalstr;
    if((this = findChildName(node, "subtype")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "subtype", item);    
    
    properties_item(node, scrollbox, TRUE);
    
    /* Orientation */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Orientation", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Horizontal", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Horizontal");
    dw_listbox_append(item, "Vertical");
    val = defvalstr;
    if((this = findChildName(node, "orientation")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "orientation", item);    
    /* Group title */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Group title", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    if((this = findChildName(node, "title")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_entryfield_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "title", item);

    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(box_create), NULL);
    }
    dw_window_redraw(hwndProperties);
}    

/* Create a new window definition */
int DWSIGNAL window_create(HWND window, void *data)
{
    xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
    xmlNodePtr windowNode = xmlNewTextChild(rootNode, NULL, (xmlChar *)"Window", (xmlChar *)"");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "tree"), treeitem;
    char buf[200], *title = dw_window_get_text((HWND)dw_window_get_data(vbox, "title"));
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(windowNode, NULL, (xmlChar *)"Children", (xmlChar *)"");
    
    snprintf(buf, 200, "Window - (%s)", title ? title : "");
    
    if(title)
        dw_free(title);
    
    treeitem = dw_tree_insert(tree, buf, 0, 0, windowNode);
    windowNode->_private = (void *)treeitem;
    
    dw_window_set_data(vbox, "node", windowNode);
    
    save_properties();
    
    properties_window(DWCurrNode);
    
    return FALSE;
}

/* Populate the properties window for a window */
void DWSIGNAL properties_window(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    char *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", vbox);
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
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    if((this = findChildName(node, "title")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_entryfield_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "title", item);
    /* Size */ 
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Size (width, height)", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = "100";
    if((this = findChildName(node, "width")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "width", item);
    val = "100";
    if((this = findChildName(node, "height")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "height", item);
    /* Positon */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Position (x, y)", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalint;
    if((this = findChildName(node, "x")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "x", item);
    val = defvalint;
    if((this = findChildName(node, "y")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "y", item);
    /* Border */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Border", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalint;
    if((this = findChildName(node, "bordersize")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "bordersize", item);
    /* Buttons */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Buttons", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_checkbox_new("Close", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvaltrue;
    if((this = findChildName(node, "close")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "close", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("Minimize", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvaltrue;
    if((this = findChildName(node, "minimize")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "minimize", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("Maximize", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvaltrue;
    if((this = findChildName(node, "maximize")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "maximize", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("Hide", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvalstr;
    if((this = findChildName(node, "hide")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "hide", item);
    /* Style */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Style", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_checkbox_new("Resize", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvaltrue;
    if((this = findChildName(node, "resize")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "resize", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("Dialog", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvalstr;
    if((this = findChildName(node, "dialog")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "dialog", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("Border", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvalstr;
    if((this = findChildName(node, "border")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "border", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("System Menu", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvaltrue;
    if((this = findChildName(node, "sysmenu")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "sysmenu", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("Task List", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    val = defvaltrue;
    if((this = findChildName(node, "tasklist")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "tasklist", item);
    /* Orientation */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Orientation", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Horizontal", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Horizontal");
    dw_listbox_append(item, "Vertical");
    val = defvalstr;
    if((this = findChildName(node, "orientation")))
    {
        val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
        if(strcmp(val, "Vertical") == 0)
        {
            val = "1";
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "orientation", item);    
    
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
        xmlIndentTreeOutput = 1;
        
        xmlSaveFormatFile(filename, DWDoc, 1);
        dw_free(filename);
    }
    return FALSE;
}

/* One of the buttons on the toolbar was clicked */
int DWSIGNAL toolbar_clicked(HWND button, void *data)
{
    int which = (int)data;
    
    if(!data)
    {
        return FALSE;
    }
    
    /* Save existing data... if any... here */
    save_properties();
    
    switch(which)
    {
        case TYPE_WINDOW:
            properties_window(NULL);
            break;
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
        case TYPE_RANGED:
            break;
        case TYPE_SPINBUTTON:
            break;
        case TYPE_BITMAP:
            break;
        case TYPE_NOTEBOOK:
            break;
        case TYPE_NOTEBOOK_PAGE:
            break;
        case TYPE_HTML:
            break;
        case TYPE_CALENDAR:
            break;
        default:
            return FALSE;
    }
    dw_window_set_data(hwndProperties, "type", (void *)which);
    return FALSE;
}

/* Handle loading a new item when selectng the tree */
int DWSIGNAL tree_select(HWND window, HTREEITEM item, char *text, void *data, void *itemdata)
{
    /* Save existing data... if any... here */
    save_properties();
    
    DWCurrNode = itemdata;
    
    if(DWCurrNode && DWCurrNode->name)
    {
        if(strcmp((char *)DWCurrNode->name, "Window") == 0)
        {
            properties_window(DWCurrNode);
        }
        else if(strcmp((char *)DWCurrNode->name, "Box") == 0)
        {
            properties_box(DWCurrNode);
        }
        else if(strcmp((char *)DWCurrNode->name, "Text") == 0)
        {
            properties_text(DWCurrNode);
        }
        else if(strcmp((char *)DWCurrNode->name, "Entryfield") == 0)
        {
            properties_entryfield(DWCurrNode);
        }
        else if(strcmp((char *)DWCurrNode->name, "Combobox") == 0)
        {
            properties_combobox(DWCurrNode);
        }
        else if(strcmp((char *)DWCurrNode->name, "Listbox") == 0)
        {
            properties_listbox(DWCurrNode);
        }
        else if(strcmp((char *)DWCurrNode->name, "Container") == 0)
        {
            properties_container(DWCurrNode);
        }
        else if(strcmp((char *)DWCurrNode->name, "Tree") == 0)
        {
            properties_tree(DWCurrNode);
        }
        else if(strcmp((char *)DWCurrNode->name, "MLE") == 0)
        {
            properties_mle(DWCurrNode);
        }
        else if(strcmp((char *)DWCurrNode->name, "Render") == 0)
        {
            properties_render(DWCurrNode);
        }
    }

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

#define TOOLBAR_WIDTH   100
#define TOOLBAR_HEIGHT  30

void dwib_init(void)
{
    HWND vbox, hbox, item;
    
    /* Create a new empty XML document */
    DWDoc = xmlNewDoc((xmlChar *)"1.0");
    DWCurrNode = xmlNewNode(NULL, (xmlChar *)"Dynamic Windows");
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
    item = dw_button_new("Spinbutton", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_SPINBUTTON);
    item = dw_button_new("Bitmap", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_BITMAP);
    item = dw_button_new("Notebook", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_NOTEBOOK);
    item = dw_button_new("NB Page", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_NOTEBOOK_PAGE);
#ifndef __OS2__
    item = dw_button_new("HTML", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_HTML);
    item = dw_button_new("Calendar", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_CALENDAR);
#endif
#if 0
    item = dw_button_new("Splitbar", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)TYPE_SPLITBAR);
#endif
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hbox, vbox, 0, 0, TRUE, TRUE, 0);
    item = dw_tree_new(0);
    dw_box_pack_start(vbox, item, 1, 1, TRUE, TRUE, 0);
    dw_signal_connect(item, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(tree_select), NULL);
    dw_window_set_data(hwndToolbar, "tree", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_button_new("Open", 0);
    dw_box_pack_start(hbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    //dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(open_clicked), NULL);
    item = dw_button_new("Save", 0);
    dw_box_pack_start(hbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(save_clicked), NULL);
    dw_box_pack_start(hbox, 0, 30, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    item = dw_button_new("Delete", 0);
    dw_box_pack_start(hbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    //dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(delete_clicked), NULL);
    dw_box_pack_start(hbox, 0, 30, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    item = dw_button_new("Refresh", 0);
    dw_box_pack_start(hbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    //dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(refresh_clicked), NULL);
    dw_signal_connect(hwndToolbar, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(toolbar_delete), NULL);
    dw_window_set_pos_size(hwndToolbar, 20, 20, 600, 600);
    dw_window_show(hwndToolbar);
    
    hwndProperties = dw_window_new(DW_DESKTOP, "Properties Inspector", DW_FCF_TITLEBAR | DW_FCF_SIZEBORDER);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", vbox);
    item = dw_text_new("No item selected", 0);
    dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
    dw_signal_connect(hwndToolbar, DW_SIGNAL_SET_FOCUS, DW_SIGNAL_FUNC(toolbar_focus), NULL);
    dw_window_set_pos_size(hwndProperties, 650, 20, 300, 500);
    dw_window_show(hwndProperties);
}

/* The main entry point.  Notice we don't use WinMain() on Windows */
int main(int argc, char *argv[])
{
    dw_init(TRUE, argc, argv);
    
    dwib_init();
    
    dw_main();

    return 0;
}

