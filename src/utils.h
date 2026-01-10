#ifndef UTILS_H
#define UTILS_H

#ifdef __GNUC__
#define ASM
#define REG(r, y) y __asm( # r )
#else
#define ASM __asm __saveds
#define REG(r, y) register __ ## r y
#endif

ULONG ASM LE32(REG(d0, ULONG a));

#endif // UTILS_H
