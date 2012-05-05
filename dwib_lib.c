/* Dynamic Windows Application: Interface Builder
 * Created by Dynamic Windows Interface Builder
 * Author: Brian Smith
 */

#include <dw.h>
#include <libxml/tree.h>
#include <string.h>
#include <ctype.h>
#include "dwib.h"
#include "dwib_int.h"
#include "resources.h"

static void *_dwib_builder = NULL;
char *_dwib_image_root = NULL;

/* Enable builder mode when linked to the main application. */
void _dwib_builder_toggle(void *val)
{
    _dwib_builder = val;
}

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

int xtoi(char c)
{
    if(isdigit(c))
    {
        return c - '0';
    }
    return (c - 'a') + 10;
}

/* Return the color index or RGB color */
int _dwib_get_color(char *color)
{
    int x;
    
    if(strcmp(color, "None") == 0)
        return DW_RGB_TRANSPARENT;
    for(x=0;x<16;x++)
    {
        if(strcmp(color, Colors[x]) == 0)
            return x;
    }
    if(*color == '#' && strlen(color) == 7)
    {
        return DW_RGB(((xtoi(color[1]) * 16) + xtoi(color[2])),
                      ((xtoi(color[3]) * 16) + xtoi(color[4])),
                      ((xtoi(color[5]) * 16) + xtoi(color[6])));
    }
    return DW_CLR_DEFAULT;
}

/* Internal function for handling packing of items into boxes */
void _dwib_item_pack(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND box, HWND item, int index)
{
    int width = 1, height = 1, hexpand = TRUE, vexpand = TRUE, padding = 0;
    int fcolor = DW_CLR_DEFAULT, bcolor = DW_CLR_DEFAULT;
    char *thisval, *dataname = NULL;
    xmlNodePtr this;
    HWND splitbox = (HWND)dw_window_get_data(box, "_dwib_box1");
    
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
    if((this = _dwib_find_child(node, "enabled")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval) == 0)
        dw_window_disable(item);
    if((this = _dwib_find_child(node, "dataname")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        dataname = thisval;
#ifdef __OS2__
    this = _dwib_find_child(node, "os2font");
#elif defined(__MAC__)
    this = _dwib_find_child(node, "macfont");
#elif defined(__WIN32__)
    this = _dwib_find_child(node, "winfont");
#elif defined(__UNIX__)
    this = _dwib_find_child(node, "unixfont");
#else 
    this = NULL;
#endif    
    if((this || (this = _dwib_find_child(node, "font"))) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && strcmp(thisval, "Default"))
        dw_window_set_font(item, thisval);
    if((this = _dwib_find_child(node, "tooltip")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && *thisval)
        dw_window_set_tooltip(item, thisval);
    if((this = _dwib_find_child(node, "fcolor")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        fcolor = _dwib_get_color(thisval);
    if((this = _dwib_find_child(node, "bcolor")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        bcolor = _dwib_get_color(thisval);
    
    if(fcolor != DW_CLR_DEFAULT || bcolor != DW_CLR_DEFAULT)
        dw_window_set_color(item, fcolor, bcolor);
   
    /* If it is a splitbox, find the correct sub-box */
    if(splitbox)
    {
        int count = DW_POINTER_TO_INT(dw_window_get_data(box, "_dwib_count"));
        if(count == 1)
            splitbox = (HWND)dw_window_get_data(box, "_dwib_box2");
        else if(count > 1)
            return;
        count++;
        dw_window_set_data(box, "_dwib_count", DW_INT_TO_POINTER(count));
        box = splitbox;                        
    }
    dw_box_pack_at_index(box, item, index, width, height, hexpand, vexpand, padding);
    /* Save the item handle in the psvi field */
    node->psvi = (void *)item;
    
    if(dataname && window)
        dw_window_set_data(window, dataname, (void *)item);
}

/* Internal function for creating a notebook page widget from an XML tree node */
HWND _dwib_notebook_page_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox)
{
    HWND box;
    xmlNodePtr this = _dwib_find_child(node, "statustext");
    char *thisval;
    int flags = 0, orient = DW_HORZ;
    unsigned long pageid = dw_notebook_page_new(packbox, flags, FALSE);
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        dw_notebook_page_set_status_text(packbox, pageid, thisval);
    if((this = _dwib_find_child(node, "orientation")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) 
       && (atoi(thisval) || strcmp(thisval, "Vertical") == 0))
        orient = DW_VERT;

    box = dw_box_new(orient, 0);
    dw_notebook_pack(packbox, pageid, box);
    /* Save the box handle in the psvi field */
    node->psvi = DW_POINTER(box);
    /* Save the page ID in the box data */
    dw_window_set_data(box, "_dwib_pageid", DW_INT_TO_POINTER(pageid));
    
    if((this = _dwib_find_child(node, "title")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        dw_notebook_page_set_text(packbox, pageid, thisval);
    return box;
}

/* Internal function for creating a notebook widget from an XML tree node */
HWND _dwib_notebook_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND notebook;
    xmlNodePtr this = _dwib_find_child(node, "position");
    char *thisval;
    int top = 1;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && strcmp(thisval, "Bottom") == 0)
        top = 0;
    
    notebook = dw_notebook_new(0, top);
    
    _dwib_item_pack(node, doc, window, packbox, notebook, index);
    return notebook;
}

/* Internal function for creating a box widget from an XML tree node */
HWND _dwib_box_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND box = 0, box1, box2;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval, *title = "";
    int orient = DW_HORZ, padding = 0, type = 0, splitper = 50;
    
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
    if((this = _dwib_find_child(node, "splitper")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        splitper = atoi(thisval);
    
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
            box1 = dw_box_new(orient, 0);
            box2 = dw_box_new(orient, 0);
            box = dw_splitbar_new(orient, box1, box2, 0);
            dw_window_set_data(box, "_dwib_box1", (void *)box1);
            dw_window_set_data(box, "_dwib_box2", (void *)box2);
            dw_splitbar_set(box, splitper);
            break;
    }
    if(box)
        _dwib_item_pack(node, doc, window, packbox, box, index);
    return box;
}

/* Internal function for creating a button widget from an XML tree node */
HWND _dwib_button_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND button = 0;
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
   
    switch(type)
    {
        case 0:
            button = dw_button_new(setting, 0);
            break;
        case 1:
        {
            int resid = atoi(setting);
            
            if(_dwib_builder && resid)
                resid = BITMAP_PLACEHOLD;
            
            if(resid)
                button = dw_bitmapbutton_new(NULL, resid);
            else
                button = dw_bitmapbutton_new_from_file(NULL, 0, setting);
            }
            break;
        case 2:
            button = dw_checkbox_new(setting, 0);
            break;
        case 3:
            button = dw_radiobutton_new(setting, 0);
            break;
    }

    if(button)
    {
        if((this = _dwib_find_child(node, "check")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
            dw_checkbox_set(button, TRUE);
        if((this = _dwib_find_child(node, "borderless")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
            dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);

        _dwib_item_pack(node, doc, window, packbox, button, index);
    }
    return button;
}

/* Internal function for creating a text widget from an XML tree node */
HWND _dwib_text_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND text = 0;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval, *label = "";
    int type = 0, flags = 0;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Status") == 0)
            type = 1;
    }
    if((this = _dwib_find_child(node, "label")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        label = thisval;
    if((this = _dwib_find_child(node, "alignment")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Center") == 0)
            flags = DW_DT_CENTER;
        else if(strcmp(thisval, "Right") == 0)
            flags = DW_DT_RIGHT;
    }
    if((this = _dwib_find_child(node, "valignment")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Center") == 0)
            flags |= DW_DT_VCENTER;
        else if(strcmp(thisval, "Bottom") == 0)
            flags |= DW_DT_BOTTOM;
    }
    
    switch(type)
    {
        case 0:
            text = dw_text_new(label, 0);
            break;
        case 1:
            text = dw_status_text_new(label, 0);
            break;
    }
    if(text)
        _dwib_item_pack(node, doc, window, packbox, text, index);
    if(flags)
        dw_window_set_style(text, flags, flags);
    return text;
}

/* Fills in a container with columns from the List node */
void _dwib_populate_container(HWND container, xmlNodePtr node, xmlDocPtr doc, int type, int splitcol)
{
    xmlNodePtr p;
    int count = 0;
    
    if(node)
    {
        for(p=node->children;p;p = p->next)
        {
            if(strcmp((char *)p->name, "Item") == 0)
            {
                count++;
            }
        }
        
        if(count > 0)
        {
            char **colnames = calloc(sizeof(char *), count);
            unsigned long *colflags = calloc(sizeof(unsigned long), count);
            count = 0;
            
            for(p=node->children;p;p = p->next)
            {
                if(strcmp((char *)p->name, "Item") == 0)
                {
                    char *thisval;
                    
                    if((thisval = (char *)xmlNodeListGetString(doc, p->children, 1)))
                    {
                        char *coltype = (char *)xmlGetProp(p, (xmlChar *)"ColType");
                        char *colalign = (char *)xmlGetProp(p, (xmlChar *)"ColAlign");
                        unsigned long ctype = DW_CFA_STRING;
                        unsigned long calign = DW_CFA_LEFT;
                        
                        if(coltype)
                        {
                            if(strcmp(coltype, "Icon") == 0)
                                ctype = DW_CFA_BITMAPORICON;
                            else if(strcmp(coltype, "Number") == 0)
                                ctype = DW_CFA_ULONG;
                            else if(strcmp(coltype, "Date") == 0)
                                ctype = DW_CFA_DATE;
                            else if(strcmp(coltype, "Time") == 0)
                                ctype = DW_CFA_TIME;
                        }
                        if(colalign)
                        {
                            if(strcmp(colalign, "Center") == 0)
                                calign = DW_CFA_CENTER;
                            else if(strcmp(colalign, "Right") == 0)
                                calign = DW_CFA_RIGHT;
                        }
                        colnames[count] = thisval;
                        colflags[count] = ctype | calign | DW_CFA_SEPARATOR;
                        count++;
                    }
                }
            }
            switch(type)
            {
                case 0:
                    dw_container_setup(container, colflags, colnames, count, splitcol);
                    break;
                case 1:
                    dw_filesystem_setup(container, colflags, colnames, count);
                    break;
            }
            free(colnames);
            free(colflags);
        }
    }
    /* If we don't have columns but we are a filesystem... */
    if(count == 0 && type == 1) 
        dw_filesystem_setup(container, NULL, NULL, 0);
}

/* Internal function for creating a container widget from an XML tree node */
HWND _dwib_container_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND container;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval;
    int type = 0, multi = 0, splitcol = 0;
    unsigned long oddcolor = DW_RGB_TRANSPARENT, evencolor = DW_RGB_TRANSPARENT;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Filesystem") == 0)
            type = 1;
    }
    if((this = _dwib_find_child(node, "multi")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        multi = atoi(thisval);
    if((this = _dwib_find_child(node, "splitcol")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        splitcol = atoi(thisval);
    
    container = dw_container_new(0, multi);
    
    /* If we have columns set them up */
    if((this = _dwib_find_child(node, "Columns")))
        _dwib_populate_container(container, this, doc, type, splitcol);
        
    if((this = _dwib_find_child(node, "oddcolor")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        oddcolor = _dwib_get_color(thisval);
    if((this = _dwib_find_child(node, "evencolor")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        evencolor = _dwib_get_color(thisval);
    
    if(oddcolor != DW_RGB_TRANSPARENT || evencolor != DW_RGB_TRANSPARENT)
        dw_container_set_stripe(container, oddcolor, evencolor);
    
    _dwib_item_pack(node, doc, window, packbox, container, index);
    return container;
}

/* Internal function for creating a ranged widget from an XML tree node */
HWND _dwib_ranged_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND ranged = 0;
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
    if(ranged)
        _dwib_item_pack(node, doc, window, packbox, ranged, index);
    return ranged;
}

/* Internal function for creating an entryfield widget from an XML tree node */
HWND _dwib_entryfield_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND entryfield = 0;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval, *deftext = "";
    int type = 0;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Password") == 0)
            type = 1;
    }
    if((this = _dwib_find_child(node, "deftext")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        deftext = thisval;
    
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
    
    if(entryfield)
        _dwib_item_pack(node, doc, window, packbox, entryfield, index);
    return entryfield;
}

/* Fills in a listbox/combobox with items from the List node */
void _dwib_populate_list(HWND list, xmlNodePtr node, xmlDocPtr doc)
{
    xmlNodePtr p;
    
    for(p=node->children;p;p = p->next)
    {
        if(strcmp((char *)p->name, "Item") == 0)
        {
            char *thisval;
            
            if((thisval = (char *)xmlNodeListGetString(doc, p->children, 1)))
            {
                dw_listbox_append(list, thisval);
            }
        }
    }
}

/* Internal function for creating a combobox widget from an XML tree node */
HWND _dwib_combobox_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND combobox;
    xmlNodePtr this = _dwib_find_child(node, "deftext");
    char *thisval, *deftext = "";
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        deftext = thisval;
    
    combobox = dw_combobox_new(deftext, 0);
    
    _dwib_item_pack(node, doc, window, packbox, combobox, index);
    
    if((this = _dwib_find_child(node, "List")))
        _dwib_populate_list(combobox, this, doc);
                            
    return combobox;
}

/* Internal function for creating a tree widget from an XML tree node */
HWND _dwib_tree_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND tree = dw_tree_new(0);
    
    _dwib_item_pack(node, doc, window, packbox, tree, index);
    
    /* If in builder mode... */
    if(_dwib_builder)
    {
        /* Pack in some dummy items so people can see it is a tree */
        HTREEITEM root = dw_tree_insert(tree, "Root", 0, 0, NULL);
        
        dw_tree_insert(tree, "Child", 0, root, NULL);
        dw_tree_item_expand(tree, root);
    }
    return tree;
}

/* Internal function for creating a render widget from an XML tree node */
HWND _dwib_render_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND render = dw_render_new(0);
    
    _dwib_item_pack(node, doc, window, packbox, render, index);
    
    if(_dwib_builder)
        dw_signal_connect(render, DW_SIGNAL_EXPOSE, DW_SIGNAL_FUNC(_dwib_builder), NULL);
    return render;
}

/* Internal function for creating an mle widget from an XML tree node */
HWND _dwib_mle_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND mle;
    xmlNodePtr this = _dwib_find_child(node, "deftext");
    char *thisval, *deftext = "";
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        deftext = thisval;
    }
    
    mle = dw_mle_new(0);
    dw_mle_import(mle, deftext, -1);
    
   if((this = _dwib_find_child(node, "wordwrap")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
      dw_mle_set_word_wrap(mle, atoi(thisval));
      
   _dwib_item_pack(node, doc, window, packbox, mle, index);
    return mle;
}

/* Internal function for creating a html widget from an XML tree node */
HWND _dwib_html_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND html;
    xmlNodePtr this = _dwib_find_child(node, "URL");
    char *thisval;
    
    html = dw_html_new(0);
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        dw_html_url(html, thisval);
    
    _dwib_item_pack(node, doc, window, packbox, html, index);
    return html;
}

/* Internal function for creating a calendar widget from an XML tree node */
HWND _dwib_calendar_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND calendar = dw_calendar_new(0);
    
    _dwib_item_pack(node, doc, window, packbox, calendar, index);
    return calendar;
}

/* Internal function for creating a bitmap widget from an XML tree node */
HWND _dwib_bitmap_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND bitmap;
    xmlNodePtr this = _dwib_find_child(node, "setting");
    char *thisval, *setting = "";
    int resid = 0;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        setting = thisval;
        resid = atoi(setting);
        if(_dwib_builder && resid)
            resid = BITMAP_PLACEHOLD;
    }
    
    bitmap = dw_bitmap_new(resid);
    if(setting && *setting && resid == 0)
        dw_window_set_bitmap(bitmap, resid, setting);
    else
        dw_window_set_bitmap(bitmap, resid, NULL);

    _dwib_item_pack(node, doc, window, packbox, bitmap, index);
    return bitmap;
}

/* Internal function for creating a listbox widget from an XML tree node */
HWND _dwib_listbox_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND listbox;
    xmlNodePtr this = _dwib_find_child(node, "multi");
    char *thisval;
    int multi = 0;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        multi = atoi(thisval);
    
    listbox = dw_listbox_new(0, multi);
    
    if((this = _dwib_find_child(node, "List")))
        _dwib_populate_list(listbox, this, doc);
    
    _dwib_item_pack(node, doc, window, packbox, listbox, index);
    
    return listbox;
}

/* Internal function for creating a menu widget from an XML tree node */
HWND _dwib_menu_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HMENUI packbox, HMENUI submenu)
{
    HWND menuitem;
    xmlNodePtr this = _dwib_find_child(node, "title");
    char *thisval, *title = "", *dataname = NULL;
    int flags = 0, checkable = 0, menuid = 0;
    
    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        title = thisval;
    if((this = _dwib_find_child(node, "checkable")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        checkable = atoi(thisval);
    if((this = _dwib_find_child(node, "menuid")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        menuid = atoi(thisval);
    if((this = _dwib_find_child(node, "checked")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        flags |= DW_MIS_CHECKED;
    if((this = _dwib_find_child(node, "enabled")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        flags |= (atoi(thisval) ? DW_MIS_ENABLED : DW_MIS_DISABLED);
    if((this = _dwib_find_child(node, "dataname")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        dataname = thisval;
    
    menuitem = dw_menu_append_item(packbox, title, menuid, flags, TRUE, checkable, submenu);
    /* Save the menu item handle in the psvi field */
    node->psvi = (void *)menuitem;
    
    if(dataname && window)
        dw_window_set_data(window, dataname, (void *)menuitem);
    
    return menuitem;
}

/* Internal function for packing padding from an XML tree node */
void _dwib_padding_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    _dwib_item_pack(node, doc, window, packbox, 0, index);
    node->psvi = DW_INT_TO_POINTER(1);
}

/* Internal function that handles creation on a single node */
HMENUI _dwib_child(xmlDocPtr doc, HWND window, HWND box, int windowlevel, xmlNodePtr p, HMENUI menu, int index)
{
    if(strcmp((char *)p->name, "Box") == 0)
    {
        HWND newbox = _dwib_box_create(p, doc, window, box, index);
        _dwib_children(p, doc, window, newbox, FALSE);
    }
    else if(strcmp((char *)p->name, "Notebook") == 0)
    {
        HWND notebook = _dwib_notebook_create(p, doc, window, box, index);
        _dwib_children(p, doc, window, notebook, FALSE);
    }
    else if(strcmp((char *)p->name, "NotebookPage") == 0)
    {
        HWND notebookpage = _dwib_notebook_page_create(p, doc, window, box);
        _dwib_children(p, doc, window, notebookpage, FALSE);
    }
    else if(strcmp((char *)p->name, "Button") == 0)
    {
        _dwib_button_create(p, doc, window, box, index);
    }
    else if(strcmp((char *)p->name, "Text") == 0)
    {
        _dwib_text_create(p, doc, window, box, index);
    }
    else if(strcmp((char *)p->name, "Container") == 0)
    {
        _dwib_container_create(p, doc, window, box, index);
    }
    else if(strcmp((char *)p->name, "Ranged") == 0)
    {
        _dwib_ranged_create(p, doc, window, box, index);
    }
    else if(strcmp((char *)p->name, "Entryfield") == 0)
    {
        _dwib_entryfield_create(p, doc, window, box, index);
    }
    /* No subtype */
    else if(strcmp((char *)p->name, "Combobox") == 0)
    {
        _dwib_combobox_create(p, doc, window, box, index);
    }
    else if(strcmp((char *)p->name, "Tree") == 0)
    {
        _dwib_tree_create(p, doc, window, box, index);
    }
    else if(strcmp((char *)p->name, "MLE") == 0)
    {
        _dwib_mle_create(p, doc, window, box, index);
    }
    else if(strcmp((char *)p->name, "Render") == 0)
    {
        _dwib_render_create(p, doc, window, box, index);
    }
    else if(strcmp((char *)p->name, "Bitmap") == 0)
    {
        _dwib_bitmap_create(p, doc, window, box, index);
    }
    else if(strcmp((char *)p->name, "HTML") == 0)
    {
        _dwib_html_create(p, doc, window, box, index);
    }
    else if(strcmp((char *)p->name, "Calendar") == 0)
    {
        _dwib_calendar_create(p, doc, window, box, index);
    }
    else if(strcmp((char *)p->name, "Listbox") == 0)
    {
        _dwib_listbox_create(p, doc, window, box, index);
    }
    else if(strcmp((char *)p->name, "Padding") == 0)
    {
        _dwib_padding_create(p, doc, window, box, index);
    }
    else if(strcmp((char *)p->name, "Menu") == 0)
    {
        HMENUI submenu;

        if(!menu)
        {
            if(windowlevel)
                menu = dw_menubar_new(window);
            else
                menu = dw_menu_new(0);
        }
        submenu = _dwib_children(p, doc, window, 0, FALSE);
        _dwib_menu_create(p, doc, window, menu, submenu);
    }
    return menu;
}

/* Internal function fo parsing the children of packable widgets... boxes, notebook pages, etc */
HMENUI _dwib_children(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND box, int windowlevel)
{
    xmlNodePtr p = _dwib_find_child(node, "Children");
    HMENUI menu = 0;
    
    for(p=p->children;p;p = p->next)
    {
        menu = _dwib_child(doc, window, box, windowlevel, p, menu, 65536);
    }
    return menu;
}

/* Internal function for creating a window from an XML tree node */
HWND _dwib_window_create(xmlNodePtr node, xmlDocPtr doc)
{
    xmlNodePtr this = _dwib_find_child(node, "title");
    char *thisval, *title = "";
    unsigned long flags = 0;
    int bordersize = -1, orient = DW_HORZ, padding = 0;
    int x = -1, y = -1, width = -1, height = -1, hgravity = 0, vgravity = 0;
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
    if((this = _dwib_find_child(node, "composited")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        flags |= DW_FCF_COMPOSITED;
    if((this = _dwib_find_child(node, "titlebar")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        flags |= DW_FCF_TITLEBAR;

    if((this = _dwib_find_child(node, "x")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        x = atoi(thisval);
    if((this = _dwib_find_child(node, "y")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        y = atoi(thisval);
    if((this = _dwib_find_child(node, "hgravity")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Center") == 0)
            hgravity = DW_GRAV_CENTER;
        else if(strcmp(thisval, "Right") == 0)
            hgravity = DW_GRAV_RIGHT;
    }
    if((this = _dwib_find_child(node, "hobstacles")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        hgravity |= DW_GRAV_OBSTACLES;
    if((this = _dwib_find_child(node, "vgravity")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Center") == 0)
            vgravity = DW_GRAV_CENTER;
        else if(strcmp(thisval, "Bottom") == 0)
            vgravity = DW_GRAV_BOTTOM;
    }
    if((this = _dwib_find_child(node, "vobstacles")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
        vgravity |= DW_GRAV_OBSTACLES;
    if((this = _dwib_find_child(node, "width")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        width = atoi(thisval);
    if((this = _dwib_find_child(node, "height")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        height = atoi(thisval);
    
    if((this = _dwib_find_child(node, "orientation")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) 
       && (atoi(thisval) || strcmp(thisval, "Vertical") == 0))
        orient = DW_VERT;
    if((this = _dwib_find_child(node, "padding")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        padding = atoi(thisval);

    ret = dw_window_new(DW_DESKTOP, title, flags);
    /* Save the window handle in the psvi field */
    node->psvi = (void *)ret;
    
    /* Save the node pointer in the window data */
    dw_window_set_data(ret, "_dwib_node", (void *)node);
    dw_window_set_data(ret, "_dwib_x", DW_INT_TO_POINTER(x));
    dw_window_set_data(ret, "_dwib_y", DW_INT_TO_POINTER(y));
    dw_window_set_data(ret, "_dwib_width", DW_INT_TO_POINTER(width));
    dw_window_set_data(ret, "_dwib_height", DW_INT_TO_POINTER(height));
    
    box = dw_box_new(orient, padding);
    
    dw_box_pack_start(ret, box, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(ret, "_dwib_box", (void *)box);
    
    if(bordersize != -1)
        dw_window_set_border(ret, bordersize);
    
    if(hgravity || vgravity)
      dw_window_set_gravity(ret, hgravity, vgravity);
      
    return ret;
}

/*
 * Loads a window with the specified name from an XML tree.
 * Parameters:
 *       handle: A handle to an XML tree.
 *       name: The name of the window to load.
 * Returns:
 *       A handle to a top-level window or NULL on failure.
 */
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
                HWND window = _dwib_window_create(p, doc);
                _dwib_children(p, doc, window, (HWND)dw_window_get_data(window, "_dwib_box"), TRUE);
                return window;
            }
        }
    }
    return 0;
}

int _dwib_check_dataname(xmlNodePtr node, xmlDocPtr doc, char *dataname)
{
    char *thisval = NULL;
    xmlNodePtr this;

    if((this = _dwib_find_child(node, "dataname")) && 
        (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) &&
        (strcmp(dataname, thisval) == 0))
        return TRUE;
    return FALSE;
}

/* Internal function fo parsing the children of packable widgets... boxes, notebook pages, etc */
int _dwib_children_search(xmlNodePtr node, xmlDocPtr doc, HWND window, char *dataname, HWND box, int index)
{
    xmlNodePtr p = _dwib_find_child(node, "Children");
    int retval = DW_ERROR_GENERAL;
    
    for(p=p->children;p;p = p->next)
    {
        if(_dwib_check_dataname(p, doc, dataname))
        {
            _dwib_child(doc, window, box, FALSE, p, 0, index);
            return DW_ERROR_NONE;
        }
        else
        {
            if(strcmp((char *)p->name, "Box") == 0)
            {
                retval = _dwib_children_search(p, doc, window, dataname, box, index);
            }
            else if(strcmp((char *)p->name, "Notebook") == 0)
            {
                retval = _dwib_children_search(p, doc, window, dataname, box, index);
            }
            else if(strcmp((char *)p->name, "NotebookPage") == 0)
            {
                retval = _dwib_children_search(p, doc, window, dataname, box, index);
            }
        }
        /* If we found the dataname... drop out of the loop */
        if(retval == DW_ERROR_NONE)
            return retval;
    }
    return retval;
}

/*
 * Loads a part of a window layout specified by dataname with the specified window name from an XML tree and packs it into box at index.
 * Parameters:
 *       handle: A handle to an XML tree.
 *       name: The name of the window to load.
 *       dataname: Data name of the item to load and pack.
 *       window: Top-level window handle to set the data on.
 *       box: Handle to the box to insert the layout into.
 *       index: Index in the box to insert the layout into.
 * Returns:
 *       DW_ERROR_GENERAL on error or DW_ERROR_NONE on success.
 */
int API dwib_load_at_index(DWIB handle, char *name, char *dataname, HWND window, HWND box, int index)
{
    xmlDocPtr doc = handle;
    xmlNodePtr p, rootNode = xmlDocGetRootElement(doc);
    
    if(!rootNode)
        return DW_ERROR_GENERAL;
    
    for(p=rootNode->children;p;p = p->next)
    {
        if(strcmp((char *)p->name, "Window") == 0)
        {
            xmlNodePtr this = _dwib_find_child(p, "title");
            char *val = (char *)xmlNodeListGetString(doc, this->children, 1);
            
            if(val && strcmp(name, val) == 0)
            {
                return _dwib_children_search(p, doc, window, dataname, box, index);
            }
        }
    }
    return DW_ERROR_GENERAL;
}

/*
 * Shows a window loaded with dwib_load() using the store settings.
 * Parameters:
 *       window: A top-level window handle created with dwib_load().
 */
void API dwib_show(HWND window)
{
    int x, y, width, height, hgravity, vgravity;
    
    /* Get the loaded window settings set on the window handle */
    x = DW_POINTER_TO_INT(dw_window_get_data(window, "_dwib_x"));
    y = DW_POINTER_TO_INT(dw_window_get_data(window, "_dwib_y"));
    width = DW_POINTER_TO_INT(dw_window_get_data(window, "_dwib_width"));
    height = DW_POINTER_TO_INT(dw_window_get_data(window, "_dwib_height"));
    hgravity = DW_POINTER_TO_INT(dw_window_get_data(window, "_dw_grav_horz"));
    vgravity = DW_POINTER_TO_INT(dw_window_get_data(window, "_dw_grav_vert"));

    /* Set size and/or position if possible...
     * Position needs to be greater than -1 or
     * gravity needs to be set to CENTER.
     */
    if(width >= 0 && height >= 0 && 
      (x >= 0 || (hgravity & 0xf) == DW_GRAV_CENTER) && 
      (y >= 0 || (vgravity & 0xf) == DW_GRAV_CENTER))
        dw_window_set_pos_size(window, x, y, width, height);
    else if (width >= 0 && height >= 0)
        dw_window_set_size(window, width, height);
    else if(x >= 0 && y >= 0)
        dw_window_set_pos(window, x, y);
    
    /* Finally show the window */
    dw_window_show(window);
}

/*
 * Loads an XML templates and returns a handle to the XML tree.
 * Parameters:
 *       buffer: Memory buffer containing the XML template data.
 *       size: Length in bytes of the memory buffer.
 * Returns:
 *       A handle to an XML tree or NULL on failure.
 */
DWIB API dwib_open_from_data(char *buffer, int size)
{
    return xmlParseMemory(buffer, size);
}

/*
 * Loads an XML templates and returns a handle to the XML tree.
 * Parameters:
 *       filename: Name of file containing the XML template data.
 * Returns:
 *       A handle to an XML tree or NULL on failure.
 */
DWIB API dwib_open(char *filename)
{
    return xmlParseFile(filename);
}

/*
 * Closes a handle to an XML tree returned by dwib_open*() and
 * frees the memory associated with the tree.
 * Parameters:
 *       handle: A handle to an XML tree.
 */
void API dwib_close(DWIB handle)
{
    xmlDocPtr doc = handle;
    xmlFreeDoc(doc);
}

/*
 * Update the location of the image root for locating image files.
 * Parameters:
 *       path: Directory to be used relative to image file location.
 * Returns:
 *       DW_ERROR_NONE if found was found and selected, or DW_ERROR_GENERAL.
 */
int API dwib_image_root_set(char *path)
{
    char *oldroot = _dwib_image_root;
    
    /* TODO: Make sure the path exists and fail if it doesn't */
    if(path && *path)
        _dwib_image_root = strdup(path);
    else
        _dwib_image_root = NULL;
    
    if(oldroot)
        free(oldroot);
        
    return DW_ERROR_NONE;
}

/*
 * Update the locale used when identifying locating strings during creation.
 * Parameters:
 *       loc: String locale identifier, such as "en_US"
 * Returns:
 *       DW_ERROR_NONE if locale was found and selected, or DW_ERROR_GENERAL.
 */
int API dwib_locale_set(char *loc)
{
    return DW_ERROR_GENERAL;
}
