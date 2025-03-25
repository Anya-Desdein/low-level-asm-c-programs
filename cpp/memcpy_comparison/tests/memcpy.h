#include <stddef.h>

#pragma once
typedef void *(*memcpy_t) (
	      void *restrict const, 
	const void *restrict const,
	size_t);
