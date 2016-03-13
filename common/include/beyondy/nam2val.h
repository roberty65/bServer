#ifndef __BEYONDY__NAM2VAL__H
#define __BEYONDY__NAM2VAL__H

namespace beyondy {

#ifndef ARRCNT
#define ARRCNT(a) (sizeof(a) / sizeof((a)[0]))
#endif /*! ARRCNT */

//
// must be { NULL, default } ended
// 
struct nam_val_struct {
	const char *nv_nam;
	int nv_val;
};

//
// name --> value
// or value --> name
const char *val2nam(const struct nam_val_struct *maps, int val);
int nam2val(const struct nam_val_struct *maps, const char *name, size_t nlen);

//
// name --> index(start from ~0)
// index --> name
//
const char *ind2nam(const char *arr[], size_t size, int ind);
int nam2ind(const char *arr[], size_t size, const char *name, size_t nlen = 0);

}; /* namespace beyondy */
#endif /*!__BEYONDY__NAM2VAL__H */
