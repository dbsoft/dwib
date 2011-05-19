/* Dynamic Windows Application: Interface Builder
 * Created by Dynamic Windows Interface Builder
 * Author: Brian Smith
 */

#include <dw.h>
#include <libxml/tree.h>
#include "resources.h"
#include "dwib.h"

HWND hwndToolbar, hwndProperties;
xmlDocPtr DWDoc;

#define PROPERTIES_HEIGHT 22
#define PROPERTIES_WIDTH 120

/* Populate the properties window for a window */
void DWSIGNAL properties_window(xmlNodePtr *node)
{
    HWND item, scrollbox, hbox, vbox = dw_window_get_data(hwndProperties, "box");
    
    /* TODO: Save existing data... if any... here */
    
    dw_window_destroy(vbox);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hwndProperties, vbox, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndProperties, "box", vbox);
    scrollbox = dw_scrollbox_new(DW_VERT, 0);
    dw_box_pack_start(vbox, scrollbox, 1, 1, TRUE, TRUE, 0);
    
    /* Create the actual properties - Title */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Title", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_entryfield_new("", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "title", item);
    /* Size */ 
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Size (width, height)", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_spinbutton_new("-1", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "width", item);
    item = dw_spinbutton_new("-1", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "height", item);
    /* Positon */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Position (x, y)", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_spinbutton_new("-1", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "x", item);
    item = dw_spinbutton_new("-1", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH/2, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(item, 2000, -1);
    dw_window_set_data(vbox, "y", item);
    /* Border */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Border", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_spinbutton_new("-1", 0);
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
    dw_window_set_data(vbox, "close", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("Minimize", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "minimize", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("Maximize", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "maximize", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("Hide", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "hide", item);
    /* Style */
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    item = dw_text_new("Style", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_style(item, DW_DT_VCENTER, DW_DT_VCENTER);
    item = dw_checkbox_new("Resize", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "resize", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("Dialog", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "dialog", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("Border", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "border", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("System Menu", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    dw_window_set_data(vbox, "sysmenu", item);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(scrollbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
    item = dw_checkbox_new("Task List", 0);
    dw_box_pack_start(hbox, item, PROPERTIES_WIDTH, PROPERTIES_HEIGHT, TRUE, FALSE, 0);
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
    dw_window_set_data(vbox, "orientation", item);    
    
    /* If it is a new window add button */
    if(!node)
    {
        item = dw_button_new("Create", 0);
        dw_box_pack_start(vbox, item, 1, 30, TRUE, FALSE, 0);
    }
    dw_window_redraw(hwndProperties);
}

/* One of the buttons on the toolbar was clicked */
int DWSIGNAL toolbar_clicked(HWND button, void *data)
{
    int which = (int)data;
    
    switch(which)
    {
        case 0:
            properties_window(NULL);
            break;
    }
    return FALSE;
}

/* Handles raising the properties inspector when the toolbar gets focus */
int DWSIGNAL toolbar_focus(HWND toolbar, void *data)
{
    dw_window_raise(hwndProperties);
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
    DWDoc = xmlNewDoc("1.0");
    
    hwndToolbar = dw_window_new(DW_DESKTOP, DWIB_NAME, 
                                DW_FCF_TITLEBAR | DW_FCF_MINMAX | DW_FCF_SYSMENU | DW_FCF_TASKLIST | DW_FCF_SIZEBORDER);
    hbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(hwndToolbar, hbox, 1, 1, TRUE, TRUE, 0);
    vbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(hbox, vbox, 0, 0, FALSE, FALSE, 0);
    item = dw_button_new("Window", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)0);
    item = dw_button_new("Box", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)1);
    item = dw_button_new("Text", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)2);
    item = dw_button_new("Entryfield", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)3);
    item = dw_button_new("Combobox", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)4);
    item = dw_button_new("Listbox", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)5);
    item = dw_button_new("Container", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)6);
    item = dw_button_new("Tree", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)7);
    item = dw_button_new("MLE", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)8);
    item = dw_button_new("Render", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)9);
    item = dw_button_new("Ranged", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)10);
    item = dw_button_new("Spinbutton", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)11);
    item = dw_button_new("Bitmap", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)12);
    item = dw_button_new("Notebook", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)13);
#ifndef __OS2__
    item = dw_button_new("HTML", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)14);
    item = dw_button_new("Calendar", 0);
    dw_box_pack_start(vbox, item, TOOLBAR_WIDTH, TOOLBAR_HEIGHT, FALSE, FALSE, 0);
    dw_signal_connect(item , DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(toolbar_clicked), (void *)15);
#endif
    item = dw_tree_new(0);
    dw_box_pack_start(hbox, item, 1, 1, TRUE, TRUE, 0);
    dw_window_set_data(hwndToolbar, "tree", item);
    dw_signal_connect(hwndToolbar, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(toolbar_delete), NULL);
    dw_window_set_pos_size(hwndToolbar, 20, 20, 600, 500);
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

