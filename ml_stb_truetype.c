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

CAMLprim value ml_stbtt_GetFontOffsetForIndex(value ba, value vindex)
{
  CAMLparam2(ba, vindex);
  unsigned char *data = Caml_ba_data_val(ba);
  int index = Int_val(vindex);
  int result = stbtt_GetFontOffsetForIndex(data, index);
  CAMLreturn(Val_int(result));
}

static struct custom_operations fontinfo_custom_ops = {
    identifier: "stbtt_fontinfo",
    finalize:    custom_finalize_default,
    compare:     custom_compare_default,
    hash:        custom_hash_default,
    serialize:   custom_serialize_default,
    deserialize: custom_deserialize_default
};

/* Layout of fontinfo and, later, pack_context:
 * (custom, buffer)
 * where custom is a custom block with library-specific data
 *       buffer is a reference kept to underlying bigarray store
 */

#define Fontinfo_val(x) (Data_custom_val(Field((x), 0)))

CAMLprim value ml_stbtt_InitFont(value ba, value voffset)
{
  CAMLparam2(ba, voffset);
  CAMLlocal3(ret, pack, fontinfo);

  unsigned char *data = Caml_ba_data_val(ba);
  int index = Int_val(voffset);

  fontinfo = caml_alloc_custom(&fontinfo_custom_ops, sizeof(stbtt_fontinfo), 0, 1);
  int result = stbtt_InitFont(Data_custom_val(fontinfo), data, index);

  if (result == 0)
    ret = Val_unit;
  else
  {
    pack = caml_alloc(2, 0);
    Store_field(pack, 0, fontinfo);
    Store_field(pack, 1, ba);

    ret = caml_alloc(1, 0);
    Store_field(ret, 0, pack);
  }

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_FindGlyphIndex(value fontinfo, value codepoint)
{
  CAMLparam2(fontinfo, codepoint);
  CAMLlocal1(ret);

  int index = stbtt_FindGlyphIndex(Fontinfo_val(fontinfo), Int_val(codepoint));

  if (index == 0)
    ret = Val_unit;
  else
  {
    ret = caml_alloc(1, 0);
    Store_field(ret, 0, Val_int(index));
  }

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_ScaleForPixelHeight(value fontinfo, value height)
{
  CAMLparam2(fontinfo, height);
  CAMLlocal1(ret);

  float scale = stbtt_ScaleForPixelHeight(Fontinfo_val(fontinfo), Double_val(height));
  ret = caml_copy_double(scale);

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_ScaleForMappingEmToPixels(value fontinfo, value height)
{
  CAMLparam2(fontinfo, height);
  CAMLlocal1(ret);

  float scale = stbtt_ScaleForMappingEmToPixels(Fontinfo_val(fontinfo), Double_val(height));
  ret = caml_copy_double(scale);

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_GetFontVMetrics(value fontinfo)
{
  CAMLparam1(fontinfo);
  CAMLlocal1(ret);

  int ascent, descent, line_gap;
  stbtt_GetFontVMetrics(Fontinfo_val(fontinfo), &ascent, &descent, &line_gap);

  ret = caml_alloc(3, 0);
  Store_field(ret, 0, Val_int(ascent));
  Store_field(ret, 1, Val_int(descent));
  Store_field(ret, 2, Val_int(line_gap));

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_GetGlyphHMetrics(value fontinfo, value glyph)
{
  CAMLparam2(fontinfo, glyph);
  CAMLlocal1(ret);

  int adv, lsb;
  stbtt_GetGlyphHMetrics(Fontinfo_val(fontinfo), Int_val(glyph), &adv, &lsb);

  ret = caml_alloc(2, 0);
  Store_field(ret, 0, Val_int(adv));
  Store_field(ret, 1, Val_int(lsb));

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_GetGlyphKernAdvance(value fontinfo, value glyph1, value glyph2)
{
  CAMLparam3(fontinfo, glyph1, glyph2);
  CAMLlocal1(ret);

  int adv = stbtt_GetGlyphKernAdvance(Fontinfo_val(fontinfo), Int_val(glyph1), Int_val(glyph2));

  ret = Val_int(adv);

  CAMLreturn(ret);
}

static value box(int x0, int y0, int x1, int y1)
{
  CAMLparam0();
  CAMLlocal1(ret);

  ret = caml_alloc(4, 0);
  Store_field(ret, 0, x0);
  Store_field(ret, 1, y0);
  Store_field(ret, 2, x1);
  Store_field(ret, 3, y1);

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_GetFontBoundingBox(value fontinfo)
{
  CAMLparam1(fontinfo);

  int x0, y0, x1, y1;
  stbtt_GetFontBoundingBox(Fontinfo_val(fontinfo), &x0, &y0, &x1, &y1);

  CAMLreturn(box(x0, y0, x1, y1));
}

CAMLprim value ml_stbtt_GetGlyphBox(value fontinfo, value glyph)
{
  CAMLparam2(fontinfo, glyph);

  int x0, y0, x1, y1;
  stbtt_GetGlyphBox(Fontinfo_val(fontinfo), Int_val(glyph), &x0, &y0, &x1, &y1);

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
    identifier: "stbtt_pack_context",
    finalize:    pack_context_finalize,
    compare:     custom_compare_default,
    hash:        custom_hash_default,
    serialize:   custom_serialize_default,
    deserialize: custom_deserialize_default
};

CAMLprim value ml_stbtt_PackBegin(value buffer, value w, value h, value s, value p)
{
  CAMLparam5(buffer, w, h, s, p);
  CAMLlocal3(ret, pack, pack_context);

  unsigned char *data = Caml_ba_data_val(buffer);
  int width = Int_val(w), height = Int_val(h), stride = Int_val(s),
      padding = Int_val(p);

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

CAMLprim value ml_stbtt_PackSetOversampling(value ctx, value h, value v)
{
  CAMLparam3(ctx, h, v);
  stbtt_PackSetOversampling(Pack_context_val(ctx), Int_val(h), Int_val(v));
  CAMLreturn(Val_unit);
}

typedef struct {
  int count;
  stbtt_packedchar chars[1];
} ml_stbtt_packed_chars;

#define Packed_chars_val(x) ((ml_stbtt_packed_chars *)String_val(x))
CAMLprim value ml_stbtt_packed_chars_count(value packed_chars)
{
  CAMLparam1(packed_chars);
  ml_stbtt_packed_chars *data = Packed_chars_val(packed_chars);
  CAMLreturn(Val_int(data->count));
}

CAMLprim value ml_stbtt_packed_chars_box(value packed_chars, value index)
{
  CAMLparam2(packed_chars, index);
  CAMLlocal1(ret);

  ml_stbtt_packed_chars *data = Packed_chars_val(packed_chars);
  unsigned int idx = Int_val(index);

  if (idx >= data->count)
    caml_invalid_argument("Stb_truetype.packed_chars_box");
  else
  {
    stbtt_packedchar *pack = &data->chars[idx];
    ret = box(pack->x0, pack->y0, pack->x1, pack->y1);
  }

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_packed_chars_metrics(value packed_chars, value index)
{
  CAMLparam2(packed_chars, index);
  CAMLlocal1(ret);

  ml_stbtt_packed_chars *data = Packed_chars_val(packed_chars);
  unsigned int idx = Int_val(index);

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

CAMLprim value ml_stbtt_packed_chars_quad(value packed_chars, value index, value bw, value bh, value sx, value sy, value int_align)
{
  CAMLparam5(packed_chars, index, bw, bh, sx);
  CAMLxparam1(sy);
  CAMLlocal3(sx2, quad, ret);

  ml_stbtt_packed_chars *data = Packed_chars_val(packed_chars);
  stbtt_aligned_quad q;
  unsigned int idx = Int_val(index);

  if (idx >= data->count)
    caml_invalid_argument("Stb_truetype.packed_chars_quad");
  else
  {
    float xpos = Double_val(sx), ypos = Double_val(sy);
    stbtt_GetPackedQuad(&data->chars[0], Int_val(bw), Int_val(bh), idx, &xpos, &ypos, &q, Int_val(int_align));

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

CAMLprim value ml_stbtt_packed_chars_quad_bc(value *argv, int argn)
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

CAMLprim value ml_stbtt_pack_font_ranges(value pack_context, value font_info, value font_ranges)
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
    ranges[i].first_unicode_char_in_range = Int_val(Field(font_range, 1));
    ranges[i].num_chars_in_range = Int_val(Field(font_range, 2));
    ranges[i].chardata_for_range = NULL;

    Store_field(packed_ranges, i, packed_chars_alloc(ranges[i].num_chars_in_range, &ranges[i]));
  }

  int result = stbtt_PackFontInfoRanges(Pack_context_val(pack_context), Fontinfo_val(font_info), ranges, num_ranges);

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

CAMLprim value ml_stbtt_string_of_packed_chars(value packed_chars)
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

CAMLprim value ml_stbtt_packed_chars_of_string(value str)
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
