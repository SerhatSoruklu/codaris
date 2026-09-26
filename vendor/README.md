# Vendored focused libraries

`nayuki-qrcodegen/` contains the C QR encoder from Project Nayuki QR Code
generator v1.8.0, pinned at commit
`720f62bddb7226106071d4728c292cb1df519ceb`. It is MIT licensed and supplies
the QR Model 2 matrix for credential URLs. Its checked-in C/H source is about
60 KiB. The API renders that matrix as SVG and vector PDF paths.

`libharu/` contains libHaru v2.4.6, pinned at commit
`3467749fd1c0ab6ca6ed424d053b1ea53c1bf67c`. It is under the ZLIB/LIBPNG
license and generates the two-page ID-1 PDF on the native C API. Only its
library source, headers, CMake files and license are retained. PNG decoding
support and examples are disabled; the project uses its raw RGB image API and
zlib.

Both libraries are compiled locally; no third-party runtime hosts or CDNs are
used. Code 128 bars use the standard Code 128B symbol table in CODARIS C code.
PNG export uses the browser's built-in SVG and Canvas APIs.
