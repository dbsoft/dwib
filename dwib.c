/* Dynamic Windows Application: Interface Builder
 * Created by Dynamic Windows Interface Builder
 * Author: Brian Smith
 */

#include "resources.h"
#include "%APP%.h"


/* The main entry point.  Notice we don't use WinMain() on Windows */
int main(int argc, char *argv[])
{
    int cx, cy;
    
    dw_init(TRUE, argc, argv);
    
    dw_main();
}

