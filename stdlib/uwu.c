#include "uwu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

typedef long long chonk;
typedef unsigned char byte;

// Memory-safe malloc with overflow checking
void* uwu_malloc(chonk size) {
    if (size == 0 || size > 1024 * 1024 * 1024) {
        fprintf(stderr, "Error: Invalid allocation size\n");
        exit(1);
    }
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Error: Out of memory\n");
        exit(1);
    }
    return ptr;
}

void* uwu_calloc(chonk nmemb, chonk size) {
    if (nmemb == 0 || size == 0) return NULL;
    chonk total = nmemb * size;
    if (total / nmemb != size) {
        fprintf(stderr, "Error: Allocation size overflow\n");
        exit(1);
    }
    return uwu_malloc(total);
}

void uwu_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

// Safe string length with null check
chonk uwu_strlen(const byte* str) {
    if (!str) {
        fprintf(stderr, "Error: Null pointer in strlen\n");
        exit(1);
    }
    return strlen((const char*)str);
}

// Safe string copy with bounds checking
byte* uwu_strcpy(byte* dest, const byte* src, chonk dest_size) {
    if (!dest || !src) {
        fprintf(stderr, "Error: Null pointer in strcpy\n");
        exit(1);
    }
    if (dest_size == 0) return dest;
    
    chonk src_len = uwu_strlen(src);
    if (src_len >= dest_size) {
        fprintf(stderr, "Error: Buffer overflow prevented in strcpy\n");
        exit(1);
    }
    memcpy(dest, src, src_len + 1);
    return dest;
}

// Safe string concatenation
byte* uwu_strcat(byte* dest, const byte* src, chonk dest_size) {
    if (!dest || !src) {
        fprintf(stderr, "Error: Null pointer in strcat\n");
        exit(1);
    }
    
    chonk dest_len = uwu_strlen(dest);
    chonk src_len = uwu_strlen(src);
    
    if (dest_len + src_len + 1 > dest_size) {
        fprintf(stderr, "Error: Buffer overflow prevented in strcat\n");
        exit(1);
    }
    
    memcpy(dest + dest_len, src, src_len + 1);
    return dest;
}

// Array bounds checking
void uwu_array_bounds_check(chonk index, chonk size, chonk line) {
    if (index >= size) {
        fprintf(stderr, "Error: Array index %lld out of bounds (size: %lld) at line %lld\n", 
               (long long)index, (long long)size, (long long)line);
        exit(1);
    }
}

// Null pointer check
void uwu_null_check(const void* ptr, chonk line) {
    if (!ptr) {
        fprintf(stderr, "Error: Null pointer dereference at line %lld\n", (long long)line);
        exit(1);
    }
}

// Safe I/O
void uwu_putchar(byte c) {
    putchar(c);
}

void uwu_puts(const byte* str) {
    if (!str) {
        fprintf(stderr, "Error: Null pointer in puts\n");
        exit(1);
    }
    puts((const char*)str);
}

chonk uwu_printf(const byte* fmt, ...) {
    if (!fmt) {
        fprintf(stderr, "Error: Null pointer in printf\n");
        exit(1);
    }
    va_list args;
    va_start(args, fmt);
    chonk result = vprintf((const char*)fmt, args);
    va_end(args);
    return result;
}

void uwu_exit(chonk code) {
    exit(code);
}
