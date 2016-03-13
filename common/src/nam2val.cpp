#include <cstring>
#include <beyondy/nam2val.h>

namespace beyondy {

const char *val2nam(const struct nam_val_struct *maps, int val)
{
	while (maps->nv_nam != NULL && maps->nv_val != val) {
		++maps;
	}

	return maps->nv_nam;
}

int nam2val(const struct nam_val_struct *maps, const char *name, size_t nlen)
{
	if (nlen == 0) nlen = strlen(name);
	while (maps->nv_nam != NULL && (strncmp(maps->nv_nam, name, nlen)) ) {
		++maps;
	}

	return maps->nv_val;
}

const char *ind2nam(const char *arr[], size_t size, int ind)
{
	if (ind < 0 || size_t(ind) >= size)
		return NULL;
	return arr[ind];
}

int nam2ind(const char *arr[], size_t size, const char *name, size_t nlen)
{
	if (nlen == 0) nlen = strlen(name);
	for (size_t i = 0; i < size; ++i) {
		if (strncmp(arr[i], name, nlen) == 0) {
			return (int)i;
		}
	}

	return -1;
}

}; /* namespace beyondy */
