void cmemcpy(
	      void* restrict dest_,
	const void* restrict src_,
	unsigned long long count
) {

	char *dest = (char *)dest_;
	char *src  = (char *)src_;
	
	while(count) {
		*dest = *src;
		dest++;
		src ++;

		count --;
	}
}
