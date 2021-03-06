/* Dynamic Windows Application: Interface Builder
 * Created by Dynamic Windows Interface Builder
 * Author: Brian Smith
 */

#include "dw.h"
#include "dwcompat.h"
#include <libxml/tree.h>
#include <string.h>
#include <ctype.h>
#include "dwib.h"
#include "dwib_int.h"
#include "resources.h"

static void *_dwib_builder = NULL;
char *_dwib_image_root = NULL;
char *_dwib_locale = NULL;

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

/* Internal function to check if a node has a locale property
 * for the current locale and return that, or return the default.
 */
char * _dwib_get_locale_string(xmlNodePtr node, xmlDocPtr doc)
{
    if(_dwib_locale)
    {
        xmlAttrPtr att = xmlHasProp(node, (xmlChar *)_dwib_locale);
        xmlChar *result;
        
        if(att && (result = xmlGetProp(node, (xmlChar *)_dwib_locale)))
            return (char *)result;
    }
    return (char *)xmlNodeListGetString(doc, node->children, 1);
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
    {
        width = atoi(thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "height")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        height = atoi(thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "hexpand")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        hexpand = atoi(thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "vexpand")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        vexpand = atoi(thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "padding")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        padding = atoi(thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "enabled")) && (thisval = _dwib_get_locale_string(this, doc)))
    {
        if(atoi(thisval) == 0)
            dw_window_disable(item);
        xmlFree(thisval);
    }
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
    if((this || (this = _dwib_find_child(node, "font"))) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Default"))
            dw_window_set_font(item, thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "tooltip")) && (thisval = _dwib_get_locale_string(this, doc)))
    {
        if(*thisval)
            dw_window_set_tooltip(item, thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "fcolor")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        fcolor = _dwib_get_color(thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "bcolor")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        bcolor = _dwib_get_color(thisval);
        xmlFree(thisval);
    }

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

    if(dataname)
    {
        if(window && *dataname)
            dw_window_set_data(window, dataname, (void *)item);
        xmlFree(dataname);
    }
}

/* Internal function for creating a notebook page widget from an XML tree node */
HWND _dwib_notebook_page_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox)
{
    HWND box;
    xmlNodePtr this = _dwib_find_child(node, "statustext");
    char *thisval;
    int flags = 0, orient = DW_HORZ;
    unsigned long pageid = dw_notebook_page_new(packbox, flags, FALSE);

    if((thisval = _dwib_get_locale_string(this, doc)))
    {
       dw_notebook_page_set_status_text(packbox, pageid, thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "orientation")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval) || strcmp(thisval, "Vertical") == 0)
            orient = DW_VERT;
        xmlFree(thisval);
    }

    box = dw_box_new(orient, 0);
    dw_notebook_pack(packbox, pageid, box);
    /* Save the box handle in the psvi field */
    node->psvi = DW_POINTER(box);
    /* Save the page ID in the box data */
    dw_window_set_data(box, "_dwib_pageid", DW_INT_TO_POINTER(pageid));

    if((this = _dwib_find_child(node, "title")) && (thisval = _dwib_get_locale_string(this, doc)))
    {
        dw_notebook_page_set_text(packbox, pageid, thisval);
        xmlFree(thisval);
    }
    return box;
}

/* Internal function for creating a notebook widget from an XML tree node */
HWND _dwib_notebook_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND notebook;
    xmlNodePtr this = _dwib_find_child(node, "position");
    char *thisval;
    int top = 1;

    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Bottom") == 0)
            top = 0;
        xmlFree(thisval);
    }

    notebook = dw_notebook_new(0, top);

    _dwib_item_pack(node, doc, window, packbox, notebook, index);
    return notebook;
}

/* Internal function for creating a box widget from an XML tree node */
HWND _dwib_box_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND box = 0, box1, box2;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval, *title = NULL;
    int orient = DW_HORZ, padding = 0, type = 0, splitper = 50;

    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Group") == 0)
            type = 1;
        else if(strcmp(thisval, "Scroll") == 0)
            type = 2;
        else if(strcmp(thisval, "Splitbar") == 0)
            type = 3;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "title")) && (thisval = _dwib_get_locale_string(this, doc)))
        title = thisval;
    if((this = _dwib_find_child(node, "orientation")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if((atoi(thisval) || strcmp(thisval, "Vertical") == 0))
            orient = DW_VERT;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "splitper")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) && atoi(thisval))
    {
        splitper = atoi(thisval);
        xmlFree(thisval);
    }

    switch(type)
    {
        case 0:
            box = dw_box_new(orient, padding);
            break;
        case 1:
            box = dw_groupbox_new(orient, padding, title ? title : "");
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
    if(title)
        xmlFree(title);
    return box;
}

static unsigned char map1[65] = {0};
static unsigned char map2[129] = {0};

/* Generate the two translation tables */
void _dwib_generate_table(void)
{
    int i = 0;
    unsigned char c;
    
    for(c='A'; c<='Z'; c++) 
        map1[i++] = c;
    for(c='a'; c<='z'; c++) 
        map1[i++] = c;
    for(c='0'; c<='9'; c++) 
        map1[i++] = c;
    map1[i++] = '+'; 
    map1[i++] = '/';
    
    for(i=0; i<128; i++) 
        map2[i] = -1;
    for(i=0; i<64; i++) 
        map2[map1[i]] = (unsigned char)i;
}

/* Decode base64 encoded text buffer */
char *_dwib_decode64(unsigned char *in, int iOff, int iLen, int *oLen) 
{
    int ip = iOff;
    int iEnd = iOff + iLen;
    int op = 0;
    unsigned char *out;
    
    /* Initialize table first time */
    if(!map1[0])
        _dwib_generate_table();
        
    /* Some sanity checks */
    if(iLen%4 != 0)
        return NULL;
    
    /* Strip off trailing padding */
    while(iLen > 0 && in[iOff+iLen-1] == '=') 
        iLen--;
    
    /* Calculate the output size */
    *oLen = (iLen*3) / 4;
 
    /* Allocate the output buffer */
    out = calloc(1, *oLen);
    
    while (ip < iEnd) 
    {
        int i0 = in[ip++];
        int i1 = in[ip++];
        int i2 = ip < iEnd ? in[ip++] : 'A';
        int i3 = ip < iEnd ? in[ip++] : 'A';
        int b0, b1, b2, b3, o0, o1, o2;
        
        /* Illegal character in Base64 encoded data. */
        if (i0 > 127 || i1 > 127 || i2 > 127 || i3 > 127)
        {
            free(out);
            return NULL;
        }
        /* Use the precalculated table to decode */
        b0 = map2[i0];
        b1 = map2[i1];
        b2 = map2[i2];
        b3 = map2[i3];
        
        /* Calculate the three bytes of the triplet */
        o0 = ( b0 <<2) | (b1>>4);
        o1 = ((b1 & 0xf)<<4) | (b2>>2);
        o2 = ((b2 & 3)<<6) | b3;
        
        /* Save the triplet in the output buffer */
        out[op++] = (unsigned char)o0;
        if(op<(*oLen)) 
            out[op++] = (unsigned char)o1;
        if(op<(*oLen)) 
            out[op++] = (unsigned char)o2; 
    }
    return (char *)out; 
}
    
/* Takes base64 encoded and line split input...
 * removes whitespace and then decodes into the
 * original binary representation. 
 */
char *_dwib_decode64_lines(char *raw, int *length) 
{
    /* Allocate the temporary representation off the stack */
    char *buf = alloca((*length) + 1);
    int ip, p = 0;
    
    /* Zero out the buffer */
    memset(buf, 0, (*length) + 1);
    
    /* Loop through the entire buffer */
    for(ip=0; ip<(*length); ip++) 
    {
        /* Remove spaces, return, newline and tab */
        if(raw[ip] != ' ' && raw[ip] != '\r' && raw[ip] != '\n' && raw[ip] != '\t')
            buf[p++] = raw[ip]; 
    }
    /* decode64() will return a malloc()ed buffer */
    return _dwib_decode64((unsigned char *)buf, 0, p, length); 
}


/* Locate the internal image based on resource ID and 
 * return the correct path to it... if the image ID 
 * was not found in the list, return a placeholder ID.
 */
char *_dwib_builder_bitmap(int *resid, xmlDocPtr doc, int *length)
{
    xmlNodePtr this = xmlDocGetRootElement(doc)->children;
    
    while(this)
    {
        if(this->name && strcmp((char *)this->name, "Image") == 0)
        {
            xmlNodePtr node = _dwib_find_child(this, "ImageID");
            char *val, *file = (char *)xmlNodeListGetString(doc, this->children, 1), *origfile = file;
            struct dwstat st;
            int iid = 0;
            
            /* Load the Icon ID if available */
            if(node && (val = (char *)xmlNodeListGetString(doc, node->children, 1)) != NULL)
            {
                iid = atoi(val);
                xmlFree(val);
            }
           
            /* Found an image with the correct resource ID */
            if(iid == *resid)
            {
                /* Generate a path to the file */
                int len = _dwib_image_root ? (int)strlen(_dwib_image_root) : 0;
                char *freeme = NULL;
                
                if((node = _dwib_find_child(this, "Embedded")) != NULL &&
                   (val = (char *)xmlNodeListGetString(doc, node->children, 1)) != NULL)
                {
                    char *data;
                    
                    *length = (int)strlen(val);
                    
                    /* Attempt to decode embedded data */
                    if((data = _dwib_decode64_lines(val, length)) != NULL)
                    {
                        /* Don't reset the resource ID to 0 when 
                         * returning embedded data.
                         */
                        xmlFree(val);
                        return data;
                    }
                    xmlFree(val);
                }
            
                if(len)
                    freeme = file = _dwib_combine_path(len, file, malloc(len + strlen(file) + 2));
                else
                    freeme = file = strdup(file);
                    
                if(origfile)
                    xmlFree(origfile);
                    
                if(stat(file, &st) == 0)
                {
                    /* Found an image... set the resource ID to 0
                     * and return the path to the image file.
                     */
                    *resid = 0;
                    return file;
                }

                if(freeme)
                    free(freeme);
            }
        }
        this=this->next;
    }
    /* Didn't find an image return placeholder in builder mode */
    if(_dwib_builder)
        *resid = BITMAP_PLACEHOLD;
    return NULL;
}

/* Internal function for creating a button widget from an XML tree node */
HWND _dwib_button_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND button = 0;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval, *setting = NULL, *origsetting = setting;
    int type = 0;

    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Bitmap") == 0)
            type = 1;
        else if(strcmp(thisval, "Check") == 0)
            type = 2;
        else if(strcmp(thisval, "Radio") == 0)
            type = 3;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "setting")) && (thisval = _dwib_get_locale_string(this, doc)))
        setting = thisval;

    switch(type)
    {
        case 0:
            button = dw_button_new(setting ? setting : "", 0);
            break;
        case 1:
            {
                struct dwstat st;
                int resid = setting ? atoi(setting) : 0;
                int length = 0;
                char *freeme = NULL;

                /* If a resource ID wasn't specified... 
                 * check to see if it is a file that exists...
                 */
                if(!resid && setting && *setting && stat(setting, &st) != 0)
                {
                    int len = _dwib_image_root ? (int)strlen(_dwib_image_root) : 0;
                    
                    if(len)
                        setting = _dwib_combine_path(len, setting, alloca(len + strlen(setting) + 2));
                }
                else if(resid)
                    freeme = setting = _dwib_builder_bitmap(&resid, doc, &length);

                /* If we have resource ID, setting data and a length it is embedded */
                if(resid && setting && length > 0)
                    button = dw_bitmapbutton_new_from_data(NULL, resid, setting, length);
                /* Just the resource ID, then it is a resource */
                else if(resid)
                    button = dw_bitmapbutton_new(NULL, resid);
                /* Otherwise try to load it from file */
                else if(setting)
                    button = dw_bitmapbutton_new_from_file(NULL, 0, setting);
                    
                if(freeme)
                    free(freeme);
            }
            break;
        case 2:
            button = dw_checkbox_new(setting ? setting : "", 0);
            break;
        case 3:
            button = dw_radiobutton_new(setting ? setting : "", 0);
            break;
    }

    if(button)
    {
        if((this = _dwib_find_child(node, "check")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        {        
            if(atoi(thisval))
                dw_checkbox_set(button, TRUE);
            xmlFree(thisval);
        }
        if((this = _dwib_find_child(node, "borderless")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        {
            if(atoi(thisval))
                dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
            xmlFree(thisval);
        }    

        _dwib_item_pack(node, doc, window, packbox, button, index);
    }
    if(origsetting)
        xmlFree(origsetting);
    return button;
}

/* Internal function for creating a text widget from an XML tree node */
HWND _dwib_text_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND text = 0;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval, *label = NULL;
    int type = 0, flags = 0;

    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Status") == 0)
            type = 1;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "label")) && (thisval = _dwib_get_locale_string(this, doc)))
        label = thisval;
    if((this = _dwib_find_child(node, "alignment")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Center") == 0)
            flags = DW_DT_CENTER;
        else if(strcmp(thisval, "Right") == 0)
            flags = DW_DT_RIGHT;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "valignment")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Center") == 0)
            flags |= DW_DT_VCENTER;
        else if(strcmp(thisval, "Bottom") == 0)
            flags |= DW_DT_BOTTOM;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "wordwrap")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_DT_WORDBREAK;
        xmlFree(thisval);
    }

    switch(type)
    {
        case 0:
            text = dw_text_new(label ? label : "", 0);
            break;
        case 1:
            text = dw_status_text_new(label ? label : "", 0);
            break;
    }
    if(text)
        _dwib_item_pack(node, doc, window, packbox, text, index);
    if(flags)
        dw_window_set_style(text, flags, flags);
    if(label)
        xmlFree(label);
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
                char *thisval = _dwib_get_locale_string(p, doc);
                char *coltype = (char *)xmlGetProp(p, (xmlChar *)"ColType");
                
                if((thisval && *thisval) || (coltype && strcmp(coltype, "Icon") == 0))
                   count++;
                if(coltype)
                    xmlFree(coltype);
                if(thisval)
                    xmlFree(thisval);
            }
        }

        if(count > 0)
        {
            char **colnames = calloc(sizeof(char *), count);
            unsigned long *colflags = calloc(sizeof(unsigned long), count);
            int x;

            count = 0;

            for(p=node->children;p;p = p->next)
            {
                if(strcmp((char *)p->name, "Item") == 0)
                {
                    char *coltype = (char *)xmlGetProp(p, (xmlChar *)"ColType");
                    char *colalign = (char *)xmlGetProp(p, (xmlChar *)"ColAlign");
                    char *thisval = _dwib_get_locale_string(p, doc);

                    if((thisval && *thisval) || (coltype && strcmp(coltype, "Icon") == 0))
                    {
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
                        colnames[count] = thisval ? thisval : (char *)xmlStrdup((xmlChar *)"");
                        colflags[count] = ctype | calign | DW_CFA_SEPARATOR;
                        count++;
                    }
                    else
                        xmlFree(thisval);
                    if(coltype)
                        xmlFree(coltype);
                    if(colalign)
                        xmlFree(colalign);
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
            for(x=0;x<count;x++)
            {
                if(colnames[x])
                    xmlFree(colnames[x]);
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
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "multi")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        multi = atoi(thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "splitcol")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        splitcol = atoi(thisval);
        xmlFree(thisval);
    }

    container = dw_container_new(0, multi);

    /* Attempt to get localized column title for the filesystem sub-type */
    if(type == 1 && (this = _dwib_find_child(node, "coltitle")) && (thisval = _dwib_get_locale_string(this, doc)))
    {
        if(*thisval)
            dw_filesystem_set_column_title(container, thisval);
        xmlFree(thisval);
    }
    
    /* If we have columns set them up */
    if((this = _dwib_find_child(node, "Columns")))
        _dwib_populate_container(container, this, doc, type, splitcol);

    if((this = _dwib_find_child(node, "oddcolor")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        oddcolor = _dwib_get_color(thisval);
        xmlFree(thisval);
    }
   if((this = _dwib_find_child(node, "evencolor")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
   {
        evencolor = _dwib_get_color(thisval);
        xmlFree(thisval);
    }

    if(oddcolor != DW_RGB_TRANSPARENT || evencolor != DW_RGB_TRANSPARENT)
        dw_container_set_stripe(container, oddcolor, evencolor);

    if((this = _dwib_find_child(node, "idstring")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            dw_window_set_data(container, "_dw_textcomp", DW_INT_TO_POINTER(1));
        xmlFree(thisval);
    }
        
    _dwib_item_pack(node, doc, window, packbox, container, index);
    return container;
}

/* Internal function for creating a ranged widget from an XML tree node */
HWND _dwib_ranged_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND ranged = 0;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    int type = 0, upper = 100, lower = 0, position = 0, orient = DW_HORZ;
    char *thisval, *pos = NULL;

    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Slider") == 0)
            type = 1;
        if(strcmp(thisval, "Scrollbar") == 0)
            type = 2;
        if(strcmp(thisval, "Spinbutton") == 0)
            type = 3;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "position")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        pos = thisval;
        position = atoi(pos);
    }
    if((this = _dwib_find_child(node, "orientation")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if((atoi(thisval) || strcmp(thisval, "Vertical") == 0))
            orient = DW_VERT;
        xmlFree(thisval);
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
            ranged = dw_spinbutton_new(pos ? pos : "", 0);
            dw_spinbutton_set_limits(ranged, upper, lower);
            break;

    }
    if(ranged)
        _dwib_item_pack(node, doc, window, packbox, ranged, index);
    if(pos)
        xmlFree(pos);
    return ranged;
}

/* Internal function for creating an entryfield widget from an XML tree node */
HWND _dwib_entryfield_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND entryfield = 0;
    xmlNodePtr this = _dwib_find_child(node, "subtype");
    char *thisval, *deftext = NULL;
    int type = 0;

    if((thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Password") == 0)
            type = 1;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "deftext")) && (thisval = _dwib_get_locale_string(this, doc)))
        deftext = thisval;

    switch(type)
    {
        case 0:
            entryfield = dw_entryfield_new(deftext ? deftext : "", 0);
            break;
        case 1:
            entryfield = dw_entryfield_password_new(deftext ? deftext : "", 0);
            break;
    }

    if((this = _dwib_find_child(node, "deftext")) && (thisval = _dwib_get_locale_string(this, doc)))
    {
        dw_entryfield_set_limit(entryfield, atoi(thisval));
        xmlFree(thisval);
    }

    if(entryfield)
        _dwib_item_pack(node, doc, window, packbox, entryfield, index);
    if(deftext)
        xmlFree(deftext);
    return entryfield;
}

/* Fills in a listbox/combobox with items from the List node */
void _dwib_populate_list(HWND list, xmlNodePtr node, xmlDocPtr doc, int locale)
{
    xmlNodePtr p;

    for(p=node->children;p;p = p->next)
    {
        if(strcmp((char *)p->name, "Item") == 0)
        {
            char *thisval = locale ? _dwib_get_locale_string(p, doc) : (char *)xmlNodeListGetString(doc, p->children, 1);

            if(thisval)
            {
                dw_listbox_append(list, thisval);
                xmlFree(thisval);
            }
        }
    }
}

/* Internal function for creating a combobox widget from an XML tree node */
HWND _dwib_combobox_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND combobox;
    xmlNodePtr this = _dwib_find_child(node, "deftext");
    char *thisval, *deftext = NULL;

    if((thisval = _dwib_get_locale_string(this, doc)))
        deftext = thisval;

    combobox = dw_combobox_new(deftext ? deftext : "", 0);

    _dwib_item_pack(node, doc, window, packbox, combobox, index);

    if((this = _dwib_find_child(node, "List")))
        _dwib_populate_list(combobox, this, doc, TRUE);

    if(deftext)
        xmlFree(deftext);
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
    char *thisval, *deftext = NULL;

    if((thisval = _dwib_get_locale_string(this, doc)))
    {
        deftext = thisval;
    }

    mle = dw_mle_new(0);
    dw_mle_import(mle, deftext ? deftext : "", -1);

    if((this = _dwib_find_child(node, "wordwrap")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        dw_mle_set_word_wrap(mle, atoi(thisval));
        xmlFree(thisval);
    }

    _dwib_item_pack(node, doc, window, packbox, mle, index);
   
    if(deftext)
        xmlFree(deftext);
    return mle;
}

/* Internal function for creating a html widget from an XML tree node */
HWND _dwib_html_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HWND packbox, int index)
{
    HWND html;
    xmlNodePtr this = _dwib_find_child(node, "URL");
    char *thisval;

    html = dw_html_new(0);

    if(html)
    {
        if((thisval = _dwib_get_locale_string(this, doc)))
        {
            dw_html_url(html, thisval);
            xmlFree(thisval);
        }
    }
    else if(_dwib_builder)
    {
        /* Some platforms currently don't support the HTML widget... or 
         * can be compiled with it turned off... in this case make sure 
         * to tell them that there would be something here.
         */
        html = dw_text_new("This platform does not support the HTML widget.", 0);
        dw_window_set_style(html, DW_DT_CENTER | DW_DT_VCENTER, DW_DT_CENTER | DW_DT_VCENTER);
    }
    
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
    char *thisval, *setting = "", *freeme = NULL, *origsetting = NULL;
    int resid = 0, length = 0;

    if((thisval = _dwib_get_locale_string(this, doc)))
    {
        struct dwstat st;
        
        origsetting = setting = thisval;
        resid = atoi(setting);
        
        /* If a resource ID wasn't specified... 
         * check to see if it is a file that exists...
         */
        if(!resid && setting && *setting && stat(setting, &st) != 0)
        {
            int len = _dwib_image_root ? (int)strlen(_dwib_image_root) : 0;
            
            if(len)
                setting = _dwib_combine_path(len, setting, alloca(len + strlen(setting) + 2));
        }
        else if(resid)
            freeme = setting = _dwib_builder_bitmap(&resid, doc, &length);
    }

    bitmap = dw_bitmap_new(resid);
    /* If we have resource ID, setting data and a length it is embedded */
    if(resid && setting && length > 0)
        dw_window_set_bitmap_from_data(bitmap, resid, setting, length);
    /* Just the resource ID, then it is a resource */
    else if(resid)
        dw_window_set_bitmap(bitmap, resid, NULL);
    /* Otherwise try to load it from file */
    else if(setting && *setting)
        dw_window_set_bitmap(bitmap, resid, setting);

    _dwib_item_pack(node, doc, window, packbox, bitmap, index);
    
    /* Free memory allocated */
    if(freeme)
        free(freeme);
    if(origsetting)
        xmlFree(origsetting);
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
    {
        multi = atoi(thisval);
        xmlFree(thisval);
    }

    listbox = dw_listbox_new(0, multi);

    if((this = _dwib_find_child(node, "List")))
        _dwib_populate_list(listbox, this, doc, TRUE);

    _dwib_item_pack(node, doc, window, packbox, listbox, index);

    return listbox;
}

/* Internal function for creating a menu widget from an XML tree node */
HWND _dwib_menu_create(xmlNodePtr node, xmlDocPtr doc, HWND window, HMENUI packbox, HMENUI submenu)
{
    HWND menuitem;
    xmlNodePtr this = _dwib_find_child(node, "title");
    char *thisval, *title = NULL, *dataname = NULL;
    int flags = 0, checkable = 0, menuid = 0;

    if((thisval = _dwib_get_locale_string(this, doc)))
        title = thisval;
    if((this = _dwib_find_child(node, "checkable")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        checkable = atoi(thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "menuid")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        menuid = atoi(thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "checked")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_MIS_CHECKED;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "enabled")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        flags |= (atoi(thisval) ? DW_MIS_ENABLED : DW_MIS_DISABLED);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "dataname")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
        dataname = thisval;

    menuitem = dw_menu_append_item(packbox, title ? title : "", menuid, flags, TRUE, checkable, submenu);
    /* Save the menu item handle in the psvi field */
    node->psvi = (void *)menuitem;

    if(dataname)
    {
        if(window && *dataname)
            dw_window_set_data(window, dataname, (void *)menuitem);
        xmlFree(dataname);
    }

    if(title)
        xmlFree(title);
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

    if(p)
    {
       for(p=p->children;p;p = p->next)
       {
           menu = _dwib_child(doc, window, box, windowlevel, p, menu, 65536);
       }
    }
    return menu;
}

/* Internal function for creating a window from an XML tree node */
HWND _dwib_window_create(xmlNodePtr node, xmlDocPtr doc)
{
    xmlNodePtr this = _dwib_find_child(node, "title");
    char *thisval, *title = "Preview: ";
    unsigned long flags = 0;
    int bordersize = -1, orient = DW_HORZ, padding = 0;
    int x = -1, y = -1, width = -1, height = -1, hgravity = 0, vgravity = 0;
    HWND ret, box;

    if((thisval = _dwib_get_locale_string(this, doc)))
    {
        if(_dwib_builder)
        {
            char *buf = alloca(strlen(thisval) + strlen(title) + 1);
            strcpy(buf, title);
            strcat(buf, thisval);
            title = buf;
        }
        else 
        {
            title = alloca(strlen(thisval) + 1);
            strcpy(title, thisval);
        }
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "bordersize")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        bordersize = atoi(thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "close")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_FCF_CLOSEBUTTON;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "minimize")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_FCF_MINBUTTON;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "maximize")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_FCF_MAXBUTTON;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "hide")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_FCF_HIDEBUTTON;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "resize")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_FCF_SIZEBORDER;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "dialog")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_FCF_DLGBORDER;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "border")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_FCF_BORDER;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "sysmenu")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_FCF_SYSMENU;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "tasklist")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_FCF_TASKLIST;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "composited")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_FCF_COMPOSITED;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "textured")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_FCF_TEXTURED;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "titlebar")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            flags |= DW_FCF_TITLEBAR;
        xmlFree(thisval);
    }

    if((this = _dwib_find_child(node, "x")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        x = atoi(thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "y")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        y = atoi(thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "hgravity")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Center") == 0)
            hgravity = DW_GRAV_CENTER;
        else if(strcmp(thisval, "Right") == 0)
            hgravity = DW_GRAV_RIGHT;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "hobstacles")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            hgravity |= DW_GRAV_OBSTACLES;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "vgravity")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(strcmp(thisval, "Center") == 0)
            vgravity = DW_GRAV_CENTER;
        else if(strcmp(thisval, "Bottom") == 0)
            vgravity = DW_GRAV_BOTTOM;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "vobstacles")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if(atoi(thisval))
            vgravity |= DW_GRAV_OBSTACLES;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "width")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        width = atoi(thisval);
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "height")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        height = atoi(thisval);
        xmlFree(thisval);
    }

    if((this = _dwib_find_child(node, "orientation")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        if((atoi(thisval) || strcmp(thisval, "Vertical") == 0))
            orient = DW_VERT;
        xmlFree(thisval);
    }
    if((this = _dwib_find_child(node, "padding")) && (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
    {
        padding = atoi(thisval);
        xmlFree(thisval);
    }

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

int _dwib_check_dataname(xmlNodePtr node, xmlDocPtr doc, const char *dataname)
{
    char *thisval = NULL;
    xmlNodePtr this;
    int retval = FALSE;

    if((this = _dwib_find_child(node, "dataname")) &&
        (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)) &&
        (strcmp(dataname, thisval) == 0))
            retval = TRUE;
    if(thisval)
        xmlFree(thisval);
    return retval;
}

/* Internal function fo parsing the children of packable widgets... boxes, notebook pages, etc */
int _dwib_children_search(xmlNodePtr node, xmlDocPtr doc, HWND window, const char *dataname, HWND box, int index)
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

/* Internal function that handles setting click defaults */
void _dwib_focus_child(HWND window, xmlNodePtr p, xmlDocPtr doc)
{
    if(strcmp((char *)p->name, "Box") == 0 ||
       strcmp((char *)p->name, "Notebook") == 0 ||
       strcmp((char *)p->name, "NotebookPage") == 0)
    {
        _dwib_focus_children(window, p, doc);
    }
    else if(strcmp((char *)p->name, "Entryfield") == 0 ||
            strcmp((char *)p->name, "Combobox") == 0 )
    {
        if(p->psvi)
        {
            char *thisval = NULL;
            xmlNodePtr this;
            HWND handle = 0;
            
            if((this = _dwib_find_child(p, "clickdefault")) &&
               (thisval = (char *)xmlNodeListGetString(doc, this->children, 1)))
                handle = dwib_window_get_handle(window, thisval);
            if(handle)
                dw_window_click_default((HWND)p->psvi, handle);
            if(thisval)
                xmlFree(thisval);
        }
    }
}

/* Internal function fo parsing the children of packable widgets... boxes, notebook pages, etc */
void _dwib_focus_children(HWND window, xmlNodePtr node, xmlDocPtr doc)
{
    xmlNodePtr p = _dwib_find_child(node, "Children");
    
    if(p)
    {
        for(p=p->children;p;p = p->next)
        {
            _dwib_focus_child(window, p, doc);
        }
    }
}


/* Internal helper function to combine two paths */
char *_dwib_combine_path(int len, char *val, char *file)
{
    strcpy(file, _dwib_image_root);
    if(_dwib_image_root[len] != '/' && _dwib_image_root[len] != '\\')
        strcat(file, DIRSEP);
    strcat(file, val);
    return file;
}

/*
 * Loads a window with the specified name from an XML tree.
 * Parameters:
 *       handle: A handle to an XML tree.
 *       name: The name of the window to load.
 * Returns:
 *       A handle to a top-level window or NULL on failure.
 */
HWND API dwib_load(DWIB handle, const char *name)
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
                char *thisval = NULL;
                xmlNodePtr focus;
                HWND handle = 0;
                
                _dwib_children(p, doc, window, (HWND)dw_window_get_data(window, "_dwib_box"), TRUE);
                
                /* Handle setting the default focus item */
                if((focus = _dwib_find_child(p, "default")) &&
                   (thisval = (char *)xmlNodeListGetString(doc, focus->children, 1)))
                    handle = dwib_window_get_handle(window, thisval);
                if(handle)
                    dw_window_default(window, handle);
                if(thisval)
                    xmlFree(thisval);
                /* Handle setting click defaults now that the window is created */
                _dwib_focus_children(window, p, doc);
                return window;
            }
        }
    }
    return 0;
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
int API dwib_load_at_index(DWIB handle, const char *name, const char *dataname, HWND window, HWND box, int index)
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

            if(val)
            {
                if(strcmp(name, val) == 0)
                {
                    xmlFree(val);
                    return _dwib_children_search(p, doc, window, dataname, box, index);
                }
                xmlFree(val);
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
DWIB API dwib_open_from_data(const char *buffer, int size)
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
DWIB API dwib_open(const char *filename)
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
int API dwib_image_root_set(const char *path)
{
    char *oldroot = _dwib_image_root;
    struct dwstat st;
        
    /* Make sure the path exists and fail if it doesn't */
    if(stat(path, &st) != 0)
        return DW_ERROR_GENERAL;
        
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
int API dwib_locale_set(const char *loc)
{
    char *oldlocale = _dwib_locale, *encode;
    
    /* Should we check that the locale is valid? */
    _dwib_locale = loc ? strdup(loc) : NULL;
    
    /* Trim off encoding string...
     * Often the LANG environment variable will be
     * en_US.UTF-8 which is <language>_<COUNTRY>.<ENCODING>
     * so trim off the .<ENCODING> to get the base...
     * since DW only supports UTF-8 it must be UTF-8 encoded.
     */
    if(_dwib_locale && (encode = strchr(_dwib_locale, '.')))
        *encode = 0;

    /* Free memory if needed */
    if(oldlocale)
        free(oldlocale);
        
    return DW_ERROR_NONE;
}

/*
 * Gets the window handle for a named widget. Effectively a wrapper for dw_window_get_data()
 * Parameters:
 *       handle: A handle to a window.
 *       name: The name of the widget who's handle is required.
 * Returns:
 *       A handle to a widget or NULL on failure.
 */
HWND API dwib_window_get_handle(HWND handle, const char *dataname)
{
    return (HWND)dw_window_get_data(handle,dataname);
}
