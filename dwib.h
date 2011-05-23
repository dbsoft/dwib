/* Dynamic Windows Application: Interface Builder
 * Created by Dynamic Windows Interface Builder
 * Author: Brian Smith
 */

typedef void *DWIB;

HWND API dwib_load(DWIB handle, char *name);
void API dwib_show(HWND window);
DWIB API dwib_open_from_data(char *buffer, int size);
DWIB API dwib_open(char *filename);
void API dwib_close(DWIB handle);