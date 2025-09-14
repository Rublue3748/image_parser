# Image Parser
This project consists of a library to take in image files and transform them into raw color values for use in other projects.
Compressed images such as PNG are parsed into their 32-bit RGBA uncompressed representations
## Supported File Types
- PNG
## Sources
### PNG
PNG decoding was implemented based on the following specifications:
- [PNG](https://www.w3.org/TR/2003/REC-PNG-20031110/)
- [DEFLATE](https://datatracker.ietf.org/doc/html/rfc1951) (for PNG data decoding)
