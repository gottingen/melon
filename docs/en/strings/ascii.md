# ascii

class ascii has some static method for char traits operation.
It has been designed to reduce computing, but using constant variable.
the interfaces of ascii have the same semantic  with std::isxxx.

## character_properties

a enum of character properties

enum | value
:--- | :---
eNone | 0x0
eControl | 0x0001
eSpace | 0x0002
ePunct | 0x0004
eDigit | 0x0008
eHexDigit | 0x0010
eAlpha | 0x0020
eLower | 0x0040
eUpper | 0x0080
eGraph | 0x0100
ePrint | 0x0200

