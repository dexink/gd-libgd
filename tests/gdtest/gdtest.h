#ifndef GD_TEST_H
#define GD_TEST_H

#include <stdarg.h>
#include "test_config.h"

#define GDTEST_STRING_MAX 1024
typedef struct CuTestImageResult CuTestImageResult;
struct CuTestImageResult {
	unsigned int pixels_changed;
	unsigned int max_diff;
};


/* Internal versions of assert functions -- use the public versions */
gdImagePtr gdTestImageFromPng(const char *filename);

void gdTestImageDiff(gdImagePtr buf_a, gdImagePtr buf_b,
                     gdImagePtr buf_diff, CuTestImageResult *result_ret);

int gdTestImageCompareToImage(const char* file, unsigned int line, const char* message,
                              gdImagePtr expected, gdImagePtr actual);

int gdTestImageCompareToFile(const char* file, unsigned int line, const char* message,
                             const char *expected_file, gdImagePtr actual);

unsigned int gdMaxPixelDiff(gdImagePtr a, gdImagePtr b);

int _gdTestAssert(const char* file, unsigned int line, const char* message, int condition);

int _gdTestErrorMsg(const char* file, unsigned int line, const char* string, ...);

/* public assert functions */
#define gdAssertImageEqualsToFile(ex,ac) gdTestImageCompareToFile(__FILE__,__LINE__,NULL,(ex),(ac))
#define gdAssertImageFileEqualsMsg(ex,ac) gdTestImageCompareFiles(__FILE__,__LINE__,(ms),(ex),(ac))

#define gdAssertImageEquals(tc,ex,ac) CuAssertImageEquals_LineMsg((tc),__FILE__,__LINE__,NULL,(ex),(ac))
#define gdAssertImageEqualsMsg(tc,ex,ac) CuAssertImageEquals_LineMsg((tc),__FILE__,__LINE__,(ms),(ex),(ac))

#define gdTestAssert(cond) _gdTestAssert(__FILE__, __LINE__, "assert failed in <%s:%i>\n", (cond))

#define gdTestErrorMsg(...) _gdTestErrorMsg(__FILE__, __LINE__, __VA_ARGS__)

void gdSilence(int priority, const char *format, va_list args);

int gdNumFailures(void);

#endif /* GD_TEST_H */
