#include <math.h>

#define APREC 16
#define ZPREC 7

#define APPROX(alpha, reg, acc) \
  ((alpha * (((int)(reg) << ZPREC) - acc)) >> APREC)

#define BLUR(reg, acc) \
  acc += APPROX(alpha, reg, acc); reg = (unsigned char)(acc >> ZPREC)

#define BLUR1(dst, x) BLUR(dst[x], z[0])
#define BLUR2(dst, x) BLUR1(dst, x); BLUR(dst[(x)+1], z[1])
#define BLUR3(dst, x) BLUR2(dst, x); BLUR(dst[(x)+2], z[2])
#define BLUR4(dst, x) BLUR3(dst, x); BLUR(dst[(x)+3], z[3])


static void expblur_row(unsigned char* dst, int w, int channels, int preserve_edge, int alpha)
{
  int x, z[4];

  for (x = 0; x < channels; ++x)
    z[x] = APPROX(alpha, dst[x], 0);

  if (preserve_edge) --w;

  switch (channels) {
    case 1: for (x = 1; x < w; x++) { BLUR1(dst, x*1); } break;
    case 2: for (x = 1; x < w; x++) { BLUR2(dst, x*2); } break;
    case 3: for (x = 1; x < w; x++) { BLUR3(dst, x*3); } break;
    case 4: for (x = 1; x < w; x++) { BLUR4(dst, x*4); } break;
  }

  if (preserve_edge) dst += channels;

  switch (channels) {
    case 1: for (x = w-2; x >= 0; x--) { BLUR1(dst, x*1); } break;
    case 2: for (x = w-2; x >= 0; x--) { BLUR2(dst, x*2); } break;
    case 3: for (x = w-2; x >= 0; x--) { BLUR3(dst, x*3); } break;
    case 4: for (x = w-2; x >= 0; x--) { BLUR4(dst, x*4); } break;
  }
}

static void expblur_col(unsigned char* dst, int h, int channels, int preserve_edge, int stride, int alpha)
{
  int y, z[4]; // force zero border

  for (y = 0; y < channels; ++y)
    z[y] = APPROX(alpha, dst[y], 0);

  if (preserve_edge) --h;

  switch (channels) {
    case 1: for (y = stride; y < h*stride; y += stride) { BLUR1(dst, y); } break;
    case 2: for (y = stride; y < h*stride; y += stride) { BLUR2(dst, y); } break;
    case 3: for (y = stride; y < h*stride; y += stride) { BLUR3(dst, y); } break;
    case 4: for (y = stride; y < h*stride; y += stride) { BLUR4(dst, y); } break;
  }

  if (preserve_edge) dst += stride;

  switch (channels) {
    case 1: for (y = (h-2)*stride; y >= 0; y -= stride) { BLUR1(dst, y); } break;
    case 2: for (y = (h-2)*stride; y >= 0; y -= stride) { BLUR2(dst, y); } break;
    case 3: for (y = (h-2)*stride; y >= 0; y -= stride) { BLUR3(dst, y); } break;
    case 4: for (y = (h-2)*stride; y >= 0; y -= stride) { BLUR4(dst, y); } break;
  }
}

static void expblur_image(unsigned char *image, int w, int h, int channels, int preserve_edge, int stride, float blur, int passes)
{
  float sigma = (float)blur / (float)passes * 0.57735f; // 1 / sqrt(3)
  int alpha = (int)((1<<APREC) * (1.0f - expf(-2.3f / (sigma+1.0f))));
  int i, p;

  for (i = 0; i < h; ++i)
  {
    for (p = 0; p < passes; ++p)
      expblur_row(image + i * stride, w, channels, preserve_edge, alpha);
  }

  for (i = 0; i < w; ++i)
  {
    for (p = 0; p < passes; ++p)
      expblur_col(image + i * w, h, channels, preserve_edge, stride, alpha);
  }
}
