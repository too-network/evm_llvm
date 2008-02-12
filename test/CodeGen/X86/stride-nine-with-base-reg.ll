; RUN: llvm-as < %s | llc -march=x86 -relocation-model=static | grep lea | count 1
; RUN: llvm-as < %s | llc -march=x86-64 -relocation-model=pic | not grep lea

; For x86 there's an lea above the loop. In both cases, there shouldn't
; be any lea instructions inside the loop.

@B = external global [1000 x i8], align 32
@A = external global [1000 x i8], align 32
@P = external global [1000 x i8], align 32

define void @foo(i32 %m, i32 %p) {
entry:
	%tmp1 = icmp sgt i32 %m, 0
	br i1 %tmp1, label %bb, label %return

bb:
	%i.019.0 = phi i32 [ %indvar.next, %bb ], [ 0, %entry ]
	%tmp2 = getelementptr [1000 x i8]* @B, i32 0, i32 %i.019.0
	%tmp3 = load i8* %tmp2, align 4
	%tmp4 = mul i8 %tmp3, 2
	%tmp5 = getelementptr [1000 x i8]* @A, i32 0, i32 %i.019.0
	store i8 %tmp4, i8* %tmp5, align 4
	%tmp8 = mul i32 %i.019.0, 9
        %tmp0 = add i32 %tmp8, %p
	%tmp10 = getelementptr [1000 x i8]* @P, i32 0, i32 %tmp0
	store i8 17, i8* %tmp10, align 4
	%indvar.next = add i32 %i.019.0, 1
	%exitcond = icmp eq i32 %indvar.next, %m
	br i1 %exitcond, label %return, label %bb

return:
	ret void
}

