/*
 * Copyright (C) 2017 Denys Vlasenko <vda.linux@googlemail.com>
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */
//config:config FACTOR
//config:	bool "factor"
//config:	default y
//config:	help
//config:	  factor factorizes integers

//applet:IF_FACTOR(APPLET(factor, BB_DIR_USR_BIN, BB_SUID_DROP))

//kbuild:lib-$(CONFIG_FACTOR) += factor.o

//usage:#define factor_trivial_usage
//usage:       "NUMBER..."
//usage:#define factor_full_usage "\n\n"
//usage:       "Print prime factors"

#include "libbb.h"

#if 0
# define dbg(...) bb_error_msg(__VA_ARGS__)
#else
# define dbg(...) ((void)0)
#endif

typedef unsigned long long wide_t;
#define WIDE_BITS (unsigned)(sizeof(wide_t)*8)
#define TOPMOST_WIDE_BIT ((wide_t)1 << (WIDE_BITS-1))

#if ULLONG_MAX == (UINT_MAX * UINT_MAX + 2 * UINT_MAX)
/* "unsigned" is half as wide as ullong */
typedef unsigned half_t;
#define HALF_MAX UINT_MAX
#define HALF_FMT ""
#elif ULLONG_MAX == (ULONG_MAX * ULONG_MAX + 2 * ULONG_MAX)
/* long is half as wide as ullong */
typedef unsigned long half_t;
#define HALF_MAX ULONG_MAX
#define HALF_FMT "l"
#else
#error Cant find an integer type which is half as wide as ullong
#endif

/* Returns such x that x+1 > sqrt(N) */
static inline half_t isqrt(wide_t N)
{
	wide_t x;
	unsigned c;

// Never called with N < 1
//	if (N == 0)
//		return 0;
//
	/* Count leading zeros */
	c = 0;
	while (!(N & TOPMOST_WIDE_BIT)) {
		c++;
		N <<= 1;
	}
	N >>= c;

	/* Make x > sqrt(n) */
	x = (wide_t)1 << ((WIDE_BITS + 1 - c) / 2);
	dbg("x:%llx", x);
	for (;;) {
		wide_t y = (x + N/x) / 2;
		dbg("y:%llx y^2:%llx (y+1)^2:%llx]", y, y*y, (y+1)*(y+1));
		if (y >= x) {
			/* Handle degenerate case N = 0xffffffffff...fffffff */
			if (y == (wide_t)HALF_MAX + 1)
				y--;
			dbg("isqrt(%llx)=%llx"HALF_FMT, N, y);
			return y;
		}
		x = y;
	}
}

static NOINLINE half_t isqrt_odd(wide_t N)
{
	half_t s = isqrt(N);
	if (s && !(s & 1)) /* even? */
		s--;
	return s;
}

static NOINLINE void factorize(wide_t N)
{
	half_t factor;
	half_t max_factor;
	unsigned count3;

	if (N < 4)
		goto end;

	while (!(N & 1)) {
		printf(" 2");
		N >>= 1;
	}

	max_factor = isqrt_odd(N);
	count3 = 3;
	factor = 3;
	for (;;) {
		/* The division is the most costly part of the loop.
		 * On 64bit CPUs, takes at best 12 cycles, often ~20.
		 */
		while ((N % factor) == 0) { /* not likely */
			N = N / factor;
			printf(" %u"HALF_FMT, factor);
			max_factor = isqrt_odd(N);
		}
 next_factor:
		if (factor >= max_factor)
			break;
		factor += 2;
		/* Rudimentary wheel sieving: skip multiples of 3:
		 * Every third odd number is divisible by three and thus isn't a prime:
		 * 5 7 9 11 13 15 17 19 21 23 25 27 29 31 33 35 37 39 41 43 45 47...
		 * ^ ^   ^  ^     ^  ^     ^  _     ^  ^     _  ^     ^  ^     ^
		 * (^ = primes, _ = would-be-primes-if-not-divisible-by-5)
		 */
		--count3;
		if (count3 == 0) {
			count3 = 3;
			goto next_factor;
		}
	}
 end:
	if (N > 1)
		printf(" %llu", N);
	bb_putchar('\n');
}

int factor_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int factor_main(int argc UNUSED_PARAM, char **argv)
{
	//// coreutils has undocumented option ---debug (three dashes)
	//getopt32(argv, "");
	//argv += optind;
	argv++;

	if (!*argv)
		//TODO: read from stdin
		bb_show_usage();

	do {
		wide_t N;
		const char *numstr;

		/* Coreutils compat */
		numstr = skip_whitespace(*argv);
		if (*numstr == '+')
			numstr++;

		N = bb_strtoull(numstr, NULL, 10);
		if (errno)
			bb_show_usage();
		printf("%llu:", N);
		factorize(N);
	} while (*++argv);

	return EXIT_SUCCESS;
}
