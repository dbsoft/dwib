/* Dynamic Windows Application: Interface Builder
 * Created by Dynamic Windows Interface Builder
 * Author: Brian Smith
 */

#ifndef DWIB_H
#define DWIB_H

#include <dw.h>

typedef void *DWIB;

HWND API dwib_load(DWIB handle, char *name);
int API dwib_load_at_index(DWIB handle, char *name, char *dataname, HWND window, HWND box, int index);
void API dwib_show(HWND window);
DWIB API dwib_open_from_data(char *buffer, int size);
DWIB API dwib_open(char *filename);
void API dwib_close(DWIB handle);

#endif
