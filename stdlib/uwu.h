#ifndef UWU_STDLIB_H
#define UWU_STDLIB_H

// memory-safe standard library that is INSANELY BADLY DONE!
// ngl might just steal it from rust....
// might just cut it out since it fucks with building the compiler too much

// alloc with bounds checking
extern void* uwu_malloc(chonk size);
extern void* uwu_calloc(chonk nmemb, chonk size);
extern void uwu_free(void* ptr);

// i guess some string checking
extern chonk uwu_strlen(const byte* str);
extern byte* uwu_strcpy(byte* dest, const byte* src, chonk dest_size);
extern byte* uwu_strcat(byte* dest, const byte* src, chonk dest_size);

// bounds check
extern void uwu_array_bounds_check(chonk index, chonk size, chonk line);

// pointer check
extern void uwu_null_check(const void* ptr, chonk line);

// input output shit
extern void uwu_putchar(byte c);
extern void uwu_puts(const byte* str);
extern chonk uwu_printf(const byte* fmt, ...);


extern void uwu_exit(chonk code);

#endif
