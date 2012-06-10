/* Dynamic Windows Application: Interface Builder
 * Created by Dynamic Windows Interface Builder
 * Author: Brian Smith
 */

#include <dw.h>
#include <dwcompat.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>
#include <string.h>
#include "resources.h"
#include "dwib_int.h"
#include "dwib.h"

HWND hwndToolbar, hwndProperties, hwndDefaultLocale, hwndImages = 0, hwndLocale = 0, hwndAbout = 0;
HMENUI menuLocale;
xmlDocPtr DWDoc;
xmlNodePtr DWCurrNode = NULL, DWClipNode = NULL;
char *DWFilename = NULL;
HICN hIcons[20];
HMENUI menuWindows;
int AutoExpand = FALSE, PropertiesInspector = TRUE, LivePreview = TRUE, BitmapButtons = TRUE;
extern char *_dwib_image_root;

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

/* Array to aid in automated configuration saving */
SaveConfig Config[] = 
{
    { "AUTOEXPAND",         TYPE_INT,   &AutoExpand },
    { "LIVEPREVIEW",        TYPE_INT,   &LivePreview },
    { "PROPINSP",           TYPE_INT,   &PropertiesInspector },
    { "BITMAPBUTTONS",      TYPE_INT,   &BitmapButtons },
    { "", 0, 0}
};

/* Write the ini file with all of the current settings */
void saveconfig(void)
{
    char *tmppath = INIDIR, *inidir, *inipath, *home = dw_user_dir(), *inifile = __TARGET__ ".ini";
    int x = 0;
    FILE *f;

    if(strcmp(INIDIR, ".") == 0)
    {
        inipath = strdup(inifile);
        inidir = strdup(INIDIR);
    }
    else
    {
        /* Need space for the filename, directory separator and NULL */
        int extra = (int)strlen(inifile) + 2;
        
        /* If the path is in the home directory... */
        if(home && tmppath[0] == '~')
        {
            /* Fill in both with the retrieved home directory */
            inipath = calloc(strlen(home) + strlen(INIDIR) + extra, 1);
            inidir = calloc(strlen(home) + strlen(INIDIR) + 1, 1);
            strcpy(inipath, home);
            strcpy(inidir, home);
            /* Append everything after the tilde */
            strcat(inipath, &tmppath[1]);
            strcat(inidir, &tmppath[1]);
        }
        else
        {
            /* Otherwise just copy the entire directory */
            inipath = calloc(strlen(INIDIR) + extra, 1);
            strcpy(inipath, INIDIR);
            inidir = strdup(INIDIR);
        }
        /* Add the separator and filename */
        strcat(inipath, DIRSEP);
        strcat(inipath, inifile);
    }

    /* Try to open the file for writing */
    f=fopen(inipath, FOPEN_WRITE_TEXT);

    /* If we couldn't open it... */
    if(f==NULL)
    {
        /*  If the ini direcotry isn't the current directory... */
        if(strcmp(INIDIR, ".") != 0)
        {
            /* Try to create the directory */
            mkdir(inidir,S_IRWXU);
            /* Then try to open it again */
            f=fopen(inipath, FOPEN_WRITE_TEXT);
        }
        /* If it still failed... */
        if(f==NULL)
        {
            /* Show an error message */
            dw_messagebox(APP_NAME, DW_MB_ERROR | DW_MB_OK, "Could not save settings. Inipath = \"%s\"", inipath);
            free(inipath);
            free(inidir);
            return;
        }
    }

    /* Free the temporary memory */
    free(inipath);
    free(inidir);

    /* Loop through all saveable settings */
    while(Config[x].type)
    {
        switch(Config[x].type)
        {
            /* Handle saving integers */
            case TYPE_INT:
            {
                int *var = (int *)Config[x].data;

                fprintf(f, "%s=%d\n", Config[x].name, *var);
                break;
            }
            /* Handle saving booleans */
            case TYPE_BOOLEAN:
            {
                int *var = (int *)Config[x].data;

                fprintf(f, "%s=%s\n", Config[x].name, *var ? "TRUE" : "FALSE");
                break;
            }
            /* Handle saving unsigned long integers */
            case TYPE_ULONG:
            {
                unsigned long *var = (unsigned long *)Config[x].data;

                fprintf(f, "%s=%lu\n", Config[x].name, *var);
                break;
            }
            /* Handle saving strings */
            case TYPE_STRING:
            {
                char **str = (char **)Config[x].data;

                fprintf(f, "%s=%s\n", Config[x].name, *str);
                break;
            }
            /* Handle saving floating point */
            case TYPE_FLOAT:
            {
                float *var = (float *)Config[x].data;

                fprintf(f, "%s=%f\n", Config[x].name, *var);
                break;
            }
        }
        x++;
    }

    fclose(f);
}

#define INI_BUFFER 256

/* Generic function to parse information from a config file */
void ini_getline(FILE *f, char *entry, char *entrydata)
{
    /* Allocate zeroed buffer from the stack */
    char in[INI_BUFFER] = { 0 };

    /* Try to read a line into the buffer */
    if(fgets(in, INI_BUFFER - 1, f))
    {
        int len = (int)strlen(in);

        /* Strip off any trailing newlines */
        if(len > 0 && in[len-1] == '\n')
            in[len-1] = 0;

        /* Skip over comment lines starting with # */
        if(in[0] != '#')
        {
            /* Locate = in the line */
            char *equalsign = strchr(in, '=');

            /* If the = was found... */
            if(equalsign)
            {
               /* Replace = with NULL terminator */
               *equalsign = 0;
               /* Copy before the = into entry */
               strcpy(entry, in);
               /* And after the = into entrydata */
               strcpy(entrydata, ++equalsign);
               return;
            }
        }
    }
    /* NULL terminate both variables */
    entrydata[0] = entry[0] = 0;
}

/* Load the ini file from disk setting all the necessary flags */
void loadconfig(void)
{
    char *tmppath = INIDIR, *inipath, *home = dw_user_dir(), *inifile = __TARGET__ ".ini";
    char entry[INI_BUFFER], entrydata[INI_BUFFER];
    FILE *f;

    if(strcmp(INIDIR, ".") == 0)
        inipath = strdup(inifile);
    else
    {
        /* Need space for the filename, directory separator and NULL */
        int extra = (int)strlen(inifile) + 2;
        
        /* If the path is in the home directory... */
        if(home && tmppath[0] == '~')
        {
            /* Fill it in with the retrieved home directory */
            inipath = calloc(strlen(home) + strlen(INIDIR) + extra, 1);
            strcpy(inipath, home);
            /* Append everything after the tilde */
            strcat(inipath, &tmppath[1]);
        }
        else
        {
            /* Otherwise just copy the entire directory */
            inipath = calloc(strlen(INIDIR) + extra, 1);
            strcpy(inipath, INIDIR);
        }
        /* Add the separator and filename */
        strcat(inipath, DIRSEP);
        strcat(inipath, inifile);
    }

    /* Try to open the file for reading */
    f = fopen(inipath, FOPEN_READ_TEXT);

    /* Free the temporary memory */
    free(inipath);

    /* If we successfully opened the ini file */
    if(f)
    {
        /* Loop through the file */
        while(!feof(f))
        {
            int x = 0;

            ini_getline(f, entry, entrydata);

            /* Cycle through the possible settings */
            while(Config[x].type)
            {
                /* If this line has a setting we are looking for */
                if(strcasecmp(entry, Config[x].name)==0)
                {
                    switch(Config[x].type)
                    {
                        /* Load an integer setting */
                        case TYPE_INT:
                        {
                            int *var = (int *)Config[x].data;

                            *var = atoi(entrydata);
                            break;
                        }
                        /* Load an boolean setting */
                        case TYPE_BOOLEAN:
                        {
                            int *var = (int *)Config[x].data;

                            if(strcasecmp(entrydata, "true")==0)
                                *var = TRUE;
                            else
                                *var = FALSE;
                            break;
                        }
                        /* Load an unsigned long integer setting */
                        case TYPE_ULONG:
                        {
                            unsigned long *var = (unsigned long *)Config[x].data;

                            sscanf(entrydata, "%lu", var);
                            break;
                        }
                        /* Load an string setting */
                        case TYPE_STRING:
                        {
                            char **str = (char **)Config[x].data;

                            *str = strdup(entrydata);
                            break;
                        }
                        /* Load an floating point setting */
                        case TYPE_FLOAT:
                        {
                            float *var = (float *)Config[x].data;

                            *var = atof(entrydata);
                            break;
                        }
                    }
                }
                x++;
            }
        }
        fclose(f);
    }
}

/* Draw something simple in the render area */
int DWSIGNAL render_expose(HWND window, DWExpose *exp, void *data)
{
    unsigned long width, height;
    int fontwidth, fontheight;
    char text[] = "DBSoft";
    
    dw_window_get_pos_size(window, NULL, NULL, &width, &height);
    dw_color_foreground_set(DW_CLR_WHITE);
    dw_draw_rect(window, 0, DW_DRAW_FILL, 0, 0, width, height);
    dw_color_foreground_set(DW_CLR_YELLOW);
    dw_draw_arc(window, 0, DW_DRAW_FILL | DW_DRAW_FULL, width/2, height/2, 0, 0, width, height);
    dw_color_foreground_set(DW_CLR_DARKBLUE);
    dw_font_text_extents_get(window, 0, text, &fontwidth, &fontheight);
    dw_draw_text(window, 0, (width - fontwidth)/2, (height - fontheight)/2, text);
    return FALSE;
}

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
    if(node->name &&
       (strcmp((char *)node->name, "Window") == 0 ||
       strcmp((char *)node->name, "Box") == 0 ||
       strcmp((char *)node->name, "NotebookPage") == 0))
    {
        return TRUE;
    }
    if(message)
    {
        dw_messagebox(APP_NAME, DW_MB_OK | DW_MB_ERROR, "Selected widget needs to be packable (window, box, splitbar, notebook page)");
    }
    return FALSE;
}

/* Returns TRUE if a menu is selected */
int is_menu(xmlNodePtr node)
{
    if(node->name && strcmp((char *)node->name, "Menu") == 0)
        return TRUE;
    return FALSE;
}

/* Returns TRUE if a menu is selected */
int is_notebook(xmlNodePtr node)
{
    if(node->name && strcmp((char *)node->name, "Notebook") == 0)
        return TRUE;
    return FALSE;
}

/* Check if this node or the parent is packable */
xmlNodePtr packableNode(xmlNodePtr node)
{
    if(node && is_packable(node, FALSE))
        return node;
    if(node && (node = node->parent) && is_packable(node, FALSE))
        return node;
    if(node && (node = node->parent) && is_packable(node, FALSE))
        return node;
    return NULL;
}

xmlNodePtr packablePageNode(xmlNodePtr node)
{
    if(node && node->name && strcmp((char *)node->name, "Notebook") == 0)
        return node;
    if(node && (node = node->parent) && node->name && strcmp((char *)node->name, "Notebook") == 0)
        return node;
    if(node && (node = node->parent) && node->name && strcmp((char *)node->name, "Notebook") == 0)
        return node;
    return NULL;
}

/* Check if this node or the parent is packable */
xmlNodePtr packableMenu(xmlNodePtr node)
{
    if(node && node->name && (is_menu(node) || strcmp((char *)node->name, "Window") == 0))
        return node;
    if(node && (node = node->parent) && node->name && (is_menu(node) || strcmp((char *)node->name, "Window") == 0))
        return node;
    if(node && (node = node->parent) && node->name && (is_menu(node) || strcmp((char *)node->name, "Window") == 0))
        return node;
    return NULL;
}

/* Sets the title of the toolbar window */
void setTitle(void)
{
    if(DWFilename)
    {
        int buflen = (int)strlen(APP_NAME) + strlen(DWFilename) + 4;
        char *tmpbuf = (char *)malloc(buflen); 

        if(tmpbuf)
        {
            snprintf(tmpbuf, buflen, "%s - %s",  APP_NAME, DWFilename);
            dw_window_set_text(hwndToolbar, tmpbuf);
            free(tmpbuf);
        }
    }
    else
        dw_window_set_text(hwndToolbar, APP_NAME);
}

/* Gets the contents of the list and puts it into the XML tree...
 * replacing any previous contents of the list.
 */
int saveList(xmlNodePtr node, HWND vbox)
{
    HWND list = (HWND)dw_window_get_data(vbox, "list");
    xmlNodePtr this = _dwib_find_child(node, "List");
    
    if(node && list && dw_window_get_data(list, "_dwib_modified"))
    {
        int x, count = dw_listbox_count(list);
        xmlNodePtr thisNode;
        char buf[101] = {0};
        
        /* Create a list node if one doesn't exist */
        if(!this)
            this = xmlNewTextChild(node, NULL, (xmlChar *)"List", (xmlChar *)"");
        
        thisNode = this->children;
        
        for(x=0;x<count;x++)
        {
            dw_listbox_get_text(list, x, buf, 100);
            
            /* Update or create a list item */
            if(thisNode)
                xmlNodeSetContent(thisNode, (xmlChar *)buf);
            else 
                thisNode = xmlNewTextChild(this, NULL, (xmlChar *)"Item", (xmlChar *)buf);
            
            if(thisNode)
                thisNode = thisNode->next;
        }
        /* Remove any trailing nodes */
        while(thisNode)
        {
            /* Otherwise remove the node if it exists */
            xmlNodePtr freeme = thisNode;
            
            thisNode = thisNode->next;
            
            xmlUnlinkNode(freeme);
            xmlFreeNode(freeme);
        }
        dw_window_set_data(list, "_dwib_modified", NULL);
        return 1;
    }
    return 0;
}

/* Gets the contents of the list and puts it into the XML tree...
 * replacing any previous contents of the list.
 */
int save_columns(xmlNodePtr node, HWND vbox)
{
    int x, count = DW_POINTER_TO_INT(dw_window_get_data(vbox, "colcount"));
    xmlNodePtr this = _dwib_find_child(node, "Columns");
    
    if(node)
    {
        char buf[50];
        xmlNodePtr thisNode;
        
        /* Create a columns node if one doesn't exist */
        if(!this)
            this = xmlNewTextChild(node, NULL, (xmlChar *)"Columns", (xmlChar *)"");
        
        thisNode = this->children;
        
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
                
                /* If we have a valid column... */
                if(colname && *colname && coltype && *coltype && colalign && *colalign)
                {
                    /* Create or update the column node */
                    if(thisNode)
                        xmlNodeSetContent(thisNode, (xmlChar *)colname);
                    else 
                        thisNode = xmlNewTextChild(this, NULL, (xmlChar *)"Item", (xmlChar *)colname);
                    xmlSetProp(thisNode, (xmlChar *)"ColType", (xmlChar *)coltype);
                    xmlSetProp(thisNode, (xmlChar *)"ColAlign", (xmlChar *)colalign);
                }
                else if(thisNode) 
                {
                    /* Otherwise remove the node if it exists */
                    xmlNodePtr freeme = thisNode;
                    
                    thisNode = thisNode->next;
                    
                    xmlUnlinkNode(freeme);
                    xmlFreeNode(freeme);
                }
                
                /* Advance to the next node if available */
                if(thisNode)
                    thisNode = thisNode->next;
                
                /* Free temporary memory */
                if(colname)
                    dw_free(colname);
                if(coltype)
                    dw_free(coltype);
                if(colalign)
                    dw_free(colalign);
            }
        }
        /* Remove any trailing nodes */
        while(thisNode)
        {
            /* Otherwise remove the node if it exists */
            xmlNodePtr freeme = thisNode;
            
            thisNode = thisNode->next;
            
            xmlUnlinkNode(freeme);
            xmlFreeNode(freeme);
        }
        return 1;
    }
    return 0;
}

/* Checks the values on the properties and updates
 * the XML node data.
 */
int updateNode(xmlNodePtr node, HWND vbox, char *name, int toggle)
{
    HWND item = (HWND)dw_window_get_data(vbox, name);
    char *val = "0";
    int retval = 0;
    
    if(!item)
        return 0;
    
    if(toggle && dw_checkbox_get(item))
    {
        val = "1";
    }
    
    if((toggle || (val = dw_window_get_text(item))))
    {
        xmlNodePtr this = _dwib_find_child(node, name);
        char *oldval = this ? (char *)xmlNodeListGetString(DWDoc, this->children, 1) : NULL;
        
        retval = 1;
                
        if(!this)
            this = xmlNewTextChild(node, NULL, (xmlChar *)name, (xmlChar *)val);
        else if((oldval && strcmp(oldval, val) != 0) || (!oldval && *val && strcmp(val, "0") != 0 && strcmp(val, "Default") != 0))
            xmlNodeSetContent(this, (xmlChar *)val);
        else
            retval = 0;
        
        if(!toggle)
            dw_free(val);
    }
    return retval;
}

int updateNodeText(xmlNodePtr node, HWND vbox, char *name)
{
    HWND item = (HWND)dw_window_get_data(vbox, name);
    unsigned long bytes;
    char *val = NULL, *oldval = NULL;
    xmlNodePtr this;
    int retval = 1;
    
    if(!item)
        return 0;
    
    dw_mle_get_size(item, &bytes, NULL);
    
    if(bytes)
    {
        val = calloc(bytes + 1, 1);
        dw_mle_export(item, val, 0, (int)bytes);
    }
    
    if((this = _dwib_find_child(node, name)) != NULL)
        oldval = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
    
    if(!this)
        this = xmlNewTextChild(node, NULL, (xmlChar *)name, (xmlChar *)(val ? val : ""));
    else if(!oldval && strcmp(oldval, val) != 0)
        xmlNodeSetContent(this, (xmlChar *)(val ? val : ""));
    else 
        retval = 0;
    
    if(val)
        free(val);
    
    return retval;
}

/* Save the properties for general items */
int save_item(xmlNodePtr node, HWND vbox)
{
    int retval = 0;
    
    retval |= updateNode(node, vbox, "dataname", FALSE);
    retval |= updateNode(node, vbox, "width", FALSE);
    retval |= updateNode(node, vbox, "height", FALSE);
    retval |= updateNode(node, vbox, "hexpand", TRUE);
    retval |= updateNode(node, vbox, "vexpand", TRUE);
    retval |= updateNode(node, vbox, "padding", FALSE);
    retval |= updateNode(node, vbox, "enabled", TRUE);
    retval |= updateNode(node, vbox, "tooltip", FALSE);
    retval |= updateNode(node, vbox, "fcolor", FALSE);
    retval |= updateNode(node, vbox, "bcolor", FALSE);
    retval |= updateNode(node, vbox, "font", FALSE);
#ifdef __OS2__
    retval |= updateNode(node, vbox, "os2font", FALSE);
#elif defined(__MAC__)    
    retval |= updateNode(node, vbox, "macfont", FALSE);
#elif defined(__WIN32__)    
    retval |= updateNode(node, vbox, "winfont", FALSE);
#elif defined(__UNIX__)    
    retval |= updateNode(node, vbox, "unixfont", FALSE);
#endif
    return retval;
}

/* Delete a preview control if one exists */
void deleteControl(xmlNodePtr node)
{
    if(LivePreview && node->psvi)
    {
        /* Handle special case of notebook page */
        if(node->name && strcmp((char *)node->name, "NotebookPage") == 0)
        {
            xmlNodePtr notebooknode = packablePageNode(node->parent);
            unsigned int pageid = DW_POINTER_TO_UINT(dw_window_get_data((HWND)node->psvi, "_dwib_pageid"));
            
            if(notebooknode && notebooknode->psvi)
                dw_notebook_page_destroy((HWND)notebooknode->psvi, pageid);
        }
        /* Handle special case for padding... need to use
         * dw_box_remove_at_index() instead of dw_window_destroy()
         * due to lack of a window handle for padding.
         */
        else if(node->name && strcmp((char *)node->name, "Padding") == 0)
        {
            int index = -1;
            xmlNodePtr p, boxnode = packableNode(node);
            
            if(boxnode && boxnode->psvi)
            {
                /* Figure out the existing index */
                for(p=node; p; p=p->prev)
                {
                    index++;
                }
                /* Remove the padding from the layout */
                dw_box_remove_at_index((HWND)boxnode->psvi, index);
            }
        }
        else /* Just destroy any other controls */
            dw_window_destroy((HWND)node->psvi);
        node->psvi = NULL;
    }
}

/* Create or recreate a preview control/widget */
void previewControl(xmlNodePtr node)
{
    if(LivePreview && node->parent && node->parent->parent)
    {
        xmlNodePtr p = node->parent;
        xmlNodePtr boxnode = p->parent;
        int iswindow = 0;
        
        /* Contents of boxes, notebook and top-level windows are currently live previewable */
        if(boxnode->name && (strcmp((char *)boxnode->name, "Box") == 0 ||
                             strcmp((char *)boxnode->name, "Window") == 0 ||
                             strcmp((char *)boxnode->name, "Notebook") == 0 ||
                             strcmp((char *)boxnode->name, "NotebookPage") == 0))
        {
            xmlNodePtr windownode = findWindow(node);
            int issplitbar;
            
            /* Move up over any splitbars, even embedded ones */
            do
            {
                xmlNodePtr this = _dwib_find_child(boxnode, "subtype");
                char *thisval;
        
                issplitbar = FALSE;
                
                /* Handle special case of splitbar box type */
                if(this && (thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
                {
                    if(strcmp(thisval, "Splitbar") == 0)
                    {
                        /* We can't repack the splitbar currently...
                         * so move everything up one level....
                         * and recreate the splitbar itself.
                         */
                        if((this = packableNode(boxnode->parent)))
                        {
                            node = boxnode;
                            p = node->parent;
                            boxnode = this;
                            issplitbar = TRUE;
                        }
                    }
                }
            }
            while(issplitbar);
            
            /* Check if the target box is a window or not */
            iswindow = (strcmp((char *)boxnode->name, "Window") == 0);
            
            /* Make sure the top-level window node has a preview handle */
            if(windownode && windownode->psvi)
            {
                int index = 0;
                HWND box = iswindow ? (HWND)dw_window_get_data((HWND)windownode->psvi, "_dwib_box") : 0;

                if(!box)
                    box = (HWND)boxnode->psvi;
                
                /* Figure out the existing index */
                for(p=p->children;p && p != node ;p=p->next)
                {
                    index++;
                }
                /* Destroy the old control */
                deleteControl(node);
                node->psvi = NULL;
                /* Recreate it at the correct location */
                _dwib_child(DWDoc, (HWND)windownode->psvi, box, FALSE, node, 0, index);
            }
        }
    }
}

/* Updates the XML tree with current settings */
void save_properties(void)
{
    int which = DW_POINTER_TO_INT(dw_window_get_data(hwndProperties, "type"));
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    xmlNodePtr node;
    /* Retval should be 0 if nothing has changed, 
     * and 1 if something has changed during update.
     */
    int retval = 0;
    
    if(!vbox || !which)
        return;
        
    if(!(node = dw_window_get_data(vbox, "node")))
        return;
    
    switch(which)
    {
        case TYPE_WINDOW:
            retval |= updateNode(node, vbox, "title", FALSE);
            retval |= updateNode(node, vbox, "width", FALSE);
            retval |= updateNode(node, vbox, "height", FALSE);
            retval |= updateNode(node, vbox, "x", FALSE);
            retval |= updateNode(node, vbox, "y", FALSE);
            retval |= updateNode(node, vbox, "hgravity", FALSE);
            retval |= updateNode(node, vbox, "vgravity", FALSE);
            retval |= updateNode(node, vbox, "hobstacles", TRUE);
            retval |= updateNode(node, vbox, "vobstacles", TRUE);
            retval |= updateNode(node, vbox, "bordersize", FALSE);
            retval |= updateNode(node, vbox, "close", TRUE);
            retval |= updateNode(node, vbox, "minimize", TRUE);
            retval |= updateNode(node, vbox, "maximize", TRUE);
            retval |= updateNode(node, vbox, "hide", TRUE);
            retval |= updateNode(node, vbox, "titlebar", TRUE);
            retval |= updateNode(node, vbox, "resize", TRUE);
            retval |= updateNode(node, vbox, "dialog", TRUE);
            retval |= updateNode(node, vbox, "border", TRUE);
            retval |= updateNode(node, vbox, "sysmenu", TRUE);
            retval |= updateNode(node, vbox, "tasklist", TRUE);
            retval |= updateNode(node, vbox, "composited", TRUE);
            retval |= updateNode(node, vbox, "orientation", FALSE);
            break;
        case TYPE_BOX:
            retval |= updateNode(node, vbox, "subtype", FALSE);
            retval |= save_item(node, vbox);
            retval |= updateNode(node, vbox, "orientation", FALSE);
            retval |= updateNode(node, vbox, "title", FALSE);
            retval |= updateNode(node, vbox, "splitper", FALSE);
            break;
        case TYPE_TEXT:
            retval |= updateNode(node, vbox, "subtype", FALSE);
            retval |= save_item(node, vbox);
            retval |= updateNode(node, vbox, "label", FALSE);
            retval |= updateNode(node, vbox, "alignment", FALSE);
            retval |= updateNode(node, vbox, "valignment", FALSE);
            break;
        case TYPE_ENTRYFIELD:
            retval |= updateNode(node, vbox, "subtype", FALSE);
            retval |= save_item(node, vbox);
            retval |= updateNode(node, vbox, "deftext", FALSE);
            retval |= updateNode(node, vbox, "limit", FALSE);
            break;
        case TYPE_COMBOBOX:
            retval |= save_item(node, vbox);
            retval |= updateNode(node, vbox, "deftext", FALSE);
            retval |= saveList(node, vbox);
            break;
        case TYPE_LISTBOX:
            retval |= save_item(node, vbox);
            retval |= updateNode(node, vbox, "multi", TRUE);
            retval |= saveList(node, vbox);
            break;
        case TYPE_CONTAINER:
            retval |= updateNode(node, vbox, "subtype", FALSE);
            retval |= save_item(node, vbox);
            retval |= updateNode(node, vbox, "coltitle", FALSE);
            retval |= updateNode(node, vbox, "multi", TRUE);
            retval |= updateNode(node, vbox, "idstring", TRUE);
            retval |= updateNode(node, vbox, "oddcolor", FALSE);
            retval |= updateNode(node, vbox, "evencolor", FALSE);
            retval |= updateNode(node, vbox, "splitcol", FALSE);
            retval |= save_columns(node, vbox);
            break;
        case TYPE_TREE:
            retval |= save_item(node, vbox);
            break;
        case TYPE_MLE:
            retval |= save_item(node, vbox);
            retval |= updateNode(node, vbox, "wordwrap", TRUE);
            retval |= updateNodeText(node, vbox, "deftext");
            break;
        case TYPE_RENDER:
            retval |= save_item(node, vbox);
            break;
        case TYPE_BUTTON:
            retval |= updateNode(node, vbox, "subtype", FALSE);
            retval |= save_item(node, vbox);
            retval |= updateNode(node, vbox, "checked", TRUE);
            retval |= updateNode(node, vbox, "setting", FALSE);
            retval |= updateNode(node, vbox, "borderless", TRUE);
            break;
        case TYPE_RANGED:
            retval |= updateNode(node, vbox, "subtype", FALSE);
            retval |= save_item(node, vbox);
            retval |= updateNode(node, vbox, "position", FALSE);
            retval |= updateNode(node, vbox, "upper", FALSE);
            retval |= updateNode(node, vbox, "lower", FALSE);
            break;
        case TYPE_BITMAP:
            retval |= save_item(node, vbox);
            retval |= updateNode(node, vbox, "setting", FALSE);
            break;
        case TYPE_NOTEBOOK:
            retval |= save_item(node, vbox);
            retval |= updateNode(node, vbox, "position", FALSE);
            break;
        case TYPE_NOTEBOOK_PAGE:
            retval |= updateNode(node, vbox, "title", FALSE);
            retval |= updateNode(node, vbox, "pagetext", FALSE);
            retval |= updateNode(node, vbox, "statustext", FALSE);
            retval |= updateNode(node, vbox, "orientation", FALSE);
            break;
        case TYPE_HTML:
            retval |= save_item(node, vbox);
            retval |= updateNode(node, vbox, "URL", FALSE);
            break;
        case TYPE_CALENDAR:
            retval |= save_item(node, vbox);
            break;
        case TYPE_PADDING:
            retval |= updateNode(node, vbox, "width", FALSE);
            retval |= updateNode(node, vbox, "height", FALSE);
            retval |= updateNode(node, vbox, "hexpand", TRUE);
            retval |= updateNode(node, vbox, "vexpand", TRUE);
            break;
        case TYPE_MENU:
            retval |= updateNode(node, vbox, "title", FALSE);
            retval |= updateNode(node, vbox, "dataname", FALSE);
            retval |= updateNode(node, vbox, "menuid", FALSE);
            retval |= updateNode(node, vbox, "checkable", TRUE);
            retval |= updateNode(node, vbox, "checked", TRUE);
            retval |= updateNode(node, vbox, "enabled", TRUE);
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
        /* Recreate the preview control */
        if(node->psvi && retval)
            previewControl(node);
    }
}

#define PROPERTIES_HEIGHT 22
#define PROPERTIES_WIDTH 124
#define BUTTON_ICON_WIDTH 40
#define BUTTON_ICON_HEIGHT 30

char *defvalstr = "", *defvalint = "-1", *defvaltrue = "1", *defvalzero = "0";

extern char *Colors[];

/* Internal function to add or update locale text from the dialog into the XML tree */
void locale_manager_update(void)
{
    if(hwndLocale)
    {
        xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
        xmlNodePtr localesNode = _dwib_find_child(rootNode, "Locales");
        HWND combo = (HWND)dw_window_get_data(hwndLocale, "combo");
        HWND entry = (HWND)dw_window_get_data(hwndLocale, "entry");
        xmlNodePtr node = (xmlNodePtr)dw_window_get_data(hwndLocale, "node");
        int selected = DW_POINTER_TO_INT(dw_window_get_data(hwndLocale, "selected"));

        if(localesNode && combo && entry && node && selected > 0)
        {
            char *localetext = dw_window_get_text(entry);
            char localename[101] = {0};
            
            dw_listbox_get_text(combo, selected-1, localename, 100);
            
            /* Make sure we have something to update with */
            if(localename[0] && localetext)
            {
                xmlNodePtr p;
                
                /* Make sure this is for a valid locale */
                for(p=localesNode->children;p;p = p->next)
                {
                    /* Yes we have a valid locale! */
                    if(strcmp((char *)p->name, localename) == 0)
                    {
                        xmlAttrPtr att = xmlHasProp(node, (xmlChar *)localename);
                        
                        /* If we have text... */ 
                        if(*localetext)
                        {
                            /* Either change or create a new property */
                            if(att)
                                xmlSetProp(node, (xmlChar *)localename, (xmlChar *)localetext);
                            else
                                xmlNewProp(node, (xmlChar *)localename, (xmlChar *)localetext);
                        }
                        /* Otherwise remove the existing property */
                        else if(att)
                            xmlRemoveProp(att);
                   }
                }
            }
            /* Free memory when done */
            if(localetext)
                dw_free(localetext);
        }
    }
}

/* Handle closing the locale manager window */
int DWSIGNAL locale_manager_delete(HWND item, void *data)
{
    HWND window = data ? (HWND)data : item;
    
    locale_manager_update();
    
    hwndLocale = 0;
    
    dw_window_destroy(window);
    return TRUE;
}

/* Handle creating locale manager */
int DWSIGNAL locale_rem_clicked(HWND button, void *data)
{
    HWND combo = (HWND)dw_window_get_data(hwndLocale, "combo");
    int selected = DW_POINTER_TO_INT(dw_window_get_data(hwndLocale, "selected"));
    xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
    xmlNodePtr localesNode = _dwib_find_child(rootNode, "Locales");

    if(selected < 1)
        dw_messagebox(APP_NAME, DW_MB_OK | DW_MB_ERROR, "No locale selected for removal.");
    else if(localesNode && combo)
    {
        char localename[101] = {0};

        dw_listbox_get_text(combo, selected-1, localename, 100);
        
        /* Make sure we have something to update with */
        if(localename[0])
        {
            xmlNodePtr p;
            
            /* Make sure this is for a valid locale */
            for(p=localesNode->children;p;p = p->next)
            {
                /* Yes we have a valid locale! */
                if(strcmp((char *)p->name, localename) == 0)
                    break;
            }
            if(p && dw_messagebox(APP_NAME, DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to remove locale \"%s\"?  Doing so may leave orphaned locale data on individual widgets.", localename) == DW_MB_RETURN_YES)
            {
                /* Remove the entry from the combobox */
                dw_listbox_delete(combo, selected-1);
                /* Remove it from the menu */
                if(p->psvi)
                    dw_window_destroy((HWND)p->psvi);
                /* Finally remove it from the XML tree */
                xmlUnlinkNode(p);
                xmlFreeNode(p);
            }
        }
    }               
    return FALSE;
}

/* Handle adding a new locale definition */
int DWSIGNAL locale_add_clicked(HWND button, void *data)
{
    HWND combo = (HWND)data;
    char *locale = dw_window_get_text(combo);
    xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
    xmlNodePtr localesNode = _dwib_find_child(rootNode, "Locales");
    
    if(locale)
    {
        if(*locale)
        {
            /* Make sure the locale doesn't already exist */
            if(localesNode)
            {
                xmlNodePtr p;
                
                for(p=localesNode->children;p;p = p->next)
                {
                    if(strcmp((char *)p->name, locale) == 0)
                    {
                        /* It does already exist so throw up an error and...
                         * reset variables so it doesn't add a new one.
                         */
                        dw_messagebox(APP_NAME, DW_MB_OK | DW_MB_ERROR, "Locale \"%s\" already exists.", locale);
                        dw_free(locale);
                        p = NULL;
                        locale = NULL;
                        break;
                    }
                }
            }            
            else /* Add a Locales node if one doesn't already exist */
                localesNode = xmlNewTextChild(rootNode, NULL, (xmlChar *)"Locales", (xmlChar *)"");
            /* If we should add it... add the new locale */
            if(locale && localesNode)
            {
                xmlNodePtr this = xmlNewTextChild(localesNode, NULL, (xmlChar *)locale, (xmlChar *)"");
                
                /* Add to the combobox and the menu */
                if(this)
                {
                    HWND item = dw_menu_append_item(menuLocale, locale, DW_MENU_AUTO, 0, TRUE, TRUE, DW_NOMENU);
                    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(preview_locale_clicked), DW_POINTER(this));
                    this->psvi = DW_POINTER(item);
                    dw_listbox_append(combo, locale);
                }
            }
        }
        else
            dw_messagebox(APP_NAME, DW_MB_OK | DW_MB_ERROR, "Enter a locale identifier in the combobox; \"de_DE\" for example.");
        /* Free memory */
        if(locale)
            dw_free(locale);
    }
    return FALSE;
}

/* Callback to handle user selection in the site combobox */
int DWSIGNAL locale_manager_select(HWND hwnd, int item, void *data)
{
    char buf[101] = {0};
    HWND entry = (HWND)data;
    xmlNodePtr node = (xmlNodePtr)dw_window_get_data(hwndLocale, "node");
    
    dw_listbox_get_text(hwnd, item, buf, 100);
    
    locale_manager_update();
    
    if(node && entry)
    {
        char *thisval = (char *)xmlGetProp(node, (xmlChar *)buf);

        if(thisval)
        {
            dw_window_set_text(entry, thisval);
            xmlFree(thisval);
        }
        else
            dw_window_set_text(entry, "");
    }
    
    /* Save the current selection so we don't have to rely on the combobox entry field */
    dw_window_set_data(hwndLocale, "selected", DW_INT_TO_POINTER((item+1)));
    return FALSE;
}

/* Internal function to reset the locale manager window...
 * so we can be sure the node it is operating on is still valid.
 */
void locale_manager_reset(char *val)
{
    if(hwndLocale)
    {
        HWND combo = (HWND)dw_window_get_data(hwndLocale, "combo");
        HWND entry = (HWND)dw_window_get_data(hwndLocale, "entry");
        HWND def = (HWND)dw_window_get_data(hwndLocale, "default");
        
        locale_manager_update();
        
        if(def)
            dw_window_set_text(def, val ? val : "");
        if(entry)
            dw_window_set_text(entry, "");
        if(combo)
            dw_window_set_text(combo, "");
        dw_window_set_data(hwndLocale, "selected", NULL);
        dw_window_set_data(hwndLocale, "node", NULL);
    }
}

/* Handle creating locale manager */
int DWSIGNAL locale_manager_clicked(HWND button, void *data)
{
    xmlNodePtr this = data;
    char *val = NULL, *thisval;
    
    if(this && (thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
        val = thisval;
        
    if(hwndLocale)
        locale_manager_reset(val);
    else
    {
        HWND vbox = dw_box_new(DW_VERT, 0);
        HWND hbox = dw_box_new(DW_HORZ, 0);
        HWND item = dw_text_new("Default Locale: ", 0);
        HWND combo;
        int width;
        xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
        xmlNodePtr localesNode = _dwib_find_child(rootNode, "Locales");
        
        dw_window_get_preferred_size(item, &width, NULL);
        
        if(width < 100)
            width = 100;
        
        /* Make sure the locale manager window isn't open */
        if(hwndLocale)
            locale_manager_delete(hwndLocale, NULL);
        
        hwndLocale = dw_window_new(DW_DESKTOP, "Locale Manager", DW_FCF_MINMAX |
                                   DW_FCF_TITLEBAR | DW_FCF_SYSMENU | DW_FCF_TASKLIST | DW_FCF_SIZEBORDER);
        
        /* Default Locale Row */
        dw_window_set_style(item, DW_DT_CENTER | DW_DT_VCENTER, DW_DT_CENTER | DW_DT_VCENTER);
        dw_box_pack_start(hwndLocale, vbox, 0, 0, TRUE, TRUE, 0);
        dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);
        dw_box_pack_start(hbox, item, width, -1, FALSE, TRUE, 0);
        item = dw_entryfield_new(val ? val : "", 0);
        dw_box_pack_start(hbox, item, -1, -1, TRUE, FALSE, 0);
        dw_window_set_data(hwndLocale, "default", DW_POINTER(item));
        dw_window_disable(item);
        
        /* Locale Text Row */
        hbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);
        combo = item = dw_combobox_new("", 0);
        dw_window_set_data(hwndLocale, "combo", DW_POINTER(combo));
        dw_box_pack_start(hbox, item, width, -1, FALSE, TRUE, 0);
        /* Populate the combobox if we can */
        if(localesNode)
        {
            xmlNodePtr p;
            
            for(p=localesNode->children;p;p = p->next)
            {
                dw_listbox_append(combo, (char *)p->name);
            }
        }
        item = dw_entryfield_new("", 0);
        dw_box_pack_start(hbox, item, -1, -1, TRUE, FALSE, 0);
        dw_window_set_data(hwndLocale, "entry", DW_POINTER(item));
        dw_signal_connect(combo, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(locale_manager_select), DW_POINTER(item));
        
        /* Something to expand between the buttons and content */
        dw_box_pack_start(vbox, 0, 0, 0, TRUE, TRUE, 0);
        
        /* Button box */
        hbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);
        item = dw_button_new("+", 0);
        dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_add_clicked), DW_POINTER(combo));
        item = dw_button_new("-", 0);
        dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_rem_clicked), DW_POINTER(combo));
        dw_box_pack_start(hbox, 0, 1, 1, TRUE, FALSE, 0);
        item = dw_button_new("Done", 0);
        dw_box_pack_start(hbox, item, -1, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
        
        /* Delete handlers */
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_delete), DW_POINTER(hwndLocale));
        dw_signal_connect(hwndLocale, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(locale_manager_delete), NULL);
        
        dw_window_set_size(hwndLocale, 0, 0);
    }
    dw_window_show(hwndLocale);
    dw_window_set_data(hwndLocale, "node", DW_POINTER(this));
    return FALSE;
}

/* Populate the properties dialog with nothing */
void properties_none(void)
{
    HWND item, vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    item = dw_text_new("No item selected", 0);
    dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
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
void properties_item(xmlNodePtr node, HWND scrollbox, int box, int tooltip)
{
    HWND item, button, tmp, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    int x, width = 0;
#ifdef __OS2__
    char *sysfont = "os2font";
    char *sysfonttext = "OS/2 Font";
#elif defined(__MAC__)
    char *sysfont = "macfont";
    char *sysfonttext = "Mac Font";
#elif defined(__WIN32__)
    char *sysfont = "winfont";
    char *sysfonttext = "Windows Font";
#elif defined(__UNIX__)
    char *sysfont = "unixfont";
    char *sysfonttext = "Unix Font";
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
    dw_window_set_tooltip(item, "Use as parameter to dwib_window_get_handle().");
    dw_window_set_data(vbox, "dataname", DW_POINTER(item));
    if(box)
    {
        /* Required size */
        hbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
        item = dw_text_new("Size", 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
        dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
        val = defvalint;
        if((this = _dwib_find_child(node, "width")))
        {
            if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
                val = thisval;
        }
        item = dw_spinbutton_new(val, 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        dw_spinbutton_set_limits(item, 2000, -1);
        dw_window_set_tooltip(item, "Width: Set to -1 to calculate the size.");
        dw_window_set_data(vbox, "width", DW_POINTER(item));
        val = defvalint;
        if((this = _dwib_find_child(node, "height")))
        {
            if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
                val = thisval;
        }
        item = dw_spinbutton_new(val, 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        dw_spinbutton_set_limits(item, 2000, -1);
        dw_window_set_tooltip(item, "Height: Set to -1 to calculate the size.");
        dw_window_set_data(vbox, "height", DW_POINTER(item));
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
        dw_window_set_data(vbox, "hexpand", DW_POINTER(item));
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
        dw_window_set_data(vbox, "vexpand", DW_POINTER(item));
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
        dw_window_set_data(vbox, "padding", DW_POINTER(item));
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
        dw_window_set_data(vbox, "enabled", DW_POINTER(item));
    }
    if(tooltip)
    {
        /* Tooltip */
        hbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
        item = dw_text_new("Tooltip", 0);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
        dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
        val = defvalstr;
        if((this = _dwib_find_child(node, "tooltip")))
        {
            if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
                val = thisval;
        }
        item = dw_entryfield_new(val, 0);
        button = dw_bitmapbutton_new("Locale", ICON_LOCALE);
        dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
        dw_window_get_preferred_size(button, &width, NULL);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
        dw_window_set_data(vbox, "tooltip", DW_POINTER(item));
        if(this)
            dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_clicked), this);
        else
            dw_window_disable(button);
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
    dw_listbox_append(item, "Default");
    for(x=0;x<16;x++)
    {
        dw_listbox_append(item, Colors[x]);
    }
    if((x = _dwib_get_color(val)) != DW_CLR_DEFAULT && x < 16 && x > -1)
    {
        dw_listbox_select(item, x + 1, TRUE);
    }
    dw_window_set_data(vbox, "fcolor", DW_POINTER(item));    
    button = dw_bitmapbutton_new("Color chooser", ICON_COLOR);
    dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_get_preferred_size(button, &width, NULL);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
    dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(color_clicked), DW_POINTER(tmp));
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
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Default");
    for(x=0;x<16;x++)
    {
        dw_listbox_append(item, Colors[x]);
    }
    if((x = _dwib_get_color(val)) != DW_CLR_DEFAULT && x < 16 && x > -1)
    {
        dw_listbox_select(item, x + 1, TRUE);
    }
    dw_window_set_data(vbox, "bcolor", DW_POINTER(item));    
    item = dw_bitmapbutton_new("Color chooser", ICON_COLOR);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_box_pack_start(hbox, item, -1, -1, FALSE, FALSE, 0);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(color_clicked), DW_POINTER(tmp));
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
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Default");
    dw_window_set_data(vbox, "font", DW_POINTER(item));    
    item = dw_bitmapbutton_new("Font chooser", ICON_FONT);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_box_pack_start(hbox, item, -1, -1, FALSE, FALSE, 0);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(font_clicked), DW_POINTER(tmp));
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
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
        dw_listbox_append(item, "Default");
        dw_window_set_data(vbox, sysfont, DW_POINTER(item));    
        item = dw_bitmapbutton_new("Font chooser", ICON_FONT);
        dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
        dw_box_pack_start(hbox, item, -1, -1, FALSE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(font_clicked), DW_POINTER(tmp));
    }
}

/* Create a new text definition */
int DWSIGNAL text_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Text", (xmlChar *)subtype);
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    snprintf(buf, 200, "Text - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_TEXT], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Handle click on the properties inspector refresh widget button */
int DWSIGNAL refresh_widget_clicked(HWND window, void *data)
{
   save_properties();
   return TRUE;
}

/* Internal function to create a title with a refresh button */
void _dwib_title(HWND scrollbox, char *title, xmlNodePtr node)
{
    if(node)
    {
        HWND item = dw_text_new(title, 0);
        HWND hbox = dw_box_new(DW_HORZ, 0);
        
        dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
        dw_box_pack_start(hbox, 0, 24, 24, FALSE, FALSE, 0);
        dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
        dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - 48, PROPERTIES_HEIGHT, TRUE, TRUE, 0);
        item = dw_bitmapbutton_new("Refresh Preview Widget", ICON_REFRESH);
        dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
        dw_box_pack_start(hbox, item, -1, -1, FALSE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(refresh_widget_clicked), NULL);
    }
    else 
    {
        HWND item = dw_text_new(title, 0);
        dw_window_set_style(item, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
        dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);        
    }
}

/* Populate the properties window for a text */
void DWSIGNAL properties_text(xmlNodePtr node)
{
    HWND button, item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    int width;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Text Widget", node);
    
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
    dw_window_set_data(vbox, "subtype", DW_POINTER(item));    
    
    properties_item(node, scrollbox, TRUE, FALSE);
    
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
    button = dw_bitmapbutton_new("Locale", ICON_LOCALE);
    dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_get_preferred_size(button, &width, NULL);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
    dw_window_set_data(vbox, "label", DW_POINTER(item));
    if(this)
        dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_clicked), this);
    else 
        dw_window_disable(button);
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
    dw_window_set_data(vbox, "alignment", DW_POINTER(item));    
    
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
    dw_window_set_data(vbox, "valignment", DW_POINTER(item));  
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(text_create), NULL);
    }
}
   
/* Create a new entryfield definition */
int DWSIGNAL entryfield_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Entryfield", (xmlChar *)subtype);
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    snprintf(buf, 200, "Entryfield - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_ENTRYFIELD], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Populate the properties window for a entryfield */
void DWSIGNAL properties_entryfield(xmlNodePtr node)
{
    HWND button, item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    int width;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Entryfield Widget", node);
    
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
    dw_window_set_data(vbox, "subtype", DW_POINTER(item));    
    
    properties_item(node, scrollbox, TRUE, TRUE);
    
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
    button = dw_bitmapbutton_new("Locale", ICON_LOCALE);
    dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_get_preferred_size(button, &width, NULL);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
    dw_window_set_data(vbox, "deftext", DW_POINTER(item));
    if(this)
        dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_clicked), this);
    else 
        dw_window_disable(button);
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
    dw_window_set_data(vbox, "limit", DW_POINTER(item));
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(entryfield_create), NULL);
    }
}

/* Create a new combobox definition */
int DWSIGNAL combobox_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Combobox", (xmlChar *)"");
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(thisNode, NULL, (xmlChar *)"List", (xmlChar *)"");
    
    treeitem = dw_tree_insert(tree, "Combobox", hIcons[TYPE_COMBOBOX], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
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
                dw_window_set_data(list, "_dwib_modified", DW_INT_TO_POINTER(1));
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
            dw_window_set_data(list, "_dwib_modified", DW_INT_TO_POINTER(1));
        }
    }
    return FALSE;
}

/* Populate the properties window for a combobox */
void DWSIGNAL properties_combobox(xmlNodePtr node)
{
    HWND button, item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    int width;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Combobox Widget", node);
    
    properties_item(node, scrollbox, TRUE, TRUE);
    
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
    button = dw_bitmapbutton_new("Locale", ICON_LOCALE);
    dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_get_preferred_size(button, &width, NULL);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
    dw_window_set_data(vbox, "deftext", DW_POINTER(item));
    if(this)
        dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_clicked), this);
    else 
        dw_window_disable(button);
    /* List */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, TRUE, 0);
    item = dw_text_new("List", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_listbox_new(0, FALSE);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, 150, TRUE, TRUE, 0);
    dw_window_set_data(vbox, "list", DW_POINTER(item));
    if((this = _dwib_find_child(node, "List")))
    {
        _dwib_populate_list(item, this, DWDoc);
    }
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_entryfield_new("", 0);
    dw_window_set_data(vbox, "list_entry", DW_POINTER(item));
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_button_new("+", 0);
    dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(add_clicked), DW_POINTER(vbox));
    item = dw_button_new("-", 0);
    dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(rem_clicked), DW_POINTER(vbox));
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(combobox_create), NULL);
    }
}

/* Create a new listbox definition */
int DWSIGNAL listbox_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Listbox", (xmlChar *)"");
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(thisNode, NULL, (xmlChar *)"List", (xmlChar *)"");
    
    treeitem = dw_tree_insert(tree, "Listbox", hIcons[TYPE_LISTBOX], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Populate the properties window for a listbox */
void DWSIGNAL properties_listbox(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *val = defvalzero, *thisval;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Listbox Widget", node);
    
    properties_item(node, scrollbox, TRUE, TRUE);
    
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
    dw_window_set_data(vbox, "multi", DW_POINTER(item));
    
    /* List */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, TRUE, 0);
    item = dw_text_new("List", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_listbox_new(0, FALSE);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, 150, TRUE, TRUE, 0);
    if((this = _dwib_find_child(node, "List")))
    {
        _dwib_populate_list(item, this, DWDoc);
    }
    dw_window_set_data(vbox, "list", DW_POINTER(item));
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_entryfield_new("", 0);
    dw_window_set_data(vbox, "list_entry", DW_POINTER(item));
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_button_new("+", 0);
    dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(add_clicked), DW_POINTER(vbox));
    item = dw_button_new("-", 0);
    dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(rem_clicked), DW_POINTER(vbox));
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(listbox_create), NULL);
    }
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
    HWND combo = (HWND)dw_window_get_data(vbox, "splitcol");
    int count = DW_POINTER_TO_INT(dw_window_get_data(vbox, "colcount"));
    char buf[51] = {0};
    
    snprintf(buf, 50, "addbutton%d", count);
    button = (HWND)dw_window_get_data(vbox, buf);
    dw_window_disable(button);
    
    count++;
    add_row(vbox, scrollbox, count, "", "", "", NULL, FALSE);
    dw_window_set_data(vbox, "colcount", DW_INT_TO_POINTER(count));
    /* Add a final entry for the blank field */
    snprintf(buf, 50, "%d", count+1);
    dw_listbox_append(combo, buf);
    return FALSE;
}

/* Add a single row */
void add_row(HWND vbox, HWND scrollbox, int count, char *colname, char *coltype, char *colalign, xmlNodePtr node, int disable)
{
    HWND button, item, hbox = dw_box_new(DW_HORZ, 0);
    char buf[50];
    int x = 0, which = 0, width;
    
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_entryfield_new(colname, 0);
    button = dw_bitmapbutton_new("Locale", ICON_LOCALE);
    dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_get_preferred_size(button, &width, NULL);
    snprintf(buf, 50, "entryfield%d", count);
    dw_window_set_data(vbox, buf, DW_POINTER(item));
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - (BUTTON_ICON_WIDTH + width), PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
    if(node)
        dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_clicked), node);
    else
        dw_window_disable(button);
    item = dw_combobox_new(coltype, 0);
    snprintf(buf, 50, "coltype%d", count);
    dw_window_set_data(vbox, buf, DW_POINTER(item));
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
    dw_window_set_data(vbox, buf, DW_POINTER(item));
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
    dw_window_set_data(vbox, buf, DW_POINTER(item));
    dw_window_set_tooltip(item, "Add a new column");
    dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(add_row_clicked), DW_POINTER(scrollbox));
    if(disable)
        dw_window_disable(item);
}

/* Add rows for columns from the XML tree */
void populate_columns(HWND vbox, HWND scrollbox, HWND combo, xmlNodePtr node)
{
    xmlNodePtr p;
    int count = 0;
    char buf[21] = {0};
    
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

                    add_row(vbox, scrollbox, count, thisval, coltype ? coltype : "", colalign ? colalign : "", p, TRUE);
                    count++;

                    /* Add the column to the OS/2 split column list */
                    snprintf(buf, 20, "%d", count);
                    dw_listbox_append(combo, buf);
                    
                    /* TODO: xmlFree now? */
                }
            }
        }
    }
    add_row(vbox, scrollbox, count, "", "", "", NULL, FALSE);
    dw_window_set_data(vbox, "colcount", DW_INT_TO_POINTER(count));
    /* Add a final entry for the blank field */
    snprintf(buf, 20, "%d", count+1);
    dw_listbox_append(combo, buf);
}

/* Create a new container definition */
int DWSIGNAL container_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Container", (xmlChar *)subtype);
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    snprintf(buf, 200, "Container - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_CONTAINER], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Populate the properties window for a container */
void DWSIGNAL properties_container(xmlNodePtr node)
{
    HWND item, button, scrollbox, tmp, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    int x, width;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Container Widget", node);
    
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
    dw_window_set_data(vbox, "subtype", DW_POINTER(item));    
    
    properties_item(node, scrollbox, TRUE, TRUE);
    
    /* Filesystem Column */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Filesystem Column", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "coltitle")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_entryfield_new(val, 0);
    button = dw_bitmapbutton_new("Locale", ICON_LOCALE);
    dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_get_preferred_size(button, &width, NULL);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
    dw_window_set_tooltip(item, "Title for the main filesytem column.");
    dw_window_set_data(vbox, "coltitle", DW_POINTER(item));
    if(this)
        dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_clicked), this);
    else
        dw_window_disable(button);
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
    dw_window_set_data(vbox, "multi", DW_POINTER(item));
    /* Row Identification */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Row Ident", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalzero;
    if((this = _dwib_find_child(node, "idstring")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_checkbox_new("String", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_checkbox_set(item, atoi(val));
    dw_window_set_tooltip(item, "Enable string comparison of row data instead of pointer. (REXX Compatible)");
    dw_window_set_data(vbox, "idstring", DW_POINTER(item));
    /* Odd Color */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Odd Row Color", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = "None";
    if((this = _dwib_find_child(node, "oddcolor")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    tmp = item = dw_combobox_new(val, 0);
    button = dw_bitmapbutton_new("Color chooser", ICON_COLOR);
    dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_get_preferred_size(button, &width, NULL);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "None");
    dw_listbox_append(item, "Default");
    for(x=0;x<16;x++)
    {
        dw_listbox_append(item, Colors[x]);
    }
    if((x = _dwib_get_color(val)) != DW_RGB_TRANSPARENT && x < 16 && x > -1)
    {
        dw_listbox_select(item, x + 2, TRUE);
    }
    else if(x == DW_CLR_DEFAULT)
        dw_listbox_select(item, 1, TRUE);
    dw_window_set_data(vbox, "oddcolor", DW_POINTER(item));    
    dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
    dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(color_clicked), DW_POINTER(tmp));
    /* Even Color */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Even Color", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = "None";
    if((this = _dwib_find_child(node, "evencolor")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    tmp = item = dw_combobox_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "None");
    dw_listbox_append(item, "Default");
    for(x=0;x<16;x++)
    {
        dw_listbox_append(item, Colors[x]);
    }
    if((x = _dwib_get_color(val)) != DW_RGB_TRANSPARENT && x < 16 && x > -1)
    {
        dw_listbox_select(item, x + 2, TRUE);
    }
    else if(x == DW_CLR_DEFAULT)
        dw_listbox_select(item, 1, TRUE);
    dw_window_set_data(vbox, "evencolor", DW_POINTER(item));    
    item = dw_bitmapbutton_new("Color chooser", ICON_COLOR);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_box_pack_start(hbox, item, -1, -1, FALSE, FALSE, 0);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(color_clicked), DW_POINTER(tmp));
    /* OS/2 Split Column */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("OS/2 Split Column", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    tmp = item = dw_combobox_new("None", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_tooltip(item, "Set the user resizable column on OS/2");
    dw_listbox_append(item, "None");
    dw_window_set_data(vbox, "splitcol", DW_POINTER(item));
    /* Columns */
    item = dw_text_new("Column names, types and alignment", 0);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    this = _dwib_find_child(node, "Columns");
    populate_columns(vbox, scrollbox, tmp, this);

    /* Finish up the split column handling */
    val = defvalzero;
    if((this = _dwib_find_child(node, "splitcol")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_listbox_select(tmp, atoi(val), TRUE);

    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(container_create), NULL);
    }
}

/* Create a new tree definition */
int DWSIGNAL tree_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Tree", (xmlChar *)"");
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "Tree", hIcons[TYPE_TREE], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Populate the properties window for a tree */
void DWSIGNAL properties_tree(xmlNodePtr node)
{
    HWND item, scrollbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Tree Widget", node);
    
    properties_item(node, scrollbox, TRUE, TRUE);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(tree_create), NULL);
    }
}

/* Create a new MLE definition */
int DWSIGNAL mle_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"MLE", (xmlChar *)"");
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "MLE", hIcons[TYPE_MLE], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Populate the properties window for a MLE */
void DWSIGNAL properties_mle(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Multi-line Edit Widget", node);
    
    properties_item(node, scrollbox, TRUE, TRUE);
    
    /* Word wrap */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Word", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvaltrue;
    if((this = _dwib_find_child(node, "wordwrap")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_checkbox_new("Wrap", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "wordwrap", DW_POINTER(item));
    /* Default Text */
    item = dw_text_new("Default text", 0);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "deftext")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_mle_new(0);
    dw_mle_import(item, val ? val : "", 0);
    dw_box_pack_start(scrollbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, TRUE, 0);
    dw_window_set_data(vbox, "deftext", DW_POINTER(item));
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(mle_create), NULL);
    }
}

/* Create a new render definition */
int DWSIGNAL render_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Render", (xmlChar *)"");
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "Render", hIcons[TYPE_RENDER], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Populate the properties window for a render */
void DWSIGNAL properties_render(xmlNodePtr node)
{
    HWND item, scrollbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Render Widget", node);
    
    properties_item(node, scrollbox, TRUE, TRUE);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(render_create), NULL);
    }
}

/* Create a new button definition */
int DWSIGNAL button_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Button", (xmlChar *)subtype);
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    snprintf(buf, 200, "Button - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_BUTTON], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Put all the available images in the properties list */
void populateImageList(HWND item)
{
    xmlNodePtr this = xmlDocGetRootElement(DWDoc)->children;
    
    while(this)
    {
        if(this->name && strcmp((char *)this->name, "Image") == 0)
        {
            xmlNodePtr node = _dwib_find_child(this, "ImageID");
            char *val, *file = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
            int iid = 0;
            
            /* Load the Icon ID if available */
            if(node && (val = (char *)xmlNodeListGetString(DWDoc, node->children, 1)) != NULL)
                iid = atoi(val);
           
            if(iid > 0)
            {
                char buf[201];
                
                snprintf(buf, 200, "%d - %s", iid, file);
                dw_listbox_append(item, buf);
            }
            else if(file)
                dw_listbox_append(item, file);
        }
        this=this->next;
    }
}

/* Populate the properties window for a button */
void DWSIGNAL properties_button(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Button Widget", node);
    
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
            xmlFree(thisval);
        }
    }
    dw_listbox_select(item, atoi(val), TRUE);
    dw_window_set_data(vbox, "subtype", DW_POINTER(item));    
    
    properties_item(node, scrollbox, TRUE, TRUE);
    
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
    dw_window_set_data(vbox, "checked", DW_POINTER(item));
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
    item = dw_combobox_new(val ? val : "", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "setting", DW_POINTER(item));
    /* Add possible images to the list */
    populateImageList(item);
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
    dw_window_set_data(vbox, "borderless", DW_POINTER(item));
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(button_create), NULL);
    }
}

/* Create a new ranged definition */
int DWSIGNAL ranged_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Ranged", (xmlChar *)subtype);
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    snprintf(buf, 200, "Ranged - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_RANGED], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Populate the properties window for a ranged */
void DWSIGNAL properties_ranged(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Ranged Widget", node);
    
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
    dw_window_set_data(vbox, "subtype", DW_POINTER(item));    
    
    properties_item(node, scrollbox, TRUE, TRUE);
    
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
    dw_window_set_data(vbox, "position", DW_POINTER(item));
    /* Ranger - Upper */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Range Upper", 0);
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
    dw_window_set_data(vbox, "upper", DW_POINTER(item));
    /* Ranger - Lower */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Range Lower", 0);
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
    dw_window_set_data(vbox, "lower", DW_POINTER(item));
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(ranged_create), NULL);
    }
}

/* Create a new bitmap definition */
int DWSIGNAL bitmap_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Bitmap", (xmlChar *)"");
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "Bitmap", hIcons[TYPE_BITMAP], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Populate the properties window for a bitmap */
void DWSIGNAL properties_bitmap(xmlNodePtr node)
{
    HWND button, item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    int width;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Bitmap Widget", node);
    
    properties_item(node, scrollbox, TRUE, TRUE);
    
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
    item = dw_combobox_new(val ? val : "", 0);
    button = dw_bitmapbutton_new("Locale", ICON_LOCALE);
    dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_get_preferred_size(button, &width, NULL);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
    dw_window_set_data(vbox, "setting", DW_POINTER(item));
    if(this)
        dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_clicked), this);
    else 
        dw_window_disable(button);
    /* Add possible images to the list */
    populateImageList(item);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(bitmap_create), NULL);
    }
}

/* Create a new HTML definition */
int DWSIGNAL html_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"HTML", (xmlChar *)"");
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "HTML", hIcons[TYPE_HTML], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Populate the properties window for a HTML */
void DWSIGNAL properties_html(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "HTML Widget", node);
    
    properties_item(node, scrollbox, TRUE, FALSE);
    
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
    dw_window_set_data(vbox, "URL", DW_POINTER(item));
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(html_create), NULL);
    }
}

/* Create a new notebook definition */
int DWSIGNAL notebook_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Notebook", (xmlChar *)"");
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(thisNode, NULL, (xmlChar *)"Children", (xmlChar *)"");
    
    treeitem = dw_tree_insert(tree, "Notebook", hIcons[TYPE_NOTEBOOK], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Populate the properties window for a notebook */
void DWSIGNAL properties_notebook(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Notebook Widget", node);
    
    properties_item(node, scrollbox, TRUE, FALSE);
    
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
    dw_window_set_data(vbox, "position", DW_POINTER(item));    
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(notebook_create), NULL);
    }
}

/* Create a new notebook page definition */
int DWSIGNAL notebook_page_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    char buf[200], *title = dw_window_get_text((HWND)dw_window_get_data(vbox, "title"));
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packablePageNode(DWCurrNode);
    
    if(currentNode && strcmp((char *)currentNode->name, "Notebook") == 0 && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"NotebookPage", (xmlChar *)"");
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(thisNode, NULL, (xmlChar *)"Children", (xmlChar *)"");
    
    snprintf(buf, 200, "Page - (%s)", title ? title : "");
    
    if(title)
        dw_free(title);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_NOTEBOOK_PAGE], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Populate the properties window for a notebook page */
void DWSIGNAL properties_notebook_page(xmlNodePtr node)
{
    HWND button, item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    int width;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Notebook Page Widget", NULL);
    
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
    button = dw_bitmapbutton_new("Locale", ICON_LOCALE);
    dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_get_preferred_size(button, &width, NULL);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
    dw_window_set_data(vbox, "title", DW_POINTER(item));
    if(this)
        dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_clicked), this);
    else 
        dw_window_disable(button);
    
    properties_item(node, scrollbox, FALSE, FALSE);
    
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
    button = dw_bitmapbutton_new("Locale", ICON_LOCALE);
    dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_get_preferred_size(button, &width, NULL);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
    dw_window_set_data(vbox, "statustext", DW_POINTER(item));
    if(this)
        dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_clicked), this);
    else 
        dw_window_disable(button);
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
    dw_window_set_data(vbox, "orientation", DW_POINTER(item));    
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(notebook_page_create), NULL);
    }
}

/* Create a new calendar definition */
int DWSIGNAL calendar_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Calendar", (xmlChar *)"");
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    treeitem = dw_tree_insert(tree, "Calendar", hIcons[TYPE_CALENDAR], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Populate the properties window for a calendar */
void DWSIGNAL properties_calendar(xmlNodePtr node)
{
    HWND item, scrollbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Calendar Widget", node);
    
    properties_item(node, scrollbox, TRUE, TRUE);
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(calendar_create), NULL);
    }
}

/* Create a new box definition */
int DWSIGNAL box_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    char buf[200], *subtype = dw_window_get_text((HWND)dw_window_get_data(vbox, "subtype"));
    xmlNodePtr boxNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        boxNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Box", (xmlChar *)subtype);
    }
    
    /* Dropout on failure */
    if(!boxNode)
        return FALSE;
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(boxNode, NULL, (xmlChar *)"Children", (xmlChar *)"");
    
    snprintf(buf, 200, "Box - (%s)", subtype ? subtype : "");
    
    if(subtype)
        dw_free(subtype);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_BOX], (HTREEITEM)currentNode->_private, boxNode);
    boxNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(boxNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(boxNode);
    
    return FALSE;
}

/* Populate the properties window for a box */
void DWSIGNAL properties_box(xmlNodePtr node)
{
    HWND button, item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    int width;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Box Widget", node);
    
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
    dw_window_set_data(vbox, "subtype", DW_POINTER(item));    
    
    properties_item(node, scrollbox, TRUE, FALSE);
    
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
    dw_window_set_data(vbox, "orientation", DW_POINTER(item));    
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
    button = dw_bitmapbutton_new("Locale", ICON_LOCALE);
    dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_get_preferred_size(button, &width, NULL);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
    dw_window_set_data(vbox, "title", DW_POINTER(item));
    if(this)
        dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_clicked), this);
    else 
        dw_window_disable(button);
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
    dw_window_set_data(vbox, "splitper", DW_POINTER(item));

    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(box_create), NULL);
    }
}    

/* Create a new padding definition */
int DWSIGNAL padding_create(HWND window, void *data)
{
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    xmlNodePtr thisNode = NULL, parentNode, currentNode = packableNode(DWCurrNode);
    
    if(currentNode && is_packable(currentNode, TRUE) && (parentNode = _dwib_find_child(currentNode, "Children")))
    {
        thisNode = xmlNewTextChild(parentNode, NULL, (xmlChar *)"Padding", (xmlChar *)"");
    }
    
    /* Dropout on failure */
    if(!thisNode)
        return FALSE;
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(thisNode, NULL, (xmlChar *)"Children", (xmlChar *)"");
    
    treeitem = dw_tree_insert(tree, "Padding", hIcons[TYPE_PADDING], (HTREEITEM)currentNode->_private, thisNode);
    thisNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)currentNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(thisNode));
    
    save_properties();
    
    properties_current();
    
    /* Try to update the preview window if there is one */
    if(currentNode->psvi)
        previewControl(thisNode);
    
    return FALSE;
}

/* Populate the properties window for padding */
void DWSIGNAL properties_padding(xmlNodePtr node)
{
    HWND item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Padding", node);

    /* Required size */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Size", 0);
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
    dw_window_set_tooltip(item, "Padding width in pixels.");
    dw_window_set_data(vbox, "width", DW_POINTER(item));
    val = defvaltrue;
    if((this = _dwib_find_child(node, "height")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, 0);
    dw_window_set_tooltip(item, "Padding height in pixels.");
    dw_window_set_data(vbox, "height", DW_POINTER(item));
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
    dw_window_set_data(vbox, "hexpand", DW_POINTER(item));
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
    dw_window_set_data(vbox, "vexpand", DW_POINTER(item));
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(padding_create), NULL);
    }
}

/* Create a new menu definition */
int DWSIGNAL menu_create(HWND window, void *data)
{
    xmlNodePtr parentNode = _dwib_find_child(DWCurrNode, "Children");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
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
    boxNode->_private = DW_POINTER(treeitem);
    dw_tree_item_expand(tree, (HTREEITEM)DWCurrNode->_private);
    
    dw_window_set_data(vbox, "node", DW_POINTER(boxNode));
    
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
    HWND button, item, checkable, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    int width;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Menu Widget", NULL);
    
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
    button = dw_bitmapbutton_new("Locale", ICON_LOCALE);
    dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_get_preferred_size(button, &width, NULL);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
    dw_window_set_data(vbox, "title", DW_POINTER(item));
    if(this)
        dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_clicked), this);
    else 
        dw_window_disable(button);
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
    dw_window_set_tooltip(item, "Use as parameter to dwib_window_get_handle().");
    dw_window_set_data(vbox, "dataname", DW_POINTER(item));
    /* Menu ID */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Menu ID", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    val = defvalstr;
    if((this = _dwib_find_child(node, "menuid")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_spinbutton_set_limits(item, 29999, 0);
    dw_spinbutton_set_pos(item, atoi(val));
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_tooltip(item, "Menu identifier or 0 to auto-generate one.");
    dw_window_set_data(vbox, "menuid", DW_POINTER(item));
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
    dw_window_set_data(vbox, "checkable", DW_POINTER(item));
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
    dw_window_set_data(vbox, "checked", DW_POINTER(item));
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toggle_clicked), DW_POINTER(vbox));
    dw_signal_connect(checkable, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toggle_clicked), DW_POINTER(vbox));
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
    dw_window_set_data(vbox, "enabled", DW_POINTER(item));
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(menu_create), NULL);
    }
}

/* Create a new window definition */
int DWSIGNAL window_create(HWND window, void *data)
{
    xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
    xmlNodePtr windowNode = xmlNewTextChild(rootNode, NULL, (xmlChar *)"Window", (xmlChar *)"");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HTREEITEM treeitem;
    char buf[200], *title = dw_window_get_text((HWND)dw_window_get_data(vbox, "title"));
    
    /* Create a sub-node for holding children */
    xmlNewTextChild(windowNode, NULL, (xmlChar *)"Children", (xmlChar *)"");
    
    snprintf(buf, 200, "Window - (%s)", title ? title : "");
    
    if(title)
        dw_free(title);
    
    treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_WINDOW], 0, windowNode);
    windowNode->_private = DW_POINTER(treeitem);
    
    dw_window_set_data(vbox, "node", DW_POINTER(windowNode));
    
    save_properties();
    
    properties_current();
    
    return FALSE;
}

/* Populate the properties window for a window */
void DWSIGNAL properties_window(xmlNodePtr node)
{
    HWND button, item, scrollbox, hbox, vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    char *thisval, *val = defvalstr;
    xmlNodePtr this;
    int width;
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", DW_POINTER(vbox));
    scrollbox = dw_scrollbox_new(DW_VERT, 2);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Title display */
    _dwib_title(scrollbox, "Top-level Window", NULL);
    
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
    button = dw_bitmapbutton_new("Locale", ICON_LOCALE);
    dw_window_set_style(button, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_get_preferred_size(button, &width, NULL);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH - width, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, button, -1, -1, FALSE, FALSE, 0);
    dw_window_set_data(vbox, "title", DW_POINTER(item));
    if(this)
        dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_clicked), this);
    else 
        dw_window_disable(button);
    /* Size */ 
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Size", 0);
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
    dw_window_set_tooltip(item, "Width: Set to 0 to let the system decide.");
    dw_window_set_data(vbox, "width", DW_POINTER(item));
    val = "100";
    if((this = _dwib_find_child(node, "height")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_tooltip(item, "Height: Set to 0 to let the system decide.");
    dw_window_set_data(vbox, "height", DW_POINTER(item));
    /* Positon */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Position", 0);
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
    dw_spinbutton_set_limits(item, 2000, -1000);
    dw_window_set_tooltip(item, "X: Set to -1 to let the system decide when gravity is not CENTER.");
    dw_window_set_data(vbox, "x", DW_POINTER(item));
    val = defvalint;
    if((this = _dwib_find_child(node, "y")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    item = dw_spinbutton_new(val, 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1000);
    dw_window_set_tooltip(item, "Y: Set to -1 to let the system decide when gravity is not CENTER.");
    dw_window_set_data(vbox, "y", DW_POINTER(item));
    /* Gravity */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Gravity", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_combobox_new("Left", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Left");
    dw_listbox_append(item, "Center");
    dw_listbox_append(item, "Right");
    val = defvalstr;
    if((this = _dwib_find_child(node, "hgravity")))
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
    dw_window_set_tooltip(item, "Horizontal anchor point for window position.");
    dw_window_set_data(vbox, "hgravity", DW_POINTER(item));
    item = dw_combobox_new("Top", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_listbox_append(item, "Top");
    dw_listbox_append(item, "Center");
    dw_listbox_append(item, "Bottom");
    val = defvalstr;
    if((this = _dwib_find_child(node, "vgravity")))
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
    dw_window_set_tooltip(item, "Vertical anchor point for window position.");
    dw_window_set_data(vbox, "vgravity", DW_POINTER(item));
    /* Obstacles */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Obstacles", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_checkbox_new("Horizontal", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    if((this = _dwib_find_child(node, "hobstacles")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)) && atoi(thisval))
            dw_checkbox_set(item, TRUE);
    }
    dw_window_set_tooltip(item, "Avoid system obstacles in the horizontal direction.");
    dw_window_set_data(vbox, "hobstacles", DW_POINTER(item));
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    item = dw_checkbox_new("Vertical", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    if((this = _dwib_find_child(node, "vobstacles")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)) && atoi(thisval))
            dw_checkbox_set(item, TRUE);
    }
    dw_window_set_tooltip(item, "Avoid system obstacles in the vertical direction.");
    dw_window_set_data(vbox, "vobstacles", DW_POINTER(item));
    
    /* Border size */
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
    dw_spinbutton_set_limits(item, 10, -1);
    dw_window_set_tooltip(item, "Override the default border size if possible, -1 for system default.");
    dw_window_set_data(vbox, "bordersize", DW_POINTER(item));
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
    dw_window_set_data(vbox, "close", DW_POINTER(item));
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
    dw_window_set_data(vbox, "minimize", DW_POINTER(item));
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
    dw_window_set_data(vbox, "maximize", DW_POINTER(item));
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
    dw_window_set_data(vbox, "hide", DW_POINTER(item));
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
    dw_window_set_data(vbox, "titlebar", DW_POINTER(item));
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
    dw_window_set_data(vbox, "resize", DW_POINTER(item));
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
    dw_window_set_data(vbox, "dialog", DW_POINTER(item));
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
    dw_window_set_data(vbox, "border", DW_POINTER(item));
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    item = dw_checkbox_new("System Menu", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_tooltip(item, "Adding this flag will also add a close button");
    val = defvaltrue;
    if((this = _dwib_find_child(node, "sysmenu")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "sysmenu", DW_POINTER(item));
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
    dw_window_set_data(vbox, "tasklist", DW_POINTER(item));
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, FALSE, FALSE, 0);
    item = dw_checkbox_new("Composited", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_tooltip(item, "Enables translucent windows on supported platforms");
    val = defvalstr;
    if((this = _dwib_find_child(node, "composited")))
    {
        if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            val = thisval;
    }
    dw_checkbox_set(item, atoi(val));
    dw_window_set_data(vbox, "composited", DW_POINTER(item));
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
    dw_window_set_data(vbox, "orientation", DW_POINTER(item));
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(window_create), NULL);
    }
}

/* Handle saving the current layout */
int DWSIGNAL save_clicked(HWND button, void *data)
{
    char *filename = dw_file_browse("Save interface", ".", "xml", DW_FILE_SAVE);
    
    if(filename)
    {
        char *tmpptr, *oldfilename = DWFilename;

        /* Make sure the XML tree is up-to-date */
        save_properties();
        
        /* Enable indenting in the output */
        xmlIndentTreeOutput = 1;
        
        xmlSaveFormatFile(filename, DWDoc, 1);

        /* Trim off the path using Unix or DOS format */
        tmpptr = strrchr(filename, '/');
        if(!tmpptr)
            tmpptr = strrchr(filename, '\\');
        /* Copy the short filename */
        DWFilename = strdup(tmpptr ? ++tmpptr : DWFilename);
        /* Free any old memory */
        if(oldfilename)
            free(oldfilename);
        dw_free(filename);
        /* Update the window title */
        setTitle();
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
    char buf[200];
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
        if(!thisnode || p == thisnode)
        {
            /* Create the title for the node */
            int index = generateNode(buf, p);
            
            if(index)
            {
                /* Delete the old node if we are recreating */
                if(p == thisnode && p->_private)
                {
                    HTREEITEM ti = (HTREEITEM)p->_private; 
                    
                    /* Make sure we don't accidentally use this later */
                    p->_private = NULL;
                    dw_tree_item_delete(tree, ti);
                }
                
                /* Create the new node */
                treeitem = _tree_insert(tree, after, buf, hIcons[index], parent, p);
                p->_private = DW_POINTER(treeitem);
                if(AutoExpand)
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

/* Cycle all nodes, expanding or collapsing them all */
void expandCollapseTree(xmlNodePtr node, HWND tree, int expand)
{
    xmlNodePtr p;
    
    if(!node)
        return;
    
    for(p=node;p;p = p->next)
    {
        /* If there is a tree item handle */
        if(p->_private)
        {
            /* Expand or collapse as necessary */
            if(expand)
                dw_tree_item_expand(tree, (HTREEITEM)p->_private);
            else
                dw_tree_item_collapse(tree, (HTREEITEM)p->_private);
        }
        /* Recurse deeper if there are children */
        if(p->children)
            expandCollapseTree(p->children, tree, expand);
    }
}

/* Destroys any preview windows currently open */
void destroyPreviews(void)
{
    xmlNodePtr p, rootNode = xmlDocGetRootElement(DWDoc);
    
    if(rootNode)
    {
        for(p=rootNode->children;p;p = p->next)
        {
            if(strcmp((char *)p->name, "Window") == 0)
            {
                if(p->psvi)
                {
                    HWND window = (HWND)p->psvi;
                    
                    p->psvi = NULL;
                    preview_delete(window, NULL);
                }
            }
        }
    }
}

/* Clears and reloads the tree data from XML */
void reloadTree(void)
{
    xmlNodePtr p, rootNode = xmlDocGetRootElement(DWDoc);
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
    HTREEITEM treeitem;
    
    /* Remove the current tree */
    dw_tree_clear(tree);
    
    /* Remove the properties */
    dw_window_destroy(vbox);
    properties_none();
    
    if(!rootNode)
        return;
    
    for(p=rootNode->children;p;p = p->next)
    {
        if(strcmp((char *)p->name, "Window") == 0)
        {
            char buf[200];
            
            generateNode(buf, p);
            treeitem = dw_tree_insert(tree, buf, hIcons[TYPE_WINDOW], 0, p);
            p->_private = DW_POINTER(treeitem);
            
            handleChildren(p, tree, NULL, NULL);
        }
    }
}

/* Internal function to destroy the locale menu items */
void destroyLocaleMenu(void)
{
    xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
    xmlNodePtr localesNode = _dwib_find_child(rootNode, "Locales");
    
    if(localesNode)
    {
        xmlNodePtr p;
        
        for(p=localesNode->children;p;p = p->next)
        {
            if(p->psvi)
            {
                /* Get rid of the menu item */
                dw_window_destroy((HWND)p->psvi);
            }
        }
    }
    /* Make sure the default locale is selected */
    dw_window_set_style(hwndDefaultLocale, DW_MIS_CHECKED, DW_MIS_CHECKED);
    dwib_locale_set(NULL);
}

/* Internal function to create the locale menu items */
void createLocaleMenu(void)
{
    xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
    xmlNodePtr localesNode = _dwib_find_child(rootNode, "Locales");
    
    if(localesNode)
    {
        xmlNodePtr p;
        
        for(p=localesNode->children;p;p = p->next)
        {
            HWND item = dw_menu_append_item(menuLocale, (char *)p->name, DW_MENU_AUTO, 0, TRUE, TRUE, DW_NOMENU);
            dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(preview_locale_clicked), DW_POINTER(p));
            p->psvi = DW_POINTER(item);
        }
    }
}

/* Handle starting a new layout */
int DWSIGNAL new_clicked(HWND button, void *data)
{
    xmlNodePtr current = DWDoc ? xmlDocGetRootElement(DWDoc) : NULL;
    
    if(!current || !current->children ||
       dw_messagebox(APP_NAME, DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to lose the current layout?"))
    {
        HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
        HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
        char *oldfilename = DWFilename;

        /* Make sure no preview windows are open */
        destroyPreviews();
        
        /* Make sure the image manager/view windows aren't open */
        if(hwndImages)
            image_manager_delete(hwndImages, NULL);
        
        /* Make sure the locale manager window isn't open */
        if(hwndLocale)
            locale_manager_delete(hwndLocale, NULL);
        
        /* Remove the current tree */
        dw_tree_clear(tree);
        
        /* Destroy menu items */
        destroyLocaleMenu();
        
        /* Remove the properties */
        dw_window_destroy(vbox);
        properties_none();
        
        /* Free the existing doc */
        if(DWDoc)
            xmlFreeDoc(DWDoc);
        
        /* Create a new empty XML document */
        DWDoc = xmlNewDoc((xmlChar *)"1.0");
        DWCurrNode = xmlNewNode(NULL, (xmlChar *)"DynamicWindows");
        xmlDocSetRootElement(DWDoc, DWCurrNode);

        /* Clear out the existing filename */
        DWFilename = NULL;
        /* Free any old memory */
        if(oldfilename)
            free(oldfilename);
        /* Update the window title */
        setTitle();
    }
    return FALSE;
}


/* Handle loading a new layout */
int DWSIGNAL open_clicked(HWND button, void *data)
{
    xmlNodePtr current = DWDoc ? xmlDocGetRootElement(DWDoc) : NULL;
    
    if(!current || !current->children ||
       dw_messagebox(APP_NAME, DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to lose the current layout?"))
    {
        char *filename = dw_file_browse("Open interface", ".", "xml", DW_FILE_OPEN);
        xmlDocPtr doc;
        
        if(filename && (doc = xmlParseFile(filename)))
        {
            char *tmpptr, *oldfilename = DWFilename;
            xmlNodePtr imageNode;
            
            /* Make sure no preview windows are open */
            destroyPreviews();
            
            /* Make sure the image manager/view windows aren't open */
            if(hwndImages)
                image_manager_delete(hwndImages, NULL);
            
            /* Make sure the locale manager window isn't open */
            if(hwndLocale)
                locale_manager_delete(hwndLocale, NULL);
        
            /* Free the existing doc */
            if(DWDoc)
                xmlFreeDoc(DWDoc);

            /* Update the doc and node then reload */
            DWDoc = doc; 
            DWCurrNode = xmlDocGetRootElement(DWDoc);
            reloadTree();
            
            /* Populate locale menu */
            createLocaleMenu();

            /* Trim off the path using Unix or DOS format */
            tmpptr = strrchr(filename, '/');
            if(!tmpptr)
                tmpptr = strrchr(filename, '\\');
            /* Copy the short filename */
            DWFilename = strdup(tmpptr ? ++tmpptr : DWFilename);
            /* Free any old memory */
            if(oldfilename)
                free(oldfilename);
            dw_free(filename);
            /* Update the window title */
            setTitle();
            /* Update the path from the file */
            if((imageNode = _dwib_find_child(xmlDocGetRootElement(DWDoc), "ImageRoot")))
            {
                char *val = (char *)xmlNodeListGetString(DWDoc, imageNode->children, 1);
                
                if(val)
                    dwib_image_root_set(val);
            }
        }
    }
    return FALSE;
}

/* Wipe out any pvsi fields in the preview tree */
void clearPreview(xmlNodePtr node)
{
    for(;node;node = node->next)
    {
        node->psvi = NULL;
        clearPreview(node->children);
    }
}

/* Reset the preview window handle when deleted */
int DWSIGNAL preview_delete(HWND window, void *data)
{
    HWND menu = (HWND)dw_window_get_data(data ? (HWND)data : window, "dwib_menu");
    xmlNodePtr node = (xmlNodePtr)dw_window_get_data(data ? (HWND)data : window, "_dwib_node");
    
    if(node)
    {
        node->psvi = NULL;
        clearPreview(node->children);
    }
    
    /* Destroy associated menu if any */ 
    if(menu)
        dw_window_destroy(menu);
    
    /* And destroy the preview window itself */
    dw_window_destroy(data ? (HWND)data : window);
    return FALSE;
}

/* Handle expanding all children of an item */
int DWSIGNAL expand_item_clicked(HWND button, void *data)
{
    xmlNodePtr node = data;
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    
    if(node)
    {
        if(node->_private)
            dw_tree_item_expand(tree, (HTREEITEM)node->_private);
        expandCollapseTree(node->children, tree, TRUE);
    }
        
    return FALSE;
}

/* Handle collapsing all children of an item */
int DWSIGNAL collapse_item_clicked(HWND button, void *data)
{
    xmlNodePtr node = data;
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
   
    if(node)
    {
        if(node->_private)
            dw_tree_item_collapse(tree, (HTREEITEM)node->_private);
        expandCollapseTree(node->children, tree, FALSE);
    }
    return FALSE;
}

/* Handle menu click to show window */
int DWSIGNAL menu_show_window(HWND button, void *data)
{
    HWND window = (HWND)data;
    
    dw_window_raise(window);
    dw_window_show(window);
    return FALSE;
}

/* Returns the node of the top-level window */
xmlNodePtr findWindow(xmlNodePtr thisnode)
{
    xmlNodePtr node = thisnode;
    
    while(node)
    {
        if(strcmp((char *)node->name, "Window") == 0)
        {
            return node;
        }
        node=node->parent;
    }
    return NULL;
}

/* Handle loading a new layout */
int DWSIGNAL refresh_clicked(HWND button, void *data)
{
    xmlNodePtr node = findWindow(data);
    xmlNodePtr this = _dwib_find_child(node, "title");
    
    if(this)
    {
        char *val = (char *)xmlNodeListGetString(DWDoc, this->children, 1);
        
        if(val && *val)
        {
            HWND preview;
            
            /* If a preview window already exists... */
            if(node->psvi)
            {
                /* Destroy it before creating a new one */
                preview_delete((HWND)node->psvi, NULL);
            }
            
            /* Make sure the XML tree is up-to-date */
            save_properties();
            
            preview = dwib_load((DWIB)DWDoc, val);
            
            if(preview)
            {
                /* Create an item on the Windows menu */
                HWND item = dw_menu_append_item(menuWindows, val, DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU);
                
                dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(menu_show_window), DW_POINTER(preview));
                dw_window_set_data(preview, "dwib_menu", DW_POINTER(item));
                
                dw_signal_connect(preview, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(preview_delete), NULL);
                
                /* And show the preview window */
                dwib_show(preview);
            }
            else
                dw_messagebox(APP_NAME, DW_MB_OK | DW_MB_ERROR, "Failed to load window definition.");
        }
        else
            dw_messagebox(APP_NAME, DW_MB_OK | DW_MB_ERROR, "Could not find a window title to load.");
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
        dw_messagebox(APP_NAME, DW_MB_OK, "No node selected.");
        return FALSE;
    }
    if(dw_messagebox(APP_NAME, DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to remove the current node (%s)?", 
                     node && node->name ? (char *)node->name : ""))
    {
        HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
        HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
        
        /* Remove the properties */
        dw_window_destroy(vbox);
        properties_none();
        
        DWCurrNode = xmlDocGetRootElement(DWDoc);
        /* If there are previews attached destroy them */
        deleteControl(node);
        /* Destroy preview before unlinking */
        xmlUnlinkNode(node);
        dw_tree_item_delete(tree, (HTREEITEM)node->_private);
        /* Then free the node and its children */
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
        dw_messagebox(APP_NAME, DW_MB_OK, "No node selected.");
        return FALSE;
    }
    if(dw_messagebox(APP_NAME, DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to cut the current node (%s) to the clipboard?", 
                     node && node->name ? (char *)node->name : ""))
    {
        HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
        HWND vbox = (HWND)dw_window_get_data(hwndProperties, "box");
        
        /* Remove the properties */
        dw_window_destroy(vbox);
        properties_none();
        
        DWCurrNode = xmlDocGetRootElement(DWDoc);
        /* If there are previews attached destroy them */
        deleteControl(node);
        /* Destroy preview before unlinking */
        xmlUnlinkNode(node);
        dw_tree_item_delete(tree, (HTREEITEM)node->_private);
        if(DWClipNode)
            xmlFreeNode(DWClipNode);
        DWClipNode = node;
        /* And clear them out from the tree */
        clearPreview(node);
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
        dw_messagebox(APP_NAME, DW_MB_OK, "No node selected.");
        return FALSE;
    }
    if(DWClipNode)
        xmlFreeNode(DWClipNode);
    DWClipNode = xmlCopyNode(node, 1);
    /* Make sure we didn't copy any preview handles */
    DWClipNode->psvi = NULL;
    clearPreview(DWClipNode);
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

/* Helper to make a copy of a node */
xmlNodePtr copyNode(HWND tree, xmlNodePtr node)
{
    xmlNodePtr this = _dwib_find_child(node, "Children");
    
    if(this)
    {
        xmlNodePtr copy = xmlCopyNode(DWClipNode, 1);
    
        xmlAddChild(this, copy);
    
        handleChildren(node, tree, copy, getPrevNode(copy));

        dw_tree_item_select(tree, (HTREEITEM)node->_private);
        
        return copy;
    }
    return NULL;
}

/* Handle paste a node layout */
int DWSIGNAL paste_clicked(HWND button, void *data)
{
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    xmlNodePtr node = DWClipNode && strcmp((char *)DWClipNode->name, "NotebookPage") == 0 ?
                      packablePageNode(data) : packableNode(data);
    
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
                        dw_messagebox(APP_NAME, DW_MB_OK, "Splitbars can't have more than two children packed.");
                        return FALSE;
                    }
                }
            }
        }
    }
    
    if(strcmp((char *)node->name, "DynamicWindows") == 0)
    {
        dw_messagebox(APP_NAME, DW_MB_OK, "No node selected.");
        return FALSE;
    }
    else if(strcmp((char *)node->name, "Window") != 0 && strcmp((char *)DWClipNode->name, "Menu") == 0)
    {
        dw_messagebox(APP_NAME, DW_MB_OK, "The menu on the clipboard needs to be pasted into a top-level window.");
        return FALSE;
    }
    else if(strcmp((char *)DWClipNode->name, "NotebookPage") == 0)
    {
        if(strcmp((char *)node->name, "Notebook") != 0)
            dw_messagebox(APP_NAME, DW_MB_OK, "The notebook page on the clipboard needs to be pasted into a notebook widget.");
        else
        {
            /* Actually copy the node */
            xmlNodePtr copy = copyNode(tree, node);
            
            /* Try to update the preview window if there is one */
            if(node->psvi && copy)
                previewControl(copy);
        }
        return FALSE;
    }
    else if(is_packable(node, FALSE))
    {
        /* Actually copy the node */
        xmlNodePtr copy = copyNode(tree, node);
        
        /* Try to update the preview window if there is one */
        if(node->psvi && copy)
            previewControl(copy);
    }
    return FALSE;
}

/* One of the buttons on the toolbar was clicked */
int DWSIGNAL toolbar_clicked(HWND button, void *data)
{
    int which = DW_POINTER_TO_INT(data);
    xmlNodePtr currentNode = (which == TYPE_NOTEBOOK_PAGE && packablePageNode(DWCurrNode)) ? packablePageNode(DWCurrNode) :
                                ((which == TYPE_WINDOW || (which == TYPE_MENU && is_menu(DWCurrNode))) 
                                ? DWCurrNode : packableNode(DWCurrNode));
    
    if(!data || !currentNode)
    {
        return FALSE;
    }
    
    /* Save existing data... if any... here */
    save_properties();
    
    /* Check for special case of splitbar */
    if(strcmp((char *)currentNode->name, "Box") == 0)
    {
        xmlNodePtr this;
        
        if((this = _dwib_find_child(currentNode, "subtype")))
        {
            char *thisval;
            
            if((thisval = (char *)xmlNodeListGetString(DWDoc, this->children, 1)))
            {
                if(strcmp(thisval, "Splitbar") == 0)
                {
                    int count = count_children(currentNode);
                    
                    if(count > 1)
                    {
                        dw_messagebox(APP_NAME, DW_MB_ERROR | DW_MB_OK, "Splitbars can't have more than two children packed.");
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
        if(strcmp((char *)currentNode->name, "Notebook") == 0)
        {
            properties_notebook_page(NULL);
        }
        else
        {
            dw_messagebox(APP_NAME, DW_MB_OK | DW_MB_ERROR, "Selected widget needs to be a notebook.");
        }
    }
    else if(which == TYPE_MENU)
    {
        if(strcmp((char *)currentNode->name, "Window") == 0 ||
           strcmp((char *)currentNode->name, "Menu") == 0)
        {
            properties_menu(NULL);
        }
        else
        {
            dw_messagebox(APP_NAME, DW_MB_OK | DW_MB_ERROR, "Selected widget needs to be a window or menu.");
        }
    }
    else if(is_packable(currentNode, TRUE))
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
    HWND vbox;
        
    if(DWCurrNode && DWCurrNode->name)
    {
        if(strcmp((char *)DWCurrNode->name, "Window") == 0)
        {
            properties_window(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_WINDOW));
        }
        else if(strcmp((char *)DWCurrNode->name, "Box") == 0)
        {
            properties_box(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_BOX));
        }
        else if(strcmp((char *)DWCurrNode->name, "Text") == 0)
        {
            properties_text(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_TEXT));
        }
        else if(strcmp((char *)DWCurrNode->name, "Entryfield") == 0)
        {
            properties_entryfield(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_ENTRYFIELD));
        }
        else if(strcmp((char *)DWCurrNode->name, "Combobox") == 0)
        {
            properties_combobox(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_COMBOBOX));
        }
        else if(strcmp((char *)DWCurrNode->name, "Listbox") == 0)
        {
            properties_listbox(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_LISTBOX));
        }
        else if(strcmp((char *)DWCurrNode->name, "Container") == 0)
        {
            properties_container(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_CONTAINER));
        }
        else if(strcmp((char *)DWCurrNode->name, "Tree") == 0)
        {
            properties_tree(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_TREE));
        }
        else if(strcmp((char *)DWCurrNode->name, "MLE") == 0)
        {
            properties_mle(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_MLE));
        }
        else if(strcmp((char *)DWCurrNode->name, "Render") == 0)
        {
            properties_render(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_RENDER));
        }
        else if(strcmp((char *)DWCurrNode->name, "Button") == 0)
        {
            properties_button(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_BUTTON));
        }
        else if(strcmp((char *)DWCurrNode->name, "Ranged") == 0)
        {
            properties_ranged(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_RANGED));
        }
        else if(strcmp((char *)DWCurrNode->name, "Bitmap") == 0)
        {
            properties_bitmap(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_BITMAP));
        }
        else if(strcmp((char *)DWCurrNode->name, "Notebook") == 0)
        {
            properties_notebook(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_NOTEBOOK));
        }
        else if(strcmp((char *)DWCurrNode->name, "NotebookPage") == 0)
        {
            properties_notebook_page(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_NOTEBOOK_PAGE));
        }
        else if(strcmp((char *)DWCurrNode->name, "HTML") == 0)
        {
            properties_html(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_HTML));
        }
        else if(strcmp((char *)DWCurrNode->name, "Calendar") == 0)
        {
            properties_calendar(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_CALENDAR));
        }
        else if(strcmp((char *)DWCurrNode->name, "Padding") == 0)
        {
            properties_padding(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_PADDING));
        }
        else if(strcmp((char *)DWCurrNode->name, "Menu") == 0)
        {
            properties_menu(DWCurrNode);
            dw_window_set_data(hwndProperties, "type", DW_INT_TO_POINTER(TYPE_MENU));
        }
    }
    
    if((vbox = (HWND)dw_window_get_data(hwndProperties, "box")))
        dw_window_set_data(vbox, "node", DW_POINTER(DWCurrNode));    
}

/* Enable and disable toolbar buttons based on the tree selection */
void toolbar_select(xmlNodePtr node)
{
    if(node && packableNode(node))
    {
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "box"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "text"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "entryfield"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "combobox"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "listbox"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "container"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "tree"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "mle"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "render"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "button"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "ranged"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "bitmap"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "notebook"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "html"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "calendar"));
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "padding"));
    }
    else
    {
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "box"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "text"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "entryfield"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "combobox"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "listbox"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "container"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "tree"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "mle"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "render"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "button"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "ranged"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "bitmap"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "notebook"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "html"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "calendar"));
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "padding"));
    }
    
    if(node && packablePageNode(node))
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "nb page"));
    else
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "nb page"));
    
    if(node && packableMenu(node))
        dw_window_enable((HWND)dw_window_get_data(hwndToolbar, "menu"));
    else
        dw_window_disable((HWND)dw_window_get_data(hwndToolbar, "menu"));
        
}

/* Handle loading a new item when selectng the tree */
int DWSIGNAL tree_select(HWND window, HTREEITEM item, char *text, void *data, void *itemdata)
{
    /* Reset the locale window */
    locale_manager_reset(NULL);
    
    /* Save existing data... if any... here */
    save_properties();
    
    DWCurrNode = itemdata;
    
    properties_current();
    
    toolbar_select(DWCurrNode);
    
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
    if(dw_messagebox(APP_NAME, DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to exit Interface Builder?"))
        dw_main_quit();
    return TRUE;
}

/* Handle moving a node up */
int DWSIGNAL up_clicked(HWND button, void *data)
{
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    xmlNodePtr node = data, prevNode;
    
    if(!node)
        return FALSE;
    
    if((prevNode = getPrevNode(data)))
    {
        /* If there are previews attached destroy them */
        deleteControl(node);
        /* Destroy preview before unlinking */
        xmlAddPrevSibling(prevNode, node);
        if(node->parent)
            handleChildren(node->parent, tree, prevNode, node);
        dw_tree_item_select(tree, (HTREEITEM)node->_private);
        /* Try to update the preview window if there is one */
        if(prevNode->psvi)
            previewControl(node);
    }
    return FALSE;
}

/* Handle moving a node down */
int DWSIGNAL down_clicked(HWND button, void *data)
{
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    xmlNodePtr node = data, nextNode;
    
    if(!node)
        return FALSE;
    
    if((nextNode = getNextNode(node)))
    {
        /* If there are previews attached destroy them */
        deleteControl(node);
        /* Destroy preview before unlinking */
        xmlAddNextSibling(nextNode, node);
        if(node->parent)
            handleChildren(node->parent, tree, node, nextNode);
        dw_tree_item_select(tree, (HTREEITEM)node->_private);
        /* Try to update the preview window if there is one */
        if(nextNode->psvi)
            previewControl(node);
    }
    return FALSE;
}

/* Pop up a tree context menu */
int DWSIGNAL tree_context(HWND window, char *text, int x, int y, void *data, void *itemdata)
{
    xmlNodePtr node = findWindow(itemdata);
    HMENUI menu = dw_menu_new(0);
    HWND item = dw_menu_append_item(menu, "~Up", DW_MENU_POPUP, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(up_clicked), itemdata);
    item = dw_menu_append_item(menu, "~Down", DW_MENU_POPUP, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(down_clicked), itemdata);
    item = dw_menu_append_item(menu, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
    item = dw_menu_append_item(menu, "C~opy", DW_MENU_POPUP, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(copy_clicked), itemdata);
    item = dw_menu_append_item(menu, "C~ut", DW_MENU_POPUP, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(cut_clicked), itemdata);
    item = dw_menu_append_item(menu, "D~elete", DW_MENU_POPUP, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(delete_clicked), itemdata);
    if(DWClipNode)
    {
        item = dw_menu_append_item(menu, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
        item = dw_menu_append_item(menu, "~Paste", DW_MENU_POPUP, 0, TRUE, FALSE, DW_NOMENU);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(paste_clicked), itemdata);
    }
    item = dw_menu_append_item(menu, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
    item = dw_menu_append_item(menu, "Expand All", DW_MENU_POPUP, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(expand_item_clicked), itemdata);
    item = dw_menu_append_item(menu, "Collapse All", DW_MENU_POPUP, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(collapse_item_clicked), itemdata);
    item = dw_menu_append_item(menu, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
    item = dw_menu_append_item(menu, (node && node->psvi) ? "~Refresh" : "Preview", DW_MENU_POPUP, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(refresh_clicked), itemdata);
    if(node && node->psvi)
    {
        item = dw_menu_append_item(menu, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
        item = dw_menu_append_item(menu, "Close Preview", DW_MENU_POPUP, 0, TRUE, FALSE, DW_NOMENU);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(preview_delete), node->psvi);
    }
    
    dw_menu_popup(&menu, hwndToolbar, x, y);
    return FALSE;
}

/* Handle toggling auto-expand */
int DWSIGNAL properties_inspector_clicked(HWND button, void *data)
{
    PropertiesInspector = !PropertiesInspector;
    dw_window_set_style(button, PropertiesInspector ? DW_MIS_CHECKED : DW_MIS_UNCHECKED, PropertiesInspector ? DW_MIS_CHECKED : DW_MIS_UNCHECKED);
    if(PropertiesInspector)
        dw_window_show(hwndProperties);
    else
        dw_window_hide(hwndProperties);
    saveconfig();
    return FALSE;
}

/* Handle toggling auto-expand */
int DWSIGNAL auto_expand_clicked(HWND button, void *data)
{
    AutoExpand = !AutoExpand;
    dw_window_set_style(button, AutoExpand ? DW_MIS_CHECKED : DW_MIS_UNCHECKED, AutoExpand ? DW_MIS_CHECKED : DW_MIS_UNCHECKED);
    saveconfig();
    return FALSE;
}

/* Handle toggling live preview */
int DWSIGNAL live_preview_clicked(HWND button, void *data)
{
    LivePreview = !LivePreview;
    dw_window_set_style(button, LivePreview ? DW_MIS_CHECKED : DW_MIS_UNCHECKED, LivePreview ? DW_MIS_CHECKED : DW_MIS_UNCHECKED);
    saveconfig();
    return FALSE;
}

/* Handle toggling bitmap button toolbar */
int DWSIGNAL bitmap_buttons_clicked(HWND button, void *data)
{
    BitmapButtons = !BitmapButtons;
    dw_window_set_style(button, BitmapButtons ? DW_MIS_CHECKED : DW_MIS_UNCHECKED, BitmapButtons ? DW_MIS_CHECKED : DW_MIS_UNCHECKED);
    saveconfig();
    if(BitmapButtons)
        toolbar_bitmap_buttons_create();
    else
        toolbar_text_buttons_create();
    toolbar_select(DWCurrNode);
    return FALSE;
}

/* Handle expanding or collapsing all */
int DWSIGNAL expand_all_clicked(HWND button, void *data)
{
    int expand = DW_POINTER_TO_INT(data);
    xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
    HWND tree = (HWND)dw_window_get_data(hwndToolbar, "treeview");
    
    if(rootNode && tree)
        expandCollapseTree(rootNode, tree, expand);
    return FALSE;
}

/* Generic window close handler */
int DWSIGNAL generic_delete(HWND window, void *data)
{
    HWND item = data ? (HWND)data : window;
    HWND menu = (HWND)dw_window_get_data(item ? item : window, "dwib_menu");
    
    /* Destroy associated menu if any */ 
    if(menu)
        dw_window_destroy(menu);
    
    /* Destroy the window */
    dw_window_destroy(item);
    
    /* If it is one of our single windows...
     * then reset the global handle to 0
     */
    if(item == hwndAbout)
        hwndAbout = 0;
    return FALSE;
}

/* Handle web back navigation */
int DWSIGNAL web_back_clicked(HWND button, void *data)
{
    HWND html = (HWND)data;
    
    dw_html_action(html, DW_HTML_GOBACK);
    return FALSE;
}

/* Handle web forward navigation */
int DWSIGNAL web_forward_clicked(HWND button, void *data)
{
    HWND html = (HWND)data;
    
    dw_html_action(html, DW_HTML_GOFORWARD);
    return FALSE;
}

/* Handle web reload */
int DWSIGNAL web_reload_clicked(HWND button, void *data)
{
    HWND html = (HWND)data;
    
    dw_html_action(html, DW_HTML_RELOAD);
    return FALSE;
}

/* Handle loading a web page */
int DWSIGNAL web_page_clicked(HWND button, void *data)
{
    char *url = data;
    
    if(url)
    {
        HWND html = dw_html_new(0);
        
        if(html)
        {
            /* We have access to the HTML widget so create a browser window */
            HWND window = dw_window_new(DW_DESKTOP, APP_NAME " Browser", DW_FCF_COMPOSITED |
                                        DW_FCF_TITLEBAR | DW_FCF_MINMAX | DW_FCF_SYSMENU | DW_FCF_TASKLIST | DW_FCF_SIZEBORDER);
            HWND vbox = dw_box_new(DW_VERT, 0);
            HWND hbox = dw_box_new(DW_HORZ, 0);
            HWND item = dw_menu_append_item(menuWindows, APP_NAME " Browser", DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU);
            
            dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(menu_show_window), DW_POINTER(window));
            dw_window_set_data(window, "dwib_menu", DW_POINTER(item));
            
            dw_box_pack_start(window, vbox, 0, 0, TRUE, TRUE, 0);
            dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);
            
            /* Add navigation buttons */
            item = dw_button_new("Back", 0);
            dw_box_pack_start(hbox, item, -1, -1, FALSE, FALSE, 0);
            dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(web_back_clicked), DW_POINTER(html));
            
            item = dw_button_new("Forward", 0);
            dw_box_pack_start(hbox, item, -1, -1, FALSE, FALSE, 0);
            dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(web_forward_clicked), DW_POINTER(html));
            
            /* Put in some extra space */
            dw_box_pack_start(hbox, 0, 5, 1, FALSE, FALSE, 0);
            
            item = dw_button_new("Reload", 0);
            dw_box_pack_start(hbox, item, -1, -1, FALSE, FALSE, 0);
            dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(web_reload_clicked), DW_POINTER(html));
              
            /* Pack in the HTML widget */
            dw_box_pack_start(vbox, html, 1, 1, TRUE, TRUE, 0);
            
            dw_signal_connect(window, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(generic_delete), NULL);
            
            dw_html_url(html, url);
            
            /* Setup the size */
            dw_window_set_size(window, 800, 600);
            dw_window_show(window);
        }
        else
        {
            /* Otherwise fall back to launching a browser */
            dw_browse(url);
        }
    }
    return FALSE;
}

/* Handle creating about box */
int DWSIGNAL about_clicked(HWND button, void *data)
{
    if(hwndAbout)
        dw_window_show(hwndAbout);
    else 
    {
        /* We have access to the HTML widget so create a browser window */
        HWND vbox = dw_box_new(DW_VERT, 0);
        HWND hbox = dw_box_new(DW_HORZ, 0);
        HWND item = dw_text_new(APP_NAME, 0);
        char verbuf[100] = {0};
        
        hwndAbout = dw_window_new(DW_DESKTOP, "About", DW_FCF_COMPOSITED |
                                  DW_FCF_TITLEBAR | DW_FCF_SYSMENU | DW_FCF_TASKLIST | DW_FCF_DLGBORDER);
        
        /* About text */
        dw_window_set_style(item, DW_DT_CENTER, DW_DT_CENTER);
        dw_box_pack_start(hwndAbout, vbox, 0, 0, TRUE, TRUE, 0);
        dw_box_pack_start(vbox, item, -1, -1, TRUE, FALSE, 0);
        item = dw_text_new("Brian Smith © 2011-2012", 0);
        dw_window_set_style(item, DW_DT_CENTER, DW_DT_CENTER);
        dw_box_pack_start(vbox, item, -1, -1, TRUE, FALSE, 0);
        sprintf(verbuf, "%d.%d.%d", VER_MAJ, VER_MIN, VER_REV);
        item = dw_text_new(verbuf, 0);
        dw_window_set_style(item, DW_DT_CENTER, DW_DT_CENTER);
        dw_box_pack_start(vbox, item, -1, -1, TRUE, FALSE, 0);
        dw_box_pack_start(vbox, 0, 1, 1, TRUE, TRUE, 0);
        
        /* Button box */
        dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);
        dw_box_pack_start(hbox, 0, 1, 1, TRUE, FALSE, 0);
        item = dw_button_new("Ok", 0);
        dw_box_pack_start(hbox, item, 60, -1, FALSE, FALSE, 0);
        dw_box_pack_start(hbox, 0, 1, 1, TRUE, FALSE, 0);
        
        /* Delete handlers */
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(generic_delete), DW_POINTER(hwndAbout));
        dw_signal_connect(hwndAbout, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(generic_delete), NULL);
        
        dw_window_set_size(hwndAbout, 250, 120);
        dw_window_show(hwndAbout);
    }
    return FALSE;
}

/* Handle browsing for a image root folder */
int DWSIGNAL image_browse_clicked(HWND button, void *data)
{
    HWND window = (HWND)data;
    HWND entry = (HWND)dw_window_get_data(window, "_dwib_directory");
    char *picked, *defpath = dw_window_get_text(entry);
    
    if(entry && (picked = dw_file_browse("Pick developer images folder:", defpath, "", DW_DIRECTORY_OPEN)))
    {
        xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
        xmlNodePtr imageNode = _dwib_find_child(rootNode, "ImageRoot");
        
        /* Create or update the node with the new image root */
        if(!imageNode)
            imageNode = xmlNewTextChild(rootNode, NULL, (xmlChar *)"ImageRoot", (xmlChar *)picked);
        else
            xmlNodeSetContent(imageNode, (xmlChar *)picked);
            
        /* Update the UI and internal loading */
        dw_window_set_text(entry, picked);
        dwib_image_root_set(picked);
        dw_free(picked);
    }
    if(defpath)
        dw_free(defpath);
    return TRUE;
}

/* Handle browsing for a image root folder */
int DWSIGNAL image_add_clicked(HWND button, void *data)
{
    HWND window = (HWND)data;
    HWND entry = (HWND)dw_window_get_data(window, "_dwib_directory");
    HWND cont = (HWND)dw_window_get_data(window, "_dwib_imagelist");
    char *file, *path = dw_window_get_text(entry);
    xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
    xmlNodePtr imageNode = _dwib_find_child(rootNode, "ImageRoot");
    
    /* Update the image root if it changed */
    if(imageNode)
    {
        char *imageroot = (char *)xmlNodeListGetString(DWDoc, imageNode->children, 1);
        
        /* Update the node unless the text is the same, or both are NULL */
        if(!((imageroot && path && strcmp(imageroot, path) == 0) ||
            (path == imageroot)))
        {
            xmlNodeSetContent(imageNode, (xmlChar *)path);
            dwib_image_root_set(path);
        }
    }
    else if(!imageNode && path && *path)
    {
        /* Add an image root if it doesn't exist and has content */
        imageNode = xmlNewTextChild(rootNode, NULL, (xmlChar *)"ImageRoot", (xmlChar *)path);
        dwib_image_root_set(path);
    }
    
    /* Try to select an image file */
    if((file = dw_file_browse("Pick an image file:", path, NULL, DW_FILE_OPEN)) != NULL)
    {
        if(*file)
        {
            HICN icon = dw_icon_load_from_file(file);
            int len = _dwib_image_root ? strlen(_dwib_image_root) : 0;
            void *continfo = dw_container_alloc(cont, 1);
            unsigned long iid = 0;
            char *val = file, *embedded = "No";
            
            /* Add a new image node */
            if(len && strlen(file) > len && memcmp(file, _dwib_image_root, len) == 0)
            {
                val = &file[len];
                /* Push past any initial separator */
                if(*val == '/' || *val == '\\')
                    val++;
            }
                
            imageNode = xmlNewTextChild(rootNode, NULL, (xmlChar *)"Image", (xmlChar *)val);
            val = (char *)xmlNodeListGetString(DWDoc, imageNode->children, 1);
            
            dw_filesystem_set_file(cont, continfo, 0, val ? val : "", icon);
            dw_filesystem_set_item(cont, continfo, 0, 0, &iid);
            dw_filesystem_set_item(cont, continfo, 1, 0, &embedded);
            dw_container_set_row_data(continfo, 0, imageNode);
            
            /* Actually insert and optimize */
            dw_container_insert(cont, continfo, 1);
            dw_container_optimize(cont);
        }
        dw_free(file);
    }
    if(path)
        dw_free(path);
    return TRUE;
}

/* Handle browsing for a image root folder */
int DWSIGNAL image_rem_clicked(HWND button, void *data)
{
    HWND window = (HWND)data;
    HWND cont = (HWND)dw_window_get_data(window, "_dwib_imagelist");
    xmlNodePtr selectedNode = (xmlNodePtr)dw_container_query_start(cont, DW_CRA_SELECTED);
    
    /* See if anything is selected */
    if(selectedNode)
    {
        /* Remove the selected row from the container */
        dw_container_delete_row(cont, (char *)selectedNode);
        /* Destroy any preview windows attached to this image */
        if(selectedNode->_private)
            dw_window_destroy((HWND)selectedNode->_private);
        /* Free any cached icons */
        if(selectedNode->psvi)
            dw_icon_free((HICN)selectedNode->psvi);
        /* Unlink and free the node from the XML */
        xmlUnlinkNode(selectedNode);
        xmlFreeNode(selectedNode);
    }
    return TRUE;
}

/* Populate or repopulate the image list */
void populateImages(HWND item, xmlNodePtr rootNode)
{
    int len = _dwib_image_root ? strlen(_dwib_image_root) : 0;
    xmlNodePtr imageNode = rootNode->children;
    
    while(imageNode)
    {
        if(imageNode->name && strcmp((char *)imageNode->name, "Image") == 0)
        {
            void *continfo = dw_container_alloc(item, 1);
            unsigned long iid = 0;
            char *embedded = "No";
            char *val = (char *)xmlNodeListGetString(DWDoc, imageNode->children, 1);
            char *file = val;
            HICN icon = (HICN)imageNode->psvi;
            xmlNodePtr node = _dwib_find_child(imageNode, "ImageID");
            xmlNodePtr embedNode = _dwib_find_child(imageNode, "Embedded");
            
            /* Combine the image root and the relative path */
            if(len && val)
                file = _dwib_combine_path(len, val, alloca(len + strlen(val) + 2));
            
            /* If we don't have a cached icon... */
            if(!icon)
            {
                char *embeddata;
                
                if(embedNode && (embeddata = (char *)xmlNodeListGetString(DWDoc, embedNode->children, 1)) != NULL)
                {
                    char *data;
                    int length = strlen(embeddata);
                    
                    /* Attempt to decode embedded data */
                    if((data = _dwib_decode64_lines(embeddata, &length)) != NULL)
                    {
                        /* Don't reset the resource ID to 0 when 
                         * returning embedded data.
                         */
                        icon = dw_icon_load_from_data(data, length);
                        free(data);
                    }
                }
                if(!icon)
                {
                    /* Attempt to load one from the file */
                    icon = file ? dw_icon_load_from_file(file) : 0;
                }
                imageNode->psvi = DW_POINTER(icon);
            }
            
            /* Save for later use */
            file = val;
            
            /* Load the Icon ID if available */
            if(node && (val = (char *)xmlNodeListGetString(DWDoc, node->children, 1)) != NULL)
                iid = atoi(val);
            
            /* Check for embedded data */
            if(embedNode)
                embedded = "Yes";
                
            dw_filesystem_set_file(item, continfo, 0, file ? file : "", icon);
            dw_filesystem_set_item(item, continfo, 0, 0, &iid);
            dw_filesystem_set_item(item, continfo, 1, 0, &embedded);
            dw_container_set_row_data(continfo, 0, imageNode);
            
            /* Actually insert the row */
            dw_container_insert(item, continfo, 1);
        }
        imageNode=imageNode->next;
    }
    /* Finally optimize the container */
    dw_container_optimize(item);
}

/* Handle closing the image view window */
int DWSIGNAL image_view_delete(HWND window, xmlNodePtr imageNode)
{
    xmlNodePtr node = _dwib_find_child(imageNode, "ImageID");
    HWND hitemid = (HWND)dw_window_get_data(window, "_dwib_imageid");
    HWND hembed = (HWND)dw_window_get_data(window, "_dwib_embedded");
    int iid = 0, newid = (int)dw_spinbutton_get_pos(hitemid);
    int changed = 0, embedded = dw_checkbox_get(hembed);
    char *val = NULL;
    
    /* Load the Icon ID if available */
    if(node && (val = (char *)xmlNodeListGetString(DWDoc, node->children, 1)) != NULL)
        iid = atoi(val);
    
    /* If the new ID is 0 and there is a node */
    if(!newid && node)
    {
        /* Unlink and free the node from the XML */
        xmlUnlinkNode(node);
        xmlFreeNode(node);
        changed = 1;
    }
    
    /* If newid isn't 0 and it has changed */
    if(newid && newid != iid)
    {
        char tmpbuf[10];
        
        snprintf(tmpbuf, 10, "%d", newid);
        
        if(!node)
            node = xmlNewTextChild(imageNode, NULL, (xmlChar *)"ImageID", (xmlChar *)tmpbuf);
        else
            xmlNodeSetContent(node, (xmlChar *)tmpbuf);
        changed = 1;
    }
    
    /* Check for embedded data */
    if((node = _dwib_find_child(imageNode, "Embedded")) != NULL && !embedded)
    {
        /* Unlink and free the node from the XML */
        xmlUnlinkNode(node);
        xmlFreeNode(node);
        node = NULL;
        changed = 1;
    }
    
    /* Encode image data */
    if(!node && embedded)
    {
        char *file = (char *)xmlNodeListGetString(DWDoc, imageNode->children, 1);
        
        if(file)
        {
            int len = _dwib_image_root ? strlen(_dwib_image_root) : 0;
            struct dwstat st;
            
            if(len)
                file = _dwib_combine_path(len, file, alloca(len + strlen(file) + 2));
                
            /* Check to see if the file exists, and what the size is */
            if(stat(file, &st) == 0 && st.st_size > 0)
            {
                /* Allocate a buffer to hold the contents and open the file */
                char *imagedata = alloca(st.st_size);
                int fd = open(file, O_RDONLY|O_BINARY);
                
                if(fd != -1)
                {
                    /* Read the file data in and if successful... */
                    if(read(fd, imagedata, st.st_size) == st.st_size)
                    {
                        /* Create a node and base 64 encode the image data on it */
                        xmlTextWriterPtr writer = xmlNewTextWriterTree(DWDoc, imageNode, 0);
                        xmlTextWriterStartElement(writer, (xmlChar *)"Embedded");
                        xmlTextWriterWriteBase64(writer, imagedata, 0, st.st_size);
                        xmlTextWriterEndElement(writer);
                        xmlFreeTextWriter(writer);
                        changed = 1;
                    }
                    close(fd);
                }
            }
        }
    }
    
    /* If anything in the image has changed....
     * repopulate the image list container.
     */
    if(changed)
    {
        HWND cont = (HWND)dw_window_get_data(hwndImages, "_dwib_imagelist");
        
        dw_container_clear(cont, FALSE);
        populateImages(cont, xmlDocGetRootElement(DWDoc));
    }
    
    /* Clean up and destroy window */
    if(imageNode)
        imageNode->_private = NULL;
    dw_window_destroy(window);
    return TRUE;
}

/* Open a dialog for extra info on an image */
int DWSIGNAL image_view_enter(HWND cont, xmlNodePtr imageNode, void *data)
{
    char *file = imageNode ? (char *)xmlNodeListGetString(DWDoc, imageNode->children, 1) : NULL;

    /* If an image view for this image is already open... */
    if(imageNode && imageNode->_private)
        dw_window_show((HWND)imageNode->_private);
    else if(file && *file)
    {
        /* Create a new image view window */
        HWND window = dw_window_new(DW_DESKTOP, file, DW_FCF_MINMAX |
                                    DW_FCF_TITLEBAR | DW_FCF_SYSMENU | DW_FCF_TASKLIST | DW_FCF_SIZEBORDER);
        HWND vbox = dw_box_new(DW_VERT, 0);
        HWND hbox = dw_box_new(DW_HORZ, 0);
        HWND item = dw_text_new("Image ID: ", 0);
        xmlNodePtr node = _dwib_find_child(imageNode, "ImageID");
        char *val;
        
        /* Save the preview window on the node */
        imageNode->_private = DW_POINTER(window);
        
        /* First row... Image ID and Embedded */
        dw_window_set_style(item, DW_DT_CENTER | DW_DT_VCENTER, DW_DT_CENTER | DW_DT_VCENTER);
        dw_box_pack_start(window, vbox, 0, 0, TRUE, TRUE, 0);
        dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);
        dw_box_pack_start(hbox, item, -1, -1, FALSE, TRUE, 0);
        val = node ? (char *)xmlNodeListGetString(DWDoc, node->children, 1) : NULL;
        item = dw_spinbutton_new(val ? val : "0", 0);
        dw_spinbutton_set_limits(item, 2000, 0);
        dw_spinbutton_set_pos(item, val ? atoi(val) : 0);
        dw_box_pack_start(hbox, item, -1, -1, TRUE, FALSE, 0);
        dw_window_set_data(window, "_dwib_imageid", DW_POINTER(item));
        item = dw_checkbox_new("Embedded", 0);
        dw_box_pack_start(hbox, item, -1, -1, FALSE, FALSE, 0);
        dw_window_set_data(window, "_dwib_embedded", DW_POINTER(item));
        if((node = _dwib_find_child(imageNode, "Embedded")) != NULL)
            dw_checkbox_set(item, TRUE);

        /* Create the preview */
        item = dw_bitmap_new(0);
        if(node)
        {
            char *embeddata;
            
            if((embeddata = (char *)xmlNodeListGetString(DWDoc, node->children, 1)) != NULL)
            {
                char *data;
                int length = strlen(embeddata);
                
                /* Attempt to decode embedded data */
                if((data = _dwib_decode64_lines(embeddata, &length)) != NULL)
                {
                    /* Don't reset the resource ID to 0 when 
                     * returning embedded data.
                     */
                    dw_window_set_bitmap_from_data(item, 0,  data, length);
                    free(data);
                }
            }
        }
        else
        {
            int len = _dwib_image_root ? strlen(_dwib_image_root) : 0;
            
            if(len && file)
                file = _dwib_combine_path(len, file, alloca(len + strlen(file) + 2));
            
            dw_window_set_bitmap(item, 0, file);
        }
        dw_box_pack_start(vbox, item, -1, -1, TRUE, TRUE, 0);

        /* Connect signal handlers */
        dw_signal_connect(window, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(image_view_delete), DW_POINTER(imageNode));
        
        dw_window_set_size(window, 0, 0);
        dw_window_show(window);
    }
    return TRUE;
}

/* Handle closing the image manager window */
int DWSIGNAL image_manager_delete(HWND item, void *data)
{
    HWND window = data ? (HWND)data : item;
    HWND entry = (HWND)dw_window_get_data(window, "_dwib_directory");
    char *path = dw_window_get_text(entry);
    xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
    xmlNodePtr imageNode =  _dwib_find_child(rootNode, "ImageRoot");
    
    /* Update the image root if it changed */
    if(imageNode)
    {
        char *imageroot = (char *)xmlNodeListGetString(DWDoc, imageNode->children, 1);
        
        /* Update the node unless the text is the same, or both are NULL */
        if(!((imageroot && path && strcmp(imageroot, path) == 0) ||
            (path == imageroot)))
        {
            xmlNodeSetContent(imageNode, (xmlChar *)path);
            dwib_image_root_set(path);
        }
    }
    else if(!imageNode && path && *path)
    {
        /* Add an image root if it doesn't exist and has content */
        imageNode = xmlNewTextChild(rootNode, NULL, (xmlChar *)"ImageRoot", (xmlChar *)path);
        dwib_image_root_set(path);
    }

    /* Free the temporary memory if needed */
    if(path)
        dw_free(path);
    
    hwndImages = 0;
    
    /* Destroy any preview windows */
    imageNode = rootNode->children;
    while(imageNode)
    {
        if(imageNode->name && strcmp((char *)imageNode->name, "Image") == 0 && imageNode->_private)
        {
            dw_window_destroy((HWND)imageNode->_private);
            imageNode->_private = NULL;
        }
        imageNode=imageNode->next;
    }
    dw_window_destroy(window);
    return TRUE;
}

/* Handle creating image manager */
int DWSIGNAL image_manager_clicked(HWND button, void *data)
{
    if(hwndImages)
        dw_window_show(hwndImages);
    else 
    {
        /* We have access to the HTML widget so create a browser window */
        HWND vbox = dw_box_new(DW_VERT, 0);
        HWND hbox = dw_box_new(DW_HORZ, 0);
        HWND item = dw_text_new("Developer Image Root: ", 0);
        char *colnames[] = { "Image ID", "Embedded" };
        unsigned long coltypes[] = {   
                DW_CFA_ULONG | DW_CFA_RIGHT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR,
                DW_CFA_STRING | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR };     
        xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
        xmlNodePtr imageNode = _dwib_find_child(rootNode, "ImageRoot");
        char *val = imageNode ? (char *)xmlNodeListGetString(DWDoc, imageNode->children, 1) : NULL;
        
        hwndImages = dw_window_new(DW_DESKTOP, "Image Manager", DW_FCF_COMPOSITED | DW_FCF_MINMAX |
                                   DW_FCF_TITLEBAR | DW_FCF_SYSMENU | DW_FCF_TASKLIST | DW_FCF_SIZEBORDER);
        
        /* Developer Image Root Row */
        dw_window_set_style(item, DW_DT_CENTER | DW_DT_VCENTER, DW_DT_CENTER | DW_DT_VCENTER);
        dw_box_pack_start(hwndImages, vbox, 0, 0, TRUE, TRUE, 0);
        dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);
        dw_box_pack_start(hbox, item, -1, -1, FALSE, TRUE, 0);
        item = dw_entryfield_new(val ? val : "", 0);
        dw_box_pack_start(hbox, item, -1, -1, TRUE, FALSE, 0);
        dw_window_set_data(hwndImages, "_dwib_directory", DW_POINTER(item));
        item = dw_button_new("Browse", 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(image_browse_clicked), DW_POINTER(hwndImages));
        dw_box_pack_start(hbox, item, -1, -1, FALSE, FALSE, 0);
        
        /* Image list container */
        item = dw_container_new(0, FALSE);
        dw_box_pack_start(vbox, item, -1, -1, TRUE, TRUE, 0);
        dw_filesystem_setup(item, coltypes, colnames, 2);
        dw_window_set_data(hwndImages, "_dwib_imagelist", DW_POINTER(item));
        /* Populate the container */
        populateImages(item, rootNode);
        dw_signal_connect(item, DW_SIGNAL_ITEM_ENTER, DW_SIGNAL_FUNC(image_view_enter), DW_POINTER(hwndImages));
        
        /* Button box */
        hbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);
        item = dw_button_new("+", 0);
        dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(image_add_clicked), DW_POINTER(hwndImages));
        item = dw_button_new("-", 0);
        dw_box_pack_start(hbox, item, BUTTON_ICON_WIDTH, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(image_rem_clicked), DW_POINTER(hwndImages));
        dw_box_pack_start(hbox, 0, 1, 1, TRUE, FALSE, 0);
        item = dw_button_new("Done", 0);
        dw_box_pack_start(hbox, item, -1, BUTTON_ICON_HEIGHT, FALSE, FALSE, 0);
        
        /* Delete handlers */
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(image_manager_delete), DW_POINTER(hwndImages));
        dw_signal_connect(hwndImages, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(image_manager_delete), NULL);
        
        dw_window_set_size(hwndImages, 450, 400);
        dw_window_show(hwndImages);
    }
    return FALSE;
}

#define TOOLBAR_WIDTH   100
#define TOOLBAR_HEIGHT  30

void toolbar_text_buttons_create(void)
{
    HWND vbox = (HWND)dw_window_get_data(hwndToolbar, "vbox");
    HWND hbox = (HWND)dw_window_get_data(hwndToolbar, "hbox");
    HWND item;
    
    if(vbox)
        dw_window_destroy(vbox);
    
    vbox = dw_box_new(DW_VERT, 0);
    dw_window_set_data(hwndToolbar, "vbox", DW_POINTER(vbox));
    dw_box_pack_end(hbox, vbox, 0, 0, FALSE, TRUE, 0);
    item = dw_button_new("Window", 0);
    dw_window_set_data(hwndToolbar, "window", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_WINDOW));
    item = dw_button_new("Box", 0);
    dw_window_set_data(hwndToolbar, "box", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_BOX));
    item = dw_button_new("Text", 0);
    dw_window_set_data(hwndToolbar, "text", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_TEXT));
    item = dw_button_new("Entryfield", 0);
    dw_window_set_data(hwndToolbar, "entryfield", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_ENTRYFIELD));
    item = dw_button_new("Combobox", 0);
    dw_window_set_data(hwndToolbar, "combobox", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_COMBOBOX));
    item = dw_button_new("Listbox", 0);
    dw_window_set_data(hwndToolbar, "listbox", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_LISTBOX));
    item = dw_button_new("Container", 0);
    dw_window_set_data(hwndToolbar, "container", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_CONTAINER));
    item = dw_button_new("Tree", 0);
    dw_window_set_data(hwndToolbar, "tree", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_TREE));
    item = dw_button_new("MLE", 0);
    dw_window_set_data(hwndToolbar, "mle", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_MLE));
    item = dw_button_new("Render", 0);
    dw_window_set_data(hwndToolbar, "render", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_RENDER));
    item = dw_button_new("Button", 0);
    dw_window_set_data(hwndToolbar, "button", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_BUTTON));
    item = dw_button_new("Ranged", 0);
    dw_window_set_data(hwndToolbar, "ranged", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_RANGED));
    item = dw_button_new("Bitmap", 0);
    dw_window_set_data(hwndToolbar, "bitmap", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_BITMAP));
    item = dw_button_new("Notebook", 0);
    dw_window_set_data(hwndToolbar, "notebook", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_NOTEBOOK));
    item = dw_button_new("NB Page", 0);
    dw_window_set_data(hwndToolbar, "nb page", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_NOTEBOOK_PAGE));
    item = dw_button_new("HTML", 0);
    dw_window_set_data(hwndToolbar, "html", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_HTML));
    item = dw_button_new("Calendar", 0);
    dw_window_set_data(hwndToolbar, "calendar", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_CALENDAR));
    item = dw_button_new("Padding", 0);
    dw_window_set_data(hwndToolbar, "padding", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_PADDING));
    item = dw_button_new("Menu", 0);
    dw_window_set_data(hwndToolbar, "menu", DW_POINTER(item));
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_MENU));
    dw_box_pack_start(vbox, 0, 1, 1, TRUE, TRUE, 0);
}

#define BUTTON_PADDING 2

void toolbar_bitmap_buttons_create(void)
{
    HWND vbox = (HWND)dw_window_get_data(hwndToolbar, "vbox");
    HWND hbox = (HWND)dw_window_get_data(hwndToolbar, "hbox");
    HWND item;
    
    if(vbox)
        dw_window_destroy(vbox);
    
    vbox = dw_box_new(DW_VERT, 0);
    dw_window_set_data(hwndToolbar, "vbox", DW_POINTER(vbox));
    dw_box_pack_end(hbox, vbox, 0, 0, FALSE, TRUE, 0);
    item = dw_bitmapbutton_new("Window", TYPE_WINDOW + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "window", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_WINDOW));
    item = dw_bitmapbutton_new("Box", TYPE_BOX + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "box", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_BOX));
    item = dw_bitmapbutton_new("Text", TYPE_TEXT + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "text", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_TEXT));
    item = dw_bitmapbutton_new("Entryfield", TYPE_ENTRYFIELD + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "entryfield", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_ENTRYFIELD));
    item = dw_bitmapbutton_new("Combobox", TYPE_COMBOBOX + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "combobox", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_COMBOBOX));
    item = dw_bitmapbutton_new("Listbox", TYPE_LISTBOX + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "listbox", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_LISTBOX));
    item = dw_bitmapbutton_new("Container", TYPE_CONTAINER + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "container", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_CONTAINER));
    item = dw_bitmapbutton_new("Tree", TYPE_TREE + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "tree", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_TREE));
    item = dw_bitmapbutton_new("MLE", TYPE_MLE + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "mle", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_MLE));
    item = dw_bitmapbutton_new("Render", TYPE_RENDER + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "render", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_RENDER));
    item = dw_bitmapbutton_new("Button", TYPE_BUTTON + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "button", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_BUTTON));
    item = dw_bitmapbutton_new("Ranged", TYPE_RANGED + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "ranged", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_RANGED));
    item = dw_bitmapbutton_new("Bitmap", TYPE_BITMAP + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "bitmap", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_BITMAP));
    item = dw_bitmapbutton_new("Notebook", TYPE_NOTEBOOK + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "notebook", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_NOTEBOOK));
    item = dw_bitmapbutton_new("Notebook Page", TYPE_NOTEBOOK_PAGE + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "nb page", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_NOTEBOOK_PAGE));
    item = dw_bitmapbutton_new("HTML", TYPE_HTML + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "html", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_HTML));
    item = dw_bitmapbutton_new("Calendar", TYPE_CALENDAR + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "calendar", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_CALENDAR));
    item = dw_bitmapbutton_new("Padding", TYPE_PADDING + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "padding", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_PADDING));
    item = dw_bitmapbutton_new("Menu", TYPE_MENU + 100);
    dw_window_set_style(item, DW_BS_NOBORDER, DW_BS_NOBORDER);
    dw_window_set_data(hwndToolbar, "menu", DW_POINTER(item));
    dw_box_pack_start(vbox, item, -1, -1, FALSE, FALSE, BUTTON_PADDING);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), DW_INT_TO_POINTER(TYPE_MENU));
    dw_box_pack_start(vbox, 0, 1, 1, TRUE, TRUE, 0);
}

/* Handle changing the default locale */
int DWSIGNAL preview_locale_clicked(HWND item, void *data)
{
    xmlNodePtr node = data;
    char *localename = NULL;
    xmlNodePtr rootNode = xmlDocGetRootElement(DWDoc);
    xmlNodePtr localesNode = _dwib_find_child(rootNode, "Locales");
   
    if(node)
    {
        dw_window_set_style(hwndDefaultLocale, DW_MIS_UNCHECKED, DW_MIS_UNCHECKED);
        localename = (char *)node->name;
    }
    dwib_locale_set(localename);
    dw_window_set_style(item, DW_MIS_CHECKED, DW_MIS_CHECKED);
    
    if(localesNode)
    {
        xmlNodePtr p;
        
        for(p=localesNode->children;p;p = p->next)
        {
            if(p->psvi && (!localename || strcmp((char *)p->name, localename) != 0))
            {
                /* Make sure all others are unchecked */
                dw_window_set_style((HWND)p->psvi, DW_MIS_UNCHECKED, DW_MIS_UNCHECKED);
            }
        }
    }
    return FALSE;
}

void dwib_init(void)
{
    HWND vbox, hbox, item;
    HMENUI menu, submenu;
    int x;
    
    /* Enable builder mode */
    _dwib_builder_toggle(DW_POINTER(render_expose));
    
    hIcons[0] = (HICN)0;
    for(x=1;x<20;x++)
    {
        hIcons[x] = dw_icon_load(0, x + 100);
    }
    
    /* Create a new empty XML document */
    DWDoc = xmlNewDoc((xmlChar *)"1.0");
    DWCurrNode = xmlNewNode(NULL, (xmlChar *)"DynamicWindows");
    xmlDocSetRootElement(DWDoc, DWCurrNode);
    
    hwndToolbar = dw_window_new(DW_DESKTOP, APP_NAME, DW_FCF_COMPOSITED |
                                DW_FCF_TITLEBAR | DW_FCF_MINMAX | DW_FCF_SYSMENU | DW_FCF_TASKLIST | DW_FCF_SIZEBORDER);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_window_set_data(hwndToolbar, "hbox", DW_POINTER(hbox));
    dw_box_pack_start(hwndToolbar, hbox, 1, 1, TRUE, TRUE, 0);
    if(BitmapButtons)
        toolbar_bitmap_buttons_create();
    else
        toolbar_text_buttons_create();
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hbox, vbox, 0, 0, TRUE, TRUE, 0);
    item = dw_tree_new(0);
    dw_box_pack_start(vbox, item, 1, 1, TRUE, TRUE, 0);
    dw_signal_connect(item, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(tree_select), NULL);
    dw_signal_connect(item, DW_SIGNAL_ITEM_CONTEXT, DW_SIGNAL_FUNC(tree_context), NULL);
    dw_window_set_data(hwndToolbar, "treeview", DW_POINTER(item));
    
    menu = dw_menubar_new(hwndToolbar);
    /* Add File menu */
    submenu = dw_menu_new(0);
    item = dw_menu_append_item(submenu, "~New", DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(new_clicked), NULL);
    item = dw_menu_append_item(submenu, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
    item = dw_menu_append_item(submenu, "~Open", DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(open_clicked), NULL);
    item = dw_menu_append_item(submenu, "~Save", DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(save_clicked), NULL);
    item = dw_menu_append_item(submenu, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
    item = dw_menu_append_item(submenu, "~Exit", DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_delete), NULL);
    item = dw_menu_append_item(menu, "~File", DW_MENU_AUTO, 0, TRUE, FALSE, submenu);
    
    /* Add View menu */
    submenu = dw_menu_new(0);
    item = dw_menu_append_item(submenu, "Auto Expand", DW_MENU_AUTO, AutoExpand ? DW_MIS_CHECKED : 0, TRUE, TRUE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(auto_expand_clicked), NULL);
    item = dw_menu_append_item(submenu, "Live Preview", DW_MENU_AUTO, LivePreview ? DW_MIS_CHECKED : 0, TRUE, TRUE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(live_preview_clicked), NULL);
    item = dw_menu_append_item(submenu, "Visual Toolbar", DW_MENU_AUTO, BitmapButtons ? DW_MIS_CHECKED : 0, TRUE, TRUE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(bitmap_buttons_clicked), NULL);
    menuLocale = dw_menu_new(0);
    hwndDefaultLocale = dw_menu_append_item(menuLocale, "Default", DW_MENU_AUTO, DW_MIS_CHECKED, TRUE, TRUE, DW_NOMENU);
    dw_signal_connect(hwndDefaultLocale, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(preview_locale_clicked), NULL);
    item = dw_menu_append_item(menuLocale, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
    item = dw_menu_append_item(submenu, "Preview Locale", DW_MENU_AUTO, 0, TRUE, FALSE, menuLocale);
    item = dw_menu_append_item(submenu, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
    item = dw_menu_append_item(submenu, "Expand All", DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(expand_all_clicked), DW_INT_TO_POINTER(1));
    item = dw_menu_append_item(submenu, "Collapse All", DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(expand_all_clicked), NULL);
    item = dw_menu_append_item(menu, "~View", DW_MENU_AUTO, 0, TRUE, FALSE, submenu);
    
    /* Add Window menu */
    menuWindows = dw_menu_new(0);
    item = dw_menu_append_item(menuWindows, "Properties Inspector", DW_MENU_AUTO, PropertiesInspector ? DW_MIS_CHECKED : 0, TRUE, TRUE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(properties_inspector_clicked), NULL);
    item = dw_menu_append_item(menuWindows, "Image Manager", DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(image_manager_clicked), NULL);
    item = dw_menu_append_item(menuWindows, "Locale Manager", DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(locale_manager_clicked), NULL);
    item = dw_menu_append_item(menuWindows, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
    item = dw_menu_append_item(menu, "~Windows", DW_MENU_AUTO, 0, TRUE, FALSE, menuWindows);
    
    /* Add Help menu */
    submenu = dw_menu_new(0);
    item = dw_menu_append_item(submenu, "Web Page", DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(web_page_clicked), DW_POINTER("http://dbsoft.org/"));
    item = dw_menu_append_item(submenu, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
    item = dw_menu_append_item(submenu, "Help Contents", DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(web_page_clicked), DW_POINTER("http://www.dbsoft.org/newsitem.php?id=41"));
    item = dw_menu_append_item(submenu, DW_MENU_SEPARATOR, 0, 0, TRUE, FALSE, DW_NOMENU);
    item = dw_menu_append_item(submenu, "About", DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(about_clicked), NULL);
    item = dw_menu_append_item(menu, "~Help", DW_MENU_AUTO, 0, TRUE, FALSE, submenu);
    
    dw_signal_connect(hwndToolbar, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(toolbar_delete), NULL);
    dw_window_set_icon(hwndToolbar, DW_RESOURCE(ICON_APP));
    dw_window_set_gravity(hwndToolbar, DW_GRAV_LEFT | DW_GRAV_OBSTACLES, DW_GRAV_TOP | DW_GRAV_OBSTACLES);
    dw_window_set_pos_size(hwndToolbar, 20, 20, 600, 650);
    
    toolbar_select(DWCurrNode);
    
    hwndProperties = dw_window_new(DW_DESKTOP, "Properties Inspector", DW_FCF_TITLEBAR | DW_FCF_SIZEBORDER);
    properties_none();
    dw_signal_connect(hwndToolbar, DW_SIGNAL_SET_FOCUS, DW_SIGNAL_FUNC(toolbar_focus), NULL);
    dw_window_set_gravity(hwndProperties, DW_GRAV_LEFT | DW_GRAV_OBSTACLES, DW_GRAV_TOP | DW_GRAV_OBSTACLES);
    dw_window_set_pos_size(hwndProperties, 650, 20, 300, 550);
    if(PropertiesInspector)
        dw_window_show(hwndProperties);
    dw_window_show(hwndToolbar);
}

/* The main entry point.  Notice we don't use WinMain() on Windows */
int main(int argc, char *argv[])
{
    loadconfig();
    
    dw_init(TRUE, argc, argv);
    
    dwib_init();
    
    dw_main();

    saveconfig();
    
    dw_window_destroy(hwndProperties);
    dw_window_destroy(hwndToolbar);
    
    if(DWDoc)
        xmlFreeDoc(DWDoc);
    
    dw_exit(0);
    return 0;
}

