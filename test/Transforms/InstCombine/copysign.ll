; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -S -instcombine < %s | FileCheck %s

declare float @llvm.fabs.f32(float)
declare float @llvm.copysign.f32(float, float)
declare <3 x double> @llvm.copysign.v3f64(<3 x double>, <3 x double>)

define float @positive_sign_arg(float %x) {
; CHECK-LABEL: @positive_sign_arg(
; CHECK-NEXT:    [[TMP1:%.*]] = call arcp float @llvm.fabs.f32(float [[X:%.*]])
; CHECK-NEXT:    ret float [[TMP1]]
;
  %r = call arcp float @llvm.copysign.f32(float %x, float 0.0)
  ret float %r
}

define <3 x double> @positive_sign_arg_vec_splat(<3 x double> %x) {
; CHECK-LABEL: @positive_sign_arg_vec_splat(
; CHECK-NEXT:    [[TMP1:%.*]] = call ninf <3 x double> @llvm.fabs.v3f64(<3 x double> [[X:%.*]])
; CHECK-NEXT:    ret <3 x double> [[TMP1]]
;
  %r = call ninf <3 x double> @llvm.copysign.v3f64(<3 x double> %x, <3 x double> <double 42.0, double 42.0, double 42.0>)
  ret <3 x double> %r
}

define float @negative_sign_arg(float %x) {
; CHECK-LABEL: @negative_sign_arg(
; CHECK-NEXT:    [[TMP1:%.*]] = call nnan float @llvm.fabs.f32(float [[X:%.*]])
; CHECK-NEXT:    [[TMP2:%.*]] = fneg nnan float [[TMP1]]
; CHECK-NEXT:    ret float [[TMP2]]
;
  %r = call nnan float @llvm.copysign.f32(float %x, float -0.0)
  ret float %r
}

define <3 x double> @negative_sign_arg_vec_splat(<3 x double> %x) {
; CHECK-LABEL: @negative_sign_arg_vec_splat(
; CHECK-NEXT:    [[TMP1:%.*]] = call fast <3 x double> @llvm.fabs.v3f64(<3 x double> [[X:%.*]])
; CHECK-NEXT:    [[TMP2:%.*]] = fneg fast <3 x double> [[TMP1]]
; CHECK-NEXT:    ret <3 x double> [[TMP2]]
;
  %r = call fast <3 x double> @llvm.copysign.v3f64(<3 x double> %x, <3 x double> <double -42.0, double -42.0, double -42.0>)
  ret <3 x double> %r
}

define float @known_positive_sign_arg(float %x, float %y) {
; CHECK-LABEL: @known_positive_sign_arg(
; CHECK-NEXT:    [[FABS:%.*]] = call float @llvm.fabs.f32(float [[Y:%.*]])
; CHECK-NEXT:    [[R:%.*]] = call ninf float @llvm.copysign.f32(float [[X:%.*]], float [[FABS]])
; CHECK-NEXT:    ret float [[R]]
;
  %fabs = call float @llvm.fabs.f32(float %y)
  %r = call ninf float @llvm.copysign.f32(float %x, float %fabs)
  ret float %r
}

define <3 x double> @known_positive_sign_arg_vec(<3 x double> %x, <3 x i32> %y) {
; CHECK-LABEL: @known_positive_sign_arg_vec(
; CHECK-NEXT:    [[YF:%.*]] = uitofp <3 x i32> [[Y:%.*]] to <3 x double>
; CHECK-NEXT:    [[R:%.*]] = call arcp <3 x double> @llvm.copysign.v3f64(<3 x double> [[X:%.*]], <3 x double> [[YF]])
; CHECK-NEXT:    ret <3 x double> [[R]]
;
  %yf = uitofp <3 x i32> %y to <3 x double>
  %r = call arcp <3 x double> @llvm.copysign.v3f64(<3 x double> %x, <3 x double> %yf)
  ret <3 x double> %r
}
