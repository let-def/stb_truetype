OCAMLC=ocamlc
OCAMLOPT=ocamlopt
OCAMLMKLIB=ocamlmklib

EXT_CONFIG=$(shell $(OCAMLC) -config | grep '^ext_')
EXT_DLL=$(shell printf "%s" "$(EXT_CONFIG)" | grep ext_dll | cut -f 2 -d ' ')
EXT_LIB=$(shell printf "%s" "$(EXT_CONFIG)" | grep ext_lib | cut -f 2 -d ' ')
EXT_OBJ=$(shell printf "%s" "$(EXT_CONFIG)" | grep ext_obj | cut -f 2 -d ' ')

CFLAGS=-O3 -ffast-math

all: stb_truetype.cma stb_truetype.cmxa

ml_stb_truetype$(EXT_OBJ): ml_stb_truetype.c
	$(OCAMLC) -c -ccopt "$(CFLAGS)" $<

dll_stb_truetype_stubs$(EXT_DLL) lib_stb_truetype_stubs$(EXT_LIB): ml_stb_truetype$(EXT_OBJ)
	$(OCAMLMKLIB) -o _stb_truetype_stubs $<

stb_truetype.cmi: stb_truetype.mli
	$(OCAMLC) -c $<

stb_truetype.cmo: stb_truetype.ml stb_truetype.cmi
	$(OCAMLC) -c $<

stb_truetype.cma: stb_truetype.cmo dll_stb_truetype_stubs$(EXT_DLL)
	$(OCAMLC) -a -custom -o $@ $< \
	       -dllib dll_stb_truetype_stubs$(EXT_DLL) \
	       -cclib -l_stb_truetype_stubs

stb_truetype.cmx: stb_truetype.ml stb_truetype.cmi
	$(OCAMLOPT) -c $<

stb_truetype.cmxa stb_truetype$(EXT_LIB): stb_truetype.cmx dll_stb_truetype_stubs$(EXT_DLL)
	$(OCAMLOPT) -a -o $@ $< \
	       -cclib -l_stb_truetype_stubs

.PHONY: clean install reinstall uninstall

clean:
	rm -f *$(EXT_LIB) *$(EXT_OBJ) *$(EXT_DLL) *.cm[ixoa] *.cmxa

DIST_FILES=                 \
	stb_truetype$(EXT_LIB)    \
	stb_truetype.cmi          \
	stb_truetype.cmo          \
	stb_truetype.cma          \
	stb_truetype.cmx          \
	stb_truetype.cmxa         \
	stb_truetype.ml           \
	stb_truetype.mli          \
	lib_stb_truetype_stubs$(EXT_LIB)  \
	dll_stb_truetype_stubs$(EXT_DLL)

install: $(DIST_FILES) META
	ocamlfind install stb_truetype $^

uninstall:
	ocamlfind remove stb_truetype

reinstall:
	-$(MAKE) uninstall
	$(MAKE) install
