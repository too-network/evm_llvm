;This isn't really an assembly file, its just here to run the test.
;This test just makes sure that llvm-ar can generate a symbol table for
;MacOSX style archives
;RUN: cp %p/MacOSX.a . 
;RUN: llvm-ranlib MacOSX.a
;RUN: llvm-ar t MacOSX.a > %t1
;RUN: sed -e '/^;.*/d' %s >%t2
;RUN: diff %t2 %t1
__.SYMDEF SORTED    
evenlen     
oddlen      
very_long_bytecode_file_name.bc     
IsNAN.o     
