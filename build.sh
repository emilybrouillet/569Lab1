mkdir -p build && cd build 

cmake ../
make 

$LLVM_DIR/bin/clang++ -emit-llvm -S ../cfi_icall.c -o cfi_icall.ll
$LLVM_DIR/bin/opt -f -enable-new-pm=0 -load ./libHelloWorld.so -hello -S cfi_icall.ll -o transformed_cfi_call.ll
$LLVM_DIR/bin/clang++ transformed_cfi_call.ll ../cfi_check_lib.cpp -o exe_cfi_icall
