/* Dynamic Windows Application: Interface Builder
 * Created by Dynamic Windows Interface Builder
 * Author: Brian Smith
 */

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

xmlNodePtr findChildName(xmlNodePtr node, char *name);

void DWSIGNAL properties_window(xmlNodePtr node);
void DWSIGNAL properties_box(xmlNodePtr node);
