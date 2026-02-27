/*
** Copyright 2013, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of Google Inc. nor the names of its contributors may
**       be used to endorse or promote products derived from this software
**       without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY Google Inc. ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
** EVENT SHALL Google Inc. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
** OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
** ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>

#include "mincrypt/rsa.h"
#include "mincrypt/sha.h"

#ifndef __unused
#define __unused __attribute__((unused))
#endif

// RSA test data taken from:
//
//   ftp://ftp.rsa.com/pub/rsalabs/tmp/pkcs1v15sign-vectors.txt

// This is the result (reformatted) of running DumpPublicKey on:
//
//   # Example 15: A 2048-bit RSA key pair
//   # -----------------------------------
//
//
//   # Public key
//   # ----------
//
//   # Modulus:
//   df 27 1f d2 5f 86 44 49 6b 0c 81 be 4b d5 02 97
//   ef 09 9b 00 2a 6f d6 77 27 eb 44 9c ea 56 6e d6
//   a3 98 1a 71 31 2a 14 1c ab c9 81 5c 12 09 e3 20
//   a2 5b 32 46 4e 99 99 f1 8c a1 3a 9f d3 89 25 58
//   f9 e0 ad ef dd 36 50 dd 23 a3 f0 36 d6 0f e3 98
//   84 37 06 a4 0b 0b 84 62 c8 be e3 bc e1 2f 1f 28
//   60 c2 44 4c dc 6a 44 47 6a 75 ff 4a a2 42 73 cc
//   be 3b f8 02 48 46 5f 8f f8 c3 a7 f3 36 7d fc 0d
//   f5 b6 50 9a 4f 82 81 1c ed d8 1c da aa 73 c4 91
//   da 41 21 70 d5 44 d4 ba 96 b9 7f 0a fc 80 65 49
//   8d 3a 49 fd 91 09 92 a1 f0 72 5b e2 4f 46 5c fe
//   7e 0e ab f6 78 99 6c 50 bc 5e 75 24 ab f7 3f 15
//   e5 be f7 d5 18 39 4e 31 38 ce 49 44 50 6a aa af
//   3f 9b 23 6d ca b8 fc 00 f8 7a f5 96 fd c3 d9 d6
//   c7 5c d5 08 36 2f ae 2c be dd cc 4c 74 50 b1 7b
//   77 6c 07 9e cc a1 f2 56 35 1a 43 b9 7d be 21 53
//
//   # Exponent:
//   01 00 01

RSAPublicKey key_15 = {
    .len = 64,
    .n0inv = 0xf0053525,
    .n = {2109612371u,890913721u,3433165398u,2003568542u,
          1951445371u,3202206796u,909094444u,3344749832u,
          4257470934u,4168807830u,3401120768u,1067131757u,
          1349167791u,953043268u,406408753u,3854497749u,
          2885107477u,3160306980u,2023320656u,2114890742u,
          1330011390u,4034026466u,2433323681u,2369407485u,
          4236272969u,2528739082u,3578057914u,3661701488u,
          2859713681u,3990363354u,1333952796u,4122366106u,
          914226189u,4173572083u,1212571535u,3191601154u,
          2722264012u,1786117962u,3697951815u,1623344204u,
          3777961768u,3367953340u,185304162u,2218198692u,
          3591365528u,597946422u,3711324381u,4192251375u,
          3548980568u,2359376543u,1318689265u,2723885638u,
          302637856u,2882109788u,824841244u,2744654449u,
          3931533014u,669729948u,711972471u,4010384128u,
          1272251031u,1795981758u,1602634825u,3743883218u},
    .rr = {820482522u,2494434288u,1082168230u,731376296u,
           1306039452u,3139792975u,2575869288u,3874938710u,
           3198185181u,153506080u,1236489694u,1061859740u,
           1174461268u,115279508u,1782749185u,238124145u,
           3587596076u,2259236093u,1112265915u,4048059865u,
           3890381098u,999426242u,794481771u,3804065613u,
           2786019148u,461403875u,3072256692u,4079652654u,
           3056719901u,1871565394u,212974856u,3359008174u,
           1397773937u,3796256698u,914342841u,1097174457u,
           3322220191u,3170814748u,2439215020u,618719336u,
           3629353460u,496817177u,317052742u,380264245u,
           1976007217u,2697736152u,312540864u,4291855337u,
           697006561u,4234182488u,3904590917u,2609582216u,
           451424084u,1805773827u,776344974u,1064489733u,
           2633377036u,1954826648u,3202815814u,2240368662u,
           2618582484u,2211196815u,4107362845u,3640258615u},
    .exponent = 65537,
};

// PKCS#1 v1.5 Signature Example 15.1

char* message_1 =
    "f4 5d 55 f3 55 51 e9 75 d6 a8 dc 7e a9 f4 88 59"
    "39 40 cc 75 69 4a 27 8f 27 e5 78 a1 63 d8 39 b3"
    "40 40 84 18 08 cf 9c 58 c9 b8 72 8b f5 f9 ce 8e"
    "e8 11 ea 91 71 4f 47 ba b9 2d 0f 6d 5a 26 fc fe"
    "ea 6c d9 3b 91 0c 0a 2c 96 3e 64 eb 18 23 f1 02"
    "75 3d 41 f0 33 59 10 ad 3a 97 71 04 f1 aa f6 c3"
    "74 27 16 a9 75 5d 11 b8 ee d6 90 47 7f 44 5c 5d"
    "27 20 8b 2e 28 43 30 fa 3d 30 14 23 fa 7f 2d 08"
    "6e 0a d0 b8 92 b9 db 54 4e 45 6d 3f 0d ab 85 d9"
    "53 c1 2d 34 0a a8 73 ed a7 27 c8 a6 49 db 7f a6"
    "37 40 e2 5e 9a f1 53 3b 30 7e 61 32 99 93 11 0e"
    "95 19 4e 03 93 99 c3 82 4d 24 c5 1f 22 b2 6b de"
    "10 24 cd 39 59 58 a2 df eb 48 16 a6 e8 ad ed b5"
    "0b 1f 6b 56 d0 b3 06 0f f0 f1 c4 cb 0d 0e 00 1d"
    "d5 9d 73 be 12";

char* signature_1 =
    "b7 5a 54 66 b6 5d 0f 30 0e f5 38 33 f2 17 5c 8a"
    "34 7a 38 04 fc 63 45 1d c9 02 f0 b7 1f 90 83 45"
    "9e d3 7a 51 79 a3 b7 23 a5 3f 10 51 64 2d 77 37"
    "4c 4c 6c 8d bb 1c a2 05 25 f5 c9 f3 2d b7 76 95"
    "35 56 da 31 29 0e 22 19 74 82 ce b6 99 06 c4 6a"
    "75 8f b0 e7 40 9b a8 01 07 7d 2a 0a 20 ea e7 d1"
    "d6 d3 92 ab 49 57 e8 6b 76 f0 65 2d 68 b8 39 88"
    "a7 8f 26 e1 11 72 ea 60 9b f8 49 fb bd 78 ad 7e"
    "dc e2 1d e6 62 a0 81 36 8c 04 06 07 ce e2 9d b0"
    "62 72 27 f4 49 63 ad 17 1d 22 93 b6 33 a3 92 e3"
    "31 dc a5 4f e3 08 27 52 f4 3f 63 c1 61 b4 47 a4"
    "c6 5a 68 75 67 0d 5f 66 00 fc c8 60 a1 ca eb 0a"
    "88 f8 fd ec 4e 56 43 98 a5 c4 6c 87 f6 8c e0 70"
    "01 f6 21 3a be 0a b5 62 5f 87 d1 90 25 f0 8d 81"
    "da c7 bd 45 86 bc 93 82 19 1f 6d 28 80 f6 22 7e"
    "5d f3 ee d2 1e 77 92 d2 49 48 04 87 f3 65 52 61";

// PKCS#1 v1.5 Signature Example 15.2

char *message_2 =
    "c1 4b 4c 60 75 b2 f9 aa d6 61 de f4 ec fd 3c b9 "
    "33 c6 23 f4 e6 3b f5 34 10 d2 f0 16 d1 ab 98 e2 "
    "72 9e cc f8 00 6c d8 e0 80 50 73 7d 95 fd bf 29 "
    "6b 66 f5 b9 79 2a 90 29 36 c4 f7 ac 69 f5 14 53 "
    "ce 43 69 45 2d c2 2d 96 f0 37 74 81 14 66 20 00 "
    "dd 9c d3 a5 e1 79 f4 e0 f8 1f a6 a0 31 1c a1 ae "
    "e6 51 9a 0f 63 ce c7 8d 27 bb 72 63 93 fb 7f 1f "
    "88 cd e7 c9 7f 8a 66 cd 66 30 12 81 da c3 f3 a4 "
    "33 24 8c 75 d6 c2 dc d7 08 b6 a9 7b 0a 3f 32 5e "
    "0b 29 64 f8 a5 81 9e 47 9b ";

char* signature_2 =
    "af a7 34 34 62 be a1 22 cc 14 9f ca 70 ab da e7"
    "94 46 67 7d b5 37 36 66 af 7d c3 13 01 5f 4d e7"
    "86 e6 e3 94 94 6f ad 3c c0 e2 b0 2b ed ba 50 47"
    "fe 9e 2d 7d 09 97 05 e4 a3 9f 28 68 32 79 cf 0a"
    "c8 5c 15 30 41 22 42 c0 e9 18 95 3b e0 00 e9 39"
    "cf 3b f1 82 52 5e 19 93 70 fa 79 07 eb a6 9d 5d"
    "b4 63 10 17 c0 e3 6d f7 03 79 b5 db 8d 4c 69 5a"
    "97 9a 8e 61 73 22 40 65 d7 dc 15 13 2e f2 8c d8"
    "22 79 51 63 06 3b 54 c6 51 14 1b e8 6d 36 e3 67"
    "35 bc 61 f3 1f ca 57 4e 53 09 f3 a3 bb df 91 ef"
    "f1 2b 99 e9 cc 17 44 f1 ee 9a 1b d2 2c 5b ad 96"
    "ad 48 19 29 25 1f 03 43 fd 36 bc f0 ac de 7f 11"
    "e5 ad 60 97 77 21 20 27 96 fe 06 1f 9a da 1f c4"
    "c8 e0 0d 60 22 a8 35 75 85 ff e9 fd d5 93 31 a2"
    "8c 4a a3 12 15 88 fb 6c f6 83 96 d8 ac 05 46 59"
    "95 00 c9 70 85 00 a5 97 2b d5 4f 72 cf 8d b0 c8";

// PKCS#1 v1.5 Signature Example 15.3

char* message_3 =
    "d0 23 71 ad 7e e4 8b bf db 27 63 de 7a 84 3b 94 "
    "08 ce 5e b5 ab f8 47 ca 3d 73 59 86 df 84 e9 06 "
    "0b db cd d3 a5 5b a5 5d de 20 d4 76 1e 1a 21 d2 "
    "25 c1 a1 86 f4 ac 4b 30 19 d3 ad f7 8f e6 33 46 "
    "67 f5 6f 70 c9 01 a0 a2 70 0c 6f 0d 56 ad d7 19 "
    "59 2d c8 8f 6d 23 06 c7 00 9f 6e 7a 63 5b 4c b3 "
    "a5 02 df e6 8d dc 58 d0 3b e1 0a 11 70 00 4f e7 "
    "4d d3 e4 6b 82 59 1f f7 54 14 f0 c4 a0 3e 60 5e "
    "20 52 4f 24 16 f1 2e ca 58 9f 11 1b 75 d6 39 c6 "
    "1b aa 80 ca fd 05 cf 35 00 24 4a 21 9e d9 ce d9 "
    "f0 b1 02 97 18 2b 65 3b 52 6f 40 0f 29 53 ba 21 "
    "4d 5b cd 47 88 41 32 87 2a e9 0d 4d 6b 1f 42 15 "
    "39 f9 f3 46 62 a5 6d c0 e7 b4 b9 23 b6 23 1e 30 "
    "d2 67 67 97 81 7f 7c 33 7b 5a c8 24 ba 93 14 3b "
    "33 81 fa 3d ce 0e 6a eb d3 8e 67 73 51 87 b1 eb "
    "d9 5c 02 ";

char* signature_3 =
    "3b ac 63 f8 6e 3b 70 27 12 03 10 6b 9c 79 aa bd"
    "9f 47 7c 56 e4 ee 58 a4 fc e5 ba f2 ca b4 96 0f"
    "88 39 1c 9c 23 69 8b e7 5c 99 ae df 9e 1a bf 17"
    "05 be 1d ac 33 14 0a db 48 eb 31 f4 50 bb 9e fe"
    "83 b7 b9 0d b7 f1 57 6d 33 f4 0c 1c ba 4b 8d 6b"
    "1d 33 23 56 4b 0f 17 74 11 4f a7 c0 8e 6d 1e 20"
    "dd 8f bb a9 b6 ac 7a d4 1e 26 b4 56 8f 4a 8a ac"
    "bf d1 78 a8 f8 d2 c9 d5 f5 b8 81 12 93 5a 8b c9"
    "ae 32 cd a4 0b 8d 20 37 55 10 73 50 96 53 68 18"
    "ce 2b 2d b7 1a 97 72 c9 b0 dd a0 9a e1 01 52 fa"
    "11 46 62 18 d0 91 b5 3d 92 54 30 61 b7 29 4a 55"
    "be 82 ff 35 d5 c3 2f a2 33 f0 5a aa c7 58 50 30"
    "7e cf 81 38 3c 11 16 74 39 7b 1a 1b 9d 3b f7 61"
    "2c cb e5 ba cd 2b 38 f0 a9 83 97 b2 4c 83 65 8f"
    "b6 c0 b4 14 0e f1 19 70 c4 63 0d 44 34 4e 76 ea"
    "ed 74 dc be e8 11 db f6 57 59 41 f0 8a 65 23 b8";

// PKCS#1 v1.5 Signature Example 15.4

char* message_4 =
    "29 03 55 84 ab 7e 02 26 a9 ec 4b 02 e8 dc f1 27 "
    "2d c9 a4 1d 73 e2 82 00 07 b0 f6 e2 1f ec cd 5b "
    "d9 db b9 ef 88 cd 67 58 76 9e e1 f9 56 da 7a d1 "
    "84 41 de 6f ab 83 86 db c6 93 ";

char* signature_4 =
    "28 d8 e3 fc d5 dd db 21 ff bd 8d f1 63 0d 73 77"
    "aa 26 51 e1 4c ad 1c 0e 43 cc c5 2f 90 7f 94 6d"
    "66 de 72 54 e2 7a 6c 19 0e b0 22 ee 89 ec f6 22"
    "4b 09 7b 71 06 8c d6 07 28 a1 ae d6 4b 80 e5 45"
    "7b d3 10 6d d9 17 06 c9 37 c9 79 5f 2b 36 36 7f"
    "f1 53 dc 25 19 a8 db 9b df 2c 80 74 30 c4 51 de"
    "17 bb cd 0c e7 82 b3 e8 f1 02 4d 90 62 4d ea 7f"
    "1e ed c7 42 0b 7e 7c aa 65 77 ce f4 31 41 a7 26"
    "42 06 58 0e 44 a1 67 df 5e 41 ee a0 e6 9a 80 54"
    "54 c4 0e ef c1 3f 48 e4 23 d7 a3 2d 02 ed 42 c0"
    "ab 03 d0 a7 cf 70 c5 86 0a c9 2e 03 ee 00 5b 60"
    "ff 35 03 42 4b 98 cc 89 45 68 c7 c5 6a 02 33 55"
    "1c eb e5 88 cf 8b 01 67 b7 df 13 ad ca d8 28 67"
    "68 10 49 9c 70 4d a7 ae 23 41 4d 69 e3 c0 d2 db"
    "5d cb c2 61 3b c1 20 42 1f 9e 36 53 c5 a8 76 72"
    "97 64 3c 7e 07 40 de 01 63 55 45 3d 6c 95 ae 72";

// PKCS#1 v1.5 Signature Example 15.5

char* message_5 =
    "bd a3 a1 c7 90 59 ea e5 98 30 8d 3d f6 09 ";

char* signature_5 =
    "a1 56 17 6c b9 67 77 c7 fb 96 10 5d bd 91 3b c4"
    "f7 40 54 f6 80 7c 60 08 a1 a9 56 ea 92 c1 f8 1c"
    "b8 97 dc 4b 92 ef 9f 4e 40 66 8d c7 c5 56 90 1a"
    "cb 6c f2 69 fe 61 5b 0f b7 2b 30 a5 13 38 69 23"
    "14 b0 e5 87 8a 88 c2 c7 77 4b d1 69 39 b5 ab d8"
    "2b 44 29 d6 7b d7 ac 8e 5e a7 fe 92 4e 20 a6 ec"
    "66 22 91 f2 54 8d 73 4f 66 34 86 8b 03 9a a5 f9"
    "d4 d9 06 b2 d0 cb 85 85 bf 42 85 47 af c9 1c 6e"
    "20 52 dd cd 00 1c 3e f8 c8 ee fc 3b 6b 2a 82 b6"
    "f9 c8 8c 56 f2 e2 c3 cb 0b e4 b8 0d a9 5e ba 37"
    "1d 8b 5f 60 f9 25 38 74 3d db b5 da 29 72 c7 1f"
    "e7 b9 f1 b7 90 26 8a 0e 77 0f c5 eb 4d 5d d8 52"
    "47 d4 8a e2 ec 3f 26 25 5a 39 85 52 02 06 a1 f2"
    "68 e4 83 e9 db b1 d5 ca b1 90 91 76 06 de 31 e7"
    "c5 18 2d 8f 15 1b f4 1d fe cc ae d7 cd e6 90 b2"
    "16 47 10 6b 49 0c 72 9d 54 a8 fe 28 02 a6 d1 26";

// PKCS#1 v1.5 Signature Example 15.6

char* message_6 =
    "c1 87 91 5e 4e 87 da 81 c0 8e d4 35 6a 0c ce ac "
    "1c 4f b5 c0 46 b4 52 81 b3 87 ec 28 f1 ab fd 56 "
    "7e 54 6b 23 6b 37 d0 1a e7 1d 3b 28 34 36 5d 3d "
    "f3 80 b7 50 61 b7 36 b0 13 0b 07 0b e5 8a e8 a4 "
    "6d 12 16 63 61 b6 13 db c4 7d fa eb 4c a7 46 45 "
    "6c 2e 88 83 85 52 5c ca 9d d1 c3 c7 a9 ad a7 6d "
    "6c ";;

char* signature_6 =
    "9c ab 74 16 36 08 66 9f 75 55 a3 33 cf 19 6f e3"
    "a0 e9 e5 eb 1a 32 d3 4b b5 c8 5f f6 89 aa ab 0e"
    "3e 65 66 8e d3 b1 15 3f 94 eb 3d 8b e3 79 b8 ee"
    "f0 07 c4 a0 2c 70 71 ce 30 d8 bb 34 1e 58 c6 20"
    "f7 3d 37 b4 ec bf 48 be 29 4f 6c 9e 0e cb 5e 63"
    "fe c4 1f 12 0e 55 53 df a0 eb eb bb 72 64 0a 95"
    "37 ba dc b4 51 33 02 29 d9 f7 10 f6 2e 3e d8 ec"
    "78 4e 50 ee 1d 92 62 b4 26 71 34 00 11 d7 d0 98"
    "c6 f2 55 7b 21 31 fa 9b d0 25 46 36 59 7e 88 ec"
    "b3 5a 24 0e f0 fd 85 95 71 24 df 80 80 fe e1 e1"
    "49 af 93 99 89 e8 6b 26 c8 5a 58 81 fa e8 67 3d"
    "9f d4 08 00 dd 13 4e b9 bd b6 41 0f 42 0b 0a a9"
    "7b 20 ef cf 2e b0 c8 07 fa eb 83 a3 cc d9 b5 1d"
    "45 53 e4 1d fc 0d f6 ca 80 a1 e8 1d c2 34 bb 83"
    "89 dd 19 5a 38 b4 2d e4 ed c4 9d 34 64 78 b9 f1"
    "1f 05 57 20 5f 5b 0b d7 ff e9 c8 50 f3 96 d7 c4";;

// PKCS#1 v1.5 Signature Example 15.7

char* message_7 =
    "ab fa 2e cb 7d 29 bd 5b cb 99 31 ce 2b ad 2f 74 "
    "38 3e 95 68 3c ee 11 02 2f 08 e8 e7 d0 b8 fa 05 "
    "8b f9 eb 7e b5 f9 88 68 b5 bb 1f b5 c3 1c ed a3 "
    "a6 4f 1a 12 cd f2 0f cd 0e 5a 24 6d 7a 17 73 d8 "
    "db a0 e3 b2 77 54 5b ab e5 8f 2b 96 e3 f4 ed c1 "
    "8e ab f5 cd 2a 56 0f ca 75 fe 96 e0 7d 85 9d ef "
    "b2 56 4f 3a 34 f1 6f 11 e9 1b 3a 71 7b 41 af 53 "
    "f6 60 53 23 00 1a a4 06 c6 ";

char* signature_7 =
    "c4 b4 37 bc f7 03 f3 52 e1 fa f7 4e b9 62 20 39"
    "42 6b 56 72 ca f2 a7 b3 81 c6 c4 f0 19 1e 7e 4a"
    "98 f0 ee bc d6 f4 17 84 c2 53 7f f0 f9 9e 74 98"
    "2c 87 20 1b fb c6 5e ae 83 2d b7 1d 16 da ca db"
    "09 77 e5 c5 04 67 9e 40 be 0f 9d b0 6f fd 84 8d"
    "d2 e5 c3 8a 7e c0 21 e7 f6 8c 47 df d3 8c c3 54"
    "49 3d 53 39 b4 59 5a 5b f3 1e 3f 8f 13 81 68 07"
    "37 3d f6 ad 0d c7 e7 31 e5 1a d1 9e b4 75 4b 13"
    "44 85 84 2f e7 09 d3 78 44 4d 8e 36 b1 72 4a 4f"
    "da 21 ca fe e6 53 ab 80 74 7f 79 52 ee 80 4d ea"
    "b1 03 9d 84 13 99 45 bb f4 be 82 00 87 53 f3 c5"
    "4c 78 21 a1 d2 41 f4 21 79 c7 94 ef 70 42 bb f9"
    "95 56 56 22 2e 45 c3 43 69 a3 84 69 7b 6a e7 42"
    "e1 8f a5 ca 7a ba d2 7d 9f e7 10 52 e3 31 0d 0f"
    "52 c8 d1 2e a3 3b f0 53 a3 00 f4 af c4 f0 98 df"
    "4e 6d 88 67 79 d6 45 94 d3 69 15 8f db c1 f6 94";

// PKCS#1 v1.5 Signature Example 15.8

char* message_8 =
    "df 40 44 a8 9a 83 e9 fc bf 12 62 54 0a e3 03 8b "
    "bc 90 f2 b2 62 8b f2 a4 46 7a c6 77 22 d8 54 6b "
    "3a 71 cb 0e a4 16 69 d5 b4 d6 18 59 c1 b4 e4 7c "
    "ec c5 93 3f 75 7e c8 6d b0 64 4e 31 18 12 d0 0f "
    "b8 02 f0 34 00 63 9c 0e 36 4d ae 5a eb c5 79 1b "
    "c6 55 76 23 61 bc 43 c5 3d 3c 78 86 76 8f 79 68 "
    "c1 c5 44 c6 f7 9f 7b e8 20 c7 e2 bd 2f 9d 73 e6 "
    "2d ed 6d 2e 93 7e 6a 6d ae f9 0e e3 7a 1a 52 a5 "
    "4f 00 e3 1a dd d6 48 94 cf 4c 02 e1 60 99 e2 9f "
    "9e b7 f1 a7 bb 7f 84 c4 7a 2b 59 48 13 be 02 a1 "
    "7b 7f c4 3b 34 c2 2c 91 92 52 64 12 6c 89 f8 6b "
    "b4 d8 7f 3e f1 31 29 6c 53 a3 08 e0 33 1d ac 8b "
    "af 3b 63 42 22 66 ec ef 2b 90 78 15 35 db da 41 "
    "cb d0 cf 22 a8 cb fb 53 2e c6 8f c6 af b2 ac 06 ";

char* signature_8 =
    "14 14 b3 85 67 ae 6d 97 3e de 4a 06 84 2d cc 0e"
    "05 59 b1 9e 65 a4 88 9b db ab d0 fd 02 80 68 29"
    "13 ba cd 5d c2 f0 1b 30 bb 19 eb 81 0b 7d 9d ed"
    "32 b2 84 f1 47 bb e7 71 c9 30 c6 05 2a a7 34 13"
    "90 a8 49 f8 1d a9 cd 11 e5 ec cf 24 6d ba e9 5f"
    "a9 58 28 e9 ae 0c a3 55 03 25 32 6d ee f9 f4 95"
    "30 ba 44 1b ed 4a c2 9c 02 9c 9a 27 36 b1 a4 19"
    "0b 85 08 4a d1 50 42 6b 46 d7 f8 5b d7 02 f4 8d"
    "ac 5f 71 33 0b c4 23 a7 66 c6 5c c1 dc ab 20 d3"
    "d3 bb a7 2b 63 b3 ef 82 44 d4 2f 15 7c b7 e3 a8"
    "ba 5c 05 27 2c 64 cc 1a d2 1a 13 49 3c 39 11 f6"
    "0b 4e 9f 4e cc 99 00 eb 05 6e e5 9d 6f e4 b8 ff"
    "6e 80 48 cc c0 f3 8f 28 36 fd 3d fe 91 bf 4a 38"
    "6e 1e cc 2c 32 83 9f 0c a4 d1 b2 7a 56 8f a9 40"
    "dd 64 ad 16 bd 01 25 d0 34 8e 38 30 85 f0 88 94"
    "86 1c a1 89 87 22 7d 37 b4 2b 58 4a 83 57 cb 04";

// PKCS#1 v1.5 Signature Example 15.9

char* message_9 =
    "ea 94 1f f0 6f 86 c2 26 92 7f cf 0e 3b 11 b0 87 "
    "26 76 17 0c 1b fc 33 bd a8 e2 65 c7 77 71 f9 d0 "
    "85 01 64 a5 ee cb cc 5c e8 27 fb fa 07 c8 52 14 "
    "79 6d 81 27 e8 ca a8 18 94 ea 61 ce b1 44 9e 72 "
    "fe a0 a4 c9 43 b2 da 6d 9b 10 5f e0 53 b9 03 9a "
    "9c c5 3d 42 0b 75 39 fa b2 23 9c 6b 51 d1 7e 69 "
    "4c 95 7d 4b 0f 09 84 46 18 79 a0 75 9c 44 01 be "
    "ec d4 c6 06 a0 af bd 7a 07 6f 50 a2 df c2 80 7f "
    "24 f1 91 9b aa 77 46 d3 a6 4e 26 8e d3 f5 f8 e6 "
    "da 83 a2 a5 c9 15 2f 83 7c b0 78 12 bd 5b a7 d3 "
    "a0 79 85 de 88 11 3c 17 96 e9 b4 66 ec 29 9c 5a "
    "c1 05 9e 27 f0 94 15 ";

char* signature_9 =
    "ce eb 84 cc b4 e9 09 92 65 65 07 21 ee a0 e8 ec"
    "89 ca 25 bd 35 4d 4f 64 56 49 67 be 9d 4b 08 b3"
    "f1 c0 18 53 9c 9d 37 1c f8 96 1f 22 91 fb e0 dc"
    "2f 2f 95 fe a4 7b 63 9f 1e 12 f4 bc 38 1c ef 0c"
    "2b 7a 7b 95 c3 ad f2 76 05 b7 f6 39 98 c3 cb ad"
    "54 28 08 c3 82 2e 06 4d 4a d1 40 93 67 9e 6e 01"
    "41 8a 6d 5c 05 96 84 cd 56 e3 4e d6 5a b6 05 b8"
    "de 4f cf a6 40 47 4a 54 a8 25 1b bb 73 26 a4 2d"
    "08 58 5c fc fc 95 67 69 b1 5b 6d 7f df 7d a8 4f"
    "81 97 6e aa 41 d6 92 38 0f f1 0e ae cf e0 a5 79"
    "68 29 09 b5 52 1f ad e8 54 d7 97 b8 a0 34 5b 9a"
    "86 4e 05 88 f6 ca dd bf 65 f1 77 99 8e 18 0d 1f"
    "10 24 43 e6 dc a5 3a 94 82 3c aa 9c 3b 35 f3 22"
    "58 3c 70 3a f6 74 76 15 9e c7 ec 93 d1 76 9b 30"
    "0a f0 e7 15 7d c2 98 c6 cd 2d ee 22 62 f8 cd dc"
    "10 f1 1e 01 74 14 71 bb fd 65 18 a1 75 73 45 75";

// PKCS#1 v1.5 Signature Example 15.10

char* message_10 =
    "d8 b8 16 45 c1 3c d7 ec f5 d0 0e d2 c9 1b 9a cd "
    "46 c1 55 68 e5 30 3c 4a 97 75 ed e7 6b 48 40 3d "
    "6b e5 6c 05 b6 b1 cf 77 c6 e7 5d e0 96 c5 cb 35 "
    "51 cb 6f a9 64 f3 c8 79 cf 58 9d 28 e1 da 2f 9d "
    "ec ";

char* signature_10 =
    "27 45 07 4c a9 71 75 d9 92 e2 b4 47 91 c3 23 c5"
    "71 67 16 5c dd 8d a5 79 cd ef 46 86 b9 bb 40 4b"
    "d3 6a 56 50 4e b1 fd 77 0f 60 bf a1 88 a7 b2 4b"
    "0c 91 e8 81 c2 4e 35 b0 4d c4 dd 4c e3 85 66 bc"
    "c9 ce 54 f4 9a 17 5f c9 d0 b2 25 22 d9 57 90 47"
    "f9 ed 42 ec a8 3f 76 4a 10 16 39 97 94 7e 7d 2b"
    "52 ff 08 98 0e 7e 7c 22 57 93 7b 23 f3 d2 79 d4"
    "cd 17 d6 f4 95 54 63 73 d9 83 d5 36 ef d7 d1 b6"
    "71 81 ca 2c b5 0a c6 16 c5 c7 ab fb b9 26 0b 91"
    "b1 a3 8e 47 24 20 01 ff 45 2f 8d e1 0c a6 ea ea"
    "dc af 9e dc 28 95 6f 28 a7 11 29 1f c9 a8 08 78"
    "b8 ba 4c fe 25 b8 28 1c b8 0b c9 cd 6d 2b d1 82"
    "52 46 ee be 25 2d 99 57 ef 93 70 73 52 08 4e 6d"
    "36 d4 23 55 1b f2 66 a8 53 40 fb 4a 6a f3 70 88"
    "0a ab 07 15 3d 01 f4 8d 08 6d f0 bf be c0 5e 7b"
    "44 3b 97 e7 17 18 97 0e 2f 4b f6 20 23 e9 5b 67";

// PKCS#1 v1.5 Signature Example 15.11

char* message_11 =
    "e5 73 9b 6c 14 c9 2d 51 0d 95 b8 26 93 33 37 ff "
    "0d 24 ef 72 1a c4 ef 64 c2 ba d2 64 be 8b 44 ef "
    "a1 51 6e 08 a2 7e b6 b6 11 d3 30 1d f0 06 2d ae "
    "fc 73 a8 c0 d9 2e 2c 52 1f ac bc 7b 26 47 38 76 "
    "7e a6 fc 97 d5 88 a0 ba f6 ce 50 ad f7 9e 60 0b "
    "d2 9e 34 5f cb 1d ba 71 ac 5c 02 89 02 3f e4 a8 "
    "2b 46 a5 40 77 19 19 7d 2e 95 8e 35 31 fd 54 ae "
    "f9 03 aa bb 43 55 f8 83 18 99 4e d3 c3 dd 62 f4 "
    "20 a7 ";

char* signature_11 =
    "be 40 a5 fb 94 f1 13 e1 b3 ef f6 b6 a3 39 86 f2"
    "02 e3 63 f0 74 83 b7 92 e6 8d fa 55 54 df 04 66"
    "cc 32 15 09 50 78 3b 4d 96 8b 63 9a 04 fd 2f b9"
    "7f 6e b9 67 02 1f 5a dc cb 9f ca 95 ac c8 f2 cd"
    "88 5a 38 0b 0a 4e 82 bc 76 07 64 db ab 88 c1 e6"
    "c0 25 5c aa 94 f2 32 19 9d 6f 59 7c c9 14 5b 00"
    "e3 d4 ba 34 6b 55 9a 88 33 ad 15 16 ad 51 63 f0"
    "16 af 6a 59 83 1c 82 ea 13 c8 22 4d 84 d0 76 5a"
    "9d 12 38 4d a4 60 a8 53 1b 4c 40 7e 04 f4 f3 50"
    "70 9e b9 f0 8f 5b 22 0f fb 45 ab f6 b7 5d 15 79"
    "fd 3f 1e b5 5f c7 5b 00 af 8b a3 b0 87 82 7f e9"
    "ae 9f b4 f6 c5 fa 63 03 1f e5 82 85 2f e2 83 4f"
    "9c 89 bf f5 3e 25 52 21 6b c7 c1 d4 a3 d5 dc 2b"
    "a6 95 5c d9 b1 7d 13 63 e7 fe e8 ed 76 29 75 3f"
    "f3 12 5e dd 48 52 1a e3 b9 b0 32 17 f4 49 6d 0d"
    "8e de 57 ac bc 5b d4 de ae 74 a5 6f 86 67 1d e2";

// PKCS#1 v1.5 Signature Example 15.12

char* message_12 =
    "7a f4 28 35 91 7a 88 d6 b3 c6 71 6b a2 f5 b0 d5 "
    "b2 0b d4 e2 e6 e5 74 e0 6a f1 ee f7 c8 11 31 be "
    "22 bf 81 28 b9 cb c6 ec 00 27 5b a8 02 94 a5 d1 "
    "17 2d 08 24 a7 9e 8f dd 83 01 83 e4 c0 0b 96 78 "
    "28 67 b1 22 7f ea 24 9a ad 32 ff c5 fe 00 7b c5 "
    "1f 21 79 2f 72 8d ed a8 b5 70 8a a9 9c ab ab 20 "
    "a4 aa 78 3e d8 6f 0f 27 b5 d5 63 f4 2e 07 15 8c "
    "ea 72 d0 97 aa 68 87 ec 41 1d d0 12 91 2a 5e 03 "
    "2b bf a6 78 50 71 44 bc c9 5f 39 b5 8b e7 bf d1 "
    "75 9a db 9a 91 fa 1d 6d 82 26 a8 34 3a 8b 84 9d "
    "ae 76 f7 b9 82 24 d5 9e 28 f7 81 f1 3e ce 60 5f "
    "84 f6 c9 0b ae 5f 8c f3 78 81 6f 40 20 a7 dd a1 "
    "be d9 0c 92 a2 36 34 d2 03 fa c3 fc d8 6d 68 d3 "
    "18 2a 7d 9c ca be 7b 07 95 f5 c6 55 e9 ac c4 e3 "
    "ec 18 51 40 d1 0c ef 05 34 64 ab 17 5c 83 bd 83 "
    "93 5e 3d ab af 34 62 ee be 63 d1 5f 57 3d 26 9a ";

char* signature_12 =
    "4e 78 c5 90 2b 80 79 14 d1 2f a5 37 ae 68 71 c8"
    "6d b8 02 1e 55 d1 ad b8 eb 0c cf 1b 8f 36 ab 7d"
    "ad 1f 68 2e 94 7a 62 70 72 f0 3e 62 73 71 78 1d"
    "33 22 1d 17 4a be 46 0d bd 88 56 0c 22 f6 90 11"
    "6e 2f bb e6 e9 64 36 3a 3e 52 83 bb 5d 94 6e f1"
    "c0 04 7e ba 03 8c 75 6c 40 be 79 23 05 58 09 b0"
    "e9 f3 4a 03 a5 88 15 eb dd e7 67 93 1f 01 8f 6f"
    "18 78 f2 ef 4f 47 dd 37 40 51 dd 48 68 5d ed 6e"
    "fb 3e a8 02 1f 44 be 1d 7d 14 93 98 f9 8e a9 c0"
    "8d 62 88 8e bb 56 19 2d 17 74 7b 6b 8e 17 09 54"
    "31 f1 25 a8 a8 e9 96 2a a3 1c 28 52 64 e0 8f b2"
    "1a ac 33 6c e6 c3 8a a3 75 e4 2b c9 2a b0 ab 91"
    "03 84 31 e1 f9 2c 39 d2 af 5d ed 7e 43 bc 15 1e"
    "6e be a4 c3 e2 58 3a f3 43 7e 82 c4 3c 5e 3b 5b"
    "07 cf 03 59 68 3d 22 98 e3 59 48 ed 80 6c 06 3c"
    "60 6e a1 78 15 0b 1e fc 15 85 69 34 c7 25 5c fe";

// PKCS#1 v1.5 Signature Example 15.13

char* message_13 =
    "eb ae f3 f9 f2 3b df e5 fa 6b 8a f4 c2 08 c1 89 "
    "f2 25 1b f3 2f 5f 13 7b 9d e4 40 63 78 68 6b 3f "
    "07 21 f6 2d 24 cb 86 88 d6 fc 41 a2 7c ba e2 1d "
    "30 e4 29 fe ac c7 11 19 41 c2 77 ";

char* signature_13 =
    "c4 8d be f5 07 11 4f 03 c9 5f af be b4 df 1b fa"
    "88 e0 18 4a 33 cc 4f 8a 9a 10 35 ff 7f 82 2a 5e"
    "38 cd a1 87 23 91 5f f0 78 24 44 29 e0 f6 08 1c"
    "14 fd 83 33 1f a6 5c 6b a7 bb 9a 12 db f6 62 23"
    "74 cd 0c a5 7d e3 77 4e 2b d7 ae 82 36 77 d0 61"
    "d5 3a e9 c4 04 0d 2d a7 ef 70 14 f3 bb dc 95 a3"
    "61 a4 38 55 c8 ce 9b 97 ec ab ce 17 4d 92 62 85"
    "14 2b 53 4a 30 87 f9 f4 ef 74 51 1e c7 42 b0 d5"
    "68 56 03 fa f4 03 b5 07 2b 98 5d f4 6a df 2d 25"
    "29 a0 2d 40 71 1e 21 90 91 70 52 37 1b 79 b7 49"
    "b8 3a bf 0a e2 94 86 c3 f2 f6 24 77 b2 bd 36 2b"
    "03 9c 01 3c 0c 50 76 ef 52 0d bb 40 5f 42 ce e9"
    "54 25 c3 73 a9 75 e1 cd d0 32 c4 96 22 c8 50 79"
    "b0 9e 88 da b2 b1 39 69 ef 7a 72 39 73 78 10 40"
    "45 9f 57 d5 01 36 38 48 3d e2 d9 1c b3 c4 90 da"
    "81 c4 6d e6 cd 76 ea 8a 0c 8f 6f e3 31 71 2d 24";

// PKCS#1 v1.5 Signature Example 15.14

char* message_14 =
    "c5 a2 71 12 78 76 1d fc dd 4f 0c 99 e6 f5 61 9d "
    "6c 48 b5 d4 c1 a8 09 82 fa a6 b4 cf 1c f7 a6 0f "
    "f3 27 ab ef 93 c8 01 42 9e fd e0 86 40 85 81 46 "
    "10 56 ac c3 3f 3d 04 f5 ad a2 12 16 ca cd 5f d1 "
    "f9 ed 83 20 3e 0e 2f e6 13 8e 3e ae 84 24 e5 91 "
    "5a 08 3f 3f 7a b7 60 52 c8 be 55 ae 88 2d 6e c1 "
    "48 2b 1e 45 c5 da e9 f4 10 15 40 53 27 02 2e c3 "
    "2f 0e a2 42 97 63 b2 55 04 3b 19 58 ee 3c f6 d6 "
    "39 83 59 6e b3 85 84 4f 85 28 cc 9a 98 65 83 5d "
    "c5 11 3c 02 b8 0d 0f ca 68 aa 25 e7 2b ca ae b3 "
    "cf 9d 79 d8 4f 98 4f d4 17 ";

char* signature_14 =
    "6b d5 25 7a a0 66 11 fb 46 60 08 7c b4 bc 4a 9e"
    "44 91 59 d3 16 52 bd 98 08 44 da f3 b1 c7 b3 53"
    "f8 e5 61 42 f7 ea 98 57 43 3b 18 57 3b 4d ee de"
    "81 8a 93 b0 29 02 97 78 3f 1a 2f 23 cb c7 27 97"
    "a6 72 53 7f 01 f6 24 84 cd 41 62 c3 21 4b 9a c6"
    "28 22 4c 5d e0 1f 32 bb 9b 76 b2 73 54 f2 b1 51"
    "d0 e8 c4 21 3e 46 15 ad 0b c7 1f 51 5e 30 0d 6a"
    "64 c6 74 34 11 ff fd e8 e5 ff 19 0e 54 92 30 43"
    "12 6e cf c4 c4 53 90 22 66 8f b6 75 f2 5c 07 e2"
    "00 99 ee 31 5b 98 d6 af ec 4b 1a 9a 93 dc 33 49"
    "6a 15 bd 6f de 16 63 a7 d4 9b 9f 1e 63 9d 38 66"
    "4b 37 a0 10 b1 f3 5e 65 86 82 d9 cd 63 e5 7d e0"
    "f1 5e 8b dd 09 65 58 f0 7e c0 ca a2 18 a8 c0 6f"
    "47 88 45 39 40 28 7c 9d 34 b6 d4 0a 3f 09 bf 77"
    "99 fe 98 ae 4e b4 9f 3f f4 1c 50 40 a5 0c ef c9"
    "bd f2 39 4b 74 9c f1 64 48 0d f1 ab 68 80 27 3b";

// PKCS#1 v1.5 Signature Example 15.15

char* message_15 =
    "9b f8 aa 25 3b 87 2e a7 7a 7e 23 47 6b e2 6b 23 "
    "29 57 8c f6 ac 9e a2 80 5b 35 7f 6f c3 ad 13 0d "
    "ba eb 3d 86 9a 13 cc e7 a8 08 bb bb c9 69 85 7e "
    "03 94 5c 7b b6 1d f1 b5 c2 58 9b 8e 04 6c 2a 5d "
    "7e 40 57 b1 a7 4f 24 c7 11 21 63 64 28 85 29 ec "
    "95 70 f2 51 97 21 3b e1 f5 c2 e5 96 f8 bf 8b 2c "
    "f3 cb 38 aa 56 ff e5 e3 1d f7 39 58 20 e9 4e cf "
    "3b 11 89 a9 65 dc f9 a9 cb 42 98 d3 c8 8b 29 23 "
    "c1 9f c6 bc 34 aa ce ca d4 e0 93 1a 7c 4e 5d 73 "
    "dc 86 df a7 98 a8 47 6d 82 46 3e ef aa 90 a8 a9 "
    "19 2a b0 8b 23 08 8d d5 8e 12 80 f7 d7 2e 45 48 "
    "39 6b aa c1 12 25 2d d5 c5 34 6a db 20 04 a2 f7 "
    "10 1c cc 89 9c c7 fa fa e8 bb e2 95 73 88 96 a5 "
    "b2 01 22 85 01 4e f6 ";

char* signature_15 =
    "27 f7 f4 da 9b d6 10 10 6e f5 7d 32 38 3a 44 8a"
    "8a 62 45 c8 3d c1 30 9c 6d 77 0d 35 7b a8 9e 73"
    "f2 ad 08 32 06 2e b0 fe 0a c9 15 57 5b cd 6b 8b"
    "ca db 4e 2b a6 fa 9d a7 3a 59 17 51 52 b2 d4 fe"
    "72 b0 70 c9 b7 37 9e 50 00 0e 55 e6 c2 69 f6 65"
    "8c 93 79 72 79 7d 3a dd 69 f1 30 e3 4b 85 bd ec"
    "9f 3a 9b 39 22 02 d6 f3 e4 30 d0 9c ac a8 22 77"
    "59 ab 82 5f 70 12 d2 ff 4b 5b 62 c8 50 4d ba d8"
    "55 c0 5e dd 5c ab 5a 4c cc dc 67 f0 1d d6 51 7c"
    "7d 41 c4 3e 2a 49 57 af f1 9d b6 f1 8b 17 85 9a"
    "f0 bc 84 ab 67 14 6e c1 a4 a6 0a 17 d7 e0 5f 8b"
    "4f 9c ed 6a d1 09 08 d8 d7 8f 7f c8 8b 76 ad c8"
    "29 0f 87 da f2 a7 be 10 ae 40 85 21 39 5d 54 ed"
    "25 56 fb 76 61 85 4a 73 0c e3 d8 2c 71 a8 d4 93"
    "ec 49 a3 78 ac 8a 3c 74 43 9f 7c c5 55 ba 13 f8"
    "59 07 08 90 ee 18 ff 65 8f a4 d7 41 96 9d 70 a5";

// PKCS#1 v1.5 Signature Example 15.16

char* message_16 =
    "32 47 48 30 e2 20 37 54 c8 bf 06 81 dc 4f 84 2a "
    "fe 36 09 30 37 86 16 c1 08 e8 33 65 6e 56 40 c8 "
    "68 56 88 5b b0 5d 1e b9 43 8e fe de 67 92 63 de "
    "07 cb 39 55 3f 6a 25 e0 06 b0 a5 23 11 a0 63 ca "
    "08 82 66 d2 56 4f f6 49 0c 46 b5 60 98 18 54 8f "
    "88 76 4d ad 34 a2 5e 3a 85 d5 75 02 3f 0b 9e 66 "
    "50 48 a0 3c 35 05 79 a9 d3 24 46 c7 bb 96 cc 92 "
    "e0 65 ab 94 d3 c8 95 2e 8d f6 8e f0 d9 fa 45 6b "
    "3a 06 bb 80 e3 bb c4 b2 8e 6a 94 b6 d0 ff 76 96 "
    "a6 4e fe 05 e7 35 fe a0 25 d7 bd bc 41 39 f3 a3 "
    "b5 46 07 5c ba 7e fa 94 73 74 d3 f0 ac 80 a6 8d "
    "76 5f 5d f6 21 0b ca 06 9a 2d 88 64 7a f7 ea 04 "
    "2d ac 69 0c b5 73 78 ec 07 77 61 4f b8 b6 5f f4 "
    "53 ca 6b 7d ce 60 98 45 1a 2f 8c 0d a9 bf ec f1 "
    "fd f3 91 bb aa 4e 2a 91 ca 18 a1 12 1a 75 23 a2 "
    "ab d4 25 14 f4 89 e8 ";

char* signature_16 =
    "69 17 43 72 57 c2 2c cb 54 03 29 0c 3d ee 82 d9"
    "cf 75 50 b3 1b d3 1c 51 bd 57 bf d3 5d 45 2a b4"
    "db 7c 4b e6 b2 e2 5a c9 a5 9a 1d 2a 7f eb 62 7f"
    "0a fd 49 76 b3 00 3c c9 cf fd 88 96 50 5e c3 82"
    "f2 65 10 4d 4c f8 c9 32 fa 9f e8 6e 00 87 07 95"
    "99 12 38 9d a4 b2 d6 b3 69 b3 6a 5e 72 e2 9d 24"
    "c9 a9 8c 9d 31 a3 ab 44 e6 43 e6 94 12 66 a4 7a"
    "45 e3 44 6c e8 77 6a be 24 1a 8f 5f c6 42 3b 24"
    "b1 ff 25 0d c2 c3 a8 17 23 53 56 10 77 e8 50 a7"
    "69 b2 5f 03 25 da c8 89 65 a3 b9 b4 72 c4 94 e9"
    "5f 71 9b 4e ac 33 2c aa 7a 65 c7 df e4 6d 9a a7"
    "e6 e0 0f 52 5f 30 3d d6 3a b7 91 92 18 90 18 68"
    "f9 33 7f 8c d2 6a af e6 f3 3b 7f b2 c9 88 10 af"
    "19 f7 fc b2 82 ba 15 77 91 2c 1d 36 89 75 fd 5d"
    "44 0b 86 e1 0c 19 97 15 fa 0b 6f 42 50 b5 33 73"
    "2d 0b ef e1 54 51 50 fc 47 b8 76 de 09 b0 0a 94";

// PKCS#1 v1.5 Signature Example 15.17

char* message_17 =
    "00 8e 59 50 5e af b5 50 aa e5 e8 45 58 4c eb b0 "
    "0b 6d e1 73 3e 9f 95 d4 2c 88 2a 5b be b5 ce 1c "
    "57 e1 19 e7 c0 d4 da ca 9f 1f f7 87 02 17 f7 cf "
    "d8 a6 b3 73 97 7c ac 9c ab 8e 71 e4 20 ";

char* signature_17 =
    "92 25 03 b6 73 ee 5f 3e 69 1e 1c a8 5e 9f f4 17"
    "3c f7 2b 05 ac 2c 13 1d a5 60 35 93 e3 bc 25 9c"
    "94 c1 f7 d3 a0 6a 5b 98 91 bf 11 3f a3 9e 59 ff"
    "7c 1e d6 46 5e 90 80 49 cb 89 e4 e1 25 cd 37 d2"
    "ff d9 22 7a 41 b4 a0 a1 9c 0a 44 fb bf 3d e5 5b"
    "ab 80 20 87 a3 bb 8d 4f f6 68 ee 6b bb 8a d8 9e"
    "68 57 a7 9a 9c 72 78 19 90 df cf 92 cd 51 94 04"
    "c9 50 f1 3d 11 43 c3 18 4f 1d 25 0c 90 e1 7a c6"
    "ce 36 16 3b 98 95 62 7a d6 ff ec 14 22 44 1f 55"
    "e4 49 9d ba 9b e8 95 46 ae 8b c6 3c ca 01 dd 08"
    "46 3a e7 f1 fc e3 d8 93 99 69 38 77 8c 18 12 e6"
    "74 ad 9c 30 9c 5a cc a3 fd e4 4e 7d d8 69 59 93"
    "e9 c1 fa 87 ac da 99 ec e5 c8 49 9e 46 89 57 ad"
    "66 35 9b f1 2a 51 ad be 78 d3 a2 13 b4 49 bf 0b"
    "5f 8d 4d 49 6a cf 03 d3 03 3b 7c cd 19 6b c2 2f"
    "68 fb 7b ef 4f 69 7c 5e a2 b3 50 62 f4 8a 36 dd";

// PKCS#1 v1.5 Signature Example 15.18

char* message_18 =
    "6a bc 54 cf 8d 1d ff 1f 53 b1 7d 81 60 36 88 78 "
    "a8 78 8c c6 d2 2f a5 c2 25 8c 88 e6 60 b0 9a 89 "
    "33 f9 f2 c0 50 4d da dc 21 f6 e7 5e 0b 83 3b eb "
    "55 52 29 de e6 56 b9 04 7b 92 f6 2e 76 b8 ff cc "
    "60 da b0 6b 80 ";

char* signature_18 =
    "0b 6d af 42 f7 a8 62 14 7e 41 74 93 c2 c4 01 ef"
    "ae 32 63 6a b4 cb d4 41 92 bb f5 f1 95 b5 0a e0"
    "96 a4 75 a1 61 4f 0a 9f a8 f7 a0 26 cb 46 c6 50"
    "6e 51 8e 33 d8 3e 56 47 7a 87 5a ca 8c 7e 71 4c"
    "e1 bd bd 61 ef 5d 53 52 39 b3 3f 2b fd d6 17 71"
    "ba b6 27 76 d7 81 71 a1 42 3c ea 87 31 f8 2e 60"
    "76 6d 64 54 26 56 20 b1 5f 5c 5a 58 4f 55 f9 5b"
    "80 2f e7 8c 57 4e d5 da cf c8 31 f3 cf 2b 05 02"
    "c0 b2 98 f2 5c cf 11 f9 73 b3 1f 85 e4 74 42 19"
    "85 f3 cf f7 02 df 39 46 ef 0a 66 05 68 21 11 b2"
    "f5 5b 1f 8a b0 d2 ea 3a 68 3c 69 98 5e ad 93 ed"
    "44 9e a4 8f 03 58 dd f7 08 02 cb 41 de 2f d8 3f"
    "3c 80 80 82 d8 49 36 94 8e 0c 84 a1 31 b4 92 78"
    "27 46 05 27 bb 5c d2 4b fa b7 b4 8e 07 1b 24 17"
    "19 30 f9 97 63 27 2f 97 97 bc b7 6f 1d 24 81 57"
    "55 58 fc f2 60 b1 f0 e5 54 eb b3 df 3c fc b9 58";

// PKCS#1 v1.5 Signature Example 15.19

char* message_19 =
    "af 2d 78 15 2c f1 0e fe 01 d2 74 f2 17 b1 77 f6 "
    "b0 1b 5e 74 9f 15 67 71 5d a3 24 85 9c d3 dd 88 "
    "db 84 8e c7 9f 48 db ba 7b 6f 1d 33 11 1e f3 1b "
    "64 89 9e 73 91 c2 bf fd 69 f4 90 25 cf 20 1f c5 "
    "85 db d1 54 2c 1c 77 8a 2c e7 a7 ee 10 8a 30 9f "
    "ec a2 6d 13 3a 5f fe dc 4e 86 9d cd 76 56 59 6a "
    "c8 42 7e a3 ef 6e 3f d7 8f e9 9d 8d dc 71 d8 39 "
    "f6 78 6e 0d a6 e7 86 bd 62 b3 a4 f1 9b 89 1a 56 "
    "15 7a 55 4e c2 a2 b3 9e 25 a1 d7 c7 d3 73 21 c7 "
    "a1 d9 46 cf 4f be 75 8d 92 76 f0 85 63 44 9d 67 "
    "41 4a 2c 03 0f 42 51 cf e2 21 3d 04 a5 41 06 37 "
    "87 ";

char* signature_19 =
    "20 9c 61 15 78 57 38 7b 71 e2 4b f3 dd 56 41 45"
    "50 50 3b ec 18 0f f5 3b dd 9b ac 06 2a 2d 49 95"
    "09 bf 99 12 81 b7 95 27 df 91 36 61 5b 7a 6d 9d"
    "b3 a1 03 b5 35 e0 20 2a 2c ac a1 97 a7 b7 4e 53"
    "56 f3 dd 59 5b 49 ac fd 9d 30 04 9a 98 ca 88 f6"
    "25 bc a1 d5 f2 2a 39 2d 8a 74 9e fb 6e ed 9b 78"
    "21 d3 11 0a c0 d2 44 19 9e cb 4a a3 d7 35 a8 3a"
    "2e 88 93 c6 bf 85 81 38 3c ca ee 83 46 35 b7 fa"
    "1f af fa 45 b1 3d 15 c1 da 33 af 71 e8 93 03 d6"
    "80 90 ff 62 ee 61 5f df 5a 84 d1 20 71 1d a5 3c"
    "28 89 19 8a b3 83 17 a9 73 4a b2 7d 67 92 4c ea"
    "74 15 6f f9 9b ef 98 76 bb 5c 33 9e 93 74 52 83"
    "e1 b3 4e 07 22 26 b8 80 45 e0 17 e9 f0 5b 2a 8c"
    "41 67 40 25 8e 22 3b 26 90 02 74 91 73 22 73 f3"
    "22 9d 9e f2 b1 b3 80 7e 32 10 18 92 0a d3 e5 3d"
    "ae 47 e6 d9 39 5c 18 4b 93 a3 74 c6 71 fa a2 ce";

// PKCS#1 v1.5 Signature Example 15.20

char* message_20 =
    "40 ee 99 24 58 d6 f6 14 86 d2 56 76 a9 6d d2 cb "
    "93 a3 7f 04 b1 78 48 2f 2b 18 6c f8 82 15 27 0d "
    "ba 29 d7 86 d7 74 b0 c5 e7 8c 7f 6e 56 a9 56 e7 "
    "f7 39 50 a2 b0 c0 c1 0a 08 db cd 67 e5 b2 10 bb "
    "21 c5 8e 27 67 d4 4f 7d d4 01 4e 39 66 14 3b f7 "
    "e3 d6 6f f0 c0 9b e4 c5 5f 93 b3 99 94 b8 51 8d "
    "9c 1d 76 d5 b4 73 74 de a0 8f 15 7d 57 d7 06 34 "
    "97 8f 38 56 e0 e5 b4 81 af bb db 5a 3a c4 8d 48 "
    "4b e9 2c 93 de 22 91 78 35 4c 2d e5 26 e9 c6 5a "
    "31 ed e1 ef 68 cb 63 98 d7 91 16 84 fe c0 ba bc "
    "3a 78 1a 66 66 07 83 50 69 74 d0 e1 48 25 10 1c "
    "3b fa ea ";

char* signature_20 =
    "92 75 02 b8 24 af c4 25 13 ca 65 70 de 33 8b 8a"
    "64 c3 a8 5e b8 28 d3 19 36 24 f2 7e 8b 10 29 c5"
    "5c 11 9c 97 33 b1 8f 58 49 b3 50 09 18 bc c0 05"
    "51 d9 a8 fd f5 3a 97 74 9f a8 dc 48 0d 6f e9 74"
    "2a 58 71 f9 73 92 65 28 97 2a 1a f4 9e 39 25 b0"
    "ad f1 4a 84 27 19 b4 a5 a2 d8 9f a9 c0 b6 60 5d"
    "21 2b ed 1e 67 23 b9 34 06 ad 30 e8 68 29 a5 c7"
    "19 b8 90 b3 89 30 6d c5 50 64 86 ee 2f 36 a8 df"
    "e0 a9 6a f6 78 c9 cb d6 af f3 97 ca 20 0e 3e dc"
    "1e 36 bd 2f 08 b3 1d 54 0c 0c b2 82 a9 55 9e 4a"
    "dd 4f c9 e6 49 2e ed 0c cb d3 a6 98 2e 5f aa 2d"
    "dd 17 be 47 41 7c 80 b4 e5 45 2d 31 f7 24 01 a0"
    "42 32 51 09 54 4d 95 4c 01 93 90 79 d4 09 a5 c3"
    "78 d7 51 2d fc 2d 2a 71 ef cc 34 32 a7 65 d1 c6"
    "a5 2c fc e8 99 cd 79 b1 5b 4f c3 72 36 41 ef 6b"
    "d0 0a cc 10 40 7e 5d f5 8d d1 c3 c5 c5 59 a5 06";


unsigned char* parsehex(char* str, int* len) {
    // result can't be longer than input
    unsigned char* result = malloc(strlen(str));

    unsigned char* p = result;
    *len = 0;

    while (*str) {
        int b;

        while (isspace(*str)) str++;

        switch (*str) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                b = (*str - '0') << 4; break;
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                b = (*str - 'a' + 10) << 4; break;
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                b = (*str - 'A' + 10) << 4; break;
            case '\0':
                return result;
            default:
                return NULL;
        }
        str++;

        while (isspace(*str)) str++;

        switch (*str) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                b |= *str - '0'; break;
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                b |= *str - 'a' + 10; break;
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                b |= *str - 'A' + 10; break;
            default:
                return NULL;
        }
        str++;

        *p++ = b;
        ++*len;
    }

    return result;
}


int main(int arg __unused, char** argv __unused) {

    unsigned char hash[SHA_DIGEST_SIZE];

    unsigned char* message;
    int mlen;
    unsigned char* signature;
    int slen;

#define TEST_MESSAGE(n) do {\
    message = parsehex(message_##n, &mlen); \
    SHA_hash(message, mlen, hash); \
    signature = parsehex(signature_##n, &slen); \
    int result = RSA_verify(&key_15, signature, slen, hash, sizeof(hash)); \
    printf("message %d: %s\n", n, result ? "verified" : "not verified"); \
    success = success && result; \
    } while(0)

    int success = 1;

    TEST_MESSAGE(1);
    TEST_MESSAGE(2);
    TEST_MESSAGE(3);
    TEST_MESSAGE(4);
    TEST_MESSAGE(5);
    TEST_MESSAGE(6);
    TEST_MESSAGE(7);
    TEST_MESSAGE(8);
    TEST_MESSAGE(9);
    TEST_MESSAGE(10);
    TEST_MESSAGE(11);
    TEST_MESSAGE(12);
    TEST_MESSAGE(13);
    TEST_MESSAGE(14);
    TEST_MESSAGE(15);
    TEST_MESSAGE(16);
    TEST_MESSAGE(17);
    TEST_MESSAGE(18);
    TEST_MESSAGE(19);
    TEST_MESSAGE(20);

    printf("\n%s\n\n", success ? "PASS" : "FAIL");

    return !success;
}
