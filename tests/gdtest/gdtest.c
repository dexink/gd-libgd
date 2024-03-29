#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "gd.h"

#include "gdtest.h"
#include "test_config.h"

static inline int max(int a, int b) {return a > b ? a : b;}

void gdSilence(int priority, const char *format, va_list args)
{
	(void)priority;
	(void)format;
	(void)args;
}

gdImagePtr gdTestImageFromPng(const char *filename)
{
	gdImagePtr image;

	FILE *fp;

	fp = fopen(filename, "rb");

	if (!fp) {
		return NULL;
	}
	image = gdImageCreateFromPng(fp);
	fclose(fp);
	return image;
}

/* Compare two buffers, returning the number of pixels that are
 * different and the maximum difference of any single color channel in
 * result_ret.
 *
 * This function should be rewritten to compare all formats supported by
 * cairo_format_t instead of taking a mask as a parameter.
 */
void gdTestImageDiff(gdImagePtr buf_a, gdImagePtr buf_b,
                     gdImagePtr buf_diff, CuTestImageResult *result_ret)
{
	int x, y;
	int c1, c2;
#   define UP_DIFF(var) result_ret->max_diff = max((var), result_ret->max_diff)

	for (y = 0; y < gdImageSY(buf_a); y++) {
		for (x = 0; x < gdImageSX(buf_a); x++) {
			c1 = gdImageGetTrueColorPixel(buf_a, x, y);
			c2 = gdImageGetTrueColorPixel(buf_b, x, y);

			/* check if the pixels are the same */
			if (c1 != c2) {
				int r1,b1,g1,a1,r2,b2,g2,a2;
				unsigned int diff_a,diff_r,diff_g,diff_b;

				a1 = gdTrueColorGetAlpha(c1);
				a2 = gdTrueColorGetAlpha(c2);
				diff_a = abs (a1 - a2);
				diff_a *= 4;  /* emphasize */

				if (diff_a) {
					diff_a += 128; /* make sure it's visible */
				}
				if (diff_a > gdAlphaMax) {
					diff_a = gdAlphaMax/2;
				}

				r1 = gdTrueColorGetRed(c1);
				r2 = gdTrueColorGetRed(c2);
				diff_r = abs (r1 - r2);
				/* diff_r *= 4;  /* emphasize */
				if (diff_r) {
					diff_r += gdRedMax/2; /* make sure it's visible */
				}
				if (diff_r > 255) {
					diff_r = 255;
				}

				g1 = gdTrueColorGetGreen(c1);
				g2 = gdTrueColorGetGreen(c2);
				diff_g = abs (g1 - g2);

				diff_g *= 4;  /* emphasize */
				if (diff_g) {
					diff_g += gdGreenMax/2; /* make sure it's visible */
				}
				if (diff_g > 255) {
					diff_g = 255;
				}

				b1 = gdTrueColorGetBlue(c1);
				b2 = gdTrueColorGetBlue(c2);
				diff_b = abs (b1 - b2);
				diff_b *= 4;  /* emphasize */
				if (diff_b) {
					diff_b += gdBlueMax/2; /* make sure it's visible */
				}
				if (diff_b > 255) {
					diff_b = 255;
				}

				result_ret->pixels_changed++;
				if (buf_diff) gdImageSetPixel(buf_diff, x,y, gdTrueColorAlpha(diff_r, diff_g, diff_b, diff_a));
			} else {
				if (buf_diff) gdImageSetPixel(buf_diff, x,y, gdTrueColorAlpha(255,255,255,0));
			}
		}
	}
#   undef UP_DIFF
}


/* Return the largest difference between two corresponding pixels and
 * channels. */
unsigned int gdMaxPixelDiff(gdImagePtr a, gdImagePtr b)
{
    int diff = 0;
    int x, y;

    if (a == NULL || b == NULL || a->sx != b->sx || a->sy != b->sy)
        return UINT_MAX;

    for (x = 0; x < a->sx; x++) {
        for (y = 0; y < a->sy; y++) {
            int c1, c2;

			c1 = gdImageGetTrueColorPixel(a, x, y);
			c2 = gdImageGetTrueColorPixel(b, x, y);
            if (c1 == c2) continue;

            diff = max(diff, abs(gdTrueColorGetAlpha(c1) - gdTrueColorGetAlpha(c2)));
            diff = max(diff, abs(gdTrueColorGetRed(c1)   - gdTrueColorGetRed(c2)));
            diff = max(diff, abs(gdTrueColorGetGreen(c1) - gdTrueColorGetGreen(c2)));
            diff = max(diff, abs(gdTrueColorGetBlue(c1)  - gdTrueColorGetBlue(c2)));
        }/* for */
    }/* for */

    return diff;
}


int gdTestImageCompareToImage(const char* file, unsigned int line, const char* message,
                              gdImagePtr expected, gdImagePtr actual)
{
	unsigned int width_a, height_a;
	unsigned int width_b, height_b;
	gdImagePtr surface_diff = NULL;
	CuTestImageResult result = {0, 0};

	(void)message;

	if (!actual) {
		_gdTestErrorMsg(file, line, "Image is NULL\n");
		goto fail;
	}

	width_a  = gdImageSX(expected);
	height_a = gdImageSY(expected);
	width_b  = gdImageSX(actual);
	height_b = gdImageSY(actual);

	if (width_a  != width_b  || height_a != height_b) {
		_gdTestErrorMsg(file, line,
				"Image size mismatch: (%ux%u) vs. (%ux%u)\n       for %s vs. buffer\n",
				width_a, height_a,
				width_b, height_b,
				file);
		goto fail;
	}

	surface_diff = gdImageCreateTrueColor(width_a, height_a);

	gdTestImageDiff(expected, actual, surface_diff, &result);
	if (result.pixels_changed>0) {
		char file_diff[255];
		char file_out[1024];
		FILE *fp;
		int len, p;

		_gdTestErrorMsg(file, line,
				"Total pixels changed: %d with a maximum channel difference of %d.\n",
				result.pixels_changed,
				result.max_diff
			);

		p = len = strlen(file);
		p--;

		/* Use only the filename (and store it in the bld dir not the src dir
		 */
		while(p > 0 && (file[p] != '/' && file[p] != '\\')) {
			p--;
		}
		sprintf(file_diff, "%s_%u_diff.png", file + p  + 1, line);
		sprintf(file_out, "%s_%u_out.png", file + p  + 1, line);

		fp = fopen(file_diff, "wb");
		if (!fp) goto fail;
		gdImagePng(surface_diff,fp);
		fclose(fp);

		fp = fopen(file_out, "wb");
		if (!fp) goto fail;
		gdImagePng(actual, fp);
		fclose(fp);
	} else {
		if (surface_diff) {
			gdImageDestroy(surface_diff);
		}
		return 1;
	}

fail:
	if (surface_diff) {
		gdImageDestroy(surface_diff);
	}
	return 0;
}

int gdTestImageCompareToFile(const char* file, unsigned int line, const char* message,
                             const char *expected_file, gdImagePtr actual)
{
	gdImagePtr expected;
	int res = 1;

	expected = gdTestImageFromPng(expected_file);

	if (!expected) {
		_gdTestErrorMsg(file, line, "Cannot open PNG <%s>", expected_file);
		res = 0;
	} else {
		res = gdTestImageCompareToImage(file, line, message, expected, actual);
		gdImageDestroy(expected);
	}
	return res;
}


static int failureCount = 0;

int gdNumFailures() {
    return failureCount;
}

int _gdTestAssert(const char* file, unsigned int line, const char* message, int condition)
{
	if (condition) return 1;
	_gdTestErrorMsg(file, line, "%s", message);

    ++failureCount;

	return 0;
}

int _gdTestErrorMsg(const char* file, unsigned int line, const char* format, ...) /* {{{ */
{
	va_list args;
	char output_buf[GDTEST_STRING_MAX];

	va_start(args, format);
	vsnprintf(output_buf, sizeof(output_buf), format, args);
	va_end(args);
	fprintf(stderr, "%s:%u: %s", file, line, output_buf);
	fflush(stderr);

    ++failureCount;

	return 0;
}
/* }}} */
