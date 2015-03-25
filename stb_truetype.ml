open Bigarray

type buffer = (int, int8_unsigned_elt, c_layout) Array1.t
type offset = int
type glyph = int

type codepoint = int

type font_info
type t = {
  buffer: buffer;
  info: font_info;
}

external ml_stbtt_GetFontOffsetForIndex: buffer -> int -> offset = "ml_stbtt_GetFontOffsetForIndex"
external ml_stbtt_InitFont : buffer -> offset -> font_info option = "ml_stbtt_InitFont"
external ml_stbtt_FindGlyphIndex : font_info -> int -> glyph = "ml_stbtt_FindGlyphIndex"

let rec enum buffer index =
  match ml_stbtt_GetFontOffsetForIndex buffer index with
  | offset when offset < 0 -> []
  | offset -> offset :: enum buffer (succ index)
let enum buffer = enum buffer 0

let init buffer offset =
  match ml_stbtt_InitFont buffer offset with
  | None -> None
  | Some info -> Some {buffer; info}

let find t codepoint =
  match ml_stbtt_FindGlyphIndex t.info codepoint with
  | 0 -> None
  | glyph -> Some glyph


external ml_stbtt_ScaleForPixelHeight : font_info -> float -> float = "ml_stbtt_ScaleForPixelHeight"
external ml_stbtt_ScaleForMappingEmToPixels : font_info -> float -> float = "ml_stbtt_ScaleForMappingEmToPixels"
let scale_for_pixel_height t h =
  let result = ml_stbtt_ScaleForPixelHeight t.info h in
  ignore t;
  result

let scale_for_mapping_em_to_pixels t h =
  let result = ml_stbtt_ScaleForMappingEmToPixels t.info h in
  ignore t;
  result

type font_size =
  | Size_max  of float
  | Size_of_M of float

let scale_for_size t = function
  | Size_max h -> scale_for_pixel_height t h
  | Size_of_M h -> scale_for_mapping_em_to_pixels t h

type vmetrics = {ascent: int; descent: int; line_gap: int}
external ml_stbtt_GetFontVMetrics : font_info -> vmetrics = "ml_stbtt_GetFontVMetrics"
let vmetrics t =
  let result = ml_stbtt_GetFontVMetrics t.info in
  ignore t;
  result

type hmetrics = {advance_width: int; left_side_bearing: int}
external ml_stbtt_GetGlyphHMetrics : font_info -> glyph -> hmetrics = "ml_stbtt_GetGlyphHMetrics"
let hmetrics t glyph =
  let result = ml_stbtt_GetGlyphHMetrics t.info glyph in
  ignore t;
  result

external ml_stbtt_GetGlyphKernAdvance : font_info -> glyph -> glyph -> int = "ml_stbtt_GetGlyphKernAdvance"
let kern_advance t glyph1 glyph2 =
  let result = ml_stbtt_GetGlyphKernAdvance t.info glyph1 glyph2 in
  ignore t;
  result

type box = {x0: int; y0: int; x1: int; y1: int}

external ml_stbtt_GetFontBoundingBox : font_info -> box = "ml_stbtt_GetFontBoundingBox"
let font_box t =
  let result = ml_stbtt_GetFontBoundingBox t.info in
  ignore t;
  result

external ml_stbtt_GetGlyphBox : font_info -> glyph -> box = "ml_stbtt_GetGlyphBox"
let glyph_box t glyph =
  let result = ml_stbtt_GetGlyphBox t.info glyph in
  ignore t;
  result

(* Bitmap packing *)

type pack_context_
external ml_stbtt_PackBegin : buffer -> width:int -> height:int -> stride:int -> padding:int -> pack_context_ option = "ml_stbtt_PackBegin"
external ml_stbtt_PackSetOversampling : pack_context_ -> h:int -> v:int -> unit = "ml_stbtt_PackSetOversampling"

type pack_context = {
  bitmap: buffer;
  context: pack_context_;
}

let pack_begin bitmap ~width ~height ~stride ~padding =
  match ml_stbtt_PackBegin bitmap ~width ~height ~stride ~padding with
  | None -> None
  | Some context -> Some {bitmap; context}

let pack_set_oversampling context ~h ~v =
  ml_stbtt_PackSetOversampling context.context ~h ~v;
  ignore context

type font_range = {
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

external packed_chars_count  : packed_chars -> int = "ml_stbtt_packed_chars_count"
external packed_chars_box    : packed_chars -> int -> box = "ml_stbtt_packed_chars_box"
external packed_chars_metrics : packed_chars -> int -> char_metrics = "ml_stbtt_packed_chars_metrics"

type char_quad = {
  bx0: float; by0: float;
  s0: float; t0: float;
  bx1: float; by1: float;
  s1: float; t1: float;
}
external packed_chars_quad : packed_chars -> int -> bitmap_width:int -> bitmap_height:int -> screen_x:float -> screen_y:float -> align_on_int:bool -> float * char_quad = "ml_stbtt_packed_chars_quad_bc" "ml_stbtt_packed_chars_quad"

external ml_stbtt_pack_font_ranges : pack_context_ -> font_info -> font_range array -> bool * packed_chars array = "ml_stbtt_pack_font_ranges"

let pack_font_ranges ctx t ranges =
  let result = ml_stbtt_pack_font_ranges ctx.context t.info ranges in
  ignore ctx;
  ignore t;
  result
