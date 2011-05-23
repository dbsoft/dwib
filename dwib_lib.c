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

/* Parse the children if packable widgets... boxes, notebook pages, etc */
void _dwib_children(xmlNodePtr node, xmlDocPtr doc)
{
    xmlNodePtr p = findChildName(node, "Children");
    char *val;
    
    for(p=p->children;p;p = p->next)
    {
        if(strcmp((char *)p->name, "Box") == 0)
        {
            xmlNodePtr this = findChildName(p, "subtype");
            
            _dwib_children(p, doc);
        }
        else if(strcmp((char *)p->name, "Notebook") == 0)
        {
            _dwib_children(p, doc);
        }
        else if(strcmp((char *)p->name, "NotebookPage") == 0)
        {
            xmlNodePtr this = findChildName(p, "title");
            
            val = (char *)xmlNodeListGetString(doc, this->children, 1);
            
            _dwib_children(p, doc);
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

/* Clears and reloads the tree data from XML */
void dwib_load(DWIB handle, char *name)
{
    xmlDocPtr doc = handle;
    xmlNodePtr p, rootNode = xmlDocGetRootElement(doc);
    
    if(!rootNode)
        return;
    
    for(p=rootNode->children;p;p = p->next)
    {
        if(strcmp((char *)p->name, "Window") == 0)
        {
            xmlNodePtr this = findChildName(p, "title");
            char *val = (char *)xmlNodeListGetString(doc, this->children, 1);
            
            _dwib_children(p, doc);
        }
    }
}
