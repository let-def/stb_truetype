(*
  Stb_truetype for OCaml by Frédéric Bour <frederic.bour(_)lakaban.net>
  To the extent possible under law, the person who associated CC0 with
  Stb_truetype for OCaml has waived all copyright and related or neighboring
  rights to Stb_truetype for OCaml.

  You should have received a copy of the CC0 legalcode along with this
  work. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

  Website: https://github.com/def-lkb/stb_truetype
  stb_truetype is a public domain library by Sean Barrett,
  http://nothings.org/

  Version 0.1, Mars 2015
*)
open Bigarray

(*##############################*)
(** {1 Manipulating font files} *)

(** [buffer] simply is an alias to bigarray of characters.
    All buffers accessed by this library are managed by the user.
    The [buffer] type serves two purposes:
    - representing input font files,
    - representing the backing store when rasterizing.
*)
type buffer = (int, int8_unsigned_elt, c_layout) Array1.t

(** A raw font is represented by a pair [(buffer,offset)], where [offset] is
    the index of the first byte of this font in the [buffer].
    This is useful during initialization, to enumerate fonts stored in a given
    buffer and then open a handle to a specific font. *)
type offset = private int

(** [enum buffer] list all fonts found in [buffer] *)
val enum : buffer -> offset list

(** A font *)
type t

(** [init buffer offset] try to open the font in [buffer] at the specified
    [offset], and return [None] if font was invalid. *)
val init : buffer -> offset -> t option

(** A unicode codepoint *)
type codepoint = int

(** A font-specific index to access data related to some codepoint. *)
type glyph = private int

(** A font associates information (vectors and metrics) to some unicode
    codepoints.
    Given a [codepoint], the first step is to turn it into a [glyph]. *)
val find : t -> codepoint -> glyph option

(*################################*)
(** {1 Manipulating font metrics}
    @see <http://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html> *)

(** Use [font_size] to express the size you desire to render the font at. *)
type font_size =
  | Size_max  of float (** [Size_max f] means that the tallest character in the
                           font should fit in [f] pixels height. *)
  | Size_of_M of float (** [Size_of_M f] means that M character should fit in
                           [f] pixels height. *)

(** Turns a [font_size] into a font-specific scale.
    All other metrics are unscaled, which means you should multiply
    them by the [scale] to get the desired size. *)
val scale_for_size : t -> font_size -> float

val scale_for_pixel_height : t -> float -> float
val scale_for_mapping_em_to_pixels : t -> float -> float

(** Vertical metrics of the font. *)
type vmetrics = {
  ascent   : int; (** the space between the baseline and the top of the characters. *)
  descent  : int; (** the space between the baseline and the bottom of the characters. *)
  line_gap : int; (** the space between the baselines of two consecutive lines. *)
}
val vmetrics : t -> vmetrics

(** Horizontal metrics of the font.
    When rendering a line, a virtual pen is moving from the left to the right;
    [hmetrics] describe the motion of the pen for a specific character.  *)
type hmetrics = {
  advance_width: int; (** distance to move-to-the-right after rendering glyph. *)
  left_side_bearing: int; (** distance from current position to the left of the character bounding box. *)
}
val hmetrics : t -> glyph -> hmetrics

(** Kerning allows to vary [advance_width] to improve quality of the rendering.
    Given two glyphs, [kern_advance] will return an eventually more specific
    [advance_width] value that matches closely this specific sequence of
    characters. *)
val kern_advance : t -> glyph -> glyph -> int

(** A bounding box, as a pair of points *)
type box = {x0: int; y0: int; x1: int; y1: int}

(** Bounding box around all possible characters *)
val font_box : t -> box

(** Bounding box around a specific glyph *)
val glyph_box : t -> glyph -> box

(*#####################*)
(** {1 Bitmap packing} *)

type pack_context
val pack_begin : buffer -> width:int -> height:int -> stride:int -> padding:int -> pack_context option

val pack_set_oversampling : pack_context -> h:int -> v:int -> unit

type font_range = {
  font_size: font_size;
  first_codepoint: int;
  count: int;
}

type packed_chars

val pack_font_ranges : pack_context -> t -> font_range array -> packed_chars array option

(*#####################*)
(** {2 Packed characters} *)

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

val packed_chars_of_string : string -> packed_chars
val string_of_packed_chars : packed_chars -> string
