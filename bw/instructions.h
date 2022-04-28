#pragma once




#define READ_NT_64_ASM \
    "vmovntdqa 0(%[addr]), %%zmm0 \n"

#define READ_NT_128_ASM \
    "vmovntdqa 0(%[addr]), %%zmm0 \n"\
    "vmovntdqa 1*64(%[addr]), %%zmm1 \n"

#define READ_NT_256_ASM \
    "vmovntdqa 0(%[addr]),      %%zmm0 \n" \
    "vmovntdqa 1*64(%[addr]),   %%zmm1 \n" \
    "vmovntdqa 2*64(%[addr]),   %%zmm1 \n" \
    "vmovntdqa 3*64(%[addr]),   %%zmm1 \n"

#define READ_NT_512_ASM \
    "vmovntdqa 0*64(%[addr]),   %%zmm0 \n" \
    "vmovntdqa 1*64(%[addr]),   %%zmm1 \n" \
    "vmovntdqa 2*64(%[addr]),   %%zmm1 \n" \
    "vmovntdqa 3*64(%[addr]),   %%zmm1 \n" \
    "vmovntdqa 4*64(%[addr]),   %%zmm1 \n" \
    "vmovntdqa 5*64(%[addr]),   %%zmm1 \n" \
    "vmovntdqa 6*64(%[addr]),   %%zmm1 \n" \
    "vmovntdqa 7*64(%[addr]),   %%zmm1 \n"
