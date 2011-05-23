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

void _dwib_item_pack(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND item)
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
    
    if(dataname && window)
        dw_window_set_data(window, dataname, item);
}

HWND _dwib_box_create(xmlNodePtr node, xmlDocPtr doc, HWND window)
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
    return box;
}

/* Parse the children if packable widgets... boxes, notebook pages, etc */
void _dwib_children(xmlNodePtr node, xmlDocPtr doc, HWND window)
{
    xmlNodePtr p = _dwib_find_child(node, "Children");
    char *val;
    
    for(p=p->children;p;p = p->next)
    {
        if(strcmp((char *)p->name, "Box") == 0)
        {
            _dwib_box_create(p, doc, window);
            _dwib_children(p, doc, window);
        }
        else if(strcmp((char *)p->name, "Notebook") == 0)
        {
            _dwib_children(p, doc, window);
        }
        else if(strcmp((char *)p->name, "NotebookPage") == 0)
        {
            xmlNodePtr this = findChildName(p, "title");
            
            val = (char *)xmlNodeListGetString(doc, this->children, 1);
            
            _dwib_children(p, doc, window);
        }
        else if(strcmp((char *)p->name, "Button") == 0 ||
                strcmp((char *)p->name, "Text") == 0 ||
                strcmp((char *)p->name, "Container") == 0 ||
                strcmp((char *)p->name, "Ranged") == 0 ||
                strcmp((char *)p->name, "Entryfield") == 0)
        {
            xmlNodePtr this = findChildName(p, "subtype");
            
            val = (char *)xmlNodeListGetString(doc, this->children, 1);
        }
        else if(strcmp((char *)p->name, "Combobox") == 0 ||
                strcmp((char *)p->name, "Tree") == 0 ||
                strcmp((char *)p->name, "MLE") == 0 ||
                strcmp((char *)p->name, "Render") == 0 ||
                strcmp((char *)p->name, "Bitmap") == 0 ||
                strcmp((char *)p->name, "HTML") == 0 ||
                strcmp((char *)p->name, "Calendar") == 0 ||
                strcmp((char *)p->name, "Listbox") == 0)
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
                _dwib_children(p, doc, ret);
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
