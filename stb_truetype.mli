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
val enum: buffer -> offset list

(** A font *)
type t

(** [init buffer offset] try to open the font in [buffer] at the specified
    [offset], and return [None] if font was invalid. *)
val init: buffer -> offset -> t option

(** A unicode codepoint *)
type codepoint = int

(** A font-specific index to access data related to some codepoint. *)
type glyph = private int

(** A font associates information (vectors and metrics) to some unicode
    codepoints.
    Given a [codepoint], the first step is to turn it into a [glyph]. *)
val find: t -> codepoint -> glyph option

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
val scale_for_size: t -> font_size -> float

val scale_for_pixel_height: t -> float -> float
val scale_for_mapping_em_to_pixels: t -> float -> float

(** Vertical metrics of the font. *)
type vmetrics = {
  ascent:   int; (** the space between the baseline and the top of the characters. *)
  descent:  int; (** the space between the baseline and the bottom of the characters. *)
  line_gap: int; (** the space between the baselines of two consecutive lines. *)
}
val vmetrics: t -> vmetrics

(** Horizontal metrics of the font.
    When rendering a line, a virtual pen is moving from the left to the right;
    [hmetrics] describe the motion of the pen for a specific character.  *)
type hmetrics = {
  advance_width: int; (** distance to move-to-the-right after rendering glyph. *)
  left_side_bearing: int; (** distance from current position to the left of the character bounding box. *)
}
val hmetrics: t -> glyph -> hmetrics

(** Kerning allows to vary [advance_width] to improve quality of the rendering.
    Given two glyphs, [kern_advance] will return an eventually more specific
    [advance_width] value that matches closely this specific sequence of
    characters. *)
val kern_advance: t -> glyph -> glyph -> int

(** A bounding box, as a pair of points *)
type box = {x0: int; y0: int; x1: int; y1: int}

(** Bounding box around all possible characters *)
val font_box: t -> box

(** Bounding box around a specific glyph *)
val glyph_box: t -> glyph -> box

(*#####################*)
(** {1 Bitmap packing}
    Rasterize glyphs on a user-provided surface, try to pack them in a compact
    way.
    Then provide an index:
    - to lookup characters on the surface
    - compute coordinates to render them using OpenGL.
*)

(** [pack_context] is the state of the packing algorithm.
    Incompatible with polymorphic operators. *)
type pack_context

(** [pack_begin buffer ~width ~height ~stride ~padding] creates a new packer
    rasterizing its contents on [buffer], interpreted as a bitmap of
    [width] x [height] pixels (1 channel, 8-bit gray).
    [stride] is the number of bytes between one line and the next one
    (if the bitmap is compact, then [width = stride]).
    [padding] is the number of pixels left blank around glyphs
    (use at least 1 when using bilinear filtering to blit glyphs)
*)
val pack_begin: buffer -> width:int -> height:int -> stride:int -> padding:int -> pack_context option

(** [pack_set_oversampling context ~h ~v] will render glyphs at a higher
    resolution to increase rendering quality.
    [1 <= {h,v} <= 8] *)
val pack_set_oversampling: pack_context -> h:int -> v:int -> unit

(** A range of characters to rasterize and pack *)
type char_range = {
  font_size: font_size; (** Size to render at *)
  first_codepoint: int; (** First codepoint in the range *)
  count: int; (** Number of consecutive characters to render *)
}

(** Results of character packing, see below. *)
type packed_chars

(** Run the packer on some ranges of characters:
    [pack_font_ranges context font ?font_index ranges] will return:
    - [None] if there wasn't enough room to pack ranges on the bitmap
    - [Some arr] if everything went well; the bitmap will have been updated
      with the characters, and [arr] will contain a [packed_chars] for each
      input [char_range]
*)
val pack_font_ranges: pack_context -> t -> ?font_index:int -> char_range array -> packed_chars array option

(*#####################*)
(** {2 Packed characters} *)

(* Number of glyphs in the given pack *)
val packed_chars_count: packed_chars -> int

(* [packed_chars_box pack n] is the bounding box of the n'th character on the
   bitmap *)
val packed_chars_box: packed_chars -> int -> box

(* Raw access to packed character metrics, see the quad interface below for a
   more convenient interface.
   Assuming cursor is at coordinates (cx, cy), the character should be rendered
   in the box defined by (cx + xoff, cy + yoff) and (cx + xoff2, cy + yoff2).
   Then cursor should be updated to (cx + xadvance, cy).
*)
type char_metrics = {
  xoff: float;
  yoff: float;
  xadvance: float;
  xoff2: float;
  yoff2: float;
}

(** [packed_chars_metrics pack n] are the metrics of the n'th character *)
val packed_chars_metrics: packed_chars -> int -> char_metrics

(** Convenient interface for OpenGL quad drawing *)
type char_quad = {
  bx0: float; by0: float; (* first vertex *)
  s0:  float; t0:  float; (* first tex coordinates *)
  bx1: float; by1: float; (* second vertex *)
  s1:  float; t1:  float; (* second tex coordinates *)
}

(** [packed_chars_quad pack n
     ~bitmap_width ~bitmap_height ~pen_x ~pen_y ~align_on_int]
    will return a pair [(pen_x', quad)] of the new [pen_x] coordinate and the
    OpenGL quad to draw.
    [bitmap_width] and [bitmap_height] are the dimension of the texture, to
    compute the coordinates in the [0,1] interval expected by OpenGL.
    [pen_x] and [pen_y] are the coordinates of the pen; in other words, where
    you want to draw the character in the framebuffer.
    if [align_on_int] is true, the first vertex of the quad will be rounded to
    integer coordinates.
*)
val packed_chars_quad: packed_chars -> int -> bitmap_width:int -> bitmap_height:int -> screen_x:float -> screen_y:float -> align_on_int:bool -> float * char_quad

(** [packed_chars] can be marshalled, but the representation will rely on host
    endianness and bit-width.
    This function turns the packed_chars into a binary string representation,
    easier to serialize/deserialize.
*)
val string_of_packed_chars: packed_chars -> string

(** Inverse to [string_of_packed_chars]. *)
val packed_chars_of_string: string -> packed_chars
