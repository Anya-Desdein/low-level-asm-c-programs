#include <stddef.h>

void *cmemcpy(
	      void *restrict const dest_,
	const void *restrict const src_,
	size_t                     size) {

	char *dest = (char *)dest_;
	char *src  = (char *)src_;
	
	while(size) {
		*dest = *src;
		dest++;
		src ++;

		size --;
	}

	return dest_;
}
