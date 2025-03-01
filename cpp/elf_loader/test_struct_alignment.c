#include <stdio.h>
#include <stdlib.h>

int main() {
	struct al {
		short s;
		long long ll;
	} align;
	
	#pragma pack(4) //Set struct align to 4 bytes
	struct unal {
		short s; 
		long long ll;
	} unalign;

	#pragma pack() // Reset to default

	struct unal1 {
		short s;
		long long ll;
	} __attribute__((packed)) unalign1;

	printf("Aligned struct size: %zu\n Pack(4) struct size %zu\n __attrubute__((packed) size: %zu\n", sizeof(align), sizeof(unalign), sizeof(unalign1));

	struct definition {
		short s; 
		long long ll;
	} var1, var2;
	typedef struct definition alias;
	
	alias var3;
	struct definition var4;

	struct {
		short s; 
		long long ll;
	} var5;

	struct holz_type {
		struct {
			int a;
			int b;
		};

		struct {
			int c;
			int d;
		};
	};

	struct holz_type h;
	h.a = 10;
	printf("Anonymous nameless structs inside: %i\n", h.a);

	struct babel_type {
		struct {
			int a;
			int b;
		}named;

		struct {
			int a;
			int d;
		};
	};

	struct babel_type b;
	b.named.a = 10;
	b.a = 12;
	printf("Named: %i\nUnnamed: %i\n", b.named.a, b.a);

	union fancy {
		float as_float;
		unsigned int as_uint;
		char as_char;
	};

	union fancy mynumber;

	mynumber.as_float = 1.2;	
	printf("Type punning went wrong: %08x\n", mynumber.as_uint);

	printf("Type punning char: %hhx\n", mynumber.as_char);
	
	mynumber.as_float = 0.0;	
	printf("Type punning went right: %u\n", mynumber.as_uint);
	

	var3.ll = 12;
	var4.ll = 16;
	var2.ll = 8;
	var1.ll = 4;
	var5.ll = 2137;
	printf("Consecutive long values: %lld %lld %lld %lld %lld\n", var1.ll, var2.ll, var3.ll, var4.ll, var5.ll);

	union fancy2 {
		struct {
			unsigned char lsb;
			unsigned char padding1;
			unsigned char padding2;
			unsigned char msb;
		};

		struct {
			float f;
		};
	};

	union fancy2 ff;

	ff.f = 1.234;
	printf("Float: %f, %.02f\n", ff.f, ff.f);
	// True on Linux 64-bit
	printf("Junk from RSI: %08x\n", ff.f);
	printf("Least significant byte: %hhx, most: %hhx\n", ff.lsb, ff.msb);
}
