#ifndef __MAC__
/* XPMs */

/* Associated IDs */
#include "resources.h"

#define RESOURCE_MAX 0

long _resource_id[RESOURCE_MAX] = {
};

char *_resource_data[RESOURCE_MAX] = {
};

typedef struct _resource_struct {
	long resource_max, resource_id;
	char **resource_data;
} DWResources;

DWResources _resources = { RESOURCE_MAX, (long)_resource_id, (char **)_resource_data };
#endif
