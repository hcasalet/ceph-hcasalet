/*****************************************************************************
 Copyright (c) 2017-2018, Intel Corporation

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <intel-ipsec-mb.h>
#include "gcm_ctr_vectors_test.h"
#include "utils.h"

int ccm_test(const enum arch_type arch, struct MB_MGR *mb_mgr);

/*
 * Test vectors from https://tools.ietf.org/html/rfc3610
 */

/*
 *  =============== Packet Vector #1 ==================
 *  AES Key =  C0 C1 C2 C3  C4 C5 C6 C7  C8 C9 CA CB  CC CD CE CF
 *  Nonce =    00 00 00 03  02 01 00 A0  A1 A2 A3 A4  A5
 *  Total packet length = 31. [Input with 8 cleartext header octets]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F
 *             10 11 12 13  14 15 16 17  18 19 1A 1B  1C 1D 1E
 *  CBC IV in: 59 00 00 00  03 02 01 00  A0 A1 A2 A3  A4 A5 00 17
 *  CBC IV out:EB 9D 55 47  73 09 55 AB  23 1E 0A 2D  FE 4B 90 D6
 *  After xor: EB 95 55 46  71 0A 51 AE  25 19 0A 2D  FE 4B 90 D6   [hdr]
 *  After AES: CD B6 41 1E  3C DC 9B 4F  5D 92 58 B6  9E E7 F0 91
 *  After xor: C5 BF 4B 15  30 D1 95 40  4D 83 4A A5  8A F2 E6 86   [msg]
 *  After AES: 9C 38 40 5E  A0 3C 1B C9  04 B5 8B 40  C7 6C A2 EB
 *  After xor: 84 21 5A 45  BC 21 05 C9  04 B5 8B 40  C7 6C A2 EB   [msg]
 *  After AES: 2D C6 97 E4  11 CA 83 A8  60 C2 C4 06  CC AA 54 2F
 *  CBC-MAC  : 2D C6 97 E4  11 CA 83 A8
 *  CTR Start: 01 00 00 00  03 02 01 00  A0 A1 A2 A3  A4 A5 00 01
 *  CTR[0001]: 50 85 9D 91  6D CB 6D DD  E0 77 C2 D1  D4 EC 9F 97
 *  CTR[0002]: 75 46 71 7A  C6 DE 9A FF  64 0C 9C 06  DE 6D 0D 8F
 *  CTR[MAC ]: 3A 2E 46 C8  EC 33 A5 48
 *  Total packet length = 39. [Authenticated and Encrypted Output]
 *             00 01 02 03  04 05 06 07  58 8C 97 9A  61 C6 63 D2
 *             F0 66 D0 C2  C0 F9 89 80  6D 5F 6B 61  DA C3 84 17
 *             E8 D1 2C FD  F9 26 E0
 */
static const uint8_t keys_01[] = {
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
        0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
};
static const uint8_t nonce_01[] = {
        0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0xA0,
        0xA1, 0xA2, 0xA3, 0xA4, 0xA5
};
static const uint8_t packet_in_01[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E
};
static const uint8_t packet_out_01[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x58, 0x8C, 0x97, 0x9A, 0x61, 0xC6, 0x63, 0xD2,
        0xF0, 0x66, 0xD0, 0xC2, 0xC0, 0xF9, 0x89, 0x80,
        0x6D, 0x5F, 0x6B, 0x61, 0xDA, 0xC3, 0x84, 0x17,
        0xE8, 0xD1, 0x2C, 0xFD, 0xF9, 0x26, 0xE0
};
#define clear_len_01 8
#define auth_len_01 8

/*
 *  =============== Packet Vector #2 ==================
 *  AES Key =  C0 C1 C2 C3  C4 C5 C6 C7  C8 C9 CA CB  CC CD CE CF
 *  Nonce =    00 00 00 04  03 02 01 A0  A1 A2 A3 A4  A5
 *  Total packet length = 32. [Input with 8 cleartext header octets]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F
 *             10 11 12 13  14 15 16 17  18 19 1A 1B  1C 1D 1E 1F
 *  CBC IV in: 59 00 00 00  04 03 02 01  A0 A1 A2 A3  A4 A5 00 18
 *  CBC IV out:F0 C2 54 D3  CA 03 E2 39  70 BD 24 A8  4C 39 9E 77
 *  After xor: F0 CA 54 D2  C8 00 E6 3C  76 BA 24 A8  4C 39 9E 77   [hdr]
 *  After AES: 48 DE 8B 86  28 EA 4A 40  00 AA 42 C2  95 BF 4A 8C
 *  After xor: 40 D7 81 8D  24 E7 44 4F  10 BB 50 D1  81 AA 5C 9B   [msg]
 *  After AES: 0F 89 FF BC  A6 2B C2 4F  13 21 5F 16  87 96 AA 33
 *  After xor: 17 90 E5 A7  BA 36 DC 50  13 21 5F 16  87 96 AA 33   [msg]
 *  After AES: F7 B9 05 6A  86 92 6C F3  FB 16 3D C4  99 EF AA 11
 *  CBC-MAC  : F7 B9 05 6A  86 92 6C F3
 *  CTR Start: 01 00 00 00  04 03 02 01  A0 A1 A2 A3  A4 A5 00 01
 *  CTR[0001]: 7A C0 10 3D  ED 38 F6 C0  39 0D BA 87  1C 49 91 F4
 *  CTR[0002]: D4 0C DE 22  D5 F9 24 24  F7 BE 9A 56  9D A7 9F 51
 *  CTR[MAC ]: 57 28 D0 04  96 D2 65 E5
 *  Total packet length = 40. [Authenticated and Encrypted Output]
 *             00 01 02 03  04 05 06 07  72 C9 1A 36  E1 35 F8 CF
 *             29 1C A8 94  08 5C 87 E3  CC 15 C4 39  C9 E4 3A 3B
 *             A0 91 D5 6E  10 40 09 16
 */
static const uint8_t keys_02[] = {
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
        0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
};
static const uint8_t nonce_02[] = {
        0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01, 0xA0,
        0xA1, 0xA2, 0xA3, 0xA4, 0xA5
};
static const uint8_t packet_in_02[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};
static const uint8_t packet_out_02[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x72, 0xC9, 0x1A, 0x36, 0xE1, 0x35, 0xF8, 0xCF,
        0x29, 0x1C, 0xA8, 0x94, 0x08, 0x5C, 0x87, 0xE3,
        0xCC, 0x15, 0xC4, 0x39, 0xC9, 0xE4, 0x3A, 0x3B,
        0xA0, 0x91, 0xD5, 0x6E, 0x10, 0x40, 0x09, 0x16
};
#define clear_len_02 8
#define auth_len_02 8

/*
 *  =============== Packet Vector #3 ==================
 *  AES Key =  C0 C1 C2 C3  C4 C5 C6 C7  C8 C9 CA CB  CC CD CE CF
 *  Nonce =    00 00 00 05  04 03 02 A0  A1 A2 A3 A4  A5
 *  Total packet length = 33. [Input with 8 cleartext header octets]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F
 *             10 11 12 13  14 15 16 17  18 19 1A 1B  1C 1D 1E 1F
 *             20
 *  CBC IV in: 59 00 00 00  05 04 03 02  A0 A1 A2 A3  A4 A5 00 19
 *  CBC IV out:6F 8A 12 F7  BF 8D 4D C5  A1 19 6E 95  DF F0 B4 27
 *  After xor: 6F 82 12 F6  BD 8E 49 C0  A7 1E 6E 95  DF F0 B4 27   [hdr]
 *  After AES: 37 E9 B7 8C  C2 20 17 E7  33 80 43 0C  BE F4 28 24
 *  After xor: 3F E0 BD 87  CE 2D 19 E8  23 91 51 1F  AA E1 3E 33   [msg]
 *  After AES: 90 CA 05 13  9F 4D 4E CF  22 6F E9 81  C5 9E 2D 40
 *  After xor: 88 D3 1F 08  83 50 50 D0  02 6F E9 81  C5 9E 2D 40   [msg]
 *  After AES: 73 B4 67 75  C0 26 DE AA  41 03 97 D6  70 FE 5F B0
 *  CBC-MAC  : 73 B4 67 75  C0 26 DE AA
 *  CTR Start: 01 00 00 00  05 04 03 02  A0 A1 A2 A3  A4 A5 00 01
 *  CTR[0001]: 59 B8 EF FF  46 14 73 12  B4 7A 1D 9D  39 3D 3C FF
 *  CTR[0002]: 69 F1 22 A0  78 C7 9B 89  77 89 4C 99  97 5C 23 78
 *  CTR[MAC ]: 39 6E C0 1A  7D B9 6E 6F
 *  Total packet length = 41. [Authenticated and Encrypted Output]
 *             00 01 02 03  04 05 06 07  51 B1 E5 F4  4A 19 7D 1D
 *             A4 6B 0F 8E  2D 28 2A E8  71 E8 38 BB  64 DA 85 96
 *             57 4A DA A7  6F BD 9F B0  C5
 */
static const uint8_t keys_03[] = {
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
        0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
};
static const uint8_t nonce_03[] = {
        0x00, 0x00, 0x00, 0x05, 0x04, 0x03, 0x02, 0xA0,
        0xA1, 0xA2, 0xA3, 0xA4, 0xA5
};
static const uint8_t packet_in_03[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0x20
};
static const uint8_t packet_out_03[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x51, 0xB1, 0xE5, 0xF4, 0x4A, 0x19, 0x7D, 0x1D,
        0xA4, 0x6B, 0x0F, 0x8E, 0x2D, 0x28, 0x2A, 0xE8,
        0x71, 0xE8, 0x38, 0xBB, 0x64, 0xDA, 0x85, 0x96,
        0x57, 0x4A, 0xDA, 0xA7, 0x6F, 0xBD, 0x9F, 0xB0,
        0xC5
};
#define clear_len_03 8
#define auth_len_03 8

/*
 *  =============== Packet Vector #4 ==================
 *  AES Key =  C0 C1 C2 C3  C4 C5 C6 C7  C8 C9 CA CB  CC CD CE CF
 *  Nonce =    00 00 00 06  05 04 03 A0  A1 A2 A3 A4  A5
 *  Total packet length = 31. [Input with 12 cleartext header octets]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F
 *             10 11 12 13  14 15 16 17  18 19 1A 1B  1C 1D 1E
 *  CBC IV in: 59 00 00 00  06 05 04 03  A0 A1 A2 A3  A4 A5 00 13
 *  CBC IV out:06 65 2C 60  0E F5 89 63  CA C3 25 A9  CD 3E 2B E1
 *  After xor: 06 69 2C 61  0C F6 8D 66  CC C4 2D A0  C7 35 2B E1   [hdr]
 *  After AES: A0 75 09 AC  15 C2 58 86  04 2F 80 60  54 FE A6 86
 *  After xor: AC 78 07 A3  05 D3 4A 95  10 3A 96 77  4C E7 BC 9D   [msg]
 *  After AES: 64 4C 09 90  D9 1B 83 E9  AB 4B 8E ED  06 6F F5 BF
 *  After xor: 78 51 17 90  D9 1B 83 E9  AB 4B 8E ED  06 6F F5 BF   [msg]
 *  After AES: 4B 4F 4B 39  B5 93 E6 BF  B0 B2 C2 B7  0F 29 CD 7A
 *  CBC-MAC  : 4B 4F 4B 39  B5 93 E6 BF
 *  CTR Start: 01 00 00 00  06 05 04 03  A0 A1 A2 A3  A4 A5 00 01
 *  CTR[0001]: AE 81 66 6A  83 8B 88 6A  EE BF 4A 5B  32 84 50 8A
 *  CTR[0002]: D1 B1 92 06  AC 93 9E 2F  B6 DD CE 10  A7 74 FD 8D
 *  CTR[MAC ]: DD 87 2A 80  7C 75 F8 4E
 *  Total packet length = 39. [Authenticated and Encrypted Output]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  A2 8C 68 65
 *             93 9A 9A 79  FA AA 5C 4C  2A 9D 4A 91  CD AC 8C 96
 *             C8 61 B9 C9  E6 1E F1
 */
static const uint8_t keys_04[] = {
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
        0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
};
static const uint8_t nonce_04[] = {
        0x00, 0x00, 0x00, 0x06, 0x05, 0x04, 0x03, 0xA0,
        0xA1, 0xA2, 0xA3, 0xA4, 0xA5
};
static const uint8_t packet_in_04[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E
};
static const uint8_t packet_out_04[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0xA2, 0x8C, 0x68, 0x65,
        0x93, 0x9A, 0x9A, 0x79, 0xFA, 0xAA, 0x5C, 0x4C,
        0x2A, 0x9D, 0x4A, 0x91, 0xCD, 0xAC, 0x8C, 0x96,
        0xC8, 0x61, 0xB9, 0xC9, 0xE6, 0x1E, 0xF1
};
#define clear_len_04 12
#define auth_len_04 8

/*
 *  =============== Packet Vector #5 ==================
 *  AES Key =  C0 C1 C2 C3  C4 C5 C6 C7  C8 C9 CA CB  CC CD CE CF
 *  Nonce =    00 00 00 07  06 05 04 A0  A1 A2 A3 A4  A5
 *  Total packet length = 32. [Input with 12 cleartext header octets]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F
 *             10 11 12 13  14 15 16 17  18 19 1A 1B  1C 1D 1E 1F
 *  CBC IV in: 59 00 00 00  07 06 05 04  A0 A1 A2 A3  A4 A5 00 14
 *  CBC IV out:00 4C 50 95  45 80 3C 48  51 CD E1 3B  56 C8 9A 85
 *  After xor: 00 40 50 94  47 83 38 4D  57 CA E9 32  5C C3 9A 85   [hdr]
 *  After AES: E2 B8 F7 CE  49 B2 21 72  84 A8 EA 84  FA AD 67 5C
 *  After xor: EE B5 F9 C1  59 A3 33 61  90 BD FC 93  E2 B4 7D 47   [msg]
 *  After AES: 3E FB 36 72  25 DB 11 01  D3 C2 2F 0E  CA FF 44 F3
 *  After xor: 22 E6 28 6D  25 DB 11 01  D3 C2 2F 0E  CA FF 44 F3   [msg]
 *  After AES: 48 B9 E8 82  55 05 4A B5  49 0A 95 F9  34 9B 4B 5E
 *  CBC-MAC  : 48 B9 E8 82  55 05 4A B5
 *  CTR Start: 01 00 00 00  07 06 05 04  A0 A1 A2 A3  A4 A5 00 01
 *  CTR[0001]: D0 FC F5 74  4D 8F 31 E8  89 5B 05 05  4B 7C 90 C3
 *  CTR[0002]: 72 A0 D4 21  9F 0D E1 D4  04 83 BC 2D  3D 0C FC 2A
 *  CTR[MAC ]: 19 51 D7 85  28 99 67 26
 *  Total packet length = 40. [Authenticated and Encrypted Output]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  DC F1 FB 7B
 *             5D 9E 23 FB  9D 4E 13 12  53 65 8A D8  6E BD CA 3E
 *             51 E8 3F 07  7D 9C 2D 93
 */
static const uint8_t keys_05[] = {
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
        0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
};
static const uint8_t nonce_05[] = {
        0x00, 0x00, 0x00, 0x07, 0x06, 0x05, 0x04, 0xA0,
        0xA1, 0xA2, 0xA3, 0xA4, 0xA5
};
static const uint8_t packet_in_05[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};
static const uint8_t packet_out_05[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0xDC, 0xF1, 0xFB, 0x7B,
        0x5D, 0x9E, 0x23, 0xFB, 0x9D, 0x4E, 0x13, 0x12,
        0x53, 0x65, 0x8A, 0xD8, 0x6E, 0xBD, 0xCA, 0x3E,
        0x51, 0xE8, 0x3F, 0x07, 0x7D, 0x9C, 0x2D, 0x93
};
#define clear_len_05 12
#define auth_len_05 8

/*
 *  =============== Packet Vector #6 ==================
 *  AES Key =  C0 C1 C2 C3  C4 C5 C6 C7  C8 C9 CA CB  CC CD CE CF
 *  Nonce =    00 00 00 08  07 06 05 A0  A1 A2 A3 A4  A5
 *  Total packet length = 33. [Input with 12 cleartext header octets]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F
 *             10 11 12 13  14 15 16 17  18 19 1A 1B  1C 1D 1E 1F
 *             20
 *  CBC IV in: 59 00 00 00  08 07 06 05  A0 A1 A2 A3  A4 A5 00 15
 *  CBC IV out:04 72 DA 4C  6F F6 0A 63  06 52 1A 06  04 80 CD E5
 *  After xor: 04 7E DA 4D  6D F5 0E 66  00 55 12 0F  0E 8B CD E5   [hdr]
 *  After AES: 64 4C 36 A5  A2 27 37 62  0B 89 F1 D7  BF F2 73 D4
 *  After xor: 68 41 38 AA  B2 36 25 71  1F 9C E7 C0  A7 EB 69 CF   [msg]
 *  After AES: 41 E1 19 CD  19 24 CE 77  F1 2F A6 60  C1 6E BB 4E
 *  After xor: 5D FC 07 D2  39 24 CE 77  F1 2F A6 60  C1 6E BB 4E   [msg]
 *  After AES: A5 27 D8 15  6A C3 59 BF  1C B8 86 E6  2F 29 91 29
 *  CBC-MAC  : A5 27 D8 15  6A C3 59 BF
 *  CTR Start: 01 00 00 00  08 07 06 05  A0 A1 A2 A3  A4 A5 00 01
 *  CTR[0001]: 63 CC BE 1E  E0 17 44 98  45 64 B2 3A  8D 24 5C 80
 *  CTR[0002]: 39 6D BA A2  A7 D2 CB D4  B5 E1 7C 10  79 45 BB C0
 *  CTR[MAC ]: E5 7D DC 56  C6 52 92 2B
 *  Total packet length = 41. [Authenticated and Encrypted Output]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  6F C1 B0 11
 *             F0 06 56 8B  51 71 A4 2D  95 3D 46 9B  25 70 A4 BD
 *             87 40 5A 04  43 AC 91 CB  94
 */
static const uint8_t keys_06[] = {
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
        0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
};
static const uint8_t nonce_06[] = {
        0x00, 0x00, 0x00, 0x08, 0x07, 0x06, 0x05, 0xA0,
        0xA1, 0xA2, 0xA3, 0xA4, 0xA5
};
static const uint8_t packet_in_06[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0x20
};
static const uint8_t packet_out_06[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x6F, 0xC1, 0xB0, 0x11,
        0xF0, 0x06, 0x56, 0x8B, 0x51, 0x71, 0xA4, 0x2D,
        0x95, 0x3D, 0x46, 0x9B, 0x25, 0x70, 0xA4, 0xBD,
        0x87, 0x40, 0x5A, 0x04, 0x43, 0xAC, 0x91, 0xCB,
        0x94
};
#define clear_len_06 12
#define auth_len_06 8

/*
 *  =============== Packet Vector #7 ==================
 *  AES Key =  C0 C1 C2 C3  C4 C5 C6 C7  C8 C9 CA CB  CC CD CE CF
 *  Nonce =    00 00 00 09  08 07 06 A0  A1 A2 A3 A4  A5
 *  Total packet length = 31. [Input with 8 cleartext header octets]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F
 *             10 11 12 13  14 15 16 17  18 19 1A 1B  1C 1D 1E
 *  CBC IV in: 61 00 00 00  09 08 07 06  A0 A1 A2 A3  A4 A5 00 17
 *  CBC IV out:60 06 C5 72  DA 23 9C BF  A0 5B 0A DE  D2 CD A8 1E
 *  After xor: 60 0E C5 73  D8 20 98 BA  A6 5C 0A DE  D2 CD A8 1E   [hdr]
 *  After AES: 41 7D E2 AE  94 E2 EA D9  00 FC 44 FC  D0 69 52 27
 *  After xor: 49 74 E8 A5  98 EF E4 D6  10 ED 56 EF  C4 7C 44 30   [msg]
 *  After AES: 2A 6C 42 CA  49 D7 C7 01  C5 7D 59 FF  87 16 49 0E
 *  After xor: 32 75 58 D1  55 CA D9 01  C5 7D 59 FF  87 16 49 0E   [msg]
 *  After AES: 89 8B D6 45  4E 27 20 BB  D2 7E F3 15  7A 7C 90 B2
 *  CBC-MAC  : 89 8B D6 45  4E 27 20 BB  D2 7E
 *  CTR Start: 01 00 00 00  09 08 07 06  A0 A1 A2 A3  A4 A5 00 01
 *  CTR[0001]: 09 3C DB B9  C5 52 4F DA  C1 C5 EC D2  91 C4 70 AF
 *  CTR[0002]: 11 57 83 86  E2 C4 72 B4  8E CC 8A AD  AB 77 6F CB
 *  CTR[MAC ]: 8D 07 80 25  62 B0 8C 00  A6 EE
 *  Total packet length = 41. [Authenticated and Encrypted Output]
 *             00 01 02 03  04 05 06 07  01 35 D1 B2  C9 5F 41 D5
 *             D1 D4 FE C1  85 D1 66 B8  09 4E 99 9D  FE D9 6C 04
 *             8C 56 60 2C  97 AC BB 74  90
 */
static const uint8_t keys_07[] = {
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
        0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
};
static const uint8_t nonce_07[] = {
        0x00, 0x00, 0x00, 0x09, 0x08, 0x07, 0x06, 0xA0,
        0xA1, 0xA2, 0xA3, 0xA4, 0xA5
};
static const uint8_t packet_in_07[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E
};
static const uint8_t packet_out_07[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x01, 0x35, 0xD1, 0xB2, 0xC9, 0x5F, 0x41, 0xD5,
        0xD1, 0xD4, 0xFE, 0xC1, 0x85, 0xD1, 0x66, 0xB8,
        0x09, 0x4E, 0x99, 0x9D, 0xFE, 0xD9, 0x6C, 0x04,
        0x8C, 0x56, 0x60, 0x2C, 0x97, 0xAC, 0xBB, 0x74,
        0x90
};
#define clear_len_07 8
#define auth_len_07 10

/*
 *  =============== Packet Vector #8 ==================
 *  AES Key =  C0 C1 C2 C3  C4 C5 C6 C7  C8 C9 CA CB  CC CD CE CF
 *  Nonce =    00 00 00 0A  09 08 07 A0  A1 A2 A3 A4  A5
 *  Total packet length = 32. [Input with 8 cleartext header octets]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F
 *             10 11 12 13  14 15 16 17  18 19 1A 1B  1C 1D 1E 1F
 *  CBC IV in: 61 00 00 00  0A 09 08 07  A0 A1 A2 A3  A4 A5 00 18
 *  CBC IV out:63 A3 FA E4  6C 79 F3 FA  78 38 B8 A2  80 36 B6 0B
 *  After xor: 63 AB FA E5  6E 7A F7 FF  7E 3F B8 A2  80 36 B6 0B   [hdr]
 *  After AES: 1C 99 1A 3D  B7 60 79 27  34 40 79 1F  AD 8B 5B 02
 *  After xor: 14 90 10 36  BB 6D 77 28  24 51 6B 0C  B9 9E 4D 15   [msg]
 *  After AES: 14 19 E8 E8  CB BE 75 58  E1 E3 BE 4B  6C 9F 82 E3
 *  After xor: 0C 00 F2 F3  D7 A3 6B 47  E1 E3 BE 4B  6C 9F 82 E3   [msg]
 *  After AES: E0 16 E8 1C  7F 7B 8A 38  A5 38 F2 CB  5B B6 C1 F2
 *  CBC-MAC  : E0 16 E8 1C  7F 7B 8A 38  A5 38
 *  CTR Start: 01 00 00 00  0A 09 08 07  A0 A1 A2 A3  A4 A5 00 01
 *  CTR[0001]: 73 7C 33 91  CC 8E 13 DD  E0 AA C5 4B  6D B7 EB 98
 *  CTR[0002]: 74 B7 71 77  C5 AA C5 3B  04 A4 F8 70  8E 92 EB 2B
 *  CTR[MAC ]: 21 6D AC 2F  8B 4F 1C 07  91 8C
 *  Total packet length = 42. [Authenticated and Encrypted Output]
 *             00 01 02 03  04 05 06 07  7B 75 39 9A  C0 83 1D D2
 *             F0 BB D7 58  79 A2 FD 8F  6C AE 6B 6C  D9 B7 DB 24
 *             C1 7B 44 33  F4 34 96 3F  34 B4
 */
static const uint8_t keys_08[] = {
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
        0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
};
static const uint8_t nonce_08[] = {
        0x00, 0x00, 0x00, 0x0a, 0x09, 0x08, 0x07, 0xA0,
        0xA1, 0xA2, 0xA3, 0xA4, 0xA5
};
static const uint8_t packet_in_08[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};
static const uint8_t packet_out_08[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x7B, 0x75, 0x39, 0x9A, 0xC0, 0x83, 0x1D, 0xD2,
        0xF0, 0xBB, 0xD7, 0x58, 0x79, 0xA2, 0xFD, 0x8F,
        0x6C, 0xAE, 0x6B, 0x6C, 0xD9, 0xB7, 0xDB, 0x24,
        0xC1, 0x7B, 0x44, 0x33, 0xF4, 0x34, 0x96, 0x3F,
        0x34, 0xB4
};
#define clear_len_08 8
#define auth_len_08 10

/*
 *  =============== Packet Vector #9 ==================
 *  AES Key =  C0 C1 C2 C3  C4 C5 C6 C7  C8 C9 CA CB  CC CD CE CF
 *  Nonce =    00 00 00 0B  0A 09 08 A0  A1 A2 A3 A4  A5
 *  Total packet length = 33. [Input with 8 cleartext header octets]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F
 *             10 11 12 13  14 15 16 17  18 19 1A 1B  1C 1D 1E 1F
 *             20
 *  CBC IV in: 61 00 00 00  0B 0A 09 08  A0 A1 A2 A3  A4 A5 00 19
 *  CBC IV out:4F 2C 86 11  1E 08 2A DD  6B 44 21 3A  B5 13 13 16
 *  After xor: 4F 24 86 10  1C 0B 2E D8  6D 43 21 3A  B5 13 13 16   [hdr]
 *  After AES: F6 EC 56 87  3C 57 12 DC  9C C5 3C A8  D4 D1 ED 0A
 *  After xor: FE E5 5C 8C  30 5A 1C D3  8C D4 2E BB  C0 C4 FB 1D   [msg]
 *  After AES: 17 C1 80 A5  31 53 D4 C3  03 85 0C 95  65 80 34 52
 *  After xor: 0F D8 9A BE  2D 4E CA DC  23 85 0C 95  65 80 34 52   [msg]
 *  After AES: 46 A1 F6 E2  B1 6E 75 F8  1C F5 6B 1A  80 04 44 1B
 *  CBC-MAC  : 46 A1 F6 E2  B1 6E 75 F8  1C F5
 *  CTR Start: 01 00 00 00  0B 0A 09 08  A0 A1 A2 A3  A4 A5 00 01
 *  CTR[0001]: 8A 5A 10 6B  C0 29 9A 55  5B 93 6B 0B  0E A0 DE 5A
 *  CTR[0002]: EA 05 FD E2  AB 22 5C FE  B7 73 12 CB  88 D9 A5 4A
 *  CTR[MAC ]: AC 3D F1 07  DA 30 C4 86  43 BB
 *  Total packet length = 43. [Authenticated and Encrypted Output]
 *             00 01 02 03  04 05 06 07  82 53 1A 60  CC 24 94 5A
 *             4B 82 79 18  1A B5 C8 4D  F2 1C E7 F9  B7 3F 42 E1
 *             97 EA 9C 07  E5 6B 5E B1  7E 5F 4E
 */
static const uint8_t keys_09[] = {
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
        0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
};
static const uint8_t nonce_09[] = {
        0x00, 0x00, 0x00, 0x0b, 0x0a, 0x09, 0x08, 0xA0,
        0xA1, 0xA2, 0xA3, 0xA4, 0xA5
};
static const uint8_t packet_in_09[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0x20
};
static const uint8_t packet_out_09[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x82, 0x53, 0x1A, 0x60, 0xCC, 0x24, 0x94, 0x5A,
        0x4B, 0x82, 0x79, 0x18, 0x1A, 0xB5, 0xC8, 0x4D,
        0xF2, 0x1C, 0xE7, 0xF9, 0xB7, 0x3F, 0x42, 0xE1,
        0x97, 0xEA, 0x9C, 0x07, 0xE5, 0x6B, 0x5E, 0xB1,
        0x7E, 0x5F, 0x4E
};
#define clear_len_09 8
#define auth_len_09 10

/*
 *  =============== Packet Vector #10 ==================
 *  AES Key =  C0 C1 C2 C3  C4 C5 C6 C7  C8 C9 CA CB  CC CD CE CF
 *  Nonce =    00 00 00 0C  0B 0A 09 A0  A1 A2 A3 A4  A5
 *  Total packet length = 31. [Input with 12 cleartext header octets]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F
 *             10 11 12 13  14 15 16 17  18 19 1A 1B  1C 1D 1E
 *  CBC IV in: 61 00 00 00  0C 0B 0A 09  A0 A1 A2 A3  A4 A5 00 13
 *  CBC IV out:7F B8 0A 32  E9 80 57 46  EC 31 6C 3A  B2 A2 EB 5D
 *  After xor: 7F B4 0A 33  EB 83 53 43  EA 36 64 33  B8 A9 EB 5D   [hdr]
 *  After AES: 7E 96 96 BF  F1 56 D6 A8  6E AC F5 7B  7F 23 47 5A
 *  After xor: 72 9B 98 B0  E1 47 C4 BB  7A B9 E3 6C  67 3A 5D 41   [msg]
 *  After AES: 8B 4A EE 42  04 24 8A 59  FA CC 88 66  57 66 DD 72
 *  After xor: 97 57 F0 42  04 24 8A 59  FA CC 88 66  57 66 DD 72   [msg]
 *  After AES: 41 63 89 36  62 ED D7 EB  CD 6E 15 C1  89 48 62 05
 *  CBC-MAC  : 41 63 89 36  62 ED D7 EB  CD 6E
 *  CTR Start: 01 00 00 00  0C 0B 0A 09  A0 A1 A2 A3  A4 A5 00 01
 *  CTR[0001]: 0B 39 2B 9B  05 66 97 06  3F 12 56 8F  2B 13 A1 0F
 *  CTR[0002]: 07 89 65 25  23 40 94 3B  9E 69 B2 56  CC 5E F7 31
 *  CTR[MAC ]: 17 09 20 76  09 A0 4E 72  45 B3
 *  Total packet length = 41. [Authenticated and Encrypted Output]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  07 34 25 94
 *             15 77 85 15  2B 07 40 98  33 0A BB 14  1B 94 7B 56
 *             6A A9 40 6B  4D 99 99 88  DD
 */
static const uint8_t keys_10[] = {
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
        0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
};
static const uint8_t nonce_10[] = {
        0x00, 0x00, 0x00, 0x0c, 0x0b, 0x0a, 0x09, 0xA0,
        0xA1, 0xA2, 0xA3, 0xA4, 0xA5
};
static const uint8_t packet_in_10[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E
};
static const uint8_t packet_out_10[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x07, 0x34, 0x25, 0x94,
        0x15, 0x77, 0x85, 0x15, 0x2B, 0x07, 0x40, 0x98,
        0x33, 0x0A, 0xBB, 0x14, 0x1B, 0x94, 0x7B, 0x56,
        0x6A, 0xA9, 0x40, 0x6B, 0x4D, 0x99, 0x99, 0x88,
        0xDD
};
#define clear_len_10 12
#define auth_len_10 10

/*
 *  =============== Packet Vector #11 ==================
 *  AES Key =  C0 C1 C2 C3  C4 C5 C6 C7  C8 C9 CA CB  CC CD CE CF
 *  Nonce =    00 00 00 0D  0C 0B 0A A0  A1 A2 A3 A4  A5
 *  Total packet length = 32. [Input with 12 cleartext header octets]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F
 *             10 11 12 13  14 15 16 17  18 19 1A 1B  1C 1D 1E 1F
 *  CBC IV in: 61 00 00 00  0D 0C 0B 0A  A0 A1 A2 A3  A4 A5 00 14
 *  CBC IV out:B0 84 85 79  51 D2 FA 42  76 EF 3A D7  14 B9 62 87
 *  After xor: B0 88 85 78  53 D1 FE 47  70 E8 32 DE  1E B2 62 87   [hdr]
 *  After AES: C9 B3 64 7E  D8 79 2A 5C  65 B7 CE CC  19 0A 97 0A
 *  After xor: C5 BE 6A 71  C8 68 38 4F  71 A2 D8 DB  01 13 8D 11   [msg]
 *  After AES: 34 0F 69 17  FA B9 19 D6  1D AC D0 35  36 D6 55 8B
 *  After xor: 28 12 77 08  FA B9 19 D6  1D AC D0 35  36 D6 55 8B   [msg]
 *  After AES: 6B 5E 24 34  12 CC C2 AD  6F 1B 11 C3  A1 A9 D8 BC
 *  CBC-MAC  : 6B 5E 24 34  12 CC C2 AD  6F 1B
 *  CTR Start: 01 00 00 00  0D 0C 0B 0A  A0 A1 A2 A3  A4 A5 00 01
 *  CTR[0001]: 6B 66 BC 0C  90 A1 F1 12  FC BE 6F 4E  12 20 77 BC
 *  CTR[0002]: 97 9E 57 2B  BE 65 8A E5  CC 20 11 83  2A 9A 9B 5B
 *  CTR[MAC ]: 9E 64 86 DD  02 B6 49 C1  6D 37
 *  Total packet length = 42. [Authenticated and Encrypted Output]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  67 6B B2 03
 *             80 B0 E3 01  E8 AB 79 59  0A 39 6D A7  8B 83 49 34
 *             F5 3A A2 E9  10 7A 8B 6C  02 2C
 */
static const uint8_t keys_11[] = {
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
        0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
};
static const uint8_t nonce_11[] = {
        0x00, 0x00, 0x00, 0x0d, 0x0c, 0x0b, 0x0a, 0xA0,
        0xA1, 0xA2, 0xA3, 0xA4, 0xA5
};
static const uint8_t packet_in_11[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};
static const uint8_t packet_out_11[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x67, 0x6B, 0xB2, 0x03,
        0x80, 0xB0, 0xE3, 0x01, 0xE8, 0xAB, 0x79, 0x59,
        0x0A, 0x39, 0x6D, 0xA7, 0x8B, 0x83, 0x49, 0x34,
        0xF5, 0x3A, 0xA2, 0xE9, 0x10, 0x7A, 0x8B, 0x6C,
        0x02, 0x2C
};
#define clear_len_11 12
#define auth_len_11 10

/*
 *  =============== Packet Vector #12 ==================
 *  AES Key =  C0 C1 C2 C3  C4 C5 C6 C7  C8 C9 CA CB  CC CD CE CF
 *  Nonce =    00 00 00 0E  0D 0C 0B A0  A1 A2 A3 A4  A5
 *  Total packet length = 33. [Input with 12 cleartext header octets]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F
 *             10 11 12 13  14 15 16 17  18 19 1A 1B  1C 1D 1E 1F
 *             20
 *  CBC IV in: 61 00 00 00  0E 0D 0C 0B  A0 A1 A2 A3  A4 A5 00 15
 *  CBC IV out:5F 8E 8D 02  AD 95 7C 5A  36 14 CF 63  40 16 97 4F
 *  After xor: 5F 82 8D 03  AF 96 78 5F  30 13 C7 6A  4A 1D 97 4F   [hdr]
 *  After AES: 63 FA BD 69  B9 55 65 FF  54 AA F4 60  88 7D EC 9F
 *  After xor: 6F F7 B3 66  A9 44 77 EC  40 BF E2 77  90 64 F6 84   [msg]
 *  After AES: 5A 76 5F 0B  93 CE 4F 6A  B4 1D 91 30  18 57 6A D7
 *  After xor: 46 6B 41 14  B3 CE 4F 6A  B4 1D 91 30  18 57 6A D7   [msg]
 *  After AES: 9D 66 92 41  01 08 D5 B6  A1 45 85 AC  AF 86 32 E8
 *  CBC-MAC  : 9D 66 92 41  01 08 D5 B6  A1 45
 *  CTR Start: 01 00 00 00  0E 0D 0C 0B  A0 A1 A2 A3  A4 A5 00 01
 *  CTR[0001]: CC F2 AE D9  E0 4A C9 74  E6 58 55 B3  2B 94 30 BF
 *  CTR[0002]: A2 CA AC 11  63 F4 07 E5  E5 F6 E3 B3  79 0F 79 F8
 *  CTR[MAC ]: 50 7C 31 57  63 EF 78 D3  77 9E
 *  Total packet length = 43. [Authenticated and Encrypted Output]
 *             00 01 02 03  04 05 06 07  08 09 0A 0B  C0 FF A0 D6
 *             F0 5B DB 67  F2 4D 43 A4  33 8D 2A A4  BE D7 B2 0E
 *             43 CD 1A A3  16 62 E7 AD  65 D6 DB
 */
static const uint8_t keys_12[] = {
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
        0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
};
static const uint8_t nonce_12[] = {
        0x00, 0x00, 0x00, 0x0e, 0x0d, 0x0c, 0x0b, 0xA0,
        0xA1, 0xA2, 0xA3, 0xA4, 0xA5
};
static const uint8_t packet_in_12[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0x20
};
static const uint8_t packet_out_12[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0xC0, 0xFF, 0xA0, 0xD6,
        0xF0, 0x5B, 0xDB, 0x67, 0xF2, 0x4D, 0x43, 0xA4,
        0x33, 0x8D, 0x2A, 0xA4, 0xBE, 0xD7, 0xB2, 0x0E,
        0x43, 0xCD, 0x1A, 0xA3, 0x16, 0x62, 0xE7, 0xAD,
        0x65, 0xD6, 0xDB
};
#define clear_len_12 12
#define auth_len_12 10

/*
 *  =============== Packet Vector #13 ==================
 *  AES Key =  D7 82 8D 13  B2 B0 BD C3  25 A7 62 36  DF 93 CC 6B
 *  Nonce =    00 41 2B 4E  A9 CD BE 3C  96 96 76 6C  FA
 *  Total packet length = 31. [Input with 8 cleartext header octets]
 *             0B E1 A8 8B  AC E0 18 B1  08 E8 CF 97  D8 20 EA 25
 *             84 60 E9 6A  D9 CF 52 89  05 4D 89 5C  EA C4 7C
 *  CBC IV in: 59 00 41 2B  4E A9 CD BE  3C 96 96 76  6C FA 00 17
 *  CBC IV out:33 AE C3 1A  1F B7 CC 35  E5 DA D2 BA  C0 90 D9 A3
 *  After xor: 33 A6 C8 FB  B7 3C 60 D5  FD 6B D2 BA  C0 90 D9 A3   [hdr]
 *  After AES: B7 56 CA 1E  5B 42 C6 9C  58 E3 0A F5  2B F7 7C FD
 *  After xor: BF BE 05 89  83 62 2C B9  DC 83 E3 9F  F2 38 2E 74   [msg]
 *  After AES: 33 3D 3A 3D  07 B5 3C 7B  22 0E 96 1A  18 A9 A1 9E
 *  After xor: 36 70 B3 61  ED 71 40 7B  22 0E 96 1A  18 A9 A1 9E   [msg]
 *  After AES: 14 BD DB 6B  F9 01 63 4D  FB 56 51 83  BC 74 93 F7
 *  CBC-MAC  : 14 BD DB 6B  F9 01 63 4D
 *  CTR Start: 01 00 41 2B  4E A9 CD BE  3C 96 96 76  6C FA 00 01
 *  CTR[0001]: 44 51 B0 11  7A 84 82 BF  03 19 AE C1  59 5E BD DA
 *  CTR[0002]: 83 EB 76 E1  3A 44 84 7F  92 20 09 07  76 B8 25 C5
 *  CTR[MAC ]: F3 31 2C A0  F5 DC B4 FE
 *  Total packet length = 39. [Authenticated and Encrypted Output]
 *             0B E1 A8 8B  AC E0 18 B1  4C B9 7F 86  A2 A4 68 9A
 *             87 79 47 AB  80 91 EF 53  86 A6 FF BD  D0 80 F8 E7
 *             8C F7 CB 0C  DD D7 B3
 */
static const uint8_t keys_13[] = {
        0xD7, 0x82, 0x8D, 0x13, 0xB2, 0xB0, 0xBD, 0xC3,
        0x25, 0xA7, 0x62, 0x36, 0xDF, 0x93, 0xCC, 0x6B
};
static const uint8_t nonce_13[] = {
        0x00, 0x41, 0x2b, 0x4e, 0xa9, 0xcd, 0xbe, 0x3c,
        0x96, 0x96, 0x76, 0x6c, 0xfa
};
static const uint8_t packet_in_13[] = {
        0x0B, 0xE1, 0xA8, 0x8B, 0xAC, 0xE0, 0x18, 0xB1,
        0x08, 0xE8, 0xCF, 0x97, 0xD8, 0x20, 0xEA, 0x25,
        0x84, 0x60, 0xE9, 0x6A, 0xD9, 0xCF, 0x52, 0x89,
        0x05, 0x4D, 0x89, 0x5C, 0xEA, 0xC4, 0x7C
};
static const uint8_t packet_out_13[] = {
        0x0B, 0xE1, 0xA8, 0x8B, 0xAC, 0xE0, 0x18, 0xB1,
        0x4C, 0xB9, 0x7F, 0x86, 0xA2, 0xA4, 0x68, 0x9A,
        0x87, 0x79, 0x47, 0xAB, 0x80, 0x91, 0xEF, 0x53,
        0x86, 0xA6, 0xFF, 0xBD, 0xD0, 0x80, 0xF8, 0xE7,
        0x8C, 0xF7, 0xCB, 0x0C, 0xDD, 0xD7, 0xB3
};
#define clear_len_13 8
#define auth_len_13 8

/*
 *  =============== Packet Vector #14 ==================
 *  AES Key =  D7 82 8D 13  B2 B0 BD C3  25 A7 62 36  DF 93 CC 6B
 *  Nonce =    00 33 56 8E  F7 B2 63 3C  96 96 76 6C  FA
 *  Total packet length = 32. [Input with 8 cleartext header octets]
 *             63 01 8F 76  DC 8A 1B CB  90 20 EA 6F  91 BD D8 5A
 *             FA 00 39 BA  4B AF F9 BF  B7 9C 70 28  94 9C D0 EC
 *  CBC IV in: 59 00 33 56  8E F7 B2 63  3C 96 96 76  6C FA 00 18
 *  CBC IV out:42 0D B1 50  BB 0C 44 DA  83 E4 52 09  55 99 67 E3
 *  After xor: 42 05 D2 51  34 7A 98 50  98 2F 52 09  55 99 67 E3   [hdr]
 *  After AES: EA D1 CA 56  02 02 09 5C  E6 12 B0 D2  18 A0 DD 44
 *  After xor: 7A F1 20 39  93 BF D1 06  1C 12 89 68  53 0F 24 FB   [msg]
 *  After AES: 51 77 41 69  C3 DE 6B 24  13 27 74 90  F5 FF C5 62
 *  After xor: E6 EB 31 41  57 42 BB C8  13 27  C5 62   [msg]
 *  After AES: D4 CC 3B 82  DF 9F CC 56  7E E5 83 61  D7 8D FB 5E
 *  CBC-MAC  : D4 CC 3B 82  DF 9F CC 56
 *  CTR Start: 01 00 33 56  8E F7 B2 63  3C 96 96 76  6C FA 00 01
 *  CTR[0001]: DC EB F4 13  38 3C 66 A0  5A 72 55 EF  98 D7 FF AD
 *  CTR[0002]: 2F 54 2C BA  15 D6 6C DF  E1 EC 46 8F  0E 68 A1 24
 *  CTR[MAC ]: 11 E2 D3 9F  A2 E8 0C DC
 *  Total packet length = 40. [Authenticated and Encrypted Output]
 *             63 01 8F 76  DC 8A 1B CB  4C CB 1E 7C  A9 81 BE FA
 *             A0 72 6C 55  D3 78 06 12  98 C8 5C 92  81 4A BC 33
 *             C5 2E E8 1D  7D 77 C0 8A
 */
static const uint8_t keys_14[] = {
        0xD7, 0x82, 0x8D, 0x13, 0xB2, 0xB0, 0xBD, 0xC3,
        0x25, 0xA7, 0x62, 0x36, 0xDF, 0x93, 0xCC, 0x6B
};
static const uint8_t nonce_14[] = {
        0x00, 0x33, 0x56, 0x8E, 0xF7, 0xB2, 0x63, 0x3C,
        0x96, 0x96, 0x76, 0x6C, 0xFA
};
static const uint8_t packet_in_14[] = {
        0x63, 0x01, 0x8F, 0x76, 0xDC, 0x8A, 0x1B, 0xCB,
        0x90, 0x20, 0xEA, 0x6F, 0x91, 0xBD, 0xD8, 0x5A,
        0xFA, 0x00, 0x39, 0xBA, 0x4B, 0xAF, 0xF9, 0xBF,
        0xB7, 0x9C, 0x70, 0x28, 0x94, 0x9C, 0xD0, 0xEC,
};
static const uint8_t packet_out_14[] = {
        0x63, 0x01, 0x8F, 0x76, 0xDC, 0x8A, 0x1B, 0xCB,
        0x4C, 0xCB, 0x1E, 0x7C, 0xA9, 0x81, 0xBE, 0xFA,
        0xA0, 0x72, 0x6C, 0x55, 0xD3, 0x78, 0x06, 0x12,
        0x98, 0xC8, 0x5C, 0x92, 0x81, 0x4A, 0xBC, 0x33,
        0xC5, 0x2E, 0xE8, 0x1D, 0x7D, 0x77, 0xC0, 0x8A
};
#define clear_len_14 8
#define auth_len_14 8

/*
 *  =============== Packet Vector #15 ==================
 *  AES Key =  D7 82 8D 13  B2 B0 BD C3  25 A7 62 36  DF 93 CC 6B
 *  Nonce =    00 10 3F E4  13 36 71 3C  96 96 76 6C  FA
 *  Total packet length = 33. [Input with 8 cleartext header octets]
 *             AA 6C FA 36  CA E8 6B 40  B9 16 E0 EA  CC 1C 00 D7
 *             DC EC 68 EC  0B 3B BB 1A  02 DE 8A 2D  1A A3 46 13
 *             2E
 *  CBC IV in: 59 00 10 3F  E4 13 36 71  3C 96 96 76  6C FA 00 19
 *  CBC IV out:B3 26 49 FF  D5 9F 56 0F  02 2D 11 E2  62 C5 BE EA
 *  After xor: B3 2E E3 93  2F A9 9C E7  69 6D 11 E2  62 C5 BE EA   [hdr]
 *  After AES: 82 50 9E E5  B2 FF DB CA  9B D0 2E 20  6B 3F B7 AD
 *  After xor: 3B 46 7E 0F  7E E3 DB 1D  47 3C 46 CC  60 04 0C B7   [msg]
 *  After AES: 80 46 0E 4C  08 3A D0 3F  B9 A9 13 BE  E4 DE 2F 66
 *  After xor: 82 98 84 61  12 99 96 2C  97 A9 13 BE  E4 DE 2F 66   [msg]
 *  After AES: 47 29 CB 00  31 F1 81 C1  92 68 4B 89  A4 71 50 E7
 *  CBC-MAC  : 47 29 CB 00  31 F1 81 C1
 *  CTR Start: 01 00 10 3F  E4 13 36 71  3C 96 96 76  6C FA 00 01
 *  CTR[0001]: 08 C4 DA C8  EC C1 C0 7B  4C E1 F2 4C  37 5A 47 EE
 *  CTR[0002]: A7 87 2E 6C  6D C4 4E 84  26 02 50 4C  3F A5 73 C5
 *  CTR[MAC ]: E0 5F B2 6E  EA 83 B4 C7
 *  Total packet length = 41. [Authenticated and Encrypted Output]
 *             AA 6C FA 36  CA E8 6B 40  B1 D2 3A 22  20 DD C0 AC
 *             90 0D 9A A0  3C 61 FC F4  A5 59 A4 41  77 67 08 97
 *             08 A7 76 79  6E DB 72 35  06
 */
static const uint8_t keys_15[] = {
        0xD7, 0x82, 0x8D, 0x13, 0xB2, 0xB0, 0xBD, 0xC3,
        0x25, 0xA7, 0x62, 0x36, 0xDF, 0x93, 0xCC, 0x6B
};
static const uint8_t nonce_15[] = {
        0x00, 0x10, 0x3F, 0xE4, 0x13, 0x36, 0x71, 0x3C,
        0x96, 0x96, 0x76, 0x6C, 0xFA
};
static const uint8_t packet_in_15[] = {
        0xAA, 0x6C, 0xFA, 0x36, 0xCA, 0xE8, 0x6B, 0x40,
        0xB9, 0x16, 0xE0, 0xEA, 0xCC, 0x1C, 0x00, 0xD7,
        0xDC, 0xEC, 0x68, 0xEC, 0x0B, 0x3B, 0xBB, 0x1A,
        0x02, 0xDE, 0x8A, 0x2D, 0x1A, 0xA3, 0x46, 0x13,
        0x2E
};
static const uint8_t packet_out_15[] = {
        0xAA, 0x6C, 0xFA, 0x36, 0xCA, 0xE8, 0x6B, 0x40,
        0xB1, 0xD2, 0x3A, 0x22, 0x20, 0xDD, 0xC0, 0xAC,
        0x90, 0x0D, 0x9A, 0xA0, 0x3C, 0x61, 0xFC, 0xF4,
        0xA5, 0x59, 0xA4, 0x41, 0x77, 0x67, 0x08, 0x97,
        0x08, 0xA7, 0x76, 0x79, 0x6E, 0xDB, 0x72, 0x35,
        0x06
};
#define clear_len_15 8
#define auth_len_15 8

/*
 *  =============== Packet Vector #16 ==================
 *  AES Key =  D7 82 8D 13  B2 B0 BD C3  25 A7 62 36  DF 93 CC 6B
 *  Nonce =    00 76 4C 63  B8 05 8E 3C  96 96 76 6C  FA
 *  Total packet length = 31. [Input with 12 cleartext header octets]
 *             D0 D0 73 5C  53 1E 1B EC  F0 49 C2 44  12 DA AC 56
 *             30 EF A5 39  6F 77 0C E1  A6 6B 21 F7  B2 10 1C
 *  CBC IV in: 59 00 76 4C  63 B8 05 8E  3C 96 96 76  6C FA 00 13
 *  CBC IV out:AB DC 4E C9  AA 72 33 97  DF 2D AD 76  33 DE 3B 0D
 *  After xor: AB D0 9E 19  D9 2E 60 89  C4 C1 5D 3F  F1 9A 3B 0D   [hdr]
 *  After AES: 62 86 F6 2F  23 42 63 B0  1C FD 8C 37  40 74 81 EB
 *  After xor: 70 5C 5A 79  13 AD C6 89  73 8A 80 D6  E6 1F A0 1C   [msg]
 *  After AES: 88 95 84 18  CF 79 CA BE  EB C0 0C C4  86 E6 01 F7
 *  After xor: 3A 85 98 18  CF 79 CA BE  EB C0 0C C4  86 E6 01 F7   [msg]
 *  After AES: C1 85 92 D9  84 CD 67 80  63 D1 D9 6D  C1 DF A1 11
 *  CBC-MAC  : C1 85 92 D9  84 CD 67 80
 *  CTR Start: 01 00 76 4C  63 B8 05 8E  3C 96 96 76  6C FA 00 01
 *  CTR[0001]: 06 08 FF 95  A6 94 D5 59  F4 0B B7 9D  EF FA 41 DF
 *  CTR[0002]: 80 55 3A 75  78 38 04 A9  64 8B 68 DD  7F DC DD 7A
 *  CTR[MAC ]: 5B EA DB 4E  DF 07 B9 2F
 *  Total packet length = 39. [Authenticated and Encrypted Output]
 *             D0 D0 73 5C  53 1E 1B EC  F0 49 C2 44  14 D2 53 C3
 *             96 7B 70 60  9B 7C BB 7C  49 91 60 28  32 45 26 9A
 *             6F 49 97 5B  CA DE AF
 */
static const uint8_t keys_16[] = {
        0xD7, 0x82, 0x8D, 0x13, 0xB2, 0xB0, 0xBD, 0xC3,
        0x25, 0xA7, 0x62, 0x36, 0xDF, 0x93, 0xCC, 0x6B
};
static const uint8_t nonce_16[] = {
        0x00, 0x76, 0x4C, 0x63, 0xB8, 0x05, 0x8E, 0x3C,
        0x96, 0x96, 0x76, 0x6C, 0xFA
};
static const uint8_t packet_in_16[] = {
        0xD0, 0xD0, 0x73, 0x5C, 0x53, 0x1E, 0x1B, 0xEC,
        0xF0, 0x49, 0xC2, 0x44, 0x12, 0xDA, 0xAC, 0x56,
        0x30, 0xEF, 0xA5, 0x39, 0x6F, 0x77, 0x0C, 0xE1,
        0xA6, 0x6B, 0x21, 0xF7, 0xB2, 0x10, 0x1C
};
static const uint8_t packet_out_16[] = {
        0xD0, 0xD0, 0x73, 0x5C, 0x53, 0x1E, 0x1B, 0xEC,
        0xF0, 0x49, 0xC2, 0x44, 0x14, 0xD2, 0x53, 0xC3,
        0x96, 0x7B, 0x70, 0x60, 0x9B, 0x7C, 0xBB, 0x7C,
        0x49, 0x91, 0x60, 0x28, 0x32, 0x45, 0x26, 0x9A,
        0x6F, 0x49, 0x97, 0x5B, 0xCA, 0xDE, 0xAF
};
#define clear_len_16 12
#define auth_len_16 8

/*
 *  =============== Packet Vector #17 ==================
 *  AES Key =  D7 82 8D 13  B2 B0 BD C3  25 A7 62 36  DF 93 CC 6B
 *  Nonce =    00 F8 B6 78  09 4E 3B 3C  96 96 76 6C  FA
 *  Total packet length = 32. [Input with 12 cleartext header octets]
 *             77 B6 0F 01  1C 03 E1 52  58 99 BC AE  E8 8B 6A 46
 *             C7 8D 63 E5  2E B8 C5 46  EF B5 DE 6F  75 E9 CC 0D
*   CBC IV in: 59 00 F8 B6  78 09 4E 3B  3C 96 96 76  6C FA 00 14
 *  CBC IV out:F4 68 FE 5D  B1 53 0B 7A  5A A5 FB 27  40 CF 6E 33
 *  After xor: F4 64 89 EB  BE 52 17 79  BB F7 A3 BE  FC 61 6E 33   [hdr]
 *  After AES: 23 29 0E 0B  33 45 9A 83  32 2D E4 06  86 67 10 04
 *  After xor: CB A2 64 4D  F4 C8 F9 66  1C 95 21 40  69 D2 CE 6B   [msg]
 *  After AES: 8F BE D4 0F  8B 89 B7 B8  20 D5 5F E0  3C E2 43 11
 *  After xor: FA 57 18 02  8B 89 B7 B8  20 D5 5F E0  3C E2 43 11   [msg]
 *  After AES: 6A DB 15 B6  71 81 B2 E2  2B E3 4A F2  B2 83 E2 29
 *  CBC-MAC  : 6A DB 15 B6  71 81 B2 E2
 *  CTR Start: 01 00 F8 B6  78 09 4E 3B  3C 96 96 76  6C FA 00 01
 *  CTR[0001]: BD CE 95 5C  CF D3 81 0A  91 EA 77 A6  A4 5B C0 4C
 *  CTR[0002]: 43 2E F2 32  AE 36 D8 92  22 BF 63 37  E6 B2 6C E8
 *  CTR[MAC ]: 1C F7 19 C1  35 7F CC DE
 *  Total packet length = 40. [Authenticated and Encrypted Output]
 *             77 B6 0F 01  1C 03 E1 52  58 99 BC AE  55 45 FF 1A
 *             08 5E E2 EF  BF 52 B2 E0  4B EE 1E 23  36 C7 3E 3F
 *             76 2C 0C 77  44 FE 7E 3C
 */
static const uint8_t keys_17[] = {
        0xD7, 0x82, 0x8D, 0x13, 0xB2, 0xB0, 0xBD, 0xC3,
        0x25, 0xA7, 0x62, 0x36, 0xDF, 0x93, 0xCC, 0x6B
};
static const uint8_t nonce_17[] = {
        0x00, 0xF8, 0xB6, 0x78, 0x09, 0x4E, 0x3B, 0x3C,
        0x96, 0x96, 0x76, 0x6C, 0xFA
};
static const uint8_t packet_in_17[] = {
        0x77, 0xB6, 0x0F, 0x01, 0x1C, 0x03, 0xE1, 0x52,
        0x58, 0x99, 0xBC, 0xAE, 0xE8, 0x8B, 0x6A, 0x46,
        0xC7, 0x8D, 0x63, 0xE5, 0x2E, 0xB8, 0xC5, 0x46,
        0xEF, 0xB5, 0xDE, 0x6F, 0x75, 0xE9, 0xCC, 0x0D
};
static const uint8_t packet_out_17[] = {
        0x77, 0xB6, 0x0F, 0x01, 0x1C, 0x03, 0xE1, 0x52,
        0x58, 0x99, 0xBC, 0xAE, 0x55, 0x45, 0xFF, 0x1A,
        0x08, 0x5E, 0xE2, 0xEF, 0xBF, 0x52, 0xB2, 0xE0,
        0x4B, 0xEE, 0x1E, 0x23, 0x36, 0xC7, 0x3E, 0x3F,
        0x76, 0x2C, 0x0C, 0x77, 0x44, 0xFE, 0x7E, 0x3C
};
#define clear_len_17 12
#define auth_len_17 8

/*
 *  =============== Packet Vector #18 ==================
 *  AES Key =  D7 82 8D 13  B2 B0 BD C3  25 A7 62 36  DF 93 CC 6B
 *  Nonce =    00 D5 60 91  2D 3F 70 3C  96 96 76 6C  FA
 *  Total packet length = 33. [Input with 12 cleartext header octets]
 *             CD 90 44 D2  B7 1F DB 81  20 EA 60 C0  64 35 AC BA
 *             FB 11 A8 2E  2F 07 1D 7C  A4 A5 EB D9  3A 80 3B A8
 *             7F
 *  CBC IV in: 59 00 D5 60  91 2D 3F 70  3C 96 96 76  6C FA 00 15
 *  CBC IV out:BA 37 74 54  D7 20 A4 59  25 97 F6 A3  D1 D6 BA 67
 *  After xor: BA 3B B9 C4  93 F2 13 46  FE 16 D6 49  B1 16 BA 67   [hdr]
 *  After AES: 81 6A 20 20  38 D0 A6 30  CB E0 B7 3C  39 BB CE 05
 *  After xor: E5 5F 8C 9A  C3 C1 0E 1E  E4 E7 AA 40  9D 1E 25 DC   [msg]
 *  After AES: 6D 5C 15 FD  85 2D 5C 3C  E3 03 3D 85  DA 57 BD AC
 *  After xor: 57 DC 2E 55  FA 2D 5C 3C  E3 03 3D 85  DA 57 BD AC   [msg]
 *  After AES: B0 4A 1C 23  BC 39 B6 51  76 FD 5B FF  9B C1 28 5E
 *  CBC-MAC  : B0 4A 1C 23  BC 39 B6 51
 *  CTR Start: 01 00 D5 60  91 2D 3F 70  3C 96 96 76  6C FA 00 01
 *  CTR[0001]: 64 A2 C5 56  50 CE E0 4C  7A 93 D8 EE  F5 43 E8 8E
 *  CTR[0002]: 18 E7 65 AC  B7 B0 E9 AF  09 2B D0 20  6C A1 C8 3C
 *  CTR[MAC ]: F7 43 82 79  5C 49 F3 00
 *  Total packet length = 41. [Authenticated and Encrypted Output]
 *             CD 90 44 D2  B7 1F DB 81  20 EA 60 C0  00 97 69 EC
 *             AB DF 48 62  55 94 C5 92  51 E6 03 57  22 67 5E 04
 *             C8 47 09 9E  5A E0 70 45  51
 */
static const uint8_t keys_18[] = {
        0xD7, 0x82, 0x8D, 0x13, 0xB2, 0xB0, 0xBD, 0xC3,
        0x25, 0xA7, 0x62, 0x36, 0xDF, 0x93, 0xCC, 0x6B
};
static const uint8_t nonce_18[] = {
        0x00, 0xD5, 0x60, 0x91, 0x2D, 0x3F, 0x70, 0x3C,
        0x96, 0x96, 0x76, 0x6C, 0xFA
};
static const uint8_t packet_in_18[] = {
        0xCD, 0x90, 0x44, 0xD2, 0xB7, 0x1F, 0xDB, 0x81,
        0x20, 0xEA, 0x60, 0xC0, 0x64, 0x35, 0xAC, 0xBA,
        0xFB, 0x11, 0xA8, 0x2E, 0x2F, 0x07, 0x1D, 0x7C,
        0xA4, 0xA5, 0xEB, 0xD9, 0x3A, 0x80, 0x3B, 0xA8,
        0x7F
};
static const uint8_t packet_out_18[] = {
        0xCD, 0x90, 0x44, 0xD2, 0xB7, 0x1F, 0xDB, 0x81,
        0x20, 0xEA, 0x60, 0xC0, 0x00, 0x97, 0x69, 0xEC,
        0xAB, 0xDF, 0x48, 0x62, 0x55, 0x94, 0xC5, 0x92,
        0x51, 0xE6, 0x03, 0x57, 0x22, 0x67, 0x5E, 0x04,
        0xC8, 0x47, 0x09, 0x9E, 0x5A, 0xE0, 0x70, 0x45,
        0x51
};
#define clear_len_18 12
#define auth_len_18 8

/*
 *  =============== Packet Vector #19 ==================
 *  AES Key =  D7 82 8D 13  B2 B0 BD C3  25 A7 62 36  DF 93 CC 6B
 *  Nonce =    00 42 FF F8  F1 95 1C 3C  96 96 76 6C  FA
 *  Total packet length = 31. [Input with 8 cleartext header octets]
 *             D8 5B C7 E6  9F 94 4F B8  8A 19 B9 50  BC F7 1A 01
 *             8E 5E 67 01  C9 17 87 65  98 09 D6 7D  BE DD 18
 *  CBC IV in: 61 00 42 FF  F8 F1 95 1C  3C 96 96 76  6C FA 00 17
 *  CBC IV out:44 F7 CC 9C  2B DD 2F 45  F6 38 25 6B  73 6E 1D 7A
 *  After xor: 44 FF 14 C7  EC 3B B0 D1  B9 80 25 6B  73 6E 1D 7A   [hdr]
 *  After AES: 57 C3 73 F8  00 AA 5F CC  7B CF 1D 1B  DD BB 4C 52
 *  After xor: DD DA CA A8  BC 5D 45 CD  F5 91 7A 1A  14 AC CB 37   [msg]
 *  After AES: 42 4E 93 72  72 C8 79 B6  11 C7 A5 9F  47 8D 9F D8
 *  After xor: DA 47 45 0F  CC 15 61 B6  11 C7 A5 9F  47 8D 9F D8   [msg]
 *  After AES: 9A CB 03 F8  B9 DB C8 D2  D2 D7 A4 B4  95 25 08 67
 *  CBC-MAC  : 9A CB 03 F8  B9 DB C8 D2  D2 D7
 *  CTR Start: 01 00 42 FF  F8 F1 95 1C  3C 96 96 76  6C FA 00 01
 *  CTR[0001]: 36 38 34 FA  28 83 3D B7  55 66 0D 98  65 0D 68 46
 *  CTR[0002]: 35 E9 63 54  87 16 72 56  3F 0C 08 AF  78 44 31 A9
 *  CTR[MAC ]: F9 B7 FA 46  7B 9B 40 45  14 6D
 *  Total packet length = 41. [Authenticated and Encrypted Output]
 *             D8 5B C7 E6  9F 94 4F B8  BC 21 8D AA  94 74 27 B6
 *             DB 38 6A 99  AC 1A EF 23  AD E0 B5 29  39 CB 6A 63
 *             7C F9 BE C2  40 88 97 C6  BA
 */
static const uint8_t keys_19[] = {
        0xD7, 0x82, 0x8D, 0x13, 0xB2, 0xB0, 0xBD, 0xC3,
        0x25, 0xA7, 0x62, 0x36, 0xDF, 0x93, 0xCC, 0x6B
};
static const uint8_t nonce_19[] = {
        0x00, 0x42, 0xFF, 0xF8, 0xF1, 0x95, 0x1C, 0x3C,
        0x96, 0x96, 0x76, 0x6C, 0xFA
};
static const uint8_t packet_in_19[] = {
        0xD8, 0x5B, 0xC7, 0xE6, 0x9F, 0x94, 0x4F, 0xB8,
        0x8A, 0x19, 0xB9, 0x50, 0xBC, 0xF7, 0x1A, 0x01,
        0x8E, 0x5E, 0x67, 0x01, 0xC9, 0x17, 0x87, 0x65,
        0x98, 0x09, 0xD6, 0x7D, 0xBE, 0xDD, 0x18
};
static const uint8_t packet_out_19[] = {
        0xD8, 0x5B, 0xC7, 0xE6, 0x9F, 0x94, 0x4F, 0xB8,
        0xBC, 0x21, 0x8D, 0xAA, 0x94, 0x74, 0x27, 0xB6,
        0xDB, 0x38, 0x6A, 0x99, 0xAC, 0x1A, 0xEF, 0x23,
        0xAD, 0xE0, 0xB5, 0x29, 0x39, 0xCB, 0x6A, 0x63,
        0x7C, 0xF9, 0xBE, 0xC2, 0x40, 0x88, 0x97, 0xC6,
        0xBA
};
#define clear_len_19 8
#define auth_len_19 10

/*
 *  ================= Packet Vector #20 ==================
 *  AES Key =  D7 82 8D 13  B2 B0 BD C3  25 A7 62 36  DF 93 CC 6B
 *  Nonce =    00 92 0F 40  E5 6C DC 3C  96 96 76 6C  FA
 *  Total packet length = 32. [Input with 8 cleartext header octets]
 *             74 A0 EB C9  06 9F 5B 37  17 61 43 3C  37 C5 A3 5F
 *             C1 F3 9F 40  63 02 EB 90  7C 61 63 BE  38 C9 84 37
 *  CBC IV in: 61 00 92 0F  40 E5 6C DC  3C 96 96 76  6C FA 00 18
 *  CBC IV out:60 CB 21 CE  40 06 50 AE  2A D2 BE 52  9F 5F 0F C2
 *  After xor: 60 C3 55 6E  AB CF 56 31  71 E5 BE 52  9F 5F 0F C2   [hdr]
 *  After AES: 03 20 64 14  35 32 5D 95  C8 A2 50 40  93 28 DA 9B
 *  After xor: 14 41 27 28  02 F7 FE CA  09 51 CF 00  F0 2A 31 0B   [msg]
 *  After AES: B9 E8 87 95  ED F7 F0 08  15 15 F0 14  E2 FE 0E 48
 *  After xor: C5 89 E4 2B  D5 3E 74 3F  15 15 F0 14  E2 FE 0E 48   [msg]
 *  After AES: 8F AD 0C 23  E9 63 7E 87  FA 21 45 51  1B 47 DE F1
 *  CBC-MAC  : 8F AD 0C 23  E9 63 7E 87  FA 21
 *  CTR Start: 01 00 92 0F  40 E5 6C DC  3C 96 96 76  6C FA 00 01
 *  CTR[0001]: 4F 71 A5 C1  12 42 E3 7D  29 F0 FE E4  1B E1 02 5F
 *  CTR[0002]: 34 2B D3 F1  7C B7 7B C1  79 0B 05 05  61 59 27 2C
 *  CTR[MAC ]: 7F 09 7B EF  C6 AA C1 D3  73 65
 *  Total packet length = 42. [Authenticated and Encrypted Output]
 *             74 A0 EB C9  06 9F 5B 37  58 10 E6 FD  25 87 40 22
 *             E8 03 61 A4  78 E3 E9 CF  48 4A B0 4F  44 7E FF F6
 *             F0 A4 77 CC  2F C9 BF 54  89 44
 */
static const uint8_t keys_20[] = {
        0xD7, 0x82, 0x8D, 0x13, 0xB2, 0xB0, 0xBD, 0xC3,
        0x25, 0xA7, 0x62, 0x36, 0xDF, 0x93, 0xCC, 0x6B
};
static const uint8_t nonce_20[] = {
        0x00, 0x92, 0x0F, 0x40, 0xE5, 0x6C, 0xDC, 0x3C,
        0x96, 0x96, 0x76, 0x6C, 0xFA
};
static const uint8_t packet_in_20[] = {
        0x74, 0xA0, 0xEB, 0xC9, 0x06, 0x9F, 0x5B, 0x37,
        0x17, 0x61, 0x43, 0x3C, 0x37, 0xC5, 0xA3, 0x5F,
        0xC1, 0xF3, 0x9F, 0x40, 0x63, 0x02, 0xEB, 0x90,
        0x7C, 0x61, 0x63, 0xBE, 0x38, 0xC9, 0x84, 0x37
};
static const uint8_t packet_out_20[] = {
        0x74, 0xA0, 0xEB, 0xC9, 0x06, 0x9F, 0x5B, 0x37,
        0x58, 0x10, 0xE6, 0xFD, 0x25, 0x87, 0x40, 0x22,
        0xE8, 0x03, 0x61, 0xA4, 0x78, 0xE3, 0xE9, 0xCF,
        0x48, 0x4A, 0xB0, 0x4F, 0x44, 0x7E, 0xFF, 0xF6,
        0xF0, 0xA4, 0x77, 0xCC, 0x2F, 0xC9, 0xBF, 0x54,
        0x89, 0x44
};
#define clear_len_20 8
#define auth_len_20 10

/*
 *  =============== Packet Vector #21 ==================
 *  AES Key =  D7 82 8D 13  B2 B0 BD C3  25 A7 62 36  DF 93 CC 6B
 *  Nonce =    00 27 CA 0C  71 20 BC 3C  96 96 76 6C  FA
 *  Total packet length = 33. [Input with 8 cleartext header octets]
 *             44 A3 AA 3A  AE 64 75 CA  A4 34 A8 E5  85 00 C6 E4
 *             15 30 53 88  62 D6 86 EA  9E 81 30 1B  5A E4 22 6B
 *             FA
 *  CBC IV in: 61 00 27 CA  0C 71 20 BC  3C 96 96 76  6C FA 00 19
 *  CBC IV out:43 07 C0 73  A8 9E E1 D5  05 27 B2 9A  62 48 D6 D2
 *  After xor: 43 0F 84 D0  02 A4 4F B1  70 ED B2 9A  62 48 D6 D2   [hdr]
 *  After AES: B6 0B C6 F5  84 01 75 BC  01 27 70 F1  11 8D 75 10
 *  After xor: 12 3F 6E 10  01 01 B3 58  14 17 23 79  73 5B F3 FA   [msg]
 *  After AES: 7D 5E 64 92  CE 2C B9 EA  7E 4C 4A 09  09 89 C8 FB
 *  After xor: E3 DF 54 89  94 C8 9B 81  84 4C 4A 09  09 89 C8 FB   [msg]
 *  After AES: 68 5F 8D 79  D2 2B 9B 74  21 DF 4C 3E  87 BA 0A AF
 *  CBC-MAC  : 68 5F 8D 79  D2 2B 9B 74  21 DF
 *  CTR Start: 01 00 27 CA  0C 71 20 BC  3C 96 96 76  6C FA 00 01
 *  CTR[0001]: 56 8A 45 9E  40 09 48 67  EB 85 E0 9E  6A 2E 64 76
 *  CTR[0002]: A6 00 AA 92  92 03 54 9A  AE EF 2C CC  59 13 7A 57
 *  CTR[MAC ]: 25 1E DC DD  3F 11 10 F3  98 11
 *  Total packet length = 43. [Authenticated and Encrypted Output]
 *             44 A3 AA 3A  AE 64 75 CA  F2 BE ED 7B  C5 09 8E 83
 *             FE B5 B3 16  08 F8 E2 9C  38 81 9A 89  C8 E7 76 F1
 *             54 4D 41 51  A4 ED 3A 8B  87 B9 CE
 */
static const uint8_t keys_21[] = {
        0xD7, 0x82, 0x8D, 0x13, 0xB2, 0xB0, 0xBD, 0xC3,
        0x25, 0xA7, 0x62, 0x36, 0xDF, 0x93, 0xCC, 0x6B
};
static const uint8_t nonce_21[] = {
        0x00, 0x27, 0xCA, 0x0C, 0x71, 0x20, 0xBC, 0x3C,
        0x96, 0x96, 0x76, 0x6C, 0xFA
};
static const uint8_t packet_in_21[] = {
        0x44, 0xA3, 0xAA, 0x3A, 0xAE, 0x64, 0x75, 0xCA,
        0xA4, 0x34, 0xA8, 0xE5, 0x85, 0x00, 0xC6, 0xE4,
        0x15, 0x30, 0x53, 0x88, 0x62, 0xD6, 0x86, 0xEA,
        0x9E, 0x81, 0x30, 0x1B, 0x5A, 0xE4, 0x22, 0x6B,
        0xFA
};
static const uint8_t packet_out_21[] = {
        0x44, 0xA3, 0xAA, 0x3A, 0xAE, 0x64, 0x75, 0xCA,
        0xF2, 0xBE, 0xED, 0x7B, 0xC5, 0x09, 0x8E, 0x83,
        0xFE, 0xB5, 0xB3, 0x16, 0x08, 0xF8, 0xE2, 0x9C,
        0x38, 0x81, 0x9A, 0x89, 0xC8, 0xE7, 0x76, 0xF1,
        0x54, 0x4D, 0x41, 0x51, 0xA4, 0xED, 0x3A, 0x8B,
        0x87, 0xB9, 0xCE
};
#define clear_len_21 8
#define auth_len_21 10

/*
 *  =============== Packet Vector #22 ==================
 *  AES Key =  D7 82 8D 13  B2 B0 BD C3  25 A7 62 36  DF 93 CC 6B
 *  Nonce =    00 5B 8C CB  CD 9A F8 3C  96 96 76 6C  FA
 *  Total packet length = 31. [Input with 12 cleartext header octets]
 *             EC 46 BB 63  B0 25 20 C3  3C 49 FD 70  B9 6B 49 E2
 *             1D 62 17 41  63 28 75 DB  7F 6C 92 43  D2 D7 C2
 *  CBC IV in: 61 00 5B 8C  CB CD 9A F8  3C 96 96 76  6C FA 00 13
 *  CBC IV out:91 14 AD 06  B6 CC 02 35  76 9A B6 14  C4 82 95 03
 *  After xor: 91 18 41 40  0D AF B2 10  56 59 8A 5D  39 F2 95 03   [hdr]
 *  After AES: 29 BD 7C 27  83 E3 E8 D3  C3 5C 01 F4  4C EC BB FA
 *  After xor: 90 D6 35 C5  9E 81 FF 92  A0 74 74 2F  33 80 29 B9   [msg]
 *  After AES: 4E DA F4 0D  21 0B D4 5F  FE 97 90 B9  AA EC 34 4C
 *  After xor: 9C 0D 36 0D  21 0B D4 5F  FE 97 90 B9  AA EC 34 4C   [msg]
 *  After AES: 21 9E F8 90  EA 64 C2 11  A5 37 88 83  E1 BA 22 0D
 *  CBC-MAC  : 21 9E F8 90  EA 64 C2 11  A5 37
 *  CTR Start: 01 00 5B 8C  CB CD 9A F8  3C 96 96 76  6C FA 00 01
 *  CTR[0001]: 88 BC 19 42  80 C1 FA 3E  BE FC EF FB  4D C6 2D 54
 *  CTR[0002]: 3E 59 7D A5  AE 21 CC A4  00 9E 4C 0C  91 F6 22 49
 *  CTR[MAC ]: 5C BC 30 98  66 02 A9 F4  64 A0
 *  Total packet length = 41. [Authenticated and Encrypted Output]
 *             EC 46 BB 63  B0 25 20 C3  3C 49 FD 70  31 D7 50 A0
 *             9D A3 ED 7F  DD D4 9A 20  32 AA BF 17  EC 8E BF 7D
 *             22 C8 08 8C  66 6B E5 C1  97
 */
static const uint8_t keys_22[] = {
        0xD7, 0x82, 0x8D, 0x13, 0xB2, 0xB0, 0xBD, 0xC3,
        0x25, 0xA7, 0x62, 0x36, 0xDF, 0x93, 0xCC, 0x6B
};
static const uint8_t nonce_22[] = {
        0x00, 0x5B, 0x8C, 0xCB, 0xCD, 0x9A, 0xF8, 0x3C,
        0x96, 0x96, 0x76, 0x6C, 0xFA
};
static const uint8_t packet_in_22[] = {
        0xEC, 0x46, 0xBB, 0x63, 0xB0, 0x25, 0x20, 0xC3,
        0x3C, 0x49, 0xFD, 0x70, 0xB9, 0x6B, 0x49, 0xE2,
        0x1D, 0x62, 0x17, 0x41, 0x63, 0x28, 0x75, 0xDB,
        0x7F, 0x6C, 0x92, 0x43, 0xD2, 0xD7, 0xC2
};
static const uint8_t packet_out_22[] = {
        0xEC, 0x46, 0xBB, 0x63, 0xB0, 0x25, 0x20, 0xC3,
        0x3C, 0x49, 0xFD, 0x70, 0x31, 0xD7, 0x50, 0xA0,
        0x9D, 0xA3, 0xED, 0x7F, 0xDD, 0xD4, 0x9A, 0x20,
        0x32, 0xAA, 0xBF, 0x17, 0xEC, 0x8E, 0xBF, 0x7D,
        0x22, 0xC8, 0x08, 0x8C, 0x66, 0x6B, 0xE5, 0xC1,
        0x97
};
#define clear_len_22 12
#define auth_len_22 10


/*
 *  =============== Packet Vector #23 ==================
 *  AES Key =  D7 82 8D 13  B2 B0 BD C3  25 A7 62 36  DF 93 CC 6B
 *  Nonce =    00 3E BE 94  04 4B 9A 3C  96 96 76 6C  FA
 *  Total packet length = 32. [Input with 12 cleartext header octets]
 *             47 A6 5A C7  8B 3D 59 42  27 E8 5E 71  E2 FC FB B8
 *             80 44 2C 73  1B F9 51 67  C8 FF D7 89  5E 33 70 76
 *  CBC IV in: 61 00 3E BE  94 04 4B 9A  3C 96 96 76  6C FA 00 14
 *  CBC IV out:0F 70 3F 5A  54 2C 44 6E  8B 74 A3 73  9B 48 B9 61
 *  After xor: 0F 7C 78 FC  0E EB CF 53  D2 36 84 9B  C5 39 B9 61   [hdr]
 *  After AES: 40 5B ED 29  D0 98 AE 91  DB 68 78 F3  68 B8 73 85
 *  After xor: A2 A7 16 91  50 DC 82 E2  C0 91 29 94  A0 47 A4 0C   [msg]
 *  After AES: 3D 03 29 3C  FD 81 1B 37  01 51 FB C7  85 6B 7A 74
 *  After xor: 63 30 59 4A  FD 81 1B 37  01 51 FB C7  85 6B 7A 74   [msg]
 *  After AES: 66 4F 27 16  3E 36 0F 72  62 0D 4E 67  7C E0 61 DE
 *  CBC-MAC  : 66 4F 27 16  3E 36 0F 72  62 0D
 *  CTR Start: 01 00 3E BE  94 04 4B 9A  3C 96 96 76  6C FA 00 01
 *  CTR[0001]: 0A 7E 0A 63  53 C8 CF 9E  BC 3B 6E 63  15 9A D0 97
 *  CTR[0002]: EA 20 32 DA  27 82 6E 13  9E 1E 72 5C  5B 0D 3E BF
 *  CTR[MAC ]: B9 31 27 CA  F0 F1 A1 20  FA 70
 *  Total packet length = 42. [Authenticated and Encrypted Output]
 *             47 A6 5A C7  8B 3D 59 42  27 E8 5E 71  E8 82 F1 DB
 *             D3 8C E3 ED  A7 C2 3F 04  DD 65 07 1E  B4 13 42 AC
 *             DF 7E 00 DC  CE C7 AE 52  98 7D
 */
static const uint8_t keys_23[] = {
        0xD7, 0x82, 0x8D, 0x13, 0xB2, 0xB0, 0xBD, 0xC3,
        0x25, 0xA7, 0x62, 0x36, 0xDF, 0x93, 0xCC, 0x6B
};
static const uint8_t nonce_23[] = {
        0x00, 0x3E, 0xBE, 0x94, 0x04, 0x4B, 0x9A, 0x3C,
        0x96, 0x96, 0x76, 0x6C, 0xFA
};
static const uint8_t packet_in_23[] = {
        0x47, 0xA6, 0x5A, 0xC7, 0x8B, 0x3D, 0x59, 0x42,
        0x27, 0xE8, 0x5E, 0x71, 0xE2, 0xFC, 0xFB, 0xB8,
        0x80, 0x44, 0x2C, 0x73, 0x1B, 0xF9, 0x51, 0x67,
        0xC8, 0xFF, 0xD7, 0x89, 0x5E, 0x33, 0x70, 0x76
};
static const uint8_t packet_out_23[] = {
        0x47, 0xA6, 0x5A, 0xC7, 0x8B, 0x3D, 0x59, 0x42,
        0x27, 0xE8, 0x5E, 0x71, 0xE8, 0x82, 0xF1, 0xDB,
        0xD3, 0x8C, 0xE3, 0xED, 0xA7, 0xC2, 0x3F, 0x04,
        0xDD, 0x65, 0x07, 0x1E, 0xB4, 0x13, 0x42, 0xAC,
        0xDF, 0x7E, 0x00, 0xDC, 0xCE, 0xC7, 0xAE, 0x52,
        0x98, 0x7D
};
#define clear_len_23 12
#define auth_len_23 10

/*
 *  =============== Packet Vector #24 ==================
 *  AES Key =  D7 82 8D 13  B2 B0 BD C3  25 A7 62 36  DF 93 CC 6B
 *  Nonce =    00 8D 49 3B  30 AE 8B 3C  96 96 76 6C  FA
 *  Total packet length = 33. [Input with 12 cleartext header octets]
 *             6E 37 A6 EF  54 6D 95 5D  34 AB 60 59  AB F2 1C 0B
 *             02 FE B8 8F  85 6D F4 A3  73 81 BC E3  CC 12 85 17
 *             D4
 *  CBC IV in: 61 00 8D 49  3B 30 AE 8B  3C 96 96 76  6C FA 00 15
 *  CBC IV out:67 AC E4 E8  06 77 7A D3  27 1D 0B 93  4C 67 98 15
 *  After xor: 67 A0 8A DF  A0 98 2E BE  B2 40 3F 38  2C 3E 98 15   [hdr]
 *  After AES: 35 58 F8 7E  CA C2 B4 39  B6 7E 75 BB  F1 5E 69 08
 *  After xor: 9E AA E4 75  C8 3C 0C B6  33 13 81 18  82 DF D5 EB   [msg]
 *  After AES: 54 E4 7B 62  22 F0 BB 87  17 D0 71 6A  EB AF 19 9E
 *  After xor: 98 F6 FE 75  F6 F0 BB 87  17 D0 71 6A  EB AF 19 9E   [msg]
 *  After AES: 23 E3 30 50  BC 57 DC 2C  3D 3E 7C 94  77 D1 49 71
 *  CBC-MAC  : 23 E3 30 50  BC 57 DC 2C  3D 3E
 *  CTR Start: 01 00 8D 49  3B 30 AE 8B  3C 96 96 76  6C FA 00 01
 *  CTR[0001]: 58 DB 19 B3  88 9A A3 8B  3C A4 0B 16  FF 42 2C 73
 *  CTR[0002]: C3 2F 24 3D  65 DC 7E 9F  4B 02 16 AB  7F B9 6B 4D
 *  CTR[MAC ]: 4E 2D AE D2  53 F6 B1 8A  1D 67
 *  Total packet length = 43. [Authenticated and Encrypted Output]
 *             6E 37 A6 EF  54 6D 95 5D  34 AB 60 59  F3 29 05 B8
 *             8A 64 1B 04  B9 C9 FF B5  8C C3 90 90  0F 3D A1 2A
 *             B1 6D CE 9E  82 EF A1 6D  A6 20 59
 */
static const uint8_t keys_24[] = {
        0xD7, 0x82, 0x8D, 0x13, 0xB2, 0xB0, 0xBD, 0xC3,
        0x25, 0xA7, 0x62, 0x36, 0xDF, 0x93, 0xCC, 0x6B
};
static const uint8_t nonce_24[] = {
        0x00, 0x8D, 0x49, 0x3B, 0x30, 0xAE, 0x8B, 0x3C,
        0x96, 0x96, 0x76, 0x6C, 0xFA
};
static const uint8_t packet_in_24[] = {
        0x6E, 0x37, 0xA6, 0xEF, 0x54, 0x6D, 0x95, 0x5D,
        0x34, 0xAB, 0x60, 0x59, 0xAB, 0xF2, 0x1C, 0x0B,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0xD4
};
static const uint8_t packet_out_24[] = {
        0x6E, 0x37, 0xA6, 0xEF, 0x54, 0x6D, 0x95, 0x5D,
        0x34, 0xAB, 0x60, 0x59, 0xF3, 0x29, 0x05, 0xB8,
        0x8A, 0x64, 0x1B, 0x04, 0xB9, 0xC9, 0xFF, 0xB5,
        0x8C, 0xC3, 0x90, 0x90, 0x0F, 0x3D, 0xA1, 0x2A,
        0xB1, 0x6D, 0xCE, 0x9E, 0x82, 0xEF, 0xA1, 0x6D,
        0xA6, 0x20, 0x59
};
#define clear_len_24 12
#define auth_len_24 10

/*
 *  =============== Packet Vector #25 ==================
 *  AES Key =  D7 82 8D 13  B2 B0 BD C3  25 A7 62 36  DF 93 CC 6B
 *  Nonce =    00 8D 49 3B  30 AE 8B 3C  96 96 76 6C  FA
 *  Total packet length = 33. [Input with 12 cleartext header octets]
 *             6E 37 A6 EF  54 6D 95 5D  34 AB 60 59  AB F2 1C 0B
 *             02 FE B8 8F  85 6D F4 A3  73 81 BC E3  CC 12 85 17
 *             D4
 *  CBC IV in: 61 00 8D 49  3B 30 AE 8B  3C 96 96 76  6C FA 00 15
 *  CBC IV out:67 AC E4 E8  06 77 7A D3  27 1D 0B 93  4C 67 98 15
 *  After xor: 67 A0 8A DF  A0 98 2E BE  B2 40 3F 38  2C 3E 98 15   [hdr]
 *  After AES: 35 58 F8 7E  CA C2 B4 39  B6 7E 75 BB  F1 5E 69 08
 *  After xor: 9E AA E4 75  C8 3C 0C B6  33 13 81 18  82 DF D5 EB   [msg]
 *  After AES: 54 E4 7B 62  22 F0 BB 87  17 D0 71 6A  EB AF 19 9E
 *  After xor: 98 F6 FE 75  F6 F0 BB 87  17 D0 71 6A  EB AF 19 9E   [msg]
 *  After AES: 23 E3 30 50  BC 57 DC 2C  3D 3E 7C 94  77 D1 49 71
 *  CBC-MAC  : 23 E3 30 50  BC 57 DC 2C  3D 3E
 *  CTR Start: 01 00 8D 49  3B 30 AE 8B  3C 96 96 76  6C FA 00 01
 *  CTR[0001]: 58 DB 19 B3  88 9A A3 8B  3C A4 0B 16  FF 42 2C 73
 *  CTR[0002]: C3 2F 24 3D  65 DC 7E 9F  4B 02 16 AB  7F B9 6B 4D
 *  CTR[MAC ]: 4E 2D AE D2  53 F6 B1 8A  1D 67
 *  Total packet length = 43. [Authenticated and Encrypted Output]
 *             6E 37 A6 EF  54 6D 95 5D  34 AB 60 59  F3 29 05 B8
 *             8A 64 1B 04  B9 C9 FF B5  8C C3 90 90  0F 3D A1 2A
 *             B1 6D CE 9E  82 EF A1 6D  A6 20 59
 */
static const uint8_t keys_25[] = {
        0xD7, 0x82, 0x8D, 0x13, 0xB2, 0xB0, 0xBD, 0xC3,
        0x25, 0xA7, 0x62, 0x36, 0xDF, 0x93, 0xCC, 0x6B
};
static const uint8_t nonce_25[] = {
        0x00, 0x8D, 0x49, 0x3B, 0x30, 0xAE, 0x8B, 0x3C,
        0x96, 0x96, 0x76, 0x6C, 0xFA
};
static const uint8_t packet_in_25[] = {
        0x6E, 0x37, 0xA6, 0xEF, 0x54, 0x6D, 0x95, 0x5D,
        0x34, 0xAB, 0x60, 0x59, 0xAB, 0xF2, 0x1C, 0x0B,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
        0x73, 0x81, 0xBC, 0xE3, 0xCC, 0x12, 0x85, 0x17,
        0x02, 0xFE, 0xB8, 0x8F, 0x85, 0x6D, 0xF4, 0xA3,
};
static const uint8_t packet_out_25[] = {
        0x6E, 0x37, 0xA6, 0xEF, 0x54, 0x6D, 0x95, 0x5D,
        0x34, 0xAB, 0x60, 0x59,
        0xF3, 0x29, 0x05, 0xB8, 0x8A, 0x64, 0x1B, 0x04,
        0xB9, 0xC9, 0xFF, 0xB5, 0x8C, 0xC3, 0x90, 0x90,
        0x0F, 0x3D, 0xA1, 0x2A, 0x67, 0x22, 0xC6, 0x10,
        0xCE, 0x6F, 0xE2, 0x08, 0x0C, 0x38, 0xD7, 0xAE,
        0x61, 0x82, 0x21, 0x3D, 0x59, 0x67, 0xF3, 0x26,
        0x72, 0x53, 0x78, 0x37, 0xE4, 0xBA, 0xA7, 0xED,
        0x92, 0x52, 0xD6, 0x01, 0xA5, 0x66, 0x52, 0x20,
        0x6C, 0x51, 0xDB, 0x15, 0x6C, 0xF8, 0x59, 0x38,
        0xAF, 0x28, 0x4D, 0xB7, 0x5F, 0x2A, 0xC8, 0xB0,
        0x6E, 0x37, 0x77, 0x89, 0xB0, 0x6D, 0xC3, 0xE5,
        0xF2, 0x2A, 0xA6, 0xF7, 0xDB, 0xCC, 0x37, 0x79,
        0x88, 0x0E, 0x8F, 0x05, 0xA9, 0xE1, 0x9E, 0x11,
        0x90, 0x3D, 0x2A, 0xB9, 0x70, 0x13, 0x9D, 0xC3,
        0x93, 0xAD, 0xE2, 0x5D, 0xFF, 0xDD, 0x19, 0x0F,
        0xB9, 0x9E, 0x74, 0x88, 0xAB, 0x82, 0x0C, 0xA0,
        0x2E, 0x71, 0xA6, 0x32, 0xEB, 0xA4, 0xBA, 0x10,
        0xF6, 0x61, 0x7B, 0x1B, 0x2A, 0x38, 0x80, 0xEB,
        0xE9, 0x09, 0x01, 0x33, 0x94, 0x8F, 0xB3, 0xD4,
        0xAD, 0x6C, 0xD8, 0x5E, 0x85, 0x98, 0xC5, 0x9C,
        0x11, 0x62, 0x2C, 0x60, 0x32, 0xAE, 0x70, 0xE1,
        0x66, 0x73, 0x09, 0x1E, 0x20, 0x55, 0xB7, 0x20,
        0x77, 0x86, 0x09, 0xC8, 0x1C, 0xFE, 0x86, 0xA7,
        0x08, 0x40, 0x43, 0xE7, 0xAD, 0xB2, 0x5B, 0x39,
        0x64, 0xCB, 0x13, 0x1F, 0x8D, 0xD2, 0x4F, 0xCC,
        0xC5, 0xAA, 0xF1, 0xD6, 0x31, 0xFC, 0x34, 0x9E,
        0x5F, 0x90, 0xC4, 0xB7, 0xE0, 0x07, 0x9C, 0xCD,
        0xFB, 0xEA, 0xE3, 0x75, 0xB5, 0x7B, 0x29, 0xD4,
        0x73, 0x81, 0xEF, 0x9C, 0x2E, 0xAC, 0xF9, 0xA7,
        0x39, 0x2A, 0xF8, 0xE2, 0xEA, 0x3A, 0x6A, 0xDF,
        0xD0, 0x3A, 0xCA, 0x29, 0xD3, 0x13, 0x13, 0x9A,
        0x2C, 0x70, 0xA4, 0xA9, 0x40, 0x1D, 0xEC, 0xC7,
        0xC9, 0x6B, 0xF7, 0x23, 0xD4, 0x53, 0x49, 0xD0,
        0x05, 0xCE, 0x15, 0x65, 0xCE, 0x1F, 0x89, 0xE2,
        0xBE, 0xE0, 0xFA, 0x3F, 0x59, 0x4A, 0x89, 0x99,
        0xE5, 0xDB, 0xA0, 0xE8, 0x54, 0x72, 0x42, 0x69,
        0x79, 0x63, 0x68, 0x91, 0xC9, 0x2C, 0xFC, 0x58,
        0xD4, 0x30, 0xFE, 0xE3, 0x62, 0x5F, 0xDC, 0x49,
        0xEF, 0x32, 0x58, 0x83, 0x27, 0xA9, 0xED, 0xEC,
        0xF3, 0x1D, 0xFB, 0xEA, 0x0A, 0x89
};
#define clear_len_25 12
#define auth_len_25 10

/** Additional AES-CCM-128 test vectors */
static const uint8_t keys_90[] = {
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F
};
static const uint8_t nonce_90[] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};
static const uint8_t packet_in_90[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x20, 0x21, 0x22, 0x23
};
static const uint8_t packet_out_90[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x71, 0x62, 0x01, 0x5B,
        0x4D, 0xAC, 0x25, 0x5D
};
#define clear_len_90 8
#define auth_len_90 4

static const uint8_t keys_91[] = {
        0xC9, 0x7C, 0x1F, 0x67, 0xCE, 0x37, 0x11, 0x85,
        0x51, 0x4A, 0x8A, 0x19, 0xF2, 0xBD, 0xD5, 0x2F
};
static const uint8_t nonce_91[] = {
        0x00, 0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xB5,
        0x03, 0x97, 0x76, 0xE7, 0x0C
};
static const uint8_t packet_in_91[] = {
        0x08, 0x40, 0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C,
        0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xAB, 0xAE,
        0xA5, 0xB8, 0xFC, 0xBA, 0x00, 0x00,
        0xF8, 0xBA, 0x1A, 0x55, 0xD0, 0x2F, 0x85, 0xAE,
        0x96, 0x7B, 0xB6, 0x2F, 0xB6, 0xCD, 0xA8, 0xEB,
        0x7E, 0x78, 0xA0, 0x50
};
static const uint8_t packet_out_91[] = {
        0x08, 0x40, 0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C,
        0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xAB, 0xAE,
        0xA5, 0xB8, 0xFC, 0xBA, 0x00, 0x00,
        0xF3, 0xD0, 0xA2, 0xFE, 0x9A, 0x3D, 0xBF, 0x23,
        0x42, 0xA6, 0x43, 0xE4, 0x32, 0x46, 0xE8, 0x0C,
        0x3C, 0x04, 0xD0, 0x19,
        0x78, 0x45, 0xCE, 0x0B, 0x16, 0xF9, 0x76, 0x23
};
#define clear_len_91 22
#define auth_len_91  8


static const uint8_t keys_92[] = {
        0xC9, 0x7C, 0x1F, 0x67, 0xCE, 0x37, 0x11, 0x85,
        0x51, 0x4A, 0x8A, 0x19, 0xF2, 0xBD, 0xD5, 0x2F
};
static const uint8_t nonce_92[] = {
        0x00, 0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xB5,
        0x03, 0x97, 0x76, 0xE7, 0x0C
};
static const uint8_t packet_in_92[] = {
        0xF8, 0xBA, 0x1A, 0x55, 0xD0, 0x2F, 0x85, 0xAE,
        0x96, 0x7B, 0xB6, 0x2F, 0xB6, 0xCD, 0xA8, 0xEB,
        0x7E, 0x78, 0xA0, 0x50
};
static const uint8_t packet_out_92[] = {
        0xF3, 0xD0, 0xA2, 0xFE, 0x9A, 0x3D, 0xBF, 0x23,
        0x42, 0xA6, 0x43, 0xE4, 0x32, 0x46, 0xE8, 0x0C,
        0x3C, 0x04, 0xD0, 0x19,
        0x41, 0x83, 0x21, 0x89, 0xA3, 0xD3, 0x1B, 0x43
};
#define clear_len_92 0
#define auth_len_92  8

static const uint8_t keys_100[] = {
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F
};
static const uint8_t nonce_100[] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};
static const uint8_t packet_in_100[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x20, 0x21, 0x22, 0x23,
};
static const uint8_t packet_out_100[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x71, 0x62, 0x01, 0x5B,
        0xB0, 0xC9, 0x5E, 0x58, 0x03, 0x6E
};
#define clear_len_100 8
#define auth_len_100  6

static const uint8_t keys_101[] = {
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F
};
static const uint8_t nonce_101[] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};
static const uint8_t packet_in_101[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x20, 0x21, 0x22, 0x23,
};
static const uint8_t packet_out_101[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x71, 0x62, 0x01, 0x5B,
        0xD0, 0xAD, 0x86, 0xFD, 0x33, 0xC2, 0x69, 0x86
};
#define clear_len_101 8
#define auth_len_101  8

static const uint8_t keys_102[] = {
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F
};
static const uint8_t nonce_102[] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};
static const uint8_t packet_in_102[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x20, 0x21, 0x22, 0x23,
};
static const uint8_t packet_out_102[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x71, 0x62, 0x01, 0x5B,
        0x05, 0x12, 0xDA, 0xBF, 0xD9, 0x72, 0xA6, 0x68,
        0x53, 0xC1
};
#define clear_len_102 8
#define auth_len_102  10

static const uint8_t keys_103[] = {
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F
};
static const uint8_t nonce_103[] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};
static const uint8_t packet_in_103[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x20, 0x21, 0x22, 0x23,
};
static const uint8_t packet_out_103[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x71, 0x62, 0x01, 0x5B,
        0xBA, 0x03, 0xBF, 0x8C, 0xE0, 0xD6, 0x00, 0xA4,
        0x48, 0x6F, 0xCC, 0xB3
};
#define clear_len_103 8
#define auth_len_103  12

static const uint8_t keys_104[] = {
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F
};
static const uint8_t nonce_104[] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};
static const uint8_t packet_in_104[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x20, 0x21, 0x22, 0x23,
};
static const uint8_t packet_out_104[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x71, 0x62, 0x01, 0x5B,
        0x6B, 0x9B, 0xFB, 0xFE, 0xA8, 0x2C, 0x04, 0x77,
        0x8E, 0x67, 0xF5, 0x18, 0x46, 0xC6
};
#define clear_len_104 8
#define auth_len_104  14

static const uint8_t keys_105[] = {
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F
};
static const uint8_t nonce_105[] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};
static const uint8_t packet_in_105[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x20, 0x21, 0x22, 0x23,
};
static const uint8_t packet_out_105[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x71, 0x62, 0x01, 0x5B,
        0x2B, 0xB5, 0x7C, 0x0A, 0xF4, 0x5E, 0x4D, 0x83,
        0x04, 0xF0, 0x5F, 0x45, 0x99, 0x3F, 0x15, 0x17
};
#define clear_len_105 8
#define auth_len_105  16

static const uint8_t keys_106[] = {
        0x4a, 0xe7, 0x01, 0x10, 0x3c, 0x63, 0xde, 0xca,
        0x5b, 0x5a, 0x39, 0x39, 0xd7, 0xd0, 0x59, 0x92
};
static const uint8_t nonce_106[] = {
        0x5a, 0x8a, 0xa4, 0x85, 0xc3, 0x16, 0xe9
};
static const uint8_t packet_out_106[] = {
        0x02, 0x20, 0x9f, 0x55
};
#define clear_len_106 0
#define auth_len_106  4

static const uint8_t keys_107[] = {
        0xC9, 0x7C, 0x1F, 0x67, 0xCE, 0x37, 0x11, 0x85,
        0x51, 0x4A, 0x8A, 0x19, 0xF2, 0xBD, 0xD5, 0x2F
};
static const uint8_t nonce_107[] = {
        0x00, 0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xB5,
        0x03, 0x97, 0x76, 0xE7, 0x0C
};
static const uint8_t packet_in_107[] = {
        0x08, 0x40, 0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C,
        0x50, 0x30, 0xF1, 0x84, 0x44, 0x08,
        0xF8, 0xBA, 0x1A, 0x55, 0xD0, 0x2F, 0x85, 0xAE,
        0x96, 0x7B, 0xB6, 0x2F, 0xB6, 0xCD, 0xA8, 0xEB,
        0x7E, 0x78, 0xA0, 0x50
};
static const uint8_t packet_out_107[] = {
        0x08, 0x40, 0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C,
        0x50, 0x30, 0xF1, 0x84, 0x44, 0x08,
        0xF3, 0xD0, 0xA2, 0xFE, 0x9A, 0x3D, 0xBF, 0x23,
        0x42, 0xA6, 0x43, 0xE4, 0x32, 0x46, 0xE8, 0x0C,
        0x3C, 0x04, 0xD0, 0x19,
        0x60, 0x76, 0xE8, 0xE2, 0x0C, 0x0A, 0xF6, 0xDF
};
#define clear_len_107 14
#define auth_len_107  8

static const uint8_t keys_108[] = {
        0xC9, 0x7C, 0x1F, 0x67, 0xCE, 0x37, 0x11, 0x85,
        0x51, 0x4A, 0x8A, 0x19, 0xF2, 0xBD, 0xD5, 0x2F
};
static const uint8_t nonce_108[] = {
        0x00, 0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xB5,
        0x03, 0x97, 0x76, 0xE7, 0x0C
};
static const uint8_t packet_in_108[] = {
        0x08, 0x40, 0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C,
        0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xAB, 0xAE,
        0xA5, 0xB8, 0xFC, 0xBA, 0x00, 0x00, 0x08, 0x40,
        0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C, 0x50, 0x30,
        0xF8, 0xBA, 0x1A, 0x55, 0xD0, 0x2F, 0x85, 0xAE,
        0x96, 0x7B, 0xB6, 0x2F, 0xB6, 0xCD, 0xA8, 0xEB,
        0x7E, 0x78, 0xA0, 0x50
};
static const uint8_t packet_out_108[] = {
        0x08, 0x40, 0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C,
        0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xAB, 0xAE,
        0xA5, 0xB8, 0xFC, 0xBA, 0x00, 0x00, 0x08, 0x40,
        0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C, 0x50, 0x30,
        0xF3, 0xD0, 0xA2, 0xFE, 0x9A, 0x3D, 0xBF, 0x23,
        0x42, 0xA6, 0x43, 0xE4, 0x32, 0x46, 0xE8, 0x0C,
        0x3C, 0x04, 0xD0, 0x19,
        0x35, 0x0D, 0xA5, 0xAA, 0x1E, 0x71, 0x82, 0x35
};
#define clear_len_108 32
#define auth_len_108  8

static const uint8_t keys_109[] = {
        0xC9, 0x7C, 0x1F, 0x67, 0xCE, 0x37, 0x11, 0x85,
        0x51, 0x4A, 0x8A, 0x19, 0xF2, 0xBD, 0xD5, 0x2F
};
static const uint8_t nonce_109[] = {
        0x00, 0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xB5,
        0x03, 0x97, 0x76, 0xE7, 0x0C
};
static const uint8_t packet_in_109[] = {
        0x08, 0x40, 0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C,
        0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xAB, 0xAE,
        0xA5, 0xB8, 0xFC, 0xBA, 0x00, 0x00, 0x08, 0x40,
        0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C, 0x50, 0x30,
        0x00, 0x01, 0x02, 0x03,
        0xF8, 0xBA, 0x1A, 0x55, 0xD0, 0x2F, 0x85, 0xAE,
        0x96, 0x7B, 0xB6, 0x2F, 0xB6, 0xCD, 0xA8, 0xEB,
        0x7E, 0x78, 0xA0, 0x50
};
static const uint8_t packet_out_109[] = {
        0x08, 0x40, 0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C,
        0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xAB, 0xAE,
        0xA5, 0xB8, 0xFC, 0xBA, 0x00, 0x00, 0x08, 0x40,
        0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C, 0x50, 0x30,
        0x00, 0x01, 0x02, 0x03,
        0xF3, 0xD0, 0xA2, 0xFE, 0x9A, 0x3D, 0xBF, 0x23,
        0x42, 0xA6, 0x43, 0xE4, 0x32, 0x46, 0xE8, 0x0C,
        0x3C, 0x04, 0xD0, 0x19,
        0x26, 0x5A, 0x04, 0xB1, 0x56, 0xFF, 0x9F, 0x0E
};
#define clear_len_109 36
#define auth_len_109  8

static const uint8_t keys_110[] = {
        0xC9, 0x7C, 0x1F, 0x67, 0xCE, 0x37, 0x11, 0x85,
        0x51, 0x4A, 0x8A, 0x19, 0xF2, 0xBD, 0xD5, 0x2F
};
static const uint8_t nonce_110[] = {
        0x00, 0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xB5,
        0x03, 0x97, 0x76, 0xE7, 0x0C
};
static const uint8_t packet_in_110[] = {
        0x08, 0x40, 0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C,
        0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xAB, 0xAE,
        0xA5, 0xB8, 0xFC, 0xBA, 0x00, 0x00, 0x08, 0x40,
        0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C, 0x50, 0x30,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
        0xF8, 0xBA, 0x1A, 0x55, 0xD0, 0x2F, 0x85, 0xAE,
        0x96, 0x7B, 0xB6, 0x2F, 0xB6, 0xCD, 0xA8, 0xEB,
        0x7E, 0x78, 0xA0, 0x50
};
static const uint8_t packet_out_110[] = {
        0x08, 0x40, 0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C,
        0x50, 0x30, 0xF1, 0x84, 0x44, 0x08, 0xAB, 0xAE,
        0xA5, 0xB8, 0xFC, 0xBA, 0x00, 0x00, 0x08, 0x40,
        0x0F, 0xD2, 0xE1, 0x28, 0xA5, 0x7C, 0x50, 0x30,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
        0xF3, 0xD0, 0xA2, 0xFE, 0x9A, 0x3D, 0xBF, 0x23,
        0x42, 0xA6, 0x43, 0xE4, 0x32, 0x46, 0xE8, 0x0C,
        0x3C, 0x04, 0xD0, 0x19,
        0x07, 0x55, 0x13, 0x40, 0x2B, 0x11, 0x6D, 0xD5
};
#define clear_len_110 46
#define auth_len_110  8

#define CCM_TEST_VEC(num)                                               \
        { keys_##num, nonce_##num, sizeof(nonce_##num),                 \
                        packet_in_##num, sizeof(packet_in_##num),       \
                        clear_len_##num, packet_out_##num,              \
                        auth_len_##num }
#define CCM_TEST_VEC_2(num)                                             \
        { keys_##num, nonce_##num, sizeof(nonce_##num),                 \
                        NULL, 0,                                        \
                        clear_len_##num, packet_out_##num,              \
                        auth_len_##num }

static const struct ccm_rfc3610_vector {
        const uint8_t *keys;
        const uint8_t *nonce;
        const size_t nonce_len;
        /* packet in = [ AAD | plain text ] */
        const uint8_t *packet_in;
        const size_t packet_len;
        const size_t clear_len;
        /* packet out = [ AAD | cipher text | authentication tag ] */
        const uint8_t *packet_out;
        const size_t auth_len;
} ccm_vectors[] = {
        CCM_TEST_VEC(01),
        CCM_TEST_VEC(02),
        CCM_TEST_VEC(03),
        CCM_TEST_VEC(04),
        CCM_TEST_VEC(05),
        CCM_TEST_VEC(06),
        CCM_TEST_VEC(07),
        CCM_TEST_VEC(08),
        CCM_TEST_VEC(09),
        CCM_TEST_VEC(10),
        CCM_TEST_VEC(11),
        CCM_TEST_VEC(12),
        CCM_TEST_VEC(13),
        CCM_TEST_VEC(14),
        CCM_TEST_VEC(15),
        CCM_TEST_VEC(16),
        CCM_TEST_VEC(17),
        CCM_TEST_VEC(18),
        CCM_TEST_VEC(19),
        CCM_TEST_VEC(20),
        CCM_TEST_VEC(21),
        CCM_TEST_VEC(22),
        CCM_TEST_VEC(23),
        CCM_TEST_VEC(24),
        CCM_TEST_VEC(25),
        CCM_TEST_VEC(90),
        CCM_TEST_VEC(91),
        CCM_TEST_VEC(92),
        CCM_TEST_VEC(100),
        CCM_TEST_VEC(101),
        CCM_TEST_VEC(102),
        CCM_TEST_VEC(103),
        CCM_TEST_VEC(104),
        CCM_TEST_VEC(105),
        CCM_TEST_VEC_2(106),
        CCM_TEST_VEC(107),
        CCM_TEST_VEC(108),
        CCM_TEST_VEC(109),
        CCM_TEST_VEC(110)
};

static int
ccm_job_ok(const struct ccm_rfc3610_vector *vec,
           const struct JOB_AES_HMAC *job,
           const uint8_t *target,
           const uint8_t *padding,
           const uint8_t *auth,
           const size_t sizeof_padding,
           const int dir,
           const int in_place)
{
        if (job->status != STS_COMPLETED) {
                printf("%d Error status:%d", __LINE__, job->status);
                return 0;
        }

        /* cipher checks */
        if (in_place) {
                if (dir == ENCRYPT) {
                        if (memcmp(vec->packet_out, target + sizeof_padding,
                                   vec->packet_len)) {
                                printf("cipher mismatched\n");
                                hexdump(stderr, "Received",
                                        target + sizeof_padding,
                                        vec->packet_len);
                                hexdump(stderr, "Expected",
                                        vec->packet_out, vec->packet_len);
                                return 0;
                        }
                } else {
                        if (memcmp(vec->packet_in, target + sizeof_padding,
                                   vec->packet_len)) {
                                printf("cipher mismatched\n");
                                hexdump(stderr, "Received",
                                        target + sizeof_padding,
                                        vec->packet_len);
                                hexdump(stderr, "Expected", vec->packet_in,
                                        vec->packet_len);
                                return 0;
                        }
                }
        } else { /* out-of-place */
                if (dir == ENCRYPT) {
                        if (memcmp(vec->packet_out + vec->clear_len,
                                   target + sizeof_padding,
                                   vec->packet_len - vec->clear_len)) {
                                printf("cipher mismatched\n");
                                hexdump(stderr, "Received",
                                        target + sizeof_padding,
                                        vec->packet_len - vec->clear_len);
                                hexdump(stderr, "Expected",
                                        vec->packet_out + vec->clear_len,
                                        vec->packet_len - vec->clear_len);
                                return 0;
                        }
                } else {
                        if (memcmp(vec->packet_in + vec->clear_len,
                                   target + sizeof_padding,
                                   vec->packet_len - vec->clear_len)) {
                                printf("cipher mismatched\n");
                                hexdump(stderr, "Received",
                                        target + sizeof_padding,
                                        vec->packet_len - vec->clear_len);
                                hexdump(stderr, "Expected",
                                        vec->packet_in + vec->clear_len,
                                        vec->packet_len - vec->clear_len);
                                return 0;
                        }
                }
        }

        if (memcmp(padding, target, sizeof_padding)) {
                printf("cipher overwrite head\n");
                hexdump(stderr, "Target", target, sizeof(padding));
                return 0;
        }

        if (in_place) {
                if (memcmp(padding, target + sizeof_padding + vec->packet_len,
                           sizeof_padding)) {
                        printf("cipher overwrite tail\n");
                        hexdump(stderr, "Target",
                                target + sizeof_padding + vec->packet_len,
                                sizeof_padding);
                        return 0;
                }
        } else {
                if (memcmp(padding, target + sizeof_padding + vec->packet_len -
                           vec->clear_len, sizeof_padding)) {
                        printf("cipher overwrite tail\n");
                        hexdump(stderr, "Target", target + sizeof_padding +
                                vec->packet_len - vec->clear_len,
                                sizeof_padding);
                        return 0;
                }
        }

        /* hash checks */
        if (memcmp(padding, &auth[sizeof_padding + vec->auth_len],
                   sizeof_padding)) {
                printf("hash overwrite tail\n");
                hexdump(stderr, "Target",
                        &auth[sizeof_padding + vec->auth_len], sizeof_padding);
                return 0;
        }

        if (memcmp(padding, &auth[0], sizeof_padding)) {
                printf("hash overwrite head\n");
                hexdump(stderr, "Target", &auth[0], sizeof_padding);
                return 0;
        }

        if (memcmp(vec->packet_out + vec->packet_len, &auth[sizeof_padding],
                   vec->auth_len)) {
                printf("hash mismatched\n");
                hexdump(stderr, "Received", &auth[sizeof_padding],
                        vec->auth_len);
                hexdump(stderr, "Expected", vec->packet_out + vec->packet_len,
                        vec->auth_len);
                return 0;
        }
        return 1;
}

static int
test_ccm(struct MB_MGR *mb_mgr,
         const struct ccm_rfc3610_vector *vec,
         const int dir, const int in_place, const int num_jobs)
{
        DECLARE_ALIGNED(uint32_t expkey[4*15], 16);
        DECLARE_ALIGNED(uint32_t dust[4*15], 16);
        struct JOB_AES_HMAC *job;
        uint8_t padding[16];
        uint8_t **targets = malloc(num_jobs * sizeof(void *));
        uint8_t **auths = malloc(num_jobs * sizeof(void *));
        int i = 0, jobs_rx = 0, ret = -1;
        const int order = (dir == ENCRYPT) ? HASH_CIPHER : CIPHER_HASH;

        if (targets == NULL || auths == NULL) {
		fprintf(stderr, "Can't allocate buffer memory\n");
		goto end2;
        }

        memset(padding, -1, sizeof(padding));
        memset(targets, 0, num_jobs * sizeof(void *));
        memset(auths, 0, num_jobs * sizeof(void *));

        for (i = 0; i < num_jobs; i++) {
                targets[i] = malloc(vec->packet_len + (sizeof(padding) * 2));
                auths[i] = malloc(16 + (sizeof(padding) * 2));
                if (targets[i] == NULL || auths[i] == NULL) {
                        fprintf(stderr, "Can't allocate buffer memory\n");
                        goto end;
                }

                memset(targets[i], -1, vec->packet_len + (sizeof(padding) * 2));
                memset(auths[i], -1, 16 + (sizeof(padding) * 2));

                if (in_place) {
                        if (dir == ENCRYPT)
                                memcpy(targets[i] + sizeof(padding),
                                       vec->packet_in, vec->packet_len);
                        else
                                memcpy(targets[i] + sizeof(padding),
                                       vec->packet_out, vec->packet_len);
                }
        }

        IMB_AES_KEYEXP_128(mb_mgr, vec->keys, expkey, dust);

        while ((job = IMB_FLUSH_JOB(mb_mgr)) != NULL)
                ;

        for (i = 0; i < num_jobs; i++) {
                job = IMB_GET_NEXT_JOB(mb_mgr);
                job->cipher_direction = dir;
                job->chain_order = order;
                if (in_place) {
                        job->dst =
                                targets[i] + sizeof(padding) + vec->clear_len;
                        job->src = targets[i] + sizeof(padding);
                } else {
                        if (dir == ENCRYPT) {
                                job->dst = targets[i] + sizeof(padding);
                                job->src = vec->packet_in;
                        } else {
                                job->dst = targets[i] + sizeof(padding);
                                job->src = vec->packet_out;
                        }
                }
                job->cipher_mode = CCM;
                job->aes_enc_key_expanded = expkey;
                job->aes_dec_key_expanded = expkey;
                job->aes_key_len_in_bytes = 16; /* AES-CCM-128 for now */
                job->iv = vec->nonce;
                job->iv_len_in_bytes = vec->nonce_len;
                job->cipher_start_src_offset_in_bytes = vec->clear_len;
                job->msg_len_to_cipher_in_bytes =
                        vec->packet_len - vec->clear_len;

                job->hash_alg = AES_CCM;
                job->hash_start_src_offset_in_bytes = vec->clear_len;
                job->msg_len_to_hash_in_bytes =
                        vec->packet_len - vec->clear_len;
                job->auth_tag_output = auths[i] + sizeof(padding);
                job->auth_tag_output_len_in_bytes = vec->auth_len;

                job->u.CCM.aad_len_in_bytes = vec->clear_len;
                job->u.CCM.aad = job->src;

                job->user_data = targets[i];
                job->user_data2 = auths[i];

                job = IMB_SUBMIT_JOB(mb_mgr);
                if (job) {
                        jobs_rx++;
                        if (num_jobs < 4) {
                                printf("%d Unexpected return from submit_job\n",
                                       __LINE__);
                                goto end;
                        }
                        if (!ccm_job_ok(vec, job, job->user_data, padding,
                                        job->user_data2, sizeof(padding),
                                        dir, in_place))
                                goto end;
                }
        }

        while ((job = IMB_FLUSH_JOB(mb_mgr)) != NULL) {
                jobs_rx++;

                if (!ccm_job_ok(vec, job, job->user_data, padding,
                                job->user_data2, sizeof(padding), dir,
                                in_place))
                        goto end;
        }

        if (jobs_rx != num_jobs) {
                printf("Expected %d jobs, received %d\n", num_jobs, jobs_rx);
                goto end;
        }
        ret = 0;

 end:
        while ((job = IMB_FLUSH_JOB(mb_mgr)) != NULL)
                ;

        for (i = 0; i < num_jobs; i++) {
                if (targets[i] != NULL)
                        free(targets[i]);
                if (auths[i] != NULL)
                        free(auths[i]);
        }

 end2:
        if (targets != NULL)
                free(targets);

        if (auths != NULL)
                free(auths);

        return ret;
}

static int
test_ccm_std_vectors(struct MB_MGR *mb_mgr, const int num_jobs)
{
	const int vectors_cnt = sizeof(ccm_vectors) / sizeof(ccm_vectors[0]);
	int vect;
	int errors = 0;

	printf("AES-CCM standard test vectors (N jobs = %d):\n", num_jobs);
	for (vect = 1; vect <= vectors_cnt; vect++) {
                const int idx = vect - 1;
#ifdef DEBUG
		printf("Standard vector [%d/%d] NONCELen:%d PktLen:%d "
                       "AADLen:%d AUTHlen:%d\n",
                       vect, vectors_cnt,
                       (int) ccm_vectors[idx].nonce_len,
                       (int) ccm_vectors[idx].packet_len,
                       (int) ccm_vectors[idx].clear_len,
                       (int) ccm_vectors[idx].auth_len);
#else
		printf(".");
#endif

                if (test_ccm(mb_mgr, &ccm_vectors[idx], ENCRYPT, 1, num_jobs)) {
                        printf("error #%d encrypt in-place\n", vect);
                        errors++;
                }

                if (test_ccm(mb_mgr, &ccm_vectors[idx], DECRYPT, 1, num_jobs)) {
                        printf("error #%d decrypt in-place\n", vect);
                        errors++;
                }

                if (test_ccm(mb_mgr, &ccm_vectors[idx], ENCRYPT, 0, num_jobs)) {
                        printf("error #%d encrypt out-of-place\n", vect);
                        errors++;
                }

                if (test_ccm(mb_mgr, &ccm_vectors[idx], DECRYPT, 0, num_jobs)) {
                        printf("error #%d decrypt out-of-place\n", vect);
                        errors++;
                }
	}
	printf("\n");
	return errors;
}

int
ccm_test(const enum arch_type arch,
         struct MB_MGR *mb_mgr)
{
        int errors = 0;

        (void) arch; /* unused */

        errors += test_ccm_std_vectors(mb_mgr, 1);
        errors += test_ccm_std_vectors(mb_mgr, 3);
        errors += test_ccm_std_vectors(mb_mgr, 4);
        errors += test_ccm_std_vectors(mb_mgr, 5);
        errors += test_ccm_std_vectors(mb_mgr, 7);
        errors += test_ccm_std_vectors(mb_mgr, 8);
        errors += test_ccm_std_vectors(mb_mgr, 9);

	if (0 == errors)
		printf("...Pass\n");
	else
		printf("...Fail\n");

	return errors;
}