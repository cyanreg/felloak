    .text
    .file "OUTPUT.wasm"
    .section .text.ff_opus_deemphasis_neon,,@
    .hidden ff_opus_deemphasis_neon
    .globl ff_opus_deemphasis_neon
    .type ff_opus_deemphasis_neon,@function
ff_opus_deemphasis_neon:
    .functype () -> ()
    .locali32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, 
    v128.load (local.get 0) (local.get 0)
    v128.load (local.get 0) (local.get 0)
    v128.load (local.get 0) (local.get 0)
    v128.load (local.get 0) (local.get 0)
    f32x4.mul (local.get 0) (local.get 0) (local.get 0)
    v128.load (local.get 0) (local.get 0)
    v128.load (local.get 0) (local.get 0)
    v128.load (local.get 0) (local.get 0)
    v128.load (local.get 0) (local.get 0)
    f32x4.mul (local.get 0) (local.get 0) (local.get 0)
    .section .text.ff_opus_postfilter_neon,,@
    .hidden ff_opus_postfilter_neon
    .globl ff_opus_postfilter_neon
    .type ff_opus_postfilter_neon,@function
ff_opus_postfilter_neon:
    .functype () -> ()
    .locali32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, 
    v128.load (local.get 4) (local.get 4)
    v128.load (local.get 0) (local.get 0)
    v128.load (local.get 0) (local.get 0)
    v128.load (local.get 0) (local.get 0)
    f32x4.add (local.get 0) (local.get 0) (local.get 0)
    v128.load (local.get 0) (local.get 0)
    f32x4.mul (local.get 0) (local.get 0) (local.get 0)
    f32x4.add (local.get 0) (local.get 0) (local.get 0)
    f32x4.add (local.get 0) (local.get 0) (local.get 0)
    f32x4.mul (local.get 0) (local.get 0) (local.get 0)
    v128.store (local.get 0) (local.get 0)
    v128.store (local.get 0) (local.get 0)
    v128.load (local.get 4) (local.get 4)
    v128.load (local.get 0) (local.get 0)
    v128.load (local.get 0) (local.get 0)
    v128.load (local.get 0) (local.get 0)
    f32x4.add (local.get 0) (local.get 0) (local.get 0)
    v128.load (local.get 0) (local.get 0)
    f32x4.mul (local.get 0) (local.get 0) (local.get 0)
    f32x4.add (local.get 0) (local.get 0) (local.get 0)
    f32x4.add (local.get 0) (local.get 0) (local.get 0)
    f32x4.mul (local.get 0) (local.get 0) (local.get 0)
    v128.store (local.get 0) (local.get 0)
    v128.store (local.get 0) (local.get 0)
    .type tab_st,@object
    .section .rodata.tab_st,,@
    .p2align 4
tab_st:
    .int8 0
    .int8 -102
    .int8 89
    .int8 63
    .int8 113
    .int8 -10
    .int8 56
    .int8 63
    .int8 42
    .int8 56
    .int8 29
    .int8 63
    .int8 47
    .int8 -93
    .int8 5
    .int8 63
.size tab_st, 16

    .type tab_x0,@object
    .section .rodata.tab_x0,,@
    .p2align 4
tab_x0:
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 -102
    .int8 89
    .int8 63
    .int8 113
    .int8 -10
    .int8 56
    .int8 63
    .int8 42
    .int8 56
    .int8 29
    .int8 63
.size tab_x0, 16

    .type tab_x1,@object
    .section .rodata.tab_x1,,@
    .p2align 4
tab_x1:
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 -102
    .int8 89
    .int8 63
    .int8 113
    .int8 -10
    .int8 56
    .int8 63
.size tab_x1, 16

    .type tab_x2,@object
    .section .rodata.tab_x2,,@
    .p2align 4
tab_x2:
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 0
    .int8 -102
    .int8 89
    .int8 63
.size tab_x2, 16

    .ident "felloak 0.1.0 53e33ec"
    .globaltype     __stack_pointer, i32
