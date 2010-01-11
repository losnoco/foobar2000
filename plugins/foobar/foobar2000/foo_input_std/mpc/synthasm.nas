;

;%define USE_FXCH

%include "tools.inc"
;

;*************************************************************************

                segment_data

                align   32
Di_opt_SIMD     dd      0.0,     -1.0,     -1.0,     -1.0
                dd    -29.0,    -31.0,    -35.0,    -38.0
                dd    213.0,    218.0,    222.0,    225.0
                dd   -459.0,   -519.0,   -581.0,   -645.0
                dd   2037.0,   2000.0,   1952.0,   1893.0
                dd  -5153.0,  -5517.0,  -5879.0,  -6237.0
                dd   6574.0,   5959.0,   5288.0,   4561.0
                dd -37489.0, -39336.0, -41176.0, -43006.0
                dd  75038.0,  74992.0,  74856.0,  74630.0
                dd  37489.0,  35640.0,  33791.0,  31947.0
                dd   6574.0,   7134.0,   7640.0,   8092.0
                dd   5153.0,   4788.0,   4425.0,   4063.0
                dd   2037.0,   2063.0,   2080.0,   2087.0
                dd    459.0,    401.0,    347.0,    294.0
                dd    213.0,    208.0,    202.0,    196.0
                dd     29.0,     26.0,     24.0,     21.0

                dd     -1.0,     -1.0,     -1.0,     -2.0
                dd    -41.0,    -45.0,    -49.0,    -53.0
                dd    227.0,    228.0,    228.0,    227.0
                dd   -711.0,   -779.0,   -848.0,   -919.0
                dd   1822.0,   1739.0,   1644.0,   1535.0
                dd  -6589.0,  -6935.0,  -7271.0,  -7597.0
                dd   3776.0,   2935.0,   2037.0,   1082.0
                dd -44821.0, -46617.0, -48390.0, -50137.0
                dd  74313.0,  73908.0,  73415.0,  72835.0
                dd  30112.0,  28289.0,  26482.0,  24694.0
                dd   8492.0,   8840.0,   9139.0,   9389.0
                dd   3705.0,   3351.0,   3004.0,   2663.0
                dd   2085.0,   2075.0,   2057.0,   2032.0
                dd    244.0,    197.0,    153.0,    111.0
                dd    190.0,    183.0,    176.0,    169.0
                dd     19.0,     17.0,     16.0,     14.0

                dd     -2.0,     -2.0,     -2.0,     -3.0
                dd    -58.0,    -63.0,    -68.0,    -73.0
                dd    224.0,    221.0,    215.0,    208.0
                dd   -991.0,  -1064.0,  -1137.0,  -1210.0
                dd   1414.0,   1280.0,   1131.0,    970.0
                dd  -7910.0,  -8209.0,  -8491.0,  -8755.0
                dd     70.0,   -998.0,  -2122.0,  -3300.0
                dd -51853.0, -53534.0, -55178.0, -56778.0
                dd  72169.0,  71420.0,  70590.0,  69679.0
                dd  22929.0,  21189.0,  19478.0,  17799.0
                dd   9592.0,   9750.0,   9863.0,   9935.0
                dd   2330.0,   2006.0,   1692.0,   1388.0
                dd   2001.0,   1962.0,   1919.0,   1870.0
                dd     72.0,     36.0,      2.0,    -29.0
                dd    161.0,    154.0,    147.0,    139.0
                dd     13.0,     11.0,     10.0,      9.0

                dd     -3.0,     -4.0,     -4.0,     -5.0
                dd    -79.0,    -85.0,    -91.0,    -97.0
                dd    200.0,    189.0,    177.0,    163.0
                dd  -1283.0,  -1356.0,  -1428.0,  -1498.0
                dd    794.0,    605.0,    402.0,    185.0
                dd  -8998.0,  -9219.0,  -9416.0,  -9585.0
                dd  -4533.0,  -5818.0,  -7154.0,  -8540.0
                dd -58333.0, -59838.0, -61289.0, -62684.0
                dd  68692.0,  67629.0,  66494.0,  65290.0
                dd  16155.0,  14548.0,  12980.0,  11455.0
                dd   9966.0,   9959.0,   9916.0,   9838.0
                dd   1095.0,    814.0,    545.0,    288.0
                dd   1817.0,   1759.0,   1698.0,   1634.0
                dd    -57.0,    -83.0,   -106.0,   -127.0
                dd    132.0,    125.0,    117.0,    111.0
                dd      8.0,      7.0,      7.0,      6.0

                dd     -5.0,     -6.0,     -7.0,     -7.0
                dd   -104.0,   -111.0,   -117.0,   -125.0
                dd    146.0,    127.0,    106.0,     83.0
                dd  -1567.0,  -1634.0,  -1698.0,  -1759.0
                dd    -45.0,   -288.0,   -545.0,   -814.0
                dd  -9727.0,  -9838.0,  -9916.0,  -9959.0
                dd  -9975.0, -11455.0, -12980.0, -14548.0
                dd -64019.0, -65290.0, -66494.0, -67629.0
                dd  64019.0,  62684.0,  61289.0,  59838.0
                dd   9975.0,   8540.0,   7154.0,   5818.0
                dd   9727.0,   9585.0,   9416.0,   9219.0
                dd     45.0,   -185.0,   -402.0,   -605.0
                dd   1567.0,   1498.0,   1428.0,   1356.0
                dd   -146.0,   -163.0,   -177.0,   -189.0
                dd    104.0,     97.0,     91.0,     85.0
                dd      5.0,      5.0,      4.0,      4.0

                dd     -8.0,     -9.0,    -10.0,    -11.0
                dd   -132.0,   -139.0,   -147.0,   -154.0
                dd     57.0,     29.0,     -2.0,    -36.0
                dd  -1817.0,  -1870.0,  -1919.0,  -1962.0
                dd  -1095.0,  -1388.0,  -1692.0,  -2006.0
                dd  -9966.0,  -9935.0,  -9863.0,  -9750.0
                dd -16155.0, -17799.0, -19478.0, -21189.0
                dd -68692.0, -69679.0, -70590.0, -71420.0
                dd  58333.0,  56778.0,  55178.0,  53534.0
                dd   4533.0,   3300.0,   2122.0,    998.0
                dd   8998.0,   8755.0,   8491.0,   8209.0
                dd   -794.0,   -970.0,  -1131.0,  -1280.0
                dd   1283.0,   1210.0,   1137.0,   1064.0
                dd   -200.0,   -208.0,   -215.0,   -221.0
                dd     79.0,     73.0,     68.0,     63.0
                dd      3.0,      3.0,      2.0,      2.0

                dd    -13.0,    -14.0,    -16.0,    -17.0
                dd   -161.0,   -169.0,   -176.0,   -183.0
                dd    -72.0,   -111.0,   -153.0,   -197.0
                dd  -2001.0,  -2032.0,  -2057.0,  -2075.0
                dd  -2330.0,  -2663.0,  -3004.0,  -3351.0
                dd  -9592.0,  -9389.0,  -9139.0,  -8840.0
                dd -22929.0, -24694.0, -26482.0, -28289.0
                dd -72169.0, -72835.0, -73415.0, -73908.0
                dd  51853.0,  50137.0,  48390.0,  46617.0
                dd    -70.0,  -1082.0,  -2037.0,  -2935.0
                dd   7910.0,   7597.0,   7271.0,   6935.0
                dd  -1414.0,  -1535.0,  -1644.0,  -1739.0
                dd    991.0,    919.0,    848.0,    779.0
                dd   -224.0,   -227.0,   -228.0,   -228.0
                dd     58.0,     53.0,     49.0,     45.0
                dd      2.0,      2.0,      1.0,      1.0

                dd    -19.0,    -21.0,    -24.0,    -26.0
                dd   -190.0,   -196.0,   -202.0,   -208.0
                dd   -244.0,   -294.0,   -347.0,   -401.0
                dd  -2085.0,  -2087.0,  -2080.0,  -2063.0
                dd  -3705.0,  -4063.0,  -4425.0,  -4788.0
                dd  -8492.0,  -8092.0,  -7640.0,  -7134.0
                dd -30112.0, -31947.0, -33791.0, -35640.0
                dd -74313.0, -74630.0, -74856.0, -74992.0
                dd  44821.0,  43006.0,  41176.0,  39336.0
                dd  -3776.0,  -4561.0,  -5288.0,  -5959.0
                dd   6589.0,   6237.0,   5879.0,   5517.0
                dd  -1822.0,  -1893.0,  -1952.0,  -2000.0
                dd    711.0,    645.0,    581.0,    519.0
                dd   -227.0,   -225.0,   -222.0,   -218.0
                dd     41.0,     38.0,     35.0,     31.0
                dd      1.0,      1.0,      1.0,      1.0

                externdef  Di_opt
				externdef  vectormult_scale;
				externdef vectormult_scale_3dnow;

%define C00     0.500000000000000000000000
%define C01     0.500602998235196301334178
%define C02     0.502419286188155705518560
%define C03     0.505470959897543659956626
%define C04     0.509795579104159168925062
%define C05     0.515447309922624546962323
%define C06     0.522498614939688880640101
%define C07     0.531042591089784174473998
%define C08     0.541196100146196984405269
%define C09     0.553103896034444527838540
%define C10     0.566944034816357703685831
%define C11     0.582934968206133873665654
%define C12     0.601344886935045280535340
%define C13     0.622504123035664816182728
%define C14     0.646821783359990129535794
%define C15     0.674808341455005746033820
%define C16     0.707106781186547524436104
%define C17     0.744536271002298449773679
%define C18     0.788154623451250224773056
%define C19     0.839349645415527038721463
%define C20     0.899976223136415704611808
%define C21     0.972568237861960693780520
%define C22     1.060677685990347471323668
%define C23     1.169439933432884955134476
%define C24     1.306562964876376527851784
%define C25     1.484164616314166277319733
%define C26     1.722447098238333927796261
%define C27     2.057781009953411550808880
%define C28     2.562915447741506178719328
%define C29     3.407608418468718785698107
%define C30     5.101148618689163857960189
%define C31    10.190008123548056810994678

                align   32
                dd  -C31, -C29                ; -C(31),-C(29)
                dd  -C25, -C27                ; -C(25),-C(27)
                dd  -C17, -C19                ; -C(17),-C(19)
                dd  -C23, -C21                ; -C(23),-C(21)
                dd   1.0,  1.0, -C08, -C24    ;  CM110824 = 1, 1, -C( 8), -C(24)
                dd   1.0,  1.0,  C08,  C24    ;  CP110824 = 1, 1,  C( 8),  C(24)
                dd   1.0, -C16,  1.0, -C16    ;  CM116116 = 1, -C(16), 1, -C(16)
                dd   1.0,  C16,  1.0,  C16    ;  CP116116 = 1,  C(16), 1,  C(16)
C               dd   C16, -C16                ;  C(16),-C(16)
                dd   C08,  C24                ;  C( 8), C(24)
                dd   C04,  C12                ;  C( 4), C(12)
                dd   C28,  C20                ;  C(28), C(20)
                dd   C02,  C06                ;  C( 2), C( 6)
                dd   C14,  C10                ;  C(14), C(10)
                dd   C30,  C26                ;  C(30), C(26)
                dd   C18,  C22                ;  C(18), C(22)
                dd   C01,  C03                ;  C( 1), C( 3)
                dd   C07,  C05                ;  C( 7), C( 5)
                dd   C15,  C13                ;  C(15), C(13)
                dd   C09,  C11                ;  C( 9), C(11)
                dd   C31,  C29                ;  C(31), C(29)
                dd   C25,  C27                ;  C(25), C(27)
                dd   C17,  C19                ;  C(17), C(19)
                dd   C23,  C21                ;  C(23), C(21)

%undef C00
%undef C01
%undef C02
%undef C03
%undef C04
%undef C05
%undef C06
%undef C07
%undef C08
%undef C09
%undef C10
%undef C11
%undef C12
%undef C13
%undef C14
%undef C15
%undef C16
%undef C17
%undef C18
%undef C19
%undef C20
%undef C21
%undef C22
%undef C23
%undef C24
%undef C25
%undef C26
%undef C27
%undef C28
%undef C29
%undef C30
%undef C31

; eax hat auf C zu zeigen, dann können die folgenden
; Makros zum Zugriff auf die Konstanten benutzt werden

%define CM31     [ eax - 12*8 ]
%define CM17     [ eax - 10*8 ]
%define CM110824 [ eax -  8*8 ]
%define CP110824 [ eax -  6*8 ]
%define CM116116 [ eax -  4*8 ]
%define CP116116 [ eax -  2*8 ]
%define CC04     [ eax +  2*8 ]
%define CC02     [ eax +  4*8 ]
%define CC30     [ eax +  6*8 ]
%define CC01     [ eax +  8*8 ]
%define CC15     [ eax + 10*8 ]

%define C16      qword [ eax +  0*8 ]
%define C08      qword [ eax +  1*8 ]
%define C04      qword [ eax +  2*8 ]
%define C28      qword [ eax +  3*8 ]
%define C02      qword [ eax +  4*8 ]
%define C14      qword [ eax +  5*8 ]
%define C30      qword [ eax +  6*8 ]
%define C18      qword [ eax +  7*8 ]
%define C01      qword [ eax +  8*8 ]
%define C07      qword [ eax +  9*8 ]
%define C15      qword [ eax + 10*8 ]
%define C09      qword [ eax + 11*8 ]
%define C31      qword [ eax + 12*8 ]
%define C25      qword [ eax + 13*8 ]
%define C17      qword [ eax + 14*8 ]
%define C23      qword [ eax + 15*8 ]

%define _C16     dword [ eax +  0*4 ]
%define _C08     dword [ eax +  2*4 ]
%define _C24     dword [ eax +  3*4 ]
%define _C04     dword [ eax +  4*4 ]
%define _C12     dword [ eax +  5*4 ]
%define _C28     dword [ eax +  6*4 ]
%define _C20     dword [ eax +  7*4 ]
%define _C02     dword [ eax +  8*4 ]
%define _C06     dword [ eax +  9*4 ]
%define _C14     dword [ eax + 10*4 ]
%define _C10     dword [ eax + 11*4 ]
%define _C30     dword [ eax + 12*4 ]
%define _C26     dword [ eax + 13*4 ]
%define _C18     dword [ eax + 14*4 ]
%define _C22     dword [ eax + 15*4 ]
%define _C01     dword [ eax + 16*4 ]
%define _C03     dword [ eax + 17*4 ]
%define _C07     dword [ eax + 18*4 ]
%define _C05     dword [ eax + 19*4 ]
%define _C15     dword [ eax + 20*4 ]
%define _C13     dword [ eax + 21*4 ]
%define _C09     dword [ eax + 22*4 ]
%define _C11     dword [ eax + 23*4 ]
%define _C31     dword [ eax + 24*4 ]
%define _C29     dword [ eax + 25*4 ]
%define _C25     dword [ eax + 26*4 ]
%define _C27     dword [ eax + 27*4 ]
%define _C17     dword [ eax + 28*4 ]
%define _C19     dword [ eax + 29*4 ]
%define _C23     dword [ eax + 30*4 ]
%define _C21     dword [ eax + 31*4 ]

                align   32
bias            dd          32768.0,         32768.0,         32768.0,         32768.0
;bias2          dd  1097364144128.0, 1097364144128.0, 1097364144128.0, 1097364144128.0
bias2           dd  1088774209536.0, 1088774209536.0, 1088774209536.0, 1088774209536.0
negativ         dd       0x80000000,      0x80000000,      0x80000000,      0x80000000
;               dd       0x80000000,      0x80000000,      0x80000000,      0x80000000          ; is line this necessary?
bias3           dd       16613376.0

;***************************************************************************

%macro          muladdi 2
                fld     dword [edx+4*(%2)]
                fmul    dword [ecx+4*(%1)]
                faddp   st1
%endmacro

                segment_code
                align   32
                times   7 nop
proc            VectorMult_i387
$buff0          arg     4
$V0             arg     4
                pushd   ebx
                mov     ebx, [sp($buff0)]
                mov     ecx, [sp($V0)]
                mov     edx, Di_opt
                mov     eax, 32
                fldz;    dword [bias3]
lbl9:
                fld     st0
%ifndef USE_FXCH
                muladdi   0,  0
                muladdi  96,  1
                muladdi 128,  2
                muladdi 224,  3
                muladdi 256,  4
                muladdi 352,  5
                muladdi 384,  6
                muladdi 480,  7
                muladdi 512,  8
                muladdi 608,  9
                muladdi 640, 10
                muladdi 736, 11
                muladdi 768, 12
                muladdi 864, 13
                muladdi 896, 14
                muladdi 992, 15
%else
                fld     dword [ecx+4*  0]
                fmul    dword [edx+4* 0]        ; prod   accu
                fld     dword [ecx+4* 96]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4* 1]        ; prod   accu
                fld     dword [ecx+4*128]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4* 2]        ; prod   accu
                fld     dword [ecx+4*224]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4* 3]        ; prod   accu
                fld     dword [ecx+4*256]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4* 4]        ; prod   accu
                fld     dword [ecx+4*352]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4* 5]        ; prod   accu
                fld     dword [ecx+4*384]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4* 6]        ; prod   accu
                fld     dword [ecx+4*480]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4* 7]        ; prod   accu
                fld     dword [ecx+4*512]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4* 8]        ; prod   accu
                fld     dword [ecx+4*608]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4* 9]        ; prod   accu
                fld     dword [ecx+4*640]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4*10]        ; prod   accu
                fld     dword [ecx+4*736]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4*11]        ; prod   accu
                fld     dword [ecx+4*768]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4*12]        ; prod   accu
                fld     dword [ecx+4*864]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4*13]        ; prod   accu
                fld     dword [ecx+4*896]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4*14]        ; prod   accu
                fld     dword [ecx+4*992]       ; s1     prod   accu
                fxch    st1                     ; prod   s1     accu
                faddp   st2                     ; s1     accu
                fmul    dword [edx+4*15]        ; prod   accu
                faddp   st1                     ; accu
%endif
                lea     edx, [edx + 64]
				fmul    dword [vectormult_scale]
                lea     ecx, [ecx +  4]

                fstp    dword [ebx]
                lea     ebx, [ebx +  8]
                dec     eax
                jnz     near lbl9

                fstp    st0

                popd    ebx
endproc


;***************************************************************************

%macro          muladd3 2
                pmov    mm2, qword [ecx+4*(%1)]
                pmov    mm3, qword [ecx+4*(%1)+8]
                pfmul   mm2, qword [edx+4*(%2)]
                pfmul   mm3, qword [edx+4*(%2)+8]
                pfadd   mm0, mm2
                pfadd   mm1, mm3
%endmacro

                segment_code
                align   32
                times   6 nop
proc            VectorMult_3DNow
$buff1          arg     4
$V1             arg     4
                pushd   ebx
                mov     ebx, [sp($buff1)]
                mov     ecx, [sp($V1)]
                mov     edx, Di_opt_SIMD
                mov     eax, 8
                pmov    mm4, qword [bias]
lbl1:
                pmov    mm0, qword [ecx]
                pmov    mm1, qword [ecx+8]
                pfmul   mm0, qword [edx]
                pfmul   mm1, qword [edx+8]
                pfadd   mm0, mm4
                pfadd   mm1, mm4

                muladd3  96,  4
                muladd3 128,  8
                muladd3 224, 12
                muladd3 256, 16
                muladd3 352, 20
                muladd3 384, 24
                muladd3 480, 28
                sub     edx, byte -128
                muladd3 512,  0
                muladd3 608,  4
                muladd3 640,  8
                muladd3 736, 12
                muladd3 768, 16
                muladd3 864, 20
                muladd3 896, 24
                muladd3 992, 28
                sub     edx, byte -128
                ;add     ecx, byte 16
                lea     ecx, [ecx+16]

;                pf2id   mm0, mm0
;                pf2id   mm1, mm1
				pfmul mm0, qword [vectormult_scale_3dnow]
				pfmul mm1, qword [vectormult_scale_3dnow]
				movd dword [ebx],mm0
				movd dword [ebx+16],mm1
				punpckhdq mm0,mm0
				punpckhdq mm1,mm1
				movd dword [ebx+8],mm0
				movd dword [ebx+24],mm1
                ;add     ebx, byte 16
                lea     ebx, [ebx+32]
                dec     eax
                jnz     near lbl1

                popd    ebx
endproc

;
;***************************************************************************************
;
%macro          muladdS 2
                movaps  xmm0, [ecx+4*(%1)]
                mulps   xmm0, [edx+4*(%2)]
                addps   xmm2, xmm0
%endmacro

                align   32
                times   6 nop
proc            VectorMult_SIMD
$buff2          arg     4
$V2             arg     4
                pushd   ebx
                mov     ebx, [sp($buff1)]
                mov     ecx, [sp($V2)]
                mov     edx, Di_opt_SIMD
                mov     eax, 8
                movaps  xmm4, [bias2]
lbl2:
                movaps  xmm2, [ecx]
                mulps   xmm2, [edx]

                muladdS  96,  4
                muladdS 128,  8
                muladdS 224, 12
                muladdS 256, 16
                muladdS 352, 20
                muladdS 384, 24
                muladdS 480, 28
                sub     edx, byte -128
                muladdS 512,  0
                muladdS 608,  4
                muladdS 640,  8
                muladdS 736, 12
                muladdS 768, 16
                muladdS 864, 20
                muladdS 896, 24
                muladdS 992, 28
                sub     edx, byte -128
                add     ecx, byte 16
                addps   xmm2, xmm4

                movups  [ebx], xmm2
                add     ebx, byte 16
                dec     eax
                jnz     near lbl2

                popd    ebx
endproc

;***************************************************************************************
;

%define A(x)    qword [ ebp + 4*(x) ]
%define _A(x)   dword [ ebp + 4*(x) ]
%define B(x)    qword [ ebp + 64 + 4*(x) ]
%define _B(x)   dword [ ebp + 64 + 4*(x) ]
%define S(x)    qword [ ecx + 4*(x) ]
%define _S(x)   dword [ ecx + 4*(x) ]
%define V(x)    qword [ edx + 4*(x) - 128]
%define _V(x)   dword [ edx + 4*(x) - 128]

%macro          turn 2                          ; dst, tmp
                punpckldq       %2, %1          ; tmp = src.l | tmp.l
                punpckhdq       %1, %2          ; src = src.l | src.h
%endmacro

%macro          copy2 2
                mov     eax, _V(%2)
                mov     ebx, _V(%2+1)
                mov     _V(%1), eax
                mov     _V(%1-1), ebx
%endmacro

%macro          copy1 2
                mov     eax, _V(%2)
                mov     _V(%1), eax
%endmacro

%macro          invcopy2 2
                mov     eax, _V(%2)
                mov     ebx, _V(%2+1)
                add     eax, ecx
                add     ebx, ecx
                ;lea    eax, [eax+ecx]
                ;lea    ebx, [ebx+ecx]
                mov     _V(%1), eax
                mov     _V(%1-1), ebx
%endmacro


;**************************************************************************************

%macro          tu_was31  0

;   B00 =  A00 + A08;
;   B01 =  A01 + A09;
;   B02 =  A02 + A10;
;   B03 =  A03 + A11;

                pmov    mm0, A(0)
                pmov    mm1, A(2)
                pfadd   mm0, A(8)
                pfadd   mm1, A(10)
                pmov    B(0), mm0
                pmov    B(2), mm1

;   B04 =  A04 + A12;
;   B05 =  A05 + A13;
;   B06 =  A06 + A14;
;   B07 =  A07 + A15;

                pmov    mm0, A(4)
                pmov    mm1, A(6)
                pfadd   mm0, A(12)
                pfadd   mm1, A(14)
                pmov    B(4), mm0
                pmov    B(6), mm1

;   B08 = (A00 - A08) * C[ 2];
;   B09 = (A01 - A09) * C[ 6];
;   B10 = (A02 - A10) * C[14];
;   B11 = (A03 - A11) * C[10];

                pmov    mm0, A(0)
                pmov    mm1, A(2)
                pfsub   mm0, A(8)
                pfsub   mm1, A(10)
                pfmul   mm0, C02
                pfmul   mm1, C14
                pmov    B(8), mm0
                pmov    B(10), mm1

;   B12 = (A04 - A12) * C[30];
;   B13 = (A05 - A13) * C[26];
;   B14 = (A06 - A14) * C[18];
;   B15 = (A07 - A15) * C[22];

                pmov    mm0, A(4)
                pmov    mm1, A(6)
                pfsub   mm0, A(12)
                pfsub   mm1, A(14)
                pfmul   mm0, C30
                pfmul   mm1, C18
                pmov    B(12), mm0
                pmov    B(14), mm1
%endmacro

%macro          tu_was32  0

;   A00 =  B00 + B04;
;   A01 =  B01 + B05;
;   A02 =  B02 + B06;
;   A03 =  B03 + B07;

                pmov    mm0, B(0)
                pmov    mm1, B(2)
                pfadd   mm0, B(4)
                pfadd   mm1, B(6)
                pmov    A(0), mm0
                pmov    A(2), mm1

;   A04 = (B00 - B04) * C[ 4];
;   A05 = (B01 - B05) * C[12];
;   A06 = (B02 - B06) * C[28];
;   A07 = (B03 - B07) * C[20];

                pmov    mm0, B(0)
                pmov    mm1, B(2)
                pfsub   mm0, B(4)
                pfsub   mm1, B(6)
                pfmul   mm0, C04
                pfmul   mm1, C28
                pmov    A(4), mm0
                pmov    A(6), mm1

;   A08 =  B08 + B12;
;   A09 =  B09 + B13;
;   A10 =  B10 + B14;
;   A11 =  B11 + B15;

                pmov    mm0, B(8)
                pmov    mm1, B(10)
                pfadd   mm0, B(12)
                pfadd   mm1, B(14)
                pmov    A(8), mm0
                pmov    A(10), mm1

;   A12 = (B08 - B12) * C[ 4];
;   A13 = (B09 - B13) * C[12];
;   A14 = (B10 - B14) * C[28];
;   A15 = (B11 - B15) * C[20];

                pmov    mm0, B(8)
                pmov    mm1, B(10)
                pfsub   mm0, B(12)
                pfsub   mm1, B(14)
                pfmul   mm0, C04
                pfmul   mm1, C28
                pmov    A(12), mm0
                pmov    A(14), mm1

%endmacro

%macro          tu_was33  0

;   B00 =  A00 + A02;
;   B01 =  A01 + A03;
;   B02 = (A00 - A02) * C[ 8];
;   B03 = (A01 - A03) * C[24];

                pmov    mm6, C08

                pmov    mm0, A(0)
                pmov    mm1, A(2)
                pmov    mm2, mm0
                pfsub   mm0, mm1
                pfadd   mm2, mm1
                pfmul   mm0, mm6
                pmov    B(0), mm2
                pmov    B(2), mm0

;   B04 =  A04 + A06;
;   B05 =  A05 + A07;
;   B06 = (A04 - A06) * C[ 8];
;   B07 = (A05 - A07) * C[24];

                pmov    mm0, A(4)
                pmov    mm1, A(6)
                pmov    mm2, mm0
                pfsub   mm0, mm1
                pfadd   mm2, mm1
                pfmul   mm0, mm6
                pmov    B(4), mm2
                pmov    B(6), mm0

;   B08 =  A08 + A10;
;   B09 =  A09 + A11;
;   B10 = (A08 - A10) * C[ 8];
;   B11 = (A09 - A11) * C[24];

                pmov    mm0, A(8)
                pmov    mm1, A(10)
                pmov    mm2, mm0
                pfsub   mm0, mm1
                pfadd   mm2, mm1
                pfmul   mm0, mm6
                pmov    B(8), mm2
                pmov    B(10), mm0

;   B12 =  A12 + A14;
;   B13 =  A13 + A15;
;   B14 = (A12 - A14) * C[ 8];
;   B15 = (A13 - A15) * C[24];

                pmov    mm0, A(12)
                pmov    mm1, A(14)
                pmov    mm2, mm0
                pfsub   mm0, mm1
                pfadd   mm2, mm1
                pfmul   mm0, mm6
                pmov    B(12), mm2
                pmov    B(14), mm0

%endmacro

%macro          tu_was34  0

;   A00 =  B00 + B01;
;   A01 = (B00 - B01) * C[16];
;   A02 =  B02 + B03;
;   A03 = (B02 - B03) * C[16];

                pmov    mm6, C16

                pmov    mm0, B(0)
                pmov    mm1, B(2)
                pmov    mm2, mm0
                pmov    mm3, mm1
                pfmul   mm0, mm6
                pfmul   mm1, mm6
                pfacc   mm2, mm0
                pfacc   mm3, mm1
                pmov    A(0), mm2
                pmov    A(2), mm3

;   A04 =  B04 + B05;
;   A05 = (B04 - B05) * C[16];
;   A06 =  B06 + B07;
;   A07 = (B06 - B07) * C[16];

                pmov    mm0, B(4)
                pmov    mm1, B(6)
                pmov    mm2, mm0
                pmov    mm3, mm1
                pfmul   mm0, mm6
                pfmul   mm1, mm6
                pfacc   mm2, mm0
                pfacc   mm3, mm1
                pmov    A(4), mm2
                pmov    A(6), mm3

;   A08 =  B08 + B09;
;   A09 = (B08 - B09) * C[16];
;   A10 =  B10 + B11;
;   A11 = (B10 - B11) * C[16];

                pmov    mm0, B(8)
                pmov    mm1, B(10)
                pmov    mm2, mm0
                pmov    mm3, mm1
                pfmul   mm0, mm6
                pfmul   mm1, mm6
                pfacc   mm2, mm0
                pfacc   mm3, mm1
                pmov    A(8), mm2
                pmov    A(10), mm3

;   A12 =  B12 + B13;
;   A13 = (B12 - B13) * C[16];
;   A14 =  B14 + B15;
;   A15 = (B14 - B15) * C[16];

                pmov    mm0, B(12)
                pmov    mm1, B(14)
                pmov    mm2, mm0
                pmov    mm3, mm1
                pfmul   mm0, mm6
                pfmul   mm1, mm6
                pfacc   mm2, mm0
                pfacc   mm3, mm1
                pmov    A(12), mm2
                pmov    A(14), mm3

%endmacro

;***************************************************************************

                align   32
proc            Calculate_New_V_3DNow
$S4             arg     4
$V4             arg     4
                mov     ecx, [sp($S4)]
                mov     edx, [sp($V4)]
                sub     edx, byte -128
                push    ebx
                push    ebp
                mov     eax, C
                mov     ebx, esp
                add     esp, byte -128
                and     esp, byte 0xFFFFFFC0
                mov     ebp, esp

;   A00 = S[ 0] + S[31];
;   A01 = S[ 1] + S[30];
;   A02 = S[ 3] + S[28];
;   A03 = S[ 2] + S[29];

                pmov    mm0, S(30)
                pmov    mm1, S(2)
                turn    mm0, mm6
                turn    mm1, mm7
                pfadd   mm0, S(0)
                pfadd   mm1, S(28)
                pmov    A(0), mm0
                pmov    A(2), mm1

;   A04 = S[ 7] + S[24];
;   A05 = S[ 6] + S[25];
;   A06 = S[ 4] + S[27];
;   A07 = S[ 5] + S[26];

                pmov    mm0, S(6)
                pmov    mm1, S(26)
                turn    mm0, mm6
                turn    mm1, mm7
                pfadd   mm0, S(24)
                pfadd   mm1, S(4)
                pmov    A(4), mm0
                pmov    A(6), mm1

;   A08 = S[15] + S[16];
;   A09 = S[14] + S[17];
;   A10 = S[12] + S[19];
;   A11 = S[13] + S[18];

                pmov    mm0, S(14)
                pmov    mm1, S(18)
                turn    mm0, mm6
                turn    mm1, mm7
                pfadd   mm0, S(16)
                pfadd   mm1, S(12)
                pmov    A(8), mm0
                pmov    A(10), mm1

;   A12 = S[ 8] + S[23];
;   A13 = S[ 9] + S[22];
;   A14 = S[11] + S[20];
;   A15 = S[10] + S[21];

                pmov    mm0, S(22)
                pmov    mm1, S(10)
                turn    mm0, mm6
                turn    mm1, mm7
                pfadd   mm0, S(8)
                pfadd   mm1, S(20)
                pmov    A(12), mm0
                pmov    A(14), mm1

                tu_was31
                tu_was32
                tu_was33
                tu_was34

                pmov    mm7, qword [negativ]

;   V[48] = -A00;
;   V[ 0] =  A01;
;   V[40] = -A02 - (V[ 8] = A03);
                                                 ; 0     1       2       3       4       5       6       7
                movd    mm2, _A(3)               ;               3                                       -
                movd    mm0, _A(0)               ; 0             3                                       -
                movd    _V(8), mm2
                movd    mm1, _A(1)               ; 0     1       3                                       -
                pfadd   mm2, A(2)                ; 0     1       2+3                                     -
                pxor    mm0, mm7                 ; -0    1       2+3                                     -
                pxor    mm2, mm7                 ; -0    1       -2-3                                    -
                movd    _V(0), mm1
                movd    _V(48), mm0
                movd    _V(40), mm2

;   V[36] = -((V[ 4] = A05 + (V[12] = A07)) + A06);
;   V[44] = - A04 - A06 - A07;

                movd    mm0, _A(7)               ; 7                                                     -
                pmov    mm1, A(6)                ; 7     6                                               -
                movd    _V(12), mm0
                pfadd   mm0, A(5)                ; 5+7   6                                               -
                movd    _V(4), mm0
                pfadd   mm0, mm1                 ; 5+6+7 6                                               -
                pfacc   mm1, mm1                 ; 5+6+7 6+7                                             -
                pfadd   mm1, A(4)                ; 5+6+7 4+6+7                                           -
                pxor    mm0, mm7                 ;-5-6-7 4+6+7                                           -
                pxor    mm1, mm7                 ;-5-6-7 -4-6-7                                          -
                movd    _V(36), mm0
                movd    _V(44), mm1

;   V[ 6] = (V[10] = A11 + (V[14] = A15)) + A13;
;   V[38] = (V[34] = -(V[ 2] = A09 + A13 + A15) - A14) + A09 - A10 - A11;

                movd    mm2, _A(15)              ;               15
                movd    mm0, _A(9)               ; 9             15
                movd    _V(14), mm2
                pfadd   mm0, A(13)               ; 9+13          15
                pfadd   mm0, mm2                 ; 9+13+15
                movd    _V(2), mm0
                pfadd   mm2, A(11)               ; 9+13+15       11+15
                pfadd   mm0, A(14)               ; 9+13+14+15
                movd    _V(10), mm2
                pxor    mm0, mm7                 ;-9-13-14-15
                pfadd   mm2, A(13)               ;-9-13-14-15    11+13+15
                pmov    mm6, A(10)               ;-9-13-14-15    11+13+15                10
                movd    _V(34), mm0
                pfacc   mm6, mm6                 ;-9-13-14-15    11+13+15                10+11
                movd    _V(6), mm2
                pfadd   mm0, A(9)                ;-13-14-15      11+13+15                10+11
                pfsub   mm0, mm6                 ;-10-11-13-14-15
                movd    _V(38), mm0

;   V[46] = (tmp = -(A12 + A14 + A15)) - A08;

                movd    mm1, _A(12)              ;       12
                pfadd   mm1, A(14)               ;       12+14
                pfadd   mm1, A(15)               ;       12+14+15
                pxor    mm1, mm7                 ;       -12-14-15
                pfsubr  mm6, mm1                 ;       -12-14-15                       -10-11-12-14-15
                pfsub   mm1, A(8)                ;       -8-12-14-15
                movd    _V(46), mm1

;   V[42] = tmp - A10 - A11;                            // abhängig vom Befehl drüber

                movd    _V(42), mm6

;   A00 = (S[ 0] - S[31]) * C[ 1];
;   A01 = (S[ 1] - S[30]) * C[ 3];
;   A02 = (S[ 3] - S[28]) * C[ 7];
;   A03 = (S[ 2] - S[29]) * C[ 5];

                pmov    mm0, S(30)
                pmov    mm1, S(2)
                turn    mm0, mm6
                turn    mm1, mm7
                pfsubr  mm0, S(0)
                pfsub   mm1, S(28)
                pfmul   mm0, C01
                pfmul   mm1, C07
                pmov    A(0), mm0
                pmov    A(2), mm1

;   A04 = (S[ 7] - S[24]) * C[15];
;   A05 = (S[ 6] - S[25]) * C[13];
;   A06 = (S[ 4] - S[27]) * C[ 9];
;   A07 = (S[ 5] - S[26]) * C[11];

                pmov    mm0, S(6)
                pmov    mm1, S(26)
                turn    mm0, mm6
                turn    mm1, mm7
                pfsub   mm0, S(24)
                pfsubr  mm1, S(4)
                pfmul   mm0, C15
                pfmul   mm1, C09
                pmov    A(4), mm0
                pmov    A(6), mm1

;   A08 = (S[15] - S[16]) * C[31];
;   A09 = (S[14] - S[17]) * C[29];
;   A10 = (S[12] - S[19]) * C[25];
;   A11 = (S[13] - S[18]) * C[27];

                pmov    mm0, S(14)
                pmov    mm1, S(18)
                turn    mm0, mm6
                turn    mm1, mm7
                pfsub   mm0, S(16)
                pfsubr  mm1, S(12)
                pfmul   mm0, C31
                pfmul   mm1, C25
                pmov    A(8), mm0
                pmov    A(10), mm1

;   A12 = (S[ 8] - S[23]) * C[17];
;   A13 = (S[ 9] - S[22]) * C[19];
;   A14 = (S[11] - S[20]) * C[23];
;   A15 = (S[10] - S[21]) * C[21];

                pmov    mm0, S(22)
                pmov    mm1, S(10)
                turn    mm0, mm6
                turn    mm1, mm7
                pfsubr  mm0, S(8)
                pfsub   mm1, S(20)
                pfmul   mm0, C17
                pfmul   mm1, C23
                pmov    A(12), mm0
                pmov    A(14), mm1

                tu_was31
                tu_was32
                tu_was33
                tu_was34

                pmov    mm7, qword [negativ]

;   V[ 5] = (V[11] = (V[13] = A07 + (V[15] = A15)) + A11) + A05 + A13;

                movd    mm0, _A(15)
                movd    _V(15), mm0
                pfadd   mm0, A(7)
                movd    _V(13), mm0
                pfadd   mm0, A(11)
                movd    _V(11), mm0
                pfadd   mm0, A(5)
                pfadd   mm0, A(13)
                movd    _V(5), mm0

;   V[ 7] = (V[ 9] = A03 + A11 + A15) + A13;

                movd    mm1, _A(3)
                pfadd   mm1, A(11)
                pfadd   mm1, A(15)
                movd    _V(9), mm1
                pfadd   mm1, A(13)
                movd    _V(7), mm1

;   V[33] = -(V[ 1] = A01 + A09 + A13 + A15) - A14;

                movd    mm4, _A(9)
                pfadd   mm4, A(13)
                pfadd   mm4, A(15)
                movd    mm2, _A(1)
                pfadd   mm2, mm4
                movd    _V(1), mm2
                pfadd   mm2, A(14)
                pxor    mm2, mm7
                movd    _V(33), mm2

;   V[35] = -(V[ 3] = A05 + A07 + A09 + A13 + A15) - A06 - A14;

                pfadd   mm4, A(5)
                pfadd   mm4, A(7)
                movd    _V(3), mm4
                pxor    mm4, mm7
                pfsub   mm4, A(6)
                pfsub   mm4, A(14)
                movd    _V(35), mm4

;   V[37] = (tmp = -(A10 + A11 + A13 + A14 + A15)) - A05 - A06 - A07;

                pmov    mm1, A(10)
                pmov    mm2, A(14)
                pfacc   mm1, mm2
                pfacc   mm1, mm1
                pfadd   mm1, A(13)
                pxor    mm1, mm7
                pmov    mm4, A(6)
                pmov    mm6, mm1
                pfacc   mm4, mm4
                pfsub   mm1, A(5)
                pfsub   mm1, mm4
                movd    _V(37), mm1

;   V[39] = tmp - A02 - A03;                                            // abhängig vom Befehl drüber

                pmov    mm3, A(2)
                pmov    mm2, mm6
                pfacc   mm3, mm3
                pfsub   mm2, mm3
                movd    _V(39), mm2

;   V[41] = (tmp += A13 - A12) - A02 - A03;                             // abhängig vom Befehl 2 drüber

                pfadd   mm6, A(13)
                pfsub   mm6, A(12)
                pfsubr  mm3, mm6
                movd    _V(41), mm3

;   V[43] = tmp - A04 - A06 - A07;                                      // abhängig von Befehlen 1 und 3 drüber

                movd    mm5, _A(4)
                pfadd   mm5, mm4
                pfsub   mm6, mm5
                movd    _V(43), mm6

;   V[47] = (tmp = -(A08 + A12 + A14 + A15)) - A00;

                movd    mm1, _A(8)
                pfadd   mm1, A(12)
                pfadd   mm1, A(14)
                pfadd   mm1, A(15)
                pxor    mm1, mm7
                pmov    mm6, mm1
                pfsub   mm1, A(0)
                movd    _V(47), mm1

;   V[45] = tmp - A04 - A06 - A07;                                      // abhängig vom Befehl drüber

                pfsub   mm6, mm5
                movd    _V(45), mm6

                mov     esp, ebx

;   V[32] = -V[ 0];
;   V[31] = -V[ 1];
;   V[30] = -V[ 2];
;   V[29] = -V[ 3];
;   V[28] = -V[ 4];
;   V[27] = -V[ 5];
;   V[26] = -V[ 6];
;   V[25] = -V[ 7];
;   V[24] = -V[ 8];
;   V[23] = -V[ 9];
;   V[22] = -V[10];
;   V[21] = -V[11];
;   V[20] = -V[12];
;   V[19] = -V[13];
;   V[18] = -V[14];
;   V[17] = -V[15];

                mov     ecx, 80000000h
                invcopy2 32,  0
                invcopy2 30,  2
                invcopy2 28,  4
                invcopy2 26,  6
                invcopy2 24,  8
                invcopy2 22, 10
                invcopy2 20, 12
                invcopy2 18, 14

;   V[63] =  V[33];
;   V[62] =  V[34];
;   V[61] =  V[35];
;   V[60] =  V[36];
;   V[59] =  V[37];
;   V[58] =  V[38];
;   V[57] =  V[39];
;   V[56] =  V[40];
;   V[55] =  V[41];
;   V[54] =  V[42];
;   V[53] =  V[43];
;   V[52] =  V[44];
;   V[51] =  V[45];
;   V[50] =  V[46];
;   V[49] =  V[47];

                add     edx, 33*4
                copy2   30,  0
                copy2   28,  2
                copy2   26,  4
                copy2   24,  6
                copy2   22,  8
                copy2   20, 10
                copy2   18, 12
                copy1   16, 14


                pop     ebp
                pop     ebx
endproc
;****************************************************************************

%macro          tu_wasS  0

;   B00 =  A00 + A08;
;   B01 =  A01 + A09;
;   B02 =  A02 + A10;
;   B03 =  A03 + A11;
;   B04 =  A04 + A12;
;   B05 =  A05 + A13;
;   B06 =  A06 + A14;
;   B07 =  A07 + A15;

                movaps  xmm2, xmm0
                movaps  xmm3, xmm1
                addps   xmm0, xmm4
                addps   xmm1, xmm5

;   B08 = (A00 - A08) * C[ 2];
;   B09 = (A01 - A09) * C[ 6];
;   B10 = (A02 - A10) * C[14];
;   B11 = (A03 - A11) * C[10];
;   B12 = (A04 - A12) * C[30];
;   B13 = (A05 - A13) * C[26];
;   B14 = (A06 - A14) * C[18];
;   B15 = (A07 - A15) * C[22];

                subps   xmm2, xmm4
                subps   xmm3, xmm5
                mulps   xmm2, CC02
                mulps   xmm3, CC30

;   A00 =  B00 + B04;
;   A01 =  B01 + B05;
;   A02 =  B02 + B06;
;   A03 =  B03 + B07;
;   A04 = (B00 - B04) * C[ 4];
;   A05 = (B01 - B05) * C[12];
;   A06 = (B02 - B06) * C[28];
;   A07 = (B03 - B07) * C[20];

                movaps  xmm5, xmm0
                movaps  xmm4, xmm0
                subps   xmm5, xmm1
                addps   xmm4, xmm1
                mulps   xmm5, CC04

;   A08 =  B08 + B12;
;   A09 =  B09 + B13;
;   A10 =  B10 + B14;
;   A11 =  B11 + B15;
;   A12 = (B08 - B12) * C[ 4];
;   A13 = (B09 - B13) * C[12];
;   A14 = (B10 - B14) * C[28];
;   A15 = (B11 - B15) * C[20];

                movaps  xmm7, xmm2
                movaps  xmm6, xmm2
                subps   xmm7, xmm3
                addps   xmm6, xmm3
                mulps   xmm7, CC04

;   B00 =  A00 + A02;                   B00 = A00 * 1    + A02 * 1
;   B01 =  A01 + A03;                   B01 = A01 * 1    + A03 * 1
;   B02 = (A00 - A02) * C[ 8];          B02 = A02 * -C8  + A00 * C8
;   B03 = (A01 - A03) * C[24];          B03 = A03 * -C24 + A01 * C24
;   B04 =  A04 + A06;
;   B05 =  A05 + A07;
;   B06 = (A04 - A06) * C[ 8];
;   B07 = (A05 - A07) * C[24];
;   B08 =  A08 + A10;
;   B09 =  A09 + A11;
;   B10 = (A08 - A10) * C[ 8];
;   B11 = (A09 - A11) * C[24];
;   B12 =  A12 + A14;
;   B13 =  A13 + A15;
;   B14 = (A12 - A14) * C[ 8];
;   B15 = (A13 - A15) * C[24];

                movaps  xmm0, xmm4
                shufps  xmm4, xmm4, 0x4E   ; 4#1032# = 0x4E
                mulps   xmm0, CM110824
                mulps   xmm4, CP110824
                addps   xmm0, xmm4

                movaps  xmm1, xmm5
                shufps  xmm5, xmm5, 0x4E   ; 4#1032# = 0x4E
                mulps   xmm1, CM110824
                mulps   xmm5, CP110824
                addps   xmm1, xmm5

                movaps  xmm2, xmm6
                shufps  xmm6, xmm6, 0x4E   ; 4#1032# = 0x4E
                mulps   xmm2, CM110824
                mulps   xmm6, CP110824
                addps   xmm2, xmm6

                movaps  xmm3, xmm7
                shufps  xmm7, xmm7, 0x4E   ; 4#1032# = 0x4E
                mulps   xmm3, CM110824
                mulps   xmm7, CP110824
                addps   xmm3, xmm7

;   A00 =  B00 + B01;                   A00 = B00 * 1    + B01 * 1
;   A01 = (B00 - B01) * C[16];          A01 = B01 * -C16 + B00 * C16
;   A02 =  B02 + B03;                   A02 = B02 * 1    + B03 * 1
;   A03 = (B02 - B03) * C[16];          A03 = B03 * -C16 + B02 * C16
;   A04 =  B04 + B05;
;   A05 = (B04 - B05) * C[16];
;   A06 =  B06 + B07;
;   A07 = (B06 - B07) * C[16];
;   A08 =  B08 + B09;
;   A09 = (B08 - B09) * C[16];
;   A10 =  B10 + B11;
;   A11 = (B10 - B11) * C[16];
;   A12 =  B12 + B13;
;   A13 = (B12 - B13) * C[16];
;   A14 =  B14 + B15;
;   A15 = (B14 - B15) * C[16];

                movaps  xmm4, xmm0
                shufps  xmm0, xmm0, 0xB1   ; 4#2301# = 0xB1
                mulps   xmm4, CM116116
                mulps   xmm0, CP116116
                addps   xmm4, xmm0

                movaps  xmm5, xmm1
                shufps  xmm1, xmm1, 0xB1   ; 4#2301# = 0xB1
                mulps   xmm5, CM116116
                mulps   xmm1, CP116116
                addps   xmm5, xmm1

                movaps  xmm6, xmm2
                shufps  xmm2, xmm2, 0xB1   ; 4#2301# = 0xB1
                mulps   xmm6, CM116116
                mulps   xmm2, CP116116
                addps   xmm6, xmm2

                movaps  xmm7, xmm3
                shufps  xmm3, xmm3, 0xB1   ; 4#2301# = 0xB1
                mulps   xmm7, CM116116
                mulps   xmm3, CP116116
                addps   xmm7, xmm3

;   Store

                movaps  [edx+4* 0], xmm4
                movaps  [edx+4* 4], xmm5
                movaps  [edx+4* 8], xmm6
                movaps  [edx+4*12], xmm7
%endmacro

                align   32
proc            New_V_Helper2
$A6             arg     4
$Sample6        arg     4
                mov     ecx, [sp($Sample6)]
                mov     edx, [sp($A6)]
                mov     eax, C

;    A[ 0] = Sample[ 0] + Sample[31];
;    A[ 1] = Sample[ 1] + Sample[30];
;    A[ 2] = Sample[ 3] + Sample[28];
;    A[ 3] = Sample[ 2] + Sample[29];
;    A[ 4] = Sample[ 7] + Sample[24];
;    A[ 5] = Sample[ 6] + Sample[25];
;    A[ 6] = Sample[ 4] + Sample[27];
;    A[ 7] = Sample[ 5] + Sample[26];
;    A[ 8] = Sample[15] + Sample[16];
;    A[ 9] = Sample[14] + Sample[17];
;    A[10] = Sample[12] + Sample[19];
;    A[11] = Sample[13] + Sample[18];
;    A[12] = Sample[ 8] + Sample[23];
;    A[13] = Sample[ 9] + Sample[22];
;    A[14] = Sample[11] + Sample[20];
;    A[15] = Sample[10] + Sample[21];

                movaps  xmm0, [ecx+  0]
                shufps  xmm0, xmm0, 0xB4   ; 4#2310# = 0xB4
                movaps  xmm1, [ecx+ 16]
                shufps  xmm1, xmm1, 0x4B   ; 4#1023# = 0x4B
                movaps  xmm2, [ecx+ 32]
                shufps  xmm2, xmm2, 0xB4   ; 4#2310# = 0xB4
                movaps  xmm3, [ecx+ 48]
                shufps  xmm3, xmm3, 0x4B   ; 4#1023# = 0x4B
                movaps  xmm4, [ecx+ 64]
                shufps  xmm4, xmm4, 0xB4   ; 4#2310# = 0xB4
                addps   xmm4, xmm3
                movaps  xmm5, [ecx+ 80]
                shufps  xmm5, xmm5, 0x4B   ; 4#1023# = 0x4B
                addps   xmm5, xmm2
                movaps  xmm6, [ecx+ 96]
                shufps  xmm6, xmm6, 0xB4   ; 4#2310# = 0xB4
                addps   xmm1, xmm6
                movaps  xmm7, [ecx+112]
                shufps  xmm7, xmm7, 0x4B   ; 4#1023# = 0x4B
                addps   xmm0, xmm7

                tu_wasS
endproc

;*********************************************************************************************

                align   32
proc            New_V_Helper3
$A7             arg     4
$Sample7        arg     4
                mov     ecx, [sp($Sample7)]
                mov     edx, [sp($A7)]
                mov     eax, C

;    A[ 0] = (Sample[ 0] - Sample[31]) * C[ 1];         Sample[ 0] + Sample[31];
;    A[ 1] = (Sample[ 1] - Sample[30]) * C[ 3];         Sample[ 1] + Sample[30];
;    A[ 2] = (Sample[ 3] - Sample[28]) * C[ 7];         Sample[ 3] + Sample[28];
;    A[ 3] = (Sample[ 2] - Sample[29]) * C[ 5];         Sample[ 2] + Sample[29];
;    A[ 4] = (Sample[ 7] - Sample[24]) * C[15];         Sample[ 7] + Sample[24];
;    A[ 5] = (Sample[ 6] - Sample[25]) * C[13];         Sample[ 6] + Sample[25];
;    A[ 6] = (Sample[ 4] - Sample[27]) * C[ 9];         Sample[ 4] + Sample[27];
;    A[ 7] = (Sample[ 5] - Sample[26]) * C[11];         Sample[ 5] + Sample[26];
;    A[ 8] = (Sample[15] - Sample[16]) * C[31];         Sample[15] + Sample[16];
;    A[ 9] = (Sample[14] - Sample[17]) * C[29];         Sample[14] + Sample[17];
;    A[10] = (Sample[12] - Sample[19]) * C[25];         Sample[12] + Sample[19];
;    A[11] = (Sample[13] - Sample[18]) * C[27];         Sample[13] + Sample[18];
;    A[12] = (Sample[ 8] - Sample[23]) * C[17];         Sample[ 8] + Sample[23];
;    A[13] = (Sample[ 9] - Sample[22]) * C[19];         Sample[ 9] + Sample[22];
;    A[14] = (Sample[11] - Sample[20]) * C[23];         Sample[11] + Sample[20];
;    A[15] = (Sample[10] - Sample[21]) * C[21];         Sample[10] + Sample[21];

                movaps  xmm0, [ecx+  0]
                shufps  xmm0, xmm0, 0xB4   ; 4#2310# = 0xB4
                movaps  xmm1, [ecx+ 16]
                shufps  xmm1, xmm1, 0x4B   ; 4#1023# = 0x4B
                movaps  xmm2, [ecx+ 32]
                shufps  xmm2, xmm2, 0xB4   ; 4#2310# = 0xB4
                movaps  xmm3, [ecx+ 48]
                shufps  xmm3, xmm3, 0x4B   ; 4#1023# = 0x4B
                movaps  xmm4, [ecx+ 64]
                shufps  xmm4, xmm4, 0xB4   ; 4#2310# = 0xB4
                subps   xmm4, xmm3
                mulps   xmm4, CM31
                movaps  xmm5, [ecx+ 80]
                shufps  xmm5, xmm5, 0x4B   ; 4#1023# = 0x4B
                subps   xmm5, xmm2
                mulps   xmm5, CM17
                movaps  xmm6, [ecx+ 96]
                shufps  xmm6, xmm6, 0xB4   ; 4#2310# = 0xB4
                subps   xmm1, xmm6
                mulps   xmm1, CC15
                movaps  xmm7, [ecx+112]
                shufps  xmm7, xmm7, 0x4B   ; 4#1023# = 0x4B
                subps   xmm0, xmm7
                mulps   xmm0, CC01

                tu_wasS
endproc

;*********************************************************************************************

                align   32
proc            New_V_Helper4
$V8             arg     4
                mov     edx,[sp($V8)]

;    V[32] = -V[ 0];
;    V[31] = -V[ 1];
;    V[30] = -V[ 2];
;    V[29] = -V[ 3];
;    V[28] = -V[ 4];
;    V[27] = -V[ 5];
;    V[26] = -V[ 6];
;    V[25] = -V[ 7];
;    V[24] = -V[ 8];
;    V[23] = -V[ 9];
;    V[22] = -V[10];
;    V[21] = -V[11];
;    V[20] = -V[12];
;    V[19] = -V[13];
;    V[18] = -V[14];
;    V[17] = -V[15];
;    V[63] =  V[33];
;    V[62] =  V[34];
;    V[61] =  V[35];
;    V[60] =  V[36];
;    V[59] =  V[37];
;    V[58] =  V[38];
;    V[57] =  V[39];
;    V[56] =  V[40];
;    V[55] =  V[41];
;    V[54] =  V[42];
;    V[53] =  V[43];
;    V[52] =  V[44];
;    V[51] =  V[45];
;    V[50] =  V[46];
;    V[49] =  V[47];

                movaps  xmm7, [negativ]
                movaps  xmm0, [edx+ 0*4]
                xorps   xmm0, xmm7
                movaps  xmm1, [edx+ 4*4]
                xorps   xmm1, xmm7
                movaps  xmm2, [edx+ 8*4]
                xorps   xmm2, xmm7
                movaps  xmm3, [edx+12*4]
                xorps   xmm3, xmm7
                shufps  xmm0, xmm0, 0x1B   ; 4#0123# = 0x1B
                shufps  xmm1, xmm1, 0x1B
                shufps  xmm2, xmm2, 0x1B
                shufps  xmm3, xmm3, 0x1B
                movups  xmm4, [edx+45*4]
                shufps  xmm4, xmm4, 0x1B   ; 4#0123# = 0x1B
                movups  xmm5, [edx+41*4]
                shufps  xmm5, xmm5, 0x1B
                movups  xmm6, [edx+37*4]
                shufps  xmm6, xmm6, 0x1B
                movups  xmm7, [edx+33*4]
                shufps  xmm7, xmm7, 0x1B

                movups  [edx+29*4], xmm0
                movups  [edx+25*4], xmm1
                movups  [edx+21*4], xmm2
                movups  [edx+17*4], xmm3
                movaps  [edx+48*4], xmm4
                movaps  [edx+52*4], xmm5
                movaps  [edx+56*4], xmm6
                movaps  [edx+60*4], xmm7

endproc


;  . . . . . . . .   . . . . . . . -
;  + . . . . . . .   . . . . . . . .
;  . . . . . . . .   . . . - . . . .
;  . . . . + . . .   . . . - . . . .
;  . . . . . . . .   . . . . . - . .
;  . . + . . . . .   . - . . . . . .
;  . . . . . . . .   . - . . . - . .
;  . . + . . . + .   . - . . . - . .
;  . . . . . . . .   . . . . . . - .
;  . + . . . . . .   - . . . . . . .
;  . . . . . . . .   . . - . - . . .
;  . . . + . + . .   . . - . - . . .
;  . . . . . . . .   . . . . - . - .
;  . + . + . . . .   - . - . . . . .
;  . . . . . . . .   - . - . - . - .
;  . + . + . + . +   - . - . - . - .


;  . . . . . . . .   . . . . . . . -
;  + . . . . . . .   - . . . . . . .
;  . . . . . . . .   . . . - - . . .
;  . . . + + . . .   . . . - - . . .
;  . . . . . . . .   . . . . . - - .
;  . + + . . . . .   . - - . . . . .
;  . . . . . . . .   . - - . . - - .
;  . + + . . + + .   . - - . . - - .
;  . . . . . . . .   . . . . . . - -
;  + + . . . . . .   - - . . . . . .
;  . . . . . . . .   . . - - - - . .
;  . . + + + + . .   . . - - - - . .
;  . . . . . . . .   . . . . - - - -
;  + + + + . . . .   - - - - . . . .
;  . . . . . . . .   - - - - - - - -
;  + + + + + + + +   - - - - - - - -


;****************************************************************************

                align 4
proc            Reset_FPU_3DNow
                femms
endproc

;*****************************************************************************

                align 4
proc            Reset_FPU
                emms
endproc

;******************************************************************************

                align   32
                times   5 nop
proc            memcpy_dn_MMX
$dst1           arg     4
$src1           arg     4
$words1         arg     4
                mov     eax, [sp($dst1)]
                mov     edx, [sp($src1)]
                mov     ecx, [sp($words1)]
                shl     ecx, 6
                lea     edx, [edx+ecx-64]
                lea     eax, [eax+ecx-64]
                mov     ecx, [sp($words1)]
lbl3:
                pmov    mm0, qword [edx+ 0]
                pmov    mm1, qword [edx+ 8]
                pmov    mm2, qword [edx+16]
                pmov    mm3, qword [edx+24]
                pmov    mm4, qword [edx+32]
                pmov    mm5, qword [edx+40]
                pmov    mm6, qword [edx+48]
                pmov    mm7, qword [edx+56]
                add     edx, byte -64

                pmov    qword [eax+ 0], mm0
                pmov    qword [eax+ 8], mm1
                pmov    qword [eax+16], mm2
                pmov    qword [eax+24], mm3
                pmov    qword [eax+32], mm4
                pmov    qword [eax+40], mm5
                pmov    qword [eax+48], mm6
                pmov    qword [eax+56], mm7
                add     eax, byte -64

                dec     ecx
                jnz     short lbl3
endproc

;********************************************************************************

                align   32
                times   5 nop
proc            memcpy_dn_SIMD
$dst2           arg     4
$src2           arg     4
$words2         arg     4
                mov     eax, [sp($dst2)]
                mov     edx, [sp($src2)]
                mov     ecx, [sp($words2)]
                shl     ecx, 7
                lea     edx, [edx+ecx-128]
                lea     eax, [eax+ecx-128]
                mov     ecx, [sp($words2)]
lbl4:
                movaps  xmm0, [edx+  0]
                movaps  xmm1, [edx+ 16]
                movaps  xmm2, [edx+ 32]
                movaps  xmm3, [edx+ 48]
                movaps  xmm4, [edx+ 64]
                movaps  xmm5, [edx+ 80]
                movaps  xmm6, [edx+ 96]
                movaps  xmm7, [edx+112]
                add     edx, byte -128

                movaps  [eax+  0], xmm0
                movaps  [eax+ 16], xmm1
                movaps  [eax+ 32], xmm2
                movaps  [eax+ 48], xmm3
                movaps  [eax+ 64], xmm4
                movaps  [eax+ 80], xmm5
                movaps  [eax+ 96], xmm6
                movaps  [eax+112], xmm7
                add     eax, byte -128

                dec     ecx
                jnz     short lbl4
endproc


;##################################################################################################################


                align   32
proc            Calculate_New_V_i387
$S9             arg     4
$V9             arg     4
                mov     ecx, [sp($S9)]
                mov     edx, [sp($V9)]
                sub     edx, byte -128
                push    ebp
                mov     eax, C
                add     esp, byte -128
                mov     ebp, esp

%macro          op1     2
                fld     _S(%1)          ; S00
                fadd    _S(31-%1)       ; A00
                fld     _S(15-%1)       ; S15           A00
                fadd    _S(16+%1)       ; A08           A00
                fld     st1             ; A00           A08     A00
                fsub    st0, st1        ; A00-A08       A08     A00
                fmul    %2              ; B08           A08     A00
                fxch    st2             ; A00           A08     B08
                faddp   st1             ; B00           B08
%endmacro

%macro          opx     2               ; B04           B12     B00     B08
                fld     st2             ; B00           B04     B12     B00     B08
                fsub    st0, st1        ; B00-B04       B04     B12     B00     B08
                fmul    %2              ; A04           B04     B12     B00     B08
                fstp    _A(%1+4)        ; B04           B12     B00     B08
                fld     st3             ; B08           B04     B12     B00     B08
                fsub    st0, st2        ; B08-B12       B04     B12     B00     B08
                fmul    %2              ; A12           B04     B12     B00     B08
                fstp    _A(%1+12)       ; B04           B12     B00     B08
                faddp   st2             ; B12           A00     B08
                faddp   st2             ; A00           A08
                fstp    _A(%1)          ; A08
                fstp    _A(%1+8)        ;
%endmacro

;    A00 = Sample[ 0] + Sample[31];
;    A08 = Sample[15] + Sample[16];
;    B00 =  A00 + A08;
;    B08 = (A00 - A08) * C[ 2];
;    A04 = Sample[ 7] + Sample[24];
;    A12 = Sample[ 8] + Sample[23];
;    B04 =  A04 + A12;
;    B12 = (A04 - A12) * C[30];

                op1      0, _C02
                op1      7, _C30

;   A00 =  B00 + B04;
;   A04 = (B00 - B04) * C[ 4];
;   A08 =  B08 + B12;
;   A12 = (B08 - B12) * C[ 4];

                opx      0, _C04

;    A01 = Sample[ 1] + Sample[30];
;    A09 = Sample[14] + Sample[17];
;    B01 =  A01 + A09;
;    B09 = (A01 - A09) * C[ 6];
;    A05 = Sample[ 6] + Sample[25];
;    A13 = Sample[ 9] + Sample[22];
;    B05 =  A05 + A13;
;    B13 = (A05 - A13) * C[26];

                op1      1, _C06
                op1      6, _C26

;    A01 =  B01 + B05;
;    A05 = (B01 - B05) * C[12];
;    A09 =  B09 + B13;
;    A13 = (B09 - B13) * C[12];

                opx      1, _C12

;    A02 = Sample[ 3] + Sample[28];
;    A10 = Sample[12] + Sample[19];
;    B02 =  A02 + A10;
;    B10 = (A02 - A10) * C[14];
;    A06 = Sample[ 4] + Sample[27];
;    A14 = Sample[11] + Sample[20];
;    B06 =  A06 + A14;
;    B14 = (A06 - A14) * C[18];

                op1      3, _C14
                op1      4, _C18

;    A02 =  B02 + B06;
;    A06 = (B02 - B06) * C[28];
;    A10 =  B10 + B14;
;    A14 = (B10 - B14) * C[28];

                opx      2, _C28

;    A03 = Sample[ 2] + Sample[29];
;    A11 = Sample[13] + Sample[18];
;    B03 =  A03 + A11;
;    B11 = (A03 - A11) * C[10];
;    A07 = Sample[ 5] + Sample[26];
;    A15 = Sample[10] + Sample[21];
;    B07 =  A07 + A15;
;    B15 = (A07 - A15) * C[22];

                op1      2, _C10
                op1      5, _C22

;    A03 =  B03 + B07;
;    A07 = (B03 - B07) * C[20];
;    A11 =  B11 + B15;
;    A15 = (B11 - B15) * C[20];

                opx      3, _C20


%macro          op2     1
                fld     _A(%1)          ; A00
                fadd    _A(%1+2)        ; B00
                fld     _A(%1+1)        ; A01           B00
                fadd    _A(%1+3)        ; B01           B00
                fld     st1             ; B00           B01     B00
                fsub    st0, st1        ; B00-B01       B01     B00
                fmul    _C16            ; A01           B01     B00
                fstp    _B(%1+1)        ; B01           B00
                faddp   st1             ; A00
                fstp    _B(%1)          ;
                fld     _A(%1)          ; A00
                fsub    _A(%1+2)        ; A00-A02
                fmul    _C08            ; B02
                fld     _A(%1+1)        ; A01           B02
                fsub    _A(%1+3)        ; A01-A03       B02
                fmul    _C24            ; B03           B02
                fld     st1             ; B02           B03     B02
                fsub    st0, st1        ; B02-B03       B03     B02
                fmul    _C16            ; A03           B03     B02
                fstp    _B(%1+3)        ; B03           B02
                faddp   st1             ; A02
                fstp    _B(%1+2)        ;
%endmacro

;    B00 =  A00 + A02;
;    B01 =  A01 + A03;
;    A00 =  B00 + B01;
;    A01 = (B00 - B01) * C[16];
;    B02 = (A00 - A02) * C[ 8];
;    B03 = (A01 - A03) * C[24];
;    A02 =  B02 + B03;
;    A03 = (B02 - B03) * C[16];

                op2     0

;    B04 =  A04 + A06;
;    B05 =  A05 + A07;
;    A04 =  B04 + B05;
;    A05 = (B04 - B05) * C[16];
;    B06 = (A04 - A06) * C[ 8];
;    B07 = (A05 - A07) * C[24];
;    A06 =  B06 + B07;
;    A07 = (B06 - B07) * C[16];

                op2     4

;    B08 =  A08 + A10;
;    B09 =  A09 + A11;
;    A08 =  B08 + B09;
;    A09 = (B08 - B09) * C[16];
;    B10 = (A08 - A10) * C[ 8];
;    B11 = (A09 - A11) * C[24];
;    A10 =  B10 + B11;
;    A11 = (B10 - B11) * C[16];

                op2     8

;    B12 =  A12 + A14;
;    B13 =  A13 + A15;
;    A12 =  B12 + B13;
;    A13 = (B12 - B13) * C[16];
;    B14 = (A12 - A14) * C[ 8];
;    B15 = (A13 - A15) * C[24];
;    A14 =  B14 + B15;
;    A15 = (B14 - B15) * C[16];

                op2     12

;   V[48] = -A00;
;   V[ 0] =  A01;
;   V[40] = -A02 - (V[ 8] = A03);

                fld     _B(0)
                fchs
                fstp    _V(48)
                fld     _B(1)
                fstp    _V(0)
                fld     _B(3)
                fst     _V(8)
                fadd    _B(2)
                fchs
                fstp    _V(40)

;   V[36] = -((V[ 4] = A05 + (V[12] = A07)) + A06);
;   V[44] = - A04 - A06 - A07;

                fld     _B(7)
                fst     _V(12)
                fadd    _B(5)
                fst     _V(4)
                fadd    _B(6)
                fchs
                fstp    _V(36)
                fld     _B(4)
                fadd    _B(6)
                fadd    _B(7)
                fchs
                fstp    _V(44)

;   V[ 6] = (V[10] = A11 + (V[14] = A15)) + A13;
;   V[38] = (V[34] = -(V[ 2] = A09 + A13 + A15) - A14) + A09 - A10 - A11;

                fld     _B(15)
                fst     _V(14)
                fadd    _B(11)
                fst     _V(10)
                fadd    _B(13)
                fstp    _V(6)
                fld     _B(9)
                fadd    _B(13)
                fadd    _B(15)
                fst     _V(2)
                fadd    _B(14)
                fchs
                fst     _V(34)
                fadd    _B(9)
                fsub    _B(10)
                fsub    _B(11)
                fstp    _V(38)

;   V[46] = (tmp = -(A12 + A14 + A15)) - A08;

                fld     _B(12)
                fadd    _B(14)
                fadd    _B(15)
                fchs
                fld     st0
                fsub    _B(8)
                fstp    _V(46)

;   V[42] = tmp - A10 - A11;                            // abhängig vom Befehl drüber

                fsub    _B(10)
                fsub    _B(11)
                fstp    _V(42)


%macro          op4     4
                fld     _S(%1)          ; S00
                fsub    _S(31-%1)       ; A00
                fmul    %2
                fld     _S(15-%1)       ; S15           A00
                fsub    _S(16+%1)       ; A08           A00
                fmul    %3
                fld     st1             ; A00           A08     A00
                fsub    st0, st1        ; A00-A08       A08     A00
                fmul    %4              ; B08           A08     A00
                fxch    st2             ; A00           A08     B08
                faddp   st1             ; B00           B08
%endmacro

;    A00 = (Sample[ 0] - Sample[31]) * C[ 1];
;    A08 = (Sample[15] - Sample[16]) * C[31];
;    B00 =  A00 + A08;
;    B08 = (A00 - A08) * C[ 2];
;    A04 = (Sample[ 7] - Sample[24]) * C[15];
;    A12 = (Sample[ 8] - Sample[23]) * C[17];
;    B04 =  A04 + A12;
;    B12 = (A04 - A12) * C[30];

                op4      0, _C01, _C31, _C02
                op4      7, _C15, _C17, _C30

;   A00 =  B00 + B04;
;   A04 = (B00 - B04) * C[ 4];
;   A08 =  B08 + B12;
;   A12 = (B08 - B12) * C[ 4];

                opx      0, _C04

;    A01 = (Sample[ 1] - Sample[30]) * C[ 3];
;    A09 = (Sample[14] - Sample[17]) * C[29];
;    B01 =  A01 + A09;
;    B09 = (A01 - A09) * C[ 6];
;    A05 = (Sample[ 6] - Sample[25]) * C[13];
;    A13 = (Sample[ 9] - Sample[22]) * C[19];
;    B05 =  A05 + A13;
;    B13 = (A05 - A13) * C[26];

                op4      1, _C03, _C29, _C06
                op4      6, _C13, _C19, _C26

;    A01 =  B01 + B05;
;    A05 = (B01 - B05) * C[12];
;    A09 =  B09 + B13;
;    A13 = (B09 - B13) * C[12];

                opx      1, _C12

;    A02 = (Sample[ 3] - Sample[28]) * C[ 7];
;    A10 = (Sample[12] - Sample[19]) * C[25];
;    B02 =  A02 + A10;
;    B10 = (A02 - A10) * C[14];
;    A06 = (Sample[ 4] - Sample[27]) * C[ 9];
;    A14 = (Sample[11] - Sample[20]) * C[23];
;    B06 =  A06 + A14;
;    B14 = (A06 - A14) * C[18];

                op4      3, _C07, _C25, _C14
                op4      4, _C09, _C23, _C18

;    A02 =  B02 + B06;
;    A06 = (B02 - B06) * C[28];
;    A10 =  B10 + B14;
;    A14 = (B10 - B14) * C[28];

                opx      2, _C28

;    A03 = (Sample[ 2] - Sample[29]) * C[ 5];
;    A11 = (Sample[13] - Sample[18]) * C[27];
;    B03 =  A03 + A11;
;    B11 = (A03 - A11) * C[10];
;    A07 = (Sample[ 5] - Sample[26]) * C[11];
;    A15 = (Sample[10] - Sample[21]) * C[21];
;    B07 =  A07 + A15;
;    B15 = (A07 - A15) * C[22];

                op4      2, _C05, _C27, _C10
                op4      5, _C11, _C21, _C22

;    A03 =  B03 + B07;
;    A07 = (B03 - B07) * C[20];
;    A11 =  B11 + B15;
;    A15 = (B11 - B15) * C[20];

                opx      3, _C20

;    B00 =  A00 + A02;
;    B01 =  A01 + A03;
;    A00 =  B00 + B01;
;    A01 = (B00 - B01) * C[16];
;    B02 = (A00 - A02) * C[ 8];
;    B03 = (A01 - A03) * C[24];
;    A02 =  B02 + B03;
;    A03 = (B02 - B03) * C[16];

                op2     0

;    B04 =  A04 + A06;
;    B05 =  A05 + A07;
;    A04 =  B04 + B05;
;    A05 = (B04 - B05) * C[16];
;    B06 = (A04 - A06) * C[ 8];
;    B07 = (A05 - A07) * C[24];
;    A06 =  B06 + B07;
;    A07 = (B06 - B07) * C[16];

                op2     4

;    B08 =  A08 + A10;
;    B09 =  A09 + A11;
;    A08 =  B08 + B09;
;    A09 = (B08 - B09) * C[16];
;    B10 = (A08 - A10) * C[ 8];
;    B11 = (A09 - A11) * C[24];
;    A10 =  B10 + B11;
;    A11 = (B10 - B11) * C[16];

                op2     8

;    B12 =  A12 + A14;
;    B13 =  A13 + A15;
;    A12 =  B12 + B13;
;    A13 = (B12 - B13) * C[16];
;    B14 = (A12 - A14) * C[ 8];
;    B15 = (A13 - A15) * C[24];
;    A14 =  B14 + B15;
;    A15 = (B14 - B15) * C[16];

                op2     12

;   V[ 5] = (V[11] = (V[13] = A07 + (V[15] = A15)) + A11) + A05 + A13;

                fld     _B(15)
                fst     _V(15)
                fadd    _B(7)
                fst     _V(13)
                fadd    _B(11)
                fst     _V(11)
                fadd    _B(5)
                fadd    _B(13)
                fstp    _V(5)

;   V[ 7] = (V[ 9] = A03 + A11 + A15) + A13;

                fld     _B(3)
                fadd    _B(11)
                fadd    _B(15)
                fst     _V(9)
                fadd    _B(13)
                fstp    _V(7)

;   V[33] = -(V[ 1] = A01 + A09 + A13 + A15) - A14;

                fld     _B(1)
                fadd    _B(9)
                fadd    _B(13)
                fadd    _B(15)
                fst     _V(1)
                fadd    _B(14)
                fchs
                fstp    _V(33)

;   V[35] = -(V[ 3] = A05 + A07 + A09 + A13 + A15) - A06 - A14;

                fld     _B(5)
                fadd    _B(7)
                fadd    _B(9)
                fadd    _B(13)
                fadd    _B(15)
                fst     _V(3)
                fadd    _B(6)
                fadd    _B(14)
                fchs
                fstp    _V(35)

;   V[37] = (tmp = -(A10 + A11 + A13 + A14 + A15)) - A05 - A06 - A07;

                fld     _B(10)
                fadd    _B(11)
                fadd    _B(13)
                fadd    _B(14)
                fadd    _B(15)
                fchs
                fld     st0
                fsub    _B(5)
                fsub    _B(6)
                fsub    _B(7)
                fstp    _V(37)

;   V[39] = tmp - A02 - A03;                                            // abhängig vom Befehl drüber

                fld     st0
                fsub    _B(2)
                fsub    _B(3)
                fstp    _V(39)

;   V[41] = (tmp += A13 - A12) - A02 - A03;                             // abhängig vom Befehl 2 drüber

                fadd    _B(13)
                fsub    _B(12)
                fld     st0
                fsub    _B(2)
                fsub    _B(3)
                fstp    _V(41)

;   V[43] = tmp - A04 - A06 - A07;                                      // abhängig von Befehlen 1 und 3 drüber

                fsub    _B(4)
                fsub    _B(6)
                fsub    _B(7)
                fstp    _V(43)

;   V[47] = (tmp = -(A08 + A12 + A14 + A15)) - A00;

                fld     _B(8)
                fadd    _B(12)
                fadd    _B(14)
                fadd    _B(15)
                fchs
                fld     st0
                fsub    _B(0)
                fstp    _V(47)

;   V[45] = tmp - A04 - A06 - A07;                                      // abhängig vom Befehl drüber

                fsub    _B(4)
                fsub    _B(6)
                fsub    _B(7)
                fstp    _V(45)

;    ((Uint32_t*)V)[32-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[ 0-32];
;    ((Uint32_t*)V)[31-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[ 1-32];
;    ((Uint32_t*)V)[30-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[ 2-32];
;    ((Uint32_t*)V)[29-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[ 3-32];
;    ((Uint32_t*)V)[28-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[ 4-32];
;    ((Uint32_t*)V)[27-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[ 5-32];
;    ((Uint32_t*)V)[26-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[ 6-32];
;    ((Uint32_t*)V)[25-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[ 7-32];
;    ((Uint32_t*)V)[24-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[ 8-32];
;    ((Uint32_t*)V)[23-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[ 9-32];
;    ((Uint32_t*)V)[22-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[10-32];
;    ((Uint32_t*)V)[21-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[11-32];
;    ((Uint32_t*)V)[20-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[12-32];
;    ((Uint32_t*)V)[19-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[13-32];
;    ((Uint32_t*)V)[18-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[14-32];
;    ((Uint32_t*)V)[17-32] = (Uint32_t)0x80000000L + ((Uint32_t*)V)[15-32];

                mov     ecx, 0x80000000
%assign i 0
%rep 16
                mov     eax, _V(i)
                add     eax, ecx
                mov     _V(32-i), eax
%assign i i+1
%endrep

;    ((Uint32_t*)V)[63-32] = ((Uint32_t*)V)[33-32];
;    ((Uint32_t*)V)[62-32] = ((Uint32_t*)V)[34-32];
;    ((Uint32_t*)V)[61-32] = ((Uint32_t*)V)[35-32];
;    ((Uint32_t*)V)[60-32] = ((Uint32_t*)V)[36-32];
;    ((Uint32_t*)V)[59-32] = ((Uint32_t*)V)[37-32];
;    ((Uint32_t*)V)[58-32] = ((Uint32_t*)V)[38-32];
;    ((Uint32_t*)V)[57-32] = ((Uint32_t*)V)[39-32];
;    ((Uint32_t*)V)[56-32] = ((Uint32_t*)V)[40-32];
;    ((Uint32_t*)V)[55-32] = ((Uint32_t*)V)[41-32];
;    ((Uint32_t*)V)[54-32] = ((Uint32_t*)V)[42-32];
;    ((Uint32_t*)V)[53-32] = ((Uint32_t*)V)[43-32];
;    ((Uint32_t*)V)[52-32] = ((Uint32_t*)V)[44-32];
;    ((Uint32_t*)V)[51-32] = ((Uint32_t*)V)[45-32];
;    ((Uint32_t*)V)[50-32] = ((Uint32_t*)V)[46-32];
;    ((Uint32_t*)V)[49-32] = ((Uint32_t*)V)[47-32];

%assign i 1
%rep 15
                mov     eax, _V(32+i)
                mov     _V(64-i), eax
%assign i i+1
%endrep

                sub     esp, byte -128
                pop     ebp
endproc

;
; end of synthasm.nas
;
