test_font: test_font.ml
	ocamlfind opt -package stb_truetype,unix,bigarray -linkpkg -o $@ $^

.PHONY: clean

clean:
	rm -f *.[oa] *.so *.cm[ixoa] *.cmxa test_font 
