; RUN: llc < %s -mtriple=x86_64-apple-darwin -mcpu=corei7 -mattr=+sse4.1 | FileCheck %s


; Verify that we produce movss instead of blendvps when possible.

;CHECK-LABEL: vsel_float:
;CHECK-NOT: blend
;CHECK: movss
;CHECK: ret
define <4 x float> @vsel_float(<4 x float> %v1, <4 x float> %v2) {
  %vsel = select <4 x i1> <i1 true, i1 false, i1 false, i1 false>, <4 x float> %v1, <4 x float> %v2
  ret <4 x float> %vsel
}

;CHECK-LABEL: vsel_4xi8:
;CHECK-NOT: blend
;CHECK: movss
;CHECK: ret
define <4 x i8> @vsel_4xi8(<4 x i8> %v1, <4 x i8> %v2) {
  %vsel = select <4 x i1> <i1 true, i1 false, i1 false, i1 false>, <4 x i8> %v1, <4 x i8> %v2
  ret <4 x i8> %vsel
}

;CHECK-LABEL: vsel_8xi16:
;CHECK: pblendw {{.*}} ## xmm0 = xmm0[0],xmm1[1,2,3],xmm0[4],xmm1[5,6,7]
;CHECK: ret
define <8 x i16> @vsel_8xi16(<8 x i16> %v1, <8 x i16> %v2) {
  %vsel = select <8 x i1> <i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 false, i1 false>, <8 x i16> %v1, <8 x i16> %v2
  ret <8 x i16> %vsel
}
