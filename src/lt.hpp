#ifndef INCLUDE_LT_HPP
#define INCLUDE_LT_HPP

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// Make type names more consistent and easier to write.
typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;
typedef i32         b32;

typedef float       f32;
typedef double      f64;

typedef size_t      usize;
typedef ptrdiff_t   isize;

static_assert(sizeof(u8) == 1, "u8 should have 1 byte");
static_assert(sizeof(u16) == 2, "u16 should have 2 bytes");
static_assert(sizeof(u32) == 4, "u16 should have 4 bytes");
static_assert(sizeof(u64) == 8, "u16 should have 8 bytes");

static_assert(sizeof(i8) == 1, "i8 should have 1 byte");
static_assert(sizeof(i16) == 2, "i16 should have 2 bytes");
static_assert(sizeof(i32) == 4, "i16 should have 4 bytes");
static_assert(sizeof(i64) == 8, "i16 should have 8 bytes");
static_assert(sizeof(b32) == 4, "b32 should have 4 bytes");

static_assert(sizeof(f32) == 4, "f32 should have 4 byte");
static_assert(sizeof(f64) == 8, "f64 should have 8 bytes");

/////////////////////////////////////////////////////////
//
// General Macros and Functions
//
// Macros that make working with c either better or more consistent.
//

#ifndef global_variable
#define global_variable static  // Global variables
#endif
#ifndef internal
#define internal        static  // Internal linkage
#endif
#ifndef local_persist
#define local_persist   static  // Local persisting variables
#endif

#ifndef LT_UNUSED
#define LT_UNUSED(x) ((void)(x))
#endif

#ifndef lt_abs
#define lt_abs(x) (((x) < 0) ? -(x) : (x))
#endif

#ifndef lt_min
#define lt_min(a, b) ((a) < (b) ? (a) : b)
#endif

#ifndef lt_max
#define lt_max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef lt_in_open_interval
#define lt_in_open_interval(x, min, max)  ((min) < (x) && (x) < (max))
#endif

#ifndef lt_in_closed_interval
#define lt_in_closed_interval(x, min, max)  ((min) <= (x) && (x) <= (max))
#endif

#ifndef LT_LEN
#define LT_LEN(x) (sizeof(x)/sizeof(x)[0])
#endif

#ifndef lt_free
#define lt_free(p) do { \
        free(p);        \
        p = NULL;       \
    } while(0)
#endif

#ifndef LT_FAIL
#  ifdef LT_DEBUG
#    define LT_FAIL(...) do {                                        \
        fprintf(stderr, "****** RUNTIME FAILURE ******\n");          \
        fprintf(stderr, "%s(line %d)", __FILE__, __LINE__);          \
        fprintf(stderr, __VA_ARGS__);                                \
        fprintf(stderr, "*****************************\n");          \
        fflush(stderr);                                              \
        __builtin_trap();                                            \
    } while(0)
#  else
#    define LT_FAIL(...)
#  endif // LT_DEBUG
#endif // LT_FAIL

#ifndef LT_ASSERT
#  ifdef LT_DEBUG
#    define LT_ASSERT2(cond, file, number) do {                         \
        if (!(cond)) {                                                  \
            fprintf(stderr, "******************************\n");        \
            fprintf(stderr, "*********** ASSERT ***********\n");        \
            fprintf(stderr, "**                            \n");        \
            fprintf(stderr, "**  Condition: %s\n", #cond);              \
            fprintf(stderr, "**  %s (line %d)\n", file, number);        \
            fprintf(stderr, "**                            \n");        \
            fprintf(stderr, "******************************\n");        \
            fprintf(stderr, "******************************\n");        \
            fflush(stderr);                                             \
            __builtin_trap();                                           \
        }                                                               \
    } while(0)
#    define LT_ASSERT(cond) LT_ASSERT2(cond, __FILE__, __LINE__)
#  else
#    define LT_ASSERT(cond)
#  endif // LT_DEBUG
#endif // LT_ASSERT

internal inline void
lt_swap(i32 *a, i32 *b)
{
    i32 tmp_a = *a;
    *a = *b;
    *b = tmp_a;
}

internal inline bool
lt_is_little_endian()
{
    i32 num = 1;
    if (*(char*)&num == 1)
        return true;
    else
        return false;
}

void get_display_dpi(i32 *x, i32 *y);


enum FileError {
    FileError_None,

    FileError_Read,
    FileError_Seek,
    FileError_NotExists,
    FileError_Unknown,

    FileError_Count,
};

struct FileContents {
    void     *data;
    isize     size;
    FileError error;
};

FileContents *file_read_contents(const char *filename);
void          file_free_contents(FileContents *fc);
isize         file_get_size(const char *filename);

/////////////////////////////////////////////////////////
//
// String
//
//

struct String {
    isize len;
    isize capacity;
    char *data;
};

String *string_make   (const char *str);
String *string_make   (const char *buf, isize len);
void    string_free   (String *str);
void    string_concat (String *str, const char *rhs);

/////////////////////////////////////////////////////////
//
// Array
//
//

template<typename T>
struct Array {
    isize len;
    isize capacity;
    T *data;
};

template<typename T> Array<T>  array_make();
template<typename T> void      array_free(Array<T> *arr);
template<typename T> void      array_push(Array<T> *arr, T val);

#endif // INCLUDE_LT_H


/* =========================================================================
 *
 *
 *
 *
 *
 *
 *  Implementation
 *
 *
 *
 *
 *
 *
 * ========================================================================= */

#if defined(LT_IMPLEMENTATION) && !defined(LT_IMPLEMENTATION_DONE)
#define LT_IMPLEMENTATION_DONE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__unix__)
#include <sys/stat.h>
#include <X11/extensions/Xrandr.h>
#endif


/////////////////////////////////////////////////////////
//
// File Implementation
//

FileContents *
file_read_contents(const char *filename)
{
    FILE *fp = fopen(filename, "rb"); // open in binary mode.
    void *file_data = NULL;

    // TODO(leo), @Robustness: Actually check if the file exists before trying to open it.
    if (fp == NULL) {
        FileContents *ret = (FileContents*)malloc(sizeof(*ret));
        ret->error = FileError_NotExists;
        ret->data = NULL;
        ret->size = -1;
        return ret;
    }

    isize file_size = file_get_size(filename);

    if (file_size == -1) {
        fclose(fp);
        FileContents *ret = (FileContents*)malloc(sizeof(*ret));
        ret->error = FileError_Unknown;
        ret->data = NULL;
        ret->size = -1;
        return ret;
    }

    file_data = (char*)malloc(sizeof(char) * file_size);

    if (file_data == NULL) {
        LT_FAIL("Failed allocating memory\n");
    }

    isize newlen = fread(file_data, sizeof(u8), file_size, fp);
    if (newlen < 0) {
        FileContents *ret = (FileContents*)malloc(sizeof(*ret));
        ret->error = FileError_Read;
        ret->data = NULL;
        ret->size = -1;
        return ret;
    }

    LT_ASSERT(newlen == file_size);

    if (ferror(fp) != 0) {
        fputs("Error reading file\n", stderr);
        fclose(fp);
        free(file_data);

        FileContents *ret = (FileContents*)malloc(sizeof(*ret));
        ret->error = FileError_Read;
        ret->data = NULL;
        ret->size = -1;
        return ret;
    }
    fclose(fp);

    FileContents *ret = (FileContents*)malloc(sizeof(*ret));
    ret->error = FileError_None;
    ret->data = file_data;
    ret->size = file_size;
    return ret;
}

void
file_free_contents(FileContents *fc)
{
    lt_free(fc->data);
    lt_free(fc);
}

isize
file_get_size(const char *filename)
{
#if defined(__unix__)
    struct stat st;
    if (stat(filename, &st) < 0) {
        return -1;
    }
    return st.st_size;
#else
#  error "Still not implemented"
#endif
}

///////////////////////////////////////////////////////
//
// String
//

String *
string__alloc(isize len, isize capacity)
{
    String *new_str   = (String*)malloc(sizeof(*new_str));
    new_str->len      = len;
    new_str->capacity = capacity;
    new_str->data     = (char*)calloc(new_str->capacity, 1);
    return new_str;
}

String *
string_make(const char *str)
{
    isize len = strlen(str);
    String *new_str = string__alloc(len, len+1);
    memcpy(new_str->data, str, len);
    return new_str;
}

String *
string_make(const char *buf, isize len)
{
    String *new_str = string__alloc(len, len+1);
    memcpy(new_str->data, buf, len);
    return new_str;
}

void
string_free(String *str)
{
    LT_ASSERT(str != NULL);

    lt_free(str->data);
    lt_free(str);
}

void
string__grow(String *str)
{
    LT_ASSERT(str != NULL);

    const f32 GROW_FACTOR = 1.5f;

    isize new_capacity = (isize)(str->capacity * GROW_FACTOR);
    char *new_data = (char*)realloc(str->data, new_capacity);

    if (new_data == NULL) {
        LT_FAIL("Could not allocate more memory\n");
    }

    memset(str->data+str->len, 0, new_capacity - str->capacity);

    str->data = new_data;
    str->capacity = new_capacity;
}

void
string_concat(String *str, const char *rhs)
{
    LT_ASSERT(str != NULL && rhs != NULL);

    isize rhs_len = strlen(rhs);

    while (str->len + rhs_len >= str->capacity) {
        // Increase str capacity while it cannot hold the new data.
        string__grow(str);
    }

    memcpy(str->data, rhs, rhs_len);
}

///////////////////////////////////////////////////////
//
// Array
//

template<typename T> Array<T>
array_make(void)
{
    const i32 INITIAL_CAP = 8;
    Array<T> arr = {};
    arr.len = 0;
    arr.capacity = INITIAL_CAP;
    arr.data = (T*)calloc(INITIAL_CAP, sizeof(T));
    return arr;
}

template<typename T> void
array_free(Array<T> *arr)
{
    lt_free(arr->data);
}

template<typename T> void
array__grow(Array<T> *arr)
{
    LT_ASSERT(arr != NULL);

    const f32 GROW_FACTOR = 1.5f;

    isize new_capacity = (isize)(arr->capacity * GROW_FACTOR);
    T *new_data = (T*)realloc(arr->data, new_capacity * sizeof(T));

    if (new_data == NULL) {
        LT_FAIL("Could not allocate more memory\n");
    }

    arr->data = new_data;
    arr->capacity = new_capacity;

    memset(arr->data+arr->len, 0, new_capacity - arr->len);
}


template<typename T> void
array_push(Array<T> *arr, T val)
{
    LT_ASSERT(arr != NULL);

    if (arr->len+1 > arr->capacity) {
        array__grow(arr);
    }

    arr->data[arr->len++] = val;
}

///////////////////////////////////////////////////////
//
// Utils
//

void
lt_get_display_dpi(i32 *x, i32 *y)
{
#ifdef __unix__
    i32 scr = 0; /* Screen number */

    if ((NULL == x) || (NULL == y)) { return; }

    char *displayname = NULL;
    Display *dpy = XOpenDisplay(displayname);

    /*
     * there are 2.54 centimeters to an inch; so there are 25.4 millimeters.
     *
     *     dpi = N pixels / (M millimeters / (25.4 millimeters / 1 inch))
     *         = N pixels / (M inch / 25.4)
     *         = N * 25.4 pixels / M inch
     */
    f64 xres = ((((f64) DisplayWidth(dpy,scr)) * 25.4) /
                ((f64) DisplayWidthMM(dpy,scr)));
    f64 yres = ((((f64) DisplayHeight(dpy,scr)) * 25.4) /
                ((f64) DisplayHeightMM(dpy,scr)));

    *x = (i32)(xres + 0.5);
    *y = (i32)(yres + 0.5);

    XCloseDisplay (dpy);
#else
    _Static_assert(false, "Not Implemented");
#endif
}

#endif // LT_IMPLEMENTATION
