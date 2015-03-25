all: stb_truetype.cma stb_truetype.cmxa

ml_stb_truetype.o: ml_stb_truetype.c
	ocamlc -c -ccopt "-O3 -std=gnu99 -ffast-math" $<

dll_stb_truetype_stubs.so lib_stb_truetype_stubs.a: ml_stb_truetype.o
	ocamlmklib \
	    -o _stb_truetype_stubs $< \
	    -ccopt -O3 -ccopt -std=gnu99 -ccopt -ffast-math

stb_truetype.cmi: stb_truetype.mli
	ocamlc -c $<

stb_truetype.cmo: stb_truetype.ml stb_truetype.cmi
	ocamlc -c $<

stb_truetype.cma: stb_truetype.cmo dll_stb_truetype_stubs.so
	ocamlc -a -custom -o $@ $< \
	       -ccopt -L/usr/local/lib \
	       -dllib dll_stb_truetype_stubs.so \
	       -cclib -l_stb_truetype_stubs

stb_truetype.cmx: stb_truetype.ml stb_truetype.cmi
	ocamlopt -c $<

stb_truetype.cmxa stb_truetype.a: stb_truetype.cmx dll_stb_truetype_stubs.so
	ocamlopt -a -o $@ $< \
	      -cclib -l_stb_truetype_stubs \
	  		-ccopt -O3 -ccopt -std=gnu99 -ccopt -ffast-math

.PHONY: clean-doc clean clean-mlpp run-opt-demo test install

clean: clean-doc clean-mlpp
	rm -f *.[oa] *.so *.cm[ixoa] *.cmxa

DIST_FILES=              \
	stb_truetype.a            \
	stb_truetype.cmi          \
	stb_truetype.cmo          \
	stb_truetype.cma          \
	stb_truetype.cmx          \
	stb_truetype.cmxa         \
	lib_stb_truetype_stubs.a  \
	dll_stb_truetype_stubs.so

install: $(DIST_FILES) META
	ocamlfind install stb_truetype $^

uninstall:
	ocamlfind remove stb_truetype

reinstall:
	-$(MAKE) uninstall
	$(MAKE) install
