open Bigarray

type buffer = (int, int8_unsigned_elt, c_layout) Array1.t
type offset = private int
type glyph = private int

type codepoint = int

type t

val enum : buffer -> offset list
val init : buffer -> offset -> t option
val find : t -> codepoint -> glyph option

type font_size =
  | Size_max  of float
  | Size_of_M of float

val scale_for_pixel_height : t -> float -> float
val scale_for_mapping_em_to_pixels : t -> float -> float
val scale_for_size : t -> font_size -> float

type vmetrics = {ascent: int; descent: int; line_gap: int}
val vmetrics : t -> vmetrics

type hmetrics = {advance_width: int; left_side_bearing: int}
val hmetrics : t -> glyph -> hmetrics

val kern_advance : t -> glyph -> glyph -> int

type box = {x0: int; y0: int; x1: int; y1: int}

val font_box : t -> box
val glyph_box : t -> glyph -> box

(* Bitmap packing *)

type pack_context
val pack_begin : buffer -> width:int -> height:int -> stride:int -> padding:int -> pack_context option

val pack_set_oversampling : pack_context -> h:int -> v:int -> unit

type font_range = {
  font_size: font_size;
  first_codepoint: int;
  count: int;
}

type packed_chars

val packed_chars_count : packed_chars -> int
val packed_chars_box   : packed_chars -> int -> box

type char_metrics = {
  xoff: float;
  yoff: float;
  xadvance: float;
  xoff2: float;
  yoff2: float;
}
val packed_chars_metrics : packed_chars -> int -> char_metrics

type char_quad = {
  bx0: float; by0: float;
  s0: float; t0: float;
  bx1: float; by1: float;
  s1: float; t1: float;
}
val packed_chars_quad : packed_chars -> int -> bitmap_width:int -> bitmap_height:int -> screen_x:float -> screen_y:float -> align_on_int:bool -> float * char_quad

val pack_font_ranges : pack_context -> t -> font_range array -> packed_chars array option
