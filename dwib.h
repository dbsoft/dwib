/* Dynamic Windows Application: Interface Builder
 * Created by Dynamic Windows Interface Builder
 * Author: Brian Smith
 */

#ifndef DWIB_H
#define DWIB_H

#include "dw.h"

typedef void *DWIB;

HWND API dwib_load(DWIB handle, const char *name);
int API dwib_load_at_index(DWIB handle, const char *name, const char *dataname, HWND window, HWND box, int index);
void API dwib_show(HWND window);
DWIB API dwib_open_from_data(const char *buffer, int size);
DWIB API dwib_open(const char *filename);
void API dwib_close(DWIB handle);
int API dwib_image_root_set(const char *path);
int API dwib_locale_set(const char *loc);
HWND API dwib_window_get_handle(HWND handle, const char *dataname);

#endif
