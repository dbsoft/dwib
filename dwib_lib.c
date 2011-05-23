/* Dynamic Windows Application: Interface Builder
 * Created by Dynamic Windows Interface Builder
 * Author: Brian Smith
 */

#include <dw.h>
#include <libxml/tree.h>
#include <string.h>
#include "dwib.h"
#include "dwib_int.h"


/* Returns a child of node with the specified name.
 * Returns NULL on failure.
 */
xmlNodePtr _dwib_find_child(xmlNodePtr node, char *name)
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

void _dwib_item_pack(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND box, HWND item)
{
    int width = 1, height = 1, hexpand = TRUE, vexpand = TRUE, padding = 0;
    char *thisval, *dataname = NULL;
    xmlNodePtr this;
    
    if((this = _dwib_find_child(node, "width")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        width = atoi(thisval);
    if((this = _dwib_find_child(node, "height")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        height = atoi(thisval);
    if((this = _dwib_find_child(node, "hexpand")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        hexpand = atoi(thisval);
    if((this = _dwib_find_child(node, "vexpand")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        vexpand = atoi(thisval);
    if((this = _dwib_find_child(node, "padding")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        padding = atoi(thisval);
    if((this = _dwib_find_child(node, "dataname")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        dataname = thisval;
    
    dw_box_pack_start(box, item, width, height, hexpand, vexpand, padding);
    
    if(dataname && window)
        dw_window_set_data(window, dataname, item);
}

HWND _dwib_notebook_page_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox)
{
    HWND box;
    xmlNodePtr this = _dwib_find_child(node, "title");
    char *thisval;
    int flags = 0, orient = DW_HORZ;
    unsigned long pageid = dw_notebook_page_new(packbox, flags, FALSE);
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        dw_notebook_page_set_text(packbox, pageid, thisval);
    if((this = _dwib_find_child(node, "statustext")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        dw_notebook_page_set_status_text(packbox, pageid, thisval);
    if((this = _dwib_find_child(node, "orientation")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) 
       && (atoi(thisval) || strcmp(thisval, "Vertical") == 0))
        orient = DW_VERT;

    box = dw_box_new(orient, 0);
    dw_notebook_pack(packbox, pageid, box);
    return box;
}

HWND _dwib_notebook_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox)
{
    HWND notebook;
    xmlNodePtr this = _dwib_find_child(node, "position");
    char *thisval;
    int position = 0;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && strcmp(thisval, "Bottom") == 0)
        position = 1;
    
    notebook = dw_notebook_new(0, position);
    
    _dwib_item_pack(node, doc, window, packbox, notebook);
    return notebook;
}

HWND _dwib_box_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox)
{
    HWND box, box1, box2;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval, *title = "";
    int orient = DW_HORZ, padding = 0, type = 0;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        title = thisval;
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Group") == 0)
            type = 1;
        else if(strcmp(thisval, "Scroll") == 0)
            type = 2;
        else if(strcmp(thisval, "Splitbar") == 0)
            type = 3;
    }
    if((this = _dwib_find_child(node, "title")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        title = thisval;
    if((this = _dwib_find_child(node, "orientation")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) 
       && (atoi(thisval) || strcmp(thisval, "Vertical") == 0))
        orient = DW_VERT;
    
    switch(type)
    {
        case 0:
            box = dw_box_new(orient, padding);
            break;
        case 1:
            box = dw_groupbox_new(orient, padding, title);
            break;
        case 2:
            box = dw_scrollbox_new(orient, padding);
            break;
        case 3:
            box1 = dw_box_new(DW_VERT, 0);
            box2 = dw_box_new(DW_VERT, 0);
            box = dw_splitbar_new(orient, box1, box2, 0);
            dw_window_set_data(box, "_dwib_box1", box1);
            dw_window_set_data(box, "_dwib_box2", box2);
            break;
    }
    _dwib_item_pack(node, doc, window, packbox, box);
    return box;
}

HWND _dwib_button_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox)
{
    HWND button;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval, *setting = "";
    int type = 0;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Bitmap") == 0)
            type = 1;
        else if(strcmp(thisval, "Check") == 0)
            type = 2;
        else if(strcmp(thisval, "Radio") == 0)
            type = 3;
    }
    if((this = _dwib_find_child(node, "setting")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        setting = thisval;
    if((this = _dwib_find_child(node, "check")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        dw_checkbox_set(button, TRUE);
    
    switch(type)
    {
        case 0:
            button = dw_button_new(setting, 0);
            break;
        case 1:
            button = dw_bitmapbutton_new("", atoi(setting));
            break;
        case 2:
            button = dw_checkbox_new(setting, 0);
            break;
        case 3:
            dw_radiobutton_new(setting, 0);
            break;
    }
    _dwib_item_pack(node, doc, window, packbox, button);
    return button;
}

HWND _dwib_text_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox)
{
    HWND text;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval, *label = "";
    int type = 0;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Status") == 0)
            type = 1;
    }
    if((this = _dwib_find_child(node, "label")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        label = thisval;
    
    switch(type)
    {
        case 0:
            text = dw_text_new(label, 0);
            break;
        case 1:
            text = dw_status_text_new(label, 0);
            break;
    }
    _dwib_item_pack(node, doc, window, packbox, text);
    return text;
}

HWND _dwib_container_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox)
{
    HWND container;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval;
    int type = 0;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Filesystem") == 0)
            type = 1;
    }
    
    container = dw_container_new(0, FALSE);
    
#if 0
    switch(type)
    {
        case 0:
            dw_container_setup(container);
            break;
        case 1:
            dw_filesystem_setup(container);
            break;
    }
#endif
    
    _dwib_item_pack(node, doc, window, packbox, container);
    return container;
}

HWND _dwib_ranged_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox)
{
    HWND ranged;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval, *pos = "";
    int type = 0, upper = 100, lower = 0, position = 0, orient = DW_HORZ;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Slider") == 0)
            type = 1;
        if(strcmp(thisval, "Scrollbar") == 0)
            type = 2;
        if(strcmp(thisval, "Spinbutton") == 0)
            type = 3;
    }
    if((this = _dwib_find_child(node, "position")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        pos = thisval;
        position = atoi(pos);
    }
    
    switch(type)
    {
        case 0:
            ranged = dw_percent_new(0);
            dw_percent_set_pos(ranged, position);
            break;
        case 1:
            ranged = dw_slider_new(orient, upper, 0);
            dw_slider_set_pos(ranged, position);
            break;
        case 2:
            ranged = dw_scrollbar_new(orient, 0);
            dw_scrollbar_set_pos(ranged, position);
            dw_scrollbar_set_range(ranged, upper, 1);
            break;
        case 3:
            ranged = dw_spinbutton_new(pos, 0);
            dw_spinbutton_set_limits(ranged, upper, lower);
            break;
            
    }
    _dwib_item_pack(node, doc, window, packbox, ranged);
    return ranged;
}

HWND _dwib_entryfield_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox)
{
    HWND entryfield;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval, *deftext = "";
    int type = 0;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Password") == 0)
            type = 1;
    }
    if((this = _dwib_find_child(node, "deftext")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        entryfield = thisval;
    
    switch(type)
    {
        case 0:
            entryfield = dw_entryfield_new(deftext, 0);
            break;
        case 1:
            entryfield = dw_entryfield_password_new(deftext, 0);
            break;
    }
    
    if((this = _dwib_find_child(node, "deftext")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        dw_entryfield_set_limit(entryfield, atoi(thisval));
    
    _dwib_item_pack(node, doc, window, packbox, entryfield);
    return entryfield;
}

/* Parse the children if packable widgets... boxes, notebook pages, etc */
void _dwib_children(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND box)
{
    xmlNodePtr p = _dwib_find_child(node, "Children");
    
    for(p=p->children;p;p = p->next)
    {
        if(strcmp((char *)p->name, "Box") == 0)
        {
            HWND newbox = _dwib_box_create(p, doc, window, box);
            _dwib_children(p, doc, window, newbox);
        }
        else if(strcmp((char *)p->name, "Notebook") == 0)
        {
            HWND notebook = _dwib_notebook_create(p, doc, window, box);
            _dwib_children(p, doc, window, notebook);
        }
        else if(strcmp((char *)p->name, "NotebookPage") == 0)
        {
            HWND notebookpage = _dwib_notebook_page_create(p, doc, window, box);
            _dwib_children(p, doc, window, notebookpage);
        }
        else if(strcmp((char *)p->name, "Button") == 0)
        {
            _dwib_button_create(p, doc, window, box);
        }
        else if(strcmp((char *)p->name, "Text") == 0)
        {
            _dwib_text_create(p, doc, window, box);
        }
        else if(strcmp((char *)p->name, "Container") == 0)
        {
            _dwib_container_create(p, doc, window, box);
        }
        else if(strcmp((char *)p->name, "Ranged") == 0)
        {
            _dwib_ranged_create(p, doc, window, box);
        }
        else if(strcmp((char *)p->name, "Entryfield") == 0)
        {
            _dwib_entryfield_create(p, doc, window, box);
        }
        /* No subtype */
        else if(strcmp((char *)p->name, "Combobox") == 0)
        {
            
        }
        else if(strcmp((char *)p->name, "Tree") == 0)
        {
            
        }
        else if(strcmp((char *)p->name, "MLE") == 0)
        {
            
        }
        else if(strcmp((char *)p->name, "Render") == 0)
        {
            
        }
        else if(strcmp((char *)p->name, "Bitmap") == 0)
        {
            
        }
        else if(strcmp((char *)p->name, "HTML") == 0)
        {
            
        }
        else if(strcmp((char *)p->name, "Calendar") == 0)
        {
            
        }
        else if(strcmp((char *)p->name, "Listbox") == 0)
        {
        }
    }
}

HWND _dwib_window_create(xmlNodePtr node, xmlDocPtr doc)
{
    xmlNodePtr this = _dwib_find_child(node, "title");
    char *thisval, *title = "";
    unsigned long flags;
    int bordersize, orient = DW_HORZ, padding = 0;
    HWND ret, box;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        title = thisval;
    if((this = _dwib_find_child(node, "bordersize")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        bordersize = atoi(thisval);
    if((this = _dwib_find_child(node, "close")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        flags |= DW_FCF_CLOSEBUTTON;
    if((this = _dwib_find_child(node, "minimize")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        flags |= DW_FCF_MINBUTTON;
    if((this = _dwib_find_child(node, "maximize")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        flags |= DW_FCF_MAXBUTTON;
    if((this = _dwib_find_child(node, "hide")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        flags |= DW_FCF_HIDEBUTTON;
    if((this = _dwib_find_child(node, "resize")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        flags |= DW_FCF_SIZEBORDER;
    if((this = _dwib_find_child(node, "dialog")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        flags |= DW_FCF_DLGBORDER;
    if((this = _dwib_find_child(node, "border")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        flags |= DW_FCF_BORDER;
    if((this = _dwib_find_child(node, "sysmenu")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        flags |= DW_FCF_SYSMENU;
    if((this = _dwib_find_child(node, "tasklist")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        flags |= DW_FCF_TASKLIST;

    if((this = _dwib_find_child(node, "orientation")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) 
       && (atoi(thisval) || strcmp(thisval, "Vertical") == 0))
        orient = DW_VERT;
    if((this = _dwib_find_child(node, "padding")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        padding = atoi(thisval);

    ret = dw_window_new(DW_DESKTOP, title, flags);
    
    box = dw_box_new(orient, padding);
    
    dw_box_pack_start(ret, box, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(ret, "_dwib_box", box);
    
    if(bordersize != -1)
        dw_window_set_border(ret, bordersize);
    
    return ret;
}

/* Clears and reloads the tree data from XML */
HWND API dwib_load(DWIB handle, char *name)
{
    xmlDocPtr doc = handle;
    xmlNodePtr p, rootNode = xmlDocGetRootElement(doc);
    
    if(!rootNode)
        return 0;
    
    for(p=rootNode->children;p;p = p->next)
    {
        if(strcmp((char *)p->name, "Window") == 0)
        {
            xmlNodePtr this = _dwib_find_child(p, "title");
            char *val = (char *)xmlNodeListGetString(doc, this->children, 1);
            
            if(val && strcmp(name, val) == 0)
            {
                HWND ret = _dwib_window_create(p, doc);
                _dwib_children(p, doc, ret, (HWND)dw_window_get_data(ret, "_dwib_box"));
                return ret;
            }
        }
    }
    return 0;
}

DWIB API dwib_open(char *filename)
{
    return xmlParseFile(filename);
}

void API dwib_close(DWIB handle)
{
    xmlDocPtr doc = handle;
    xmlFreeDoc(doc);
}
