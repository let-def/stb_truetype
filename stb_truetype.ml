(*
  Stb_truetype for OCaml by Frédéric Bour <frederic.bour(_)lakaban.net>
  To the extent possible under law, the person who associated CC0 with
  Stb_truetype for OCaml has waived all copyright and related or neighboring
  rights to Stb_truetype for OCaml.

  You should have received a copy of the CC0 legalcode along with this
  work. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

  Website: https://github.com/let-def/stb_truetype
  stb_truetype is a public domain library by Sean Barrett,
  http://nothings.org/

  Version 0.1, Mars 2015
*)
open Bigarray

type buffer = (int, int8_unsigned_elt, c_layout) Array1.t
type offset = int
type glyph = int

type codepoint = int

type t

external ml_stbtt_GetFontOffsetForIndex: buffer -> int -> offset =
  "ml_stbtt_GetFontOffsetForIndex" [@@noalloc]

let rec enum buffer index =
  match ml_stbtt_GetFontOffsetForIndex buffer index with
  | offset when offset < 0 -> []
  | offset -> offset :: enum buffer (succ index)
let enum buffer = enum buffer 0

external init : buffer -> offset -> t option = "ml_stbtt_InitFont"

external get : t -> int -> glyph = "ml_stbtt_FindGlyphIndex"

let invalid_glyph : glyph = 0

let find t cp =
  let result = get t cp in
  if result = 0 then None else Some result

external scale_for_pixel_height : t -> (float [@unboxed]) -> (float [@unboxed]) =
  "ml_stbtt_ScaleForPixelHeight_bc" "ml_stbtt_ScaleForPixelHeight" [@@noalloc]

external scale_for_mapping_em_to_pixels : t -> (float [@unboxed]) -> (float [@unboxed]) =
  "ml_stbtt_ScaleForMappingEmToPixels_bc" "ml_stbtt_ScaleForMappingEmToPixels" [@@noalloc]

type font_size =
  | Size_max  of float
  | Size_of_M of float

let scale_for_size t = function
  | Size_max h -> scale_for_pixel_height t h
  | Size_of_M h -> scale_for_mapping_em_to_pixels t h

type vmetrics = {ascent: int; descent: int; line_gap: int}
external vmetrics : t -> vmetrics = "ml_stbtt_GetFontVMetrics"

type hmetrics = {advance_width: int; left_side_bearing: int}
external hmetrics : t -> glyph -> hmetrics = "ml_stbtt_GetGlyphHMetrics"

external glyph_advance : t -> glyph -> int = "ml_stbtt_GetGlyphAdvance" [@@noalloc]
external kern_advance : t -> glyph -> glyph -> int = "ml_stbtt_GetGlyphKernAdvance" [@@noalloc]

type box = {x0: int; y0: int; x1: int; y1: int}

external font_box : t -> box = "ml_stbtt_GetFontBoundingBox"
external glyph_box : t -> glyph -> box = "ml_stbtt_GetGlyphBox"

(* Bitmap packing *)

type pack_context

external pack_begin : buffer -> width:int -> height:int -> stride:int -> padding:int -> pack_context option = "ml_stbtt_PackBegin"
external pack_set_oversampling : pack_context -> h:int -> v:int -> unit = "ml_stbtt_PackSetOversampling" [@@noalloc]

type char_range = {
  font_size: font_size;
  first_codepoint: int;
  count: int;
}

type packed_chars
type char_metrics = {
  xoff: float;
  yoff: float;
  xadvance: float;
  xoff2: float;
  yoff2: float;
}

external packed_chars_count  : packed_chars -> int = "ml_stbtt_packed_chars_count" [@@noalloc]
external packed_chars_box    : packed_chars -> int -> box = "ml_stbtt_packed_chars_box"
external packed_chars_metrics : packed_chars -> int -> char_metrics = "ml_stbtt_packed_chars_metrics"

type char_quad = {
  bx0: float; by0: float;
  s0: float; t0: float;
  bx1: float; by1: float;
  s1: float; t1: float;
}
external packed_chars_quad : packed_chars -> int -> bitmap_width:int -> bitmap_height:int -> screen_x:float -> screen_y:float -> align_on_int:bool -> float * char_quad = "ml_stbtt_packed_chars_quad_bc" "ml_stbtt_packed_chars_quad"

external pack_font_ranges : pack_context -> t -> char_range array -> packed_chars array option = "ml_stbtt_pack_font_ranges"

external packed_chars_of_string : string -> packed_chars = "ml_stbtt_packed_chars_of_string"
external string_of_packed_chars : packed_chars -> string = "ml_stbtt_string_of_packed_chars"


external make_glyph_bitmap: t -> buffer -> offset:int -> gw:int -> gh:int -> stride:int -> scale_x:float -> scale_y:float -> glyph -> unit
  = "ml_stbtt_MakeGlyphBitmap_bc" "ml_stbtt_MakeGlyphBitmap"
  [@@noalloc]

let make_glyph_bitmap t buffer ~width ~height ~scale_x ~scale_y box glyph =
  let dim = Bigarray.Array1.dim buffer in
  if width * height > dim then
    invalid_arg "Stb_truetype.make_glyph_bitmap: \
                 width * height bigger than buffer";
  if box.x0 > box.x1 || box.y0 > box.y1 then
    Printf.ksprintf invalid_arg
      "Stb_truetype.make_glyph_bitmap: malformed box \
       {x0=%d; y0=%d; x1=%d; y1=%d}"
      box.x0 box.x1 box.y0 box.y1;
  if box.x0 < 0 || box.y0 < 0 || box.x1 > width || box.y1 > height then
    invalid_arg "Stb_truetype.make_glyph_bitmap: box outside of buffer";
  make_glyph_bitmap t buffer
    ~stride:width
    ~offset:(box.y0 * width + box.x0)
    ~gw:(box.x1-box.x0)
    ~gh:(box.y1-box.y0)
    ~scale_x ~scale_y glyph

external make_glyph_bitmap_subpixel: t -> buffer -> offset:int -> gw:int -> gh:int -> stride:int -> scale_x:float -> scale_y:float -> shift_x:float -> shift_y:float -> glyph -> unit
  = "ml_stbtt_MakeGlyphBitmapSubpixel_bc" "ml_stbtt_MakeGlyphBitmapSubpixel"
  [@@noalloc]

let make_glyph_bitmap_subpixel t buffer ~width ~height ~scale_x ~scale_y ~shift_x ~shift_y box glyph =
  let dim = Bigarray.Array1.dim buffer in
  if width * height > dim then
    invalid_arg "Stb_truetype.make_glyph_bitmap_subpixel: \
                 width * height bigger than buffer";
  if box.x0 > box.x1 || box.y0 > box.y1 then
    Printf.ksprintf invalid_arg
      "Stb_truetype.make_glyph_bitmap_subpixel: malformed box \
       {x0=%d; y0=%d; x1=%d; y1=%d}"
      box.x0 box.x1 box.y0 box.y1;
  if box.x0 < 0 || box.y0 < 0 || box.x1 > width || box.y1 > height then
    invalid_arg "Stb_truetype.make_glyph_bitmap_subpixel: box outside of buffer";
  make_glyph_bitmap_subpixel t buffer
    ~stride:width
    ~offset:(box.y0 * width + box.x0)
    ~gw:(box.x1-box.x0)
    ~gh:(box.y1-box.y0)
    ~scale_x ~scale_y
    ~shift_x ~shift_y
    glyph

external get_glyph_bitmap_box: t -> glyph -> scale_x:float -> scale_y:float -> box
  = "ml_stbtt_GetGlyphBitmapBox"

external get_glyph_bitmap_box_subpixel: t -> glyph -> scale_x:float -> scale_y:float -> shift_x:float -> shift_y:float -> box
  = "ml_stbtt_GetGlyphBitmapBoxSubpixel_bc" "ml_stbtt_GetGlyphBitmapBoxSubpixel"

external blur_glyph_bitmap: buffer -> offset:int -> gw:int -> gh:int -> stride:int -> float -> unit
  = "ml_stbtt_BlurGlyphBitmap_bc" "ml_stbtt_BlurGlyphBitmap"
  [@@noalloc]

let blur_glyph_bitmap buffer ~width ~height box amount =
  let dim = Bigarray.Array1.dim buffer in
  if width * height > dim then
    invalid_arg "Stb_truetype.blur_glyph_bitmap: \
                 width * height bigger than buffer";
  if box.x0 > box.x1 || box.y0 > box.y1 then
    invalid_arg "Stb_truetype.blur_glyph_bitmap: malformed box";
  if box.x0 < 0 || box.y0 < 0 || box.x1 > width || box.y1 > height then
    invalid_arg "Stb_truetype.blur_glyph_bitmap: box outside of buffer";
  blur_glyph_bitmap buffer
    ~stride:width
    ~offset:(box.y0 * width + box.x0)
    ~gw:(box.x1-box.x0)
    ~gh:(box.y1-box.y0)
    amount

type glyph_bitmap = {
  buf : buffer;
  w : int;
  h : int;
  xoff : int;
  yoff : int;
}

external get_glyph_bitmap: t -> glyph -> scale_x:float -> scale_y:float -> glyph_bitmap
  = "ml_stbtt_GetGlyphBitmap"

external get_glyph_bitmap_subpixel: t -> glyph -> scale_x:float -> scale_y:float -> shift_x:float -> shift_y:float -> glyph_bitmap
  = "ml_stbtt_GetGlyphBitmapSubpixel_bc" "ml_stbtt_GetGlyphBitmapSubpixel"
