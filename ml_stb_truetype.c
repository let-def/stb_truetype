#include <assert.h>
#include <stdio.h>
#include <caml/mlvalues.h>
#include <caml/fail.h>
#include <caml/memory.h>
#include <caml/alloc.h>
#include <caml/bigarray.h>
#include <caml/custom.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

value ml_stbtt_GetFontOffsetForIndex(value ba, value vindex)
{
  unsigned char *data = Caml_ba_data_val(ba);
  int index = Long_val(vindex);
  int result = stbtt_GetFontOffsetForIndex(data, index);
  return Val_long(result);
}

#define ml_fontinfo_data(v) ((stbtt_fontinfo*)Data_abstract_val(v))

/* Layout of fontinfo and, later, pack_context:
 * (custom, buffer)
 * where custom is a custom block with library-specific data
 *       buffer is a reference kept to underlying bigarray store
 */

#define Fontinfo_val(x) (ml_fontinfo_data(Field((x), 0)))

value ml_stbtt_InitFont(value ba, value voffset)
{
  CAMLparam2(ba, voffset);
  CAMLlocal3(ret, pack, fontinfo);

  unsigned char *data = Caml_ba_data_val(ba);
  int index = Long_val(voffset);

  size_t wosize = 1 + (sizeof(stbtt_fontinfo) + sizeof(value) - 1) / sizeof(value);
  fontinfo = caml_alloc(wosize, Abstract_tag);
  int result = stbtt_InitFont(ml_fontinfo_data(fontinfo), data, index);
  static intnat ids = 0;

  if (result == 0)
    ret = Val_unit;
  else
  {
    pack = caml_alloc(3, Object_tag);
    Store_field(pack, 0, fontinfo);
    Store_field(pack, 1, Val_long(ids++));
    Store_field(pack, 2, ba);

    ret = caml_alloc(1, 0);
    Store_field(ret, 0, pack);
  }

  CAMLreturn(ret);
}

value ml_stbtt_FindGlyphIndex(value fontinfo, value codepoint)
{
  return Val_long(stbtt_FindGlyphIndex(Fontinfo_val(fontinfo), Long_val(codepoint)));
}

double ml_stbtt_ScaleForPixelHeight(value fontinfo, double height)
{
  return stbtt_ScaleForPixelHeight(Fontinfo_val(fontinfo), height);
}

value ml_stbtt_ScaleForPixelHeight_bc(value fontinfo, value height)
{
  double result = ml_stbtt_ScaleForPixelHeight(fontinfo, height);
  return caml_copy_double(result);
}

double ml_stbtt_ScaleForMappingEmToPixels(value fontinfo, double height)
{
  return stbtt_ScaleForMappingEmToPixels(Fontinfo_val(fontinfo), height);
}

value ml_stbtt_ScaleForMappingEmToPixels_bc(value fontinfo, value height)
{
  double result = ml_stbtt_ScaleForMappingEmToPixels(fontinfo, height);
  return caml_copy_double(result);
}

value ml_stbtt_GetFontVMetrics(value fontinfo)
{
  CAMLparam1(fontinfo);
  CAMLlocal1(ret);

  int ascent, descent, line_gap;
  stbtt_GetFontVMetrics(Fontinfo_val(fontinfo), &ascent, &descent, &line_gap);

  ret = caml_alloc(3, 0);
  Store_field(ret, 0, Val_long(ascent));
  Store_field(ret, 1, Val_long(descent));
  Store_field(ret, 2, Val_long(line_gap));

  CAMLreturn(ret);
}

value ml_stbtt_GetGlyphHMetrics(value fontinfo, value glyph)
{
  CAMLparam2(fontinfo, glyph);
  CAMLlocal1(ret);

  int adv, lsb;
  stbtt_GetGlyphHMetrics(Fontinfo_val(fontinfo), Long_val(glyph), &adv, &lsb);

  ret = caml_alloc(2, 0);
  Store_field(ret, 0, Val_long(adv));
  Store_field(ret, 1, Val_long(lsb));

  CAMLreturn(ret);
}

value ml_stbtt_GetGlyphAdvance(value fontinfo, value glyph)
{
  int adv = 0, lsb = 0;
  stbtt_GetGlyphHMetrics(Fontinfo_val(fontinfo), Long_val(glyph), &adv, &lsb);
  return Val_long(adv);
}

value ml_stbtt_GetGlyphKernAdvance(value fontinfo, value glyph1, value glyph2)
{
  return Val_long(stbtt_GetGlyphKernAdvance(Fontinfo_val(fontinfo), Long_val(glyph1), Long_val(glyph2)));
}

static value box(int x0, int y0, int x1, int y1)
{
  CAMLparam0();
  CAMLlocal1(ret);

  ret = caml_alloc(4, 0);
  Store_field(ret, 0, Val_long(x0));
  Store_field(ret, 1, Val_long(y0));
  Store_field(ret, 2, Val_long(x1));
  Store_field(ret, 3, Val_long(y1));

  CAMLreturn(ret);
}

value ml_stbtt_GetFontBoundingBox(value fontinfo)
{
  CAMLparam1(fontinfo);

  int x0, y0, x1, y1;
  stbtt_GetFontBoundingBox(Fontinfo_val(fontinfo), &x0, &y0, &x1, &y1);

  CAMLreturn(box(x0, y0, x1, y1));
}

value ml_stbtt_GetGlyphBox(value fontinfo, value glyph)
{
  CAMLparam2(fontinfo, glyph);

  int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  stbtt_GetGlyphBox(Fontinfo_val(fontinfo), Long_val(glyph), &x0, &y0, &x1, &y1);

  CAMLreturn(box(x0, y0, x1, y1));
}

// Bitmap packer
#define Pack_context_val(x) (Data_custom_val(Field((x), 0)))

static void pack_context_finalize(value v)
{
  CAMLparam1(v);
  stbtt_PackEnd(Data_custom_val(v));
  CAMLreturn0;
}

static struct custom_operations pack_context_custom_ops = {
  .identifier  = "stbtt_pack_context",
  .finalize    = pack_context_finalize,
  .compare     = custom_compare_default,
  .hash        = custom_hash_default,
  .serialize   = custom_serialize_default,
  .deserialize = custom_deserialize_default
};

value ml_stbtt_PackBegin(value buffer, value w, value h, value s, value p)
{
  CAMLparam5(buffer, w, h, s, p);
  CAMLlocal3(ret, pack, pack_context);

  unsigned char *data = Caml_ba_data_val(buffer);
  int width = Long_val(w), height = Long_val(h), stride = Long_val(s),
      padding = Long_val(p);

  pack_context = caml_alloc_custom(&pack_context_custom_ops, sizeof(stbtt_pack_context), 0, 1);
  int result = stbtt_PackBegin(Data_custom_val(pack_context), data, width, height, stride, padding, NULL);

  if (result == 0)
    ret = Val_unit;
  else
  {
    pack = caml_alloc(2, 0);
    Store_field(pack, 0, pack_context);
    Store_field(pack, 1, buffer);

    ret = caml_alloc(1, 0);
    Store_field(ret, 0, pack);
  }

  CAMLreturn(ret);
}

value ml_stbtt_PackSetOversampling(value ctx, value h, value v)
{
  stbtt_PackSetOversampling(Pack_context_val(ctx), Long_val(h), Long_val(v));
  return Val_unit;
}

typedef struct {
  int count;
  stbtt_packedchar chars[1];
} ml_stbtt_packed_chars;

#define Packed_chars_val(x) ((ml_stbtt_packed_chars *)String_val(x))
value ml_stbtt_packed_chars_count(value packed_chars)
{
  ml_stbtt_packed_chars *data = Packed_chars_val(packed_chars);
  return Val_long(data->count);
}

value ml_stbtt_packed_chars_box(value packed_chars, value index)
{
  CAMLparam2(packed_chars, index);
  CAMLlocal1(ret);

  ml_stbtt_packed_chars *data = Packed_chars_val(packed_chars);
  unsigned int idx = Long_val(index);

  if (idx >= data->count)
    caml_invalid_argument("Stb_truetype.packed_chars_box");
  else
  {
    stbtt_packedchar *pack = &data->chars[idx];
    ret = box(pack->x0, pack->y0, pack->x1, pack->y1);
  }

  CAMLreturn(ret);
}

value ml_stbtt_packed_chars_metrics(value packed_chars, value index)
{
  CAMLparam2(packed_chars, index);
  CAMLlocal1(ret);

  ml_stbtt_packed_chars *data = Packed_chars_val(packed_chars);
  unsigned int idx = Long_val(index);

  if (idx >= data->count)
    caml_invalid_argument("Stb_truetype.packed_chars_metrics");
  else
  {
    stbtt_packedchar *pack = &data->chars[idx];

    ret = caml_alloc(5 * Double_wosize, Double_array_tag);
    Store_double_field(ret, 0, pack->xoff);
    Store_double_field(ret, 1, pack->yoff);
    Store_double_field(ret, 2, pack->xadvance);
    Store_double_field(ret, 3, pack->xoff2);
    Store_double_field(ret, 4, pack->yoff2);
  }

  CAMLreturn(ret);
}

value ml_stbtt_packed_chars_quad(value packed_chars, value index, value bw, value bh, value sx, value sy, value int_align)
{
  CAMLparam5(packed_chars, index, bw, bh, sx);
  CAMLxparam1(sy);
  CAMLlocal3(sx2, quad, ret);

  ml_stbtt_packed_chars *data = Packed_chars_val(packed_chars);
  stbtt_aligned_quad q;
  unsigned int idx = Long_val(index);

  if (idx >= data->count)
    caml_invalid_argument("Stb_truetype.packed_chars_quad");
  else
  {
    float xpos = Double_val(sx), ypos = Double_val(sy);
    stbtt_GetPackedQuad(&data->chars[0], Long_val(bw), Long_val(bh), idx, &xpos, &ypos, &q, Long_val(int_align));

    quad = caml_alloc(8 * Double_wosize, Double_array_tag);
    Store_double_field(quad, 0, q.x0);
    Store_double_field(quad, 1, q.y0);
    Store_double_field(quad, 2, q.s0);
    Store_double_field(quad, 3, q.t0);
    Store_double_field(quad, 4, q.x1);
    Store_double_field(quad, 5, q.y1);
    Store_double_field(quad, 6, q.s1);
    Store_double_field(quad, 7, q.t1);

    sx2 = caml_copy_double(xpos);

    ret = caml_alloc(2, 0);
    Store_field(ret, 0, sx2);
    Store_field(ret, 1, quad);
  }

  CAMLreturn(ret);
}

value ml_stbtt_packed_chars_quad_bc(value *argv, int argn)
{
  return ml_stbtt_packed_chars_quad(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
}

static float font_range_font_size(value font_range)
{
  value sz = Field(font_range, 0);
  if (Tag_val(sz) == 0)
    return Double_val(Field(sz, 0));
  else
    return STBTT_POINT_SIZE(Double_val(Field(sz, 0)));
}

static value packed_chars_alloc(int count, stbtt_pack_range* range)
{
  CAMLparam0();
  CAMLlocal1(ret);
  int size = sizeof(ml_stbtt_packed_chars) + sizeof(stbtt_packedchar) * (count - 1);

  ret = caml_alloc_string(size);
  ml_stbtt_packed_chars *data = Packed_chars_val(ret);
  data->count = count;
  if (range)
    range->chardata_for_range = &data->chars[0];

  CAMLreturn(ret);
}

static int ml_stbtt_PackFontRanges(stbtt_pack_context *spc,
    stbtt_fontinfo *info, stbtt_pack_range *ranges, int num_ranges)
{
   int i,j,n, return_value = 1;
   //stbrp_context *context = (stbrp_context *) spc->pack_info;
   stbrp_rect    *rects;

   // flag all characters as NOT packed
   for (i=0; i < num_ranges; ++i)
      for (j=0; j < ranges[i].num_chars; ++j)
         ranges[i].chardata_for_range[j].x0 =
         ranges[i].chardata_for_range[j].y0 =
         ranges[i].chardata_for_range[j].x1 =
         ranges[i].chardata_for_range[j].y1 = 0;

   n = 0;
   for (i=0; i < num_ranges; ++i)
      n += ranges[i].num_chars;

   rects = (stbrp_rect *) STBTT_malloc(sizeof(*rects) * n, spc->user_allocator_context);
   if (rects == NULL)
      return 0;

   n = stbtt_PackFontRangesGatherRects(spc, info, ranges, num_ranges, rects);

   stbtt_PackFontRangesPackRects(spc, rects, n);

   return_value = stbtt_PackFontRangesRenderIntoRects(spc, info, ranges, num_ranges, rects);

   STBTT_free(rects, spc->user_allocator_context);
   return return_value;
}

value ml_stbtt_pack_font_ranges(value pack_context, value font_info, value font_ranges)
{
  CAMLparam3(pack_context, font_info, font_ranges);
  CAMLlocal3(font_range, packed_ranges, ret);

  int num_ranges = Wosize_val(font_ranges), i;
  stbtt_pack_range *ranges = alloca(sizeof (stbtt_pack_range) * num_ranges);

  packed_ranges = caml_alloc(num_ranges, 0);
  for (i = 0; i < num_ranges; ++i)
  {
    font_range = Field(font_ranges, i);
    // Validate font_range input?
    ranges[i].font_size = font_range_font_size(font_range);
    ranges[i].first_unicode_codepoint_in_range = Long_val(Field(font_range, 1));
    ranges[i].num_chars = Long_val(Field(font_range, 2));
    ranges[i].chardata_for_range = NULL;

    Store_field(packed_ranges, i, packed_chars_alloc(ranges[i].num_chars, &ranges[i]));
  }

  int result = ml_stbtt_PackFontRanges(Pack_context_val(pack_context), Fontinfo_val(font_info), ranges, num_ranges);

  if (result == 0)
    ret = Val_unit;
  else
  {
    ret = caml_alloc(1, 0);
    Store_field(ret, 0, packed_ranges);
  }

  CAMLreturn(ret);
}

static void put_short(unsigned char **s, unsigned short x)
{
  (*s)[0] = x & 0xFF;
  (*s)[1] = (x >> 8) & 0xFF;
  (*s) += 2;
}

static unsigned short get_short(unsigned char **s)
{
  unsigned short x;
  x = (*s)[0] | ((*s)[1] << 8);
  (*s) += 2;
  return x;
}

static void put_long(unsigned char **s, unsigned long x)
{
  (*s)[0] = x & 0xFF;
  (*s)[1] = (x >> 8) & 0xFF;
  (*s)[2] = (x >> 16) & 0xFF;
  (*s)[3] = (x >> 24) & 0xFF;
  (*s) += 4;
}

static unsigned long get_long(unsigned char **s)
{
  unsigned long x;
  x = (*s)[0] | ((*s)[1] << 8) | ((*s)[2] << 16) | ((*s)[3] << 24);
  (*s) += 4;
  return x;
}

// Less portable but... not the first undefined behavior in this file, <3 C
static void put_float(unsigned char **s, float f)
{
  union {
    unsigned long x;
    float f;
  } u;
  u.f = f;
  put_long(s, u.x);
}

static float get_float(unsigned char **s)
{
  union {
    unsigned long x;
    float f;
  } u;
  u.x = get_long(s);
  return u.f;
}

#define PACKED_CHARS_VERSION 1

value ml_stbtt_string_of_packed_chars(value packed_chars)
{
  CAMLparam1(packed_chars);
  CAMLlocal1(ret);

  ml_stbtt_packed_chars *data = Packed_chars_val(ret);

  /* Compute size of portable string */
  /* version 1 byte,
   * count   4 bytes,
   * content count * (4 * 2 bytes (shorts) + 5 * 4 bytes (floats))
   */
  int size = 1 + 4 + 28 * data->count;
  ret = caml_alloc_string(size);

  unsigned char *s = (unsigned char *)String_val(ret);
  *s = PACKED_CHARS_VERSION;
  s++;
  put_long(&s, data->count);

  int i;
  for (i = 0; i < data->count; ++i)
  {
    stbtt_packedchar *p = &data->chars[i];
    put_short(&s, p->x0);
    put_short(&s, p->y0);
    put_short(&s, p->x1);
    put_short(&s, p->y1);
    put_float(&s, p->xoff);
    put_float(&s, p->yoff);
    put_float(&s, p->xadvance);
    put_float(&s, p->xoff2);
    put_float(&s, p->yoff2);
  }

  CAMLreturn(ret);
}

value ml_stbtt_packed_chars_of_string(value str)
{
  CAMLparam1(str);
  CAMLlocal1(ret);

  unsigned char *s = (unsigned char *)String_val(str);

  if (*s != PACKED_CHARS_VERSION)
    caml_invalid_argument("Stb_truetype.packed_chars_of_string");
  else
  {
    s++;
    unsigned long count = get_long(&s);
    ret = packed_chars_alloc(count, NULL);
    ml_stbtt_packed_chars *data = Packed_chars_val(ret);

    int i;
    for (i = 0; i < count; ++i)
    {
      stbtt_packedchar *p = &data->chars[i];
      p->x0       = get_short(&s);
      p->y0       = get_short(&s);
      p->x1       = get_short(&s);
      p->y1       = get_short(&s);
      p->xoff     = get_float(&s);
      p->yoff     = get_float(&s);
      p->xadvance = get_float(&s);
      p->xoff2    = get_float(&s);
      p->yoff2    = get_float(&s);
    }
  }

  CAMLreturn(ret);
}

value ml_stbtt_MakeGlyphBitmap(value fontinfo, value buffer, value offset, value gw, value gh, value stride, value scale_x, value scale_y, value glyph)
{
  stbtt_MakeGlyphBitmap(
        Fontinfo_val(fontinfo),
        Caml_ba_data_val(buffer) + Long_val(offset),
        Long_val(gw), Long_val(gh),
        Long_val(stride),
        Double_val(scale_x), Double_val(scale_y),
        Long_val(glyph)
      );
  return Val_unit;
}

value ml_stbtt_MakeGlyphBitmap_bc(value *argv, int argn)
{
  if (argn != 9) abort();
  return ml_stbtt_MakeGlyphBitmap(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]);
}

value ml_stbtt_GetGlyphBitmapBox(value fontinfo, value glyph, value scale_x, value scale_y)
{
  int x0, y0, x1, y1;
  stbtt_GetGlyphBitmapBox(Fontinfo_val(fontinfo), Long_val(glyph), Double_val(scale_x), Double_val(scale_y), &x0, &y0, &x1, &y1);
  return box(x0, y0, x1, y1);
}

// Based on Exponential blur, Jani Huhtanen, 2006
// and [https://github.com/memononen/fontstash](fontstash), Mikko Mononen, 2014

#define APREC 16
#define ZPREC 7

#define APPROX(alpha, reg, acc) \
  ((alpha * (((int)(reg) << ZPREC) - acc)) >> APREC)

#define BLUR(reg, acc) \
  do { \
    acc += APPROX(alpha, reg, acc); \
    reg = (unsigned char)(acc >> ZPREC); \
  } while (0)

static void expblur_row(unsigned char* dst, int w, int alpha)
{
  int x, acc;

  for (x = 1, acc = 0; x < w; ++x) BLUR(dst[x], acc);
  dst[w-1] = 0;

  for (x = w - 2, acc = 0; x >= 0; --x) BLUR(dst[x], acc);
  dst[0] = 0;
}

static void expblur_col(unsigned char* dst, int h, int stride, int alpha)
{
  int y, acc;

  for (y = stride, acc = 0; y < h*stride; y += stride) BLUR(dst[y], acc);
  dst[(h-1)*stride] = 0;

  for (y = (h - 2) * stride, acc = 0; y >= 0; y -= stride) BLUR(dst[y], acc);
  dst[0] = 0;
}

static void expblur(unsigned char* dst, int w, int h, int stride, float blur)
{
	int i, alpha;
	float sigma;

  if (blur < 0.01) return;

	// Calculate the alpha such that 90% of the kernel is within the radius. (Kernel extends to infinity)
	sigma = blur * 0.57735f; // 1 / sqrt(3)
	alpha = (int)((1<<APREC) * (1.0f - expf(-2.3f / (sigma + 1.0f))));

	for (i = 0; i < h; ++i)
  {
    expblur_row(dst + i * stride, w, alpha);
    expblur_row(dst + i * stride, w, alpha);
	}

	for (i = 0; i < w; ++i)
  {
    expblur_col(dst + i, h, stride, alpha);
    expblur_col(dst + i, h, stride, alpha);
	}
}

value ml_stbtt_BlurGlyphBitmap(value buffer, value offset, value gw, value gh, value stride, value blur)
{
  expblur(
        Caml_ba_data_val(buffer) + Long_val(offset),
        Long_val(gw),
        Long_val(gh),
        Long_val(stride),
        Double_val(blur)
      );
  return Val_unit;
}

value ml_stbtt_BlurGlyphBitmap_bc(value *argv, int argn)
{
  if (argn != 6) abort();
  return ml_stbtt_BlurGlyphBitmap(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
}

