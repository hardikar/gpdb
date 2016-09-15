awk '/#include <llvm\/Analysis\/Verifier.h>/ { sub(/Analysis/, "IR") } /^Module\* makeLLVMModule\(\);/ { main=1 } /^Module\* makeLLVMModule\(\) {/ { main=0; print "Module* makeLLVMModule(Module* mod) {";next}  /\/\/ Module Construction/ { mod=1 } /\/\/ Type Definitions/ {mod=0} /args\+\+/ { sub(/args\+\+/,"*\\&args; ++args")} 1 { if (!mod && !main) print }' $1 > tmp
sed -ie 's/getGetElementPtr(\([^,]*\)/getGetElementPtr(nullptr, \1/' tmp
sed -ie 's/GetElementPtrInst::Create(\([^,]*\)/GetElementPtrInst::Create(nullptr, \1/' tmp
sed -ie 's/ConstantVector::get([^,]*,/ConstantVector::get(/' tmp
mv tmp ${1}
