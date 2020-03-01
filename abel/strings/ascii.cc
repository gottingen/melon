
#include <abel/strings/ascii.h>

namespace abel {

    const character_properties kCtrlOrSpace = character_properties::eControl | character_properties::eSpace;

    const character_properties kPPG = character_properties::ePunct |
                                      character_properties::ePrint | character_properties::eGraph;

    const character_properties kDHGP = character_properties::eHexDigit |
                                       character_properties::ePrint | character_properties::eGraph |
                                       character_properties::eDigit;

    const character_properties kHAUGP = character_properties::eHexDigit |
                                        character_properties::eAlpha |
                                        character_properties::eUpper |
                                        character_properties::eGraph |
                                        character_properties::ePrint;

    const character_properties kHALGP = character_properties::eHexDigit |
                                        character_properties::eAlpha |
                                        character_properties::eLower |
                                        character_properties::eGraph |
                                        character_properties::ePrint;

    const character_properties kAUGP = character_properties::eAlpha |
                                       character_properties::eUpper |
                                       character_properties::eGraph |
                                       character_properties::ePrint;

    const character_properties kALGP = character_properties::eAlpha |
                                       character_properties::eLower |
                                       character_properties::eGraph |
                                       character_properties::ePrint;

    const character_properties ascii::kCharacterProperties[128] = {
            /* 00 . */ character_properties::eControl,
            /* 01 . */ character_properties::eControl,
            /* 02 . */ character_properties::eControl,
            /* 03 . */ character_properties::eControl,
            /* 04 . */ character_properties::eControl,
            /* 05 . */ character_properties::eControl,
            /* 06 . */ character_properties::eControl,
            /* 07 . */ character_properties::eControl,
            /* 08 . */ character_properties::eControl,
            /* 09 . */ kCtrlOrSpace,
            /* 0a . */ kCtrlOrSpace,
            /* 0b . */ kCtrlOrSpace,
            /* 0c . */ kCtrlOrSpace,
            /* 0d . */ kCtrlOrSpace,
            /* 0e . */ character_properties::eControl,
            /* 0f . */ character_properties::eControl,
            /* 10 . */ character_properties::eControl,
            /* 11 . */ character_properties::eControl,
            /* 12 . */ character_properties::eControl,
            /* 13 . */ character_properties::eControl,
            /* 14 . */ character_properties::eControl,
            /* 15 . */ character_properties::eControl,
            /* 16 . */ character_properties::eControl,
            /* 17 . */ character_properties::eControl,
            /* 18 . */ character_properties::eControl,
            /* 19 . */ character_properties::eControl,
            /* 1a . */ character_properties::eControl,
            /* 1b . */ character_properties::eControl,
            /* 1c . */ character_properties::eControl,
            /* 1d . */ character_properties::eControl,
            /* 1e . */ character_properties::eControl,
            /* 1f . */ character_properties::eControl,
            /* 20   */ character_properties::eSpace | character_properties::ePrint,
            /* 21 ! */ kPPG,
            /* 22 " */ kPPG,
            /* 23 # */ kPPG,
            /* 24 $ */ kPPG,
            /* 25 % */ kPPG,
            /* 26 & */ kPPG,
            /* 27 ' */ kPPG,
            /* 28 ( */ kPPG,
            /* 29 ) */ kPPG,
            /* 2a * */ kPPG,
            /* 2b + */ kPPG,
            /* 2c , */ kPPG,
            /* 2d - */ kPPG,
            /* 2e . */ kPPG,
            /* 2f / */ kPPG,
            /* 30 0 */ kDHGP,
            /* 31 1 */ kDHGP,
            /* 32 2 */ kDHGP,
            /* 33 3 */ kDHGP,
            /* 34 4 */ kDHGP,
            /* 35 5 */ kDHGP,
            /* 36 6 */ kDHGP,
            /* 37 7 */ kDHGP,
            /* 38 8 */ kDHGP,
            /* 39 9 */ kDHGP,
            /* 3a : */ kPPG,
            /* 3b ; */ kPPG,
            /* 3c < */ kPPG,
            /* 3d = */ kPPG,
            /* 3e > */ kPPG,
            /* 3f ? */ kPPG,
            /* 40 @ */ kPPG,
            /* 41 A */ kHAUGP,
            /* 42 B */ kHAUGP,
            /* 43 C */ kHAUGP,
            /* 44 D */ kHAUGP,
            /* 45 E */ kHAUGP,
            /* 46 F */ kHAUGP,
            /* 47 G */ kAUGP,
            /* 48 H */ kAUGP,
            /* 49 I */ kAUGP,
            /* 4a J */ kAUGP,
            /* 4b K */ kAUGP,
            /* 4c L */ kAUGP,
            /* 4d M */ kAUGP,
            /* 4e N */ kAUGP,
            /* 4f O */ kAUGP,
            /* 50 P */ kAUGP,
            /* 51 Q */ kAUGP,
            /* 52 R */ kAUGP,
            /* 53 S */ kAUGP,
            /* 54 T */ kAUGP,
            /* 55 U */ kAUGP,
            /* 56 V */ kAUGP,
            /* 57 W */ kAUGP,
            /* 58 X */ kAUGP,
            /* 59 Y */ kAUGP,
            /* 5a Z */ kAUGP,
            /* 5b [ */ kPPG,
            /* 5c \ */ kPPG,
            /* 5d ] */ kPPG,
            /* 5e ^ */ kPPG,
            /* 5f _ */ kPPG,
            /* 60 ` */ kPPG,
            /* 61 a */ kHALGP,
            /* 62 b */ kHALGP,
            /* 63 c */ kHALGP,
            /* 64 d */ kHALGP,
            /* 65 e */ kHALGP,
            /* 66 f */ kHALGP,
            /* 67 g */ kALGP,
            /* 68 h */ kALGP,
            /* 69 i */ kALGP,
            /* 6a j */ kALGP,
            /* 6b k */ kALGP,
            /* 6c l */ kALGP,
            /* 6d m */ kALGP,
            /* 6e n */ kALGP,
            /* 6f o */ kALGP,
            /* 70 p */ kALGP,
            /* 71 q */ kALGP,
            /* 72 r */ kALGP,
            /* 73 s */ kALGP,
            /* 74 t */ kALGP,
            /* 75 u */ kALGP,
            /* 76 v */ kALGP,
            /* 77 w */ kALGP,
            /* 78 x */ kALGP,
            /* 79 y */ kALGP,
            /* 7a z */ kALGP,
            /* 7b { */ kPPG,
            /* 7c | */ kPPG,
            /* 7d } */ kPPG,
            /* 7e ~ */ kPPG,
            /* 7f . */ character_properties::eControl

    };


    const char ascii::kToLower[256] = {
            '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
            '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
            '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
            '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
            '\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27',
            '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
            '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37',
            '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e', '\x3f',
            '\x40', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
            'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
            'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
            'x', 'y', 'z', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
            '\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67',
            '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d', '\x6e', '\x6f',
            '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',
            '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f',
            '\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87',
            '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',
            '\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97',
            '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f',
            '\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7',
            '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf',
            '\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7',
            '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',
            '\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7',
            '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd', '\xce', '\xcf',
            '\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7',
            '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf',
            '\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7',
            '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',
            '\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7',
            '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd', '\xfe', '\xff',
    };

    const char ascii::kToUpper[256] = {
            '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
            '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
            '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
            '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
            '\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27',
            '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
            '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37',
            '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e', '\x3f',
            '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
            '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f',
            '\x50', '\x51', '\x52', '\x53', '\x54', '\x55', '\x56', '\x57',
            '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
            '\x60', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
            'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
            'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
            'X', 'Y', 'Z', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f',
            '\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87',
            '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',
            '\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97',
            '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f',
            '\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7',
            '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf',
            '\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7',
            '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',
            '\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7',
            '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd', '\xce', '\xcf',
            '\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7',
            '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf',
            '\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7',
            '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',
            '\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7',
            '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd', '\xfe', '\xff',
    };

} //namespace abel