#include <stdio.h>
#include <stdlib.h>

/* This is complicated... and stolen from Linux
In C, bit-field size must be non-negative
On positive, we're turning it to -1
On 0, it stays 0 */
#define BUILD_BUG_ON_ZERO(expr) ((int)(sizeof(struct { int:(-!!(expr)); })))

/* Are two types/vars the same (ignoring qualifiers)? 
^ Stolen from linux compiler_types.h :P */

/* If two values are of the same type, return 1, else 0  */
#define __same_type(a,b) __builtin_types_compatible_p(typeof(a), typeof(b))

/* Using macros above, 
If arr[x], it will end up as
0, ARR_EL_COUNT
But if it's a pointer, it will end up as
1...
and fail at compile time.

Due to comma operator, it will ignore the first arg and
pass the second one as the result.

Without the first argument it would be
, ARR_EL_COUNT
Which, on the other hand, is incorrect syntax and wouldn't work.
*/
#define ARRAY_SIZE(arr) \
	(BUILD_BUG_ON_ZERO(\
		__same_type((arr), &(arr)[0])), \
		(sizeof(arr) / sizeof((arr)[0])) \
	)

int main(int argc, char *argv[]) {
	
	// Testing ARRAY_SIZE
	int tab[12];
	printf("Correct array of size %d\n", ARRAY_SIZE(tab));

	//ARRAY_SIZE(&tab[0]);
	return 0;	
}
