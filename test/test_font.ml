let map_filename filename =
  let fd = Unix.openfile filename [Unix.O_RDONLY] 0 in
  let sz = Unix.lseek fd 0 Unix.SEEK_END in
  assert (Unix.lseek fd 0 Unix.SEEK_SET = 0);
  let arr = Bigarray.Array1.map_file fd Bigarray.int8_unsigned
      Bigarray.c_layout false sz in
  Unix.close fd;
  arr

let save_buffer filename arr =
  let fd = Unix.openfile filename [Unix.O_CREAT; Unix.O_RDWR] 0o644 in
  Unix.ftruncate fd (Bigarray.Array1.dim arr);
  let arr' = Bigarray.Array1.map_file fd Bigarray.int8_unsigned
      Bigarray.c_layout true (Bigarray.Array1.dim arr) in
  Bigarray.Array1.blit arr arr';
  Unix.close fd

let box_to_string {Stb_truetype. x0; y0; x1; y1} scale =
  Printf.sprintf "((%d,%d),(%d,%d)) unscaled, or ((%.2f,%.2f),(%.2f,%.2f)) scaled"
    x0 y0 x1 y1
    (float_of_int x0 *.scale)
    (float_of_int y0 *.scale)
    (float_of_int x1 *.scale)
    (float_of_int y1 *.scale)

let scaled_int i scale =
  Printf.sprintf "%d unscaled, or %.2f scaled" i (float_of_int i *. scale)

let main filename =
  Printf.eprintf "Trying %s\n" filename;
  let buffer = map_filename filename in
  Printf.eprintf "Loaded %s\n" filename;
  let offsets = Stb_truetype.enum buffer in
  Printf.eprintf "%d fonts found\n" (List.length offsets);
  let dump_font idx font =
    (* Scale *)
    let scale = Stb_truetype.scale_for_size font (Stb_truetype.Size_of_M 20.) in
    Printf.eprintf "font scale for a 20 pixels tall M is %f\n" scale;
    (* Bounding boxes *)
    let box = Stb_truetype.font_box font in
    Printf.eprintf "font bounding box is %s\n" (box_to_string box scale);
    begin match Stb_truetype.find font (Char.code 'M') with
      | None -> Printf.eprintf "font contains no glyph for character M\n";
      | Some glyph ->
        Printf.eprintf "glyph M bounding box is %s\n"
          (box_to_string (Stb_truetype.glyph_box font glyph) scale);
    end;
    (* Metrics *)
    let vmetrics = Stb_truetype.vmetrics font in
    Printf.eprintf "Font metric:\n- ascent %s\n- descent %s\n- line gap %s\n"
      (scaled_int vmetrics.Stb_truetype.ascent scale)
      (scaled_int vmetrics.Stb_truetype.descent scale)
      (scaled_int vmetrics.Stb_truetype.line_gap scale);
    begin match Stb_truetype.find font (Char.code 'f') with
      | None -> Printf.eprintf "font contains no glyph for character f\n";
      | Some glyph ->
        let hmetrics = Stb_truetype.hmetrics font glyph in
        Printf.eprintf "Glyph f metric:\n- advance %s\n- left side bearing %s\n"
          (scaled_int hmetrics.Stb_truetype.advance_width scale)
          (scaled_int hmetrics.Stb_truetype.left_side_bearing scale);
        Printf.eprintf "Kerning for ff: %s\n"
          (scaled_int (Stb_truetype.kern_advance font glyph glyph) scale);
    end;
    (* Packing atlas *)
    let buffer = Bigarray.(Array1.create int8_unsigned c_layout (512 * 256)) in
    begin match Stb_truetype.pack_begin buffer
                  ~width:512 ~height:256 ~stride:512 ~padding:1 with
    | None -> Printf.eprintf "Internal error, could not initialize packer\n"
    | Some packer ->
      let range = [|{Stb_truetype. font_size = Stb_truetype.Size_of_M 20.;
                     first_codepoint = Char.code 'A';
                     count = Char.code 'z' - Char.code 'A' + 1}|] in
      let pack () =
        match Stb_truetype.pack_font_ranges packer font range with
        | Some _atlas -> ()
        | None -> Printf.eprintf "Not enough room for packing\n"
      in
      Printf.eprintf "Packing A-z at low quality (os = 1)\n";
      Stb_truetype.pack_set_oversampling packer ~h:1 ~v:1;
      pack ();

      Printf.eprintf "Packing A-z at high quality (os = 3)\n";
      Stb_truetype.pack_set_oversampling packer ~h:3 ~v:3;
      pack ();

      Printf.eprintf "Saving to tmp_%d.raw, use:\n  convert -depth 8 -size 256x256 gray:tmp_%d.raw tmp_%d.png\nto display.\n" idx idx idx;
      save_buffer (Printf.sprintf "tmp_%d.raw" idx) buffer
    end;
  in
  let dump_offset (idx : int) offset =
    match Stb_truetype.init buffer offset with
    | None -> Printf.eprintf "%%%% could not load font %d\n" idx
    | Some font ->
      Printf.eprintf "%%%% FONT %d\n" idx;
      dump_font idx font
  in
  List.iteri dump_offset offsets

let () =
  if Array.length Sys.argv > 1 then
    main Sys.argv.(1)
  else
    Printf.eprintf "Usage: %s <path-to-font.ttf>\n" Sys.argv.(0)
