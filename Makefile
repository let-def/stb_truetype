all:
	dune build --profile=release

test:
	dune test --profile=release

.PHONY: all test clean install reinstall uninstall

clean:
	dune clean

install: all
	dune install

uninstall:
	dune uninstall

reinstall:
	-$(MAKE) uninstall
	$(MAKE) install
