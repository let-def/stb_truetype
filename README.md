Stb\_truetype is an OCaml binding to stb\_truetype from Sean Barrett, [Nothings](http://nothings.org/):

  stb\_truetype.h: public domain C truetype rasterization library 

The OCaml binding is released under CC-0 license.  It has no dependency beside
working OCaml and C compilers (stb\_truetype is self-contained).

```shell
$ make
$ make install
```

## CHANGELOG

Version 0.5, Sat Feb  3 07:05:53 CET 2018
  FFI code was wrong with OCaml < 4.05.0 due to missing macro!

Version 0.4, Wed Jan 17 20:43:52 JST 2018
  Change font representation to behave well with OCaml ad-hoc primitives.
  Support only OCaml >=4.02 because of `[@@noalloc]` (could be made optional?)

Version 0.3, Sun Nov 12 11:52:38 CET 2017
  Add glyph bluring primitive

Version 0.2, Sun Nov 27 19:59:41 CET 2016
  Update to stb\_truetype.h v1.12 and stb\_rect\_pack.h v0.10

Version 0.1, Fri Sep 18 20:53:03 CET 2015
  Initial release
