#pragma once

#ifdef _WIN32
#include <limits.h>
#include <intrin.h>
typedef unsigned __int32  uint32_t;

#else
#include <stdint.h>
#endif

class PNCPUID {
	uint32_t regs[4];

public:
	explicit PNCPUID(unsigned i) {
#ifdef _WIN32
		__cpuid((int *)regs, (int)i);

#else
		__asm__ volatile("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "a" (i), "c" (0));
#endif
	}

	const uint32_t &eax() const { return regs[0]; }
	const uint32_t &ebx() const { return regs[1]; }
	const uint32_t &ecx() const { return regs[2]; }
	const uint32_t &edx() const { return regs[3]; }
};
