#include <dw.h>
#include <dwib.h>

#define APP_NAME "DWIB Example"

/* Handle exiting the application */
int DWSIGNAL exit_handler(HWND win, void *data)
{
    HWND window = (HWND)data;
    
    if(dw_messagebox(APP_NAME, DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to exit"))
    {
        /* Exit the application cleanly */
        dw_main_quit();
    }
    return TRUE;
}

/* The main entry point.  Notice we don't use WinMain() on Windows */
int main(int argc, char *argv[])
{
    HWND window;
    DWIB handle;
   
    /* Initialize Dynamic Windows */
    dw_init(TRUE, argc, argv);
    
    /* Load the interface XML file */
    handle = dwib_open("example.xml");
    
    /* Show an error if it fails to load */
    if(!handle)
    {
        dw_messagebox(APP_NAME, DW_MB_OK | DW_MB_ERROR, "Unable to load the interface XML.");
        dw_exit(1);
    }
    
    /* Create the loading window... */
    window = dwib_load(handle, "Test");
    dwib_show(window);
    
    /* Connect the signal handlers */
    dw_signal_connect(window, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(exit_handler), (void *)window);
    
    dw_main();

    /* Destroy the main window */
    dw_window_destroy(window);
    /* Close the Interface Builder XML */
    dwib_close(handle);

    dw_exit(0);
    return 0;
}
