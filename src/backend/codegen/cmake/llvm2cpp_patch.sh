awk '/#include <llvm\/Analysis\/Verifier.h>/ { sub(/Analysis/, "IR") } /^Module\* makeLLVMModule\(\);/ { main=1 } /^Module\* makeLLVMModule\(\) {/ { main=0; print "Module* makeLLVMModule(Module* mod) {";next}  /\/\/ Module Construction/ { mod=1 } /\/\/ Type Definitions/ {mod=0} /args\+\+/ { sub(/args\+\+/,"*\\&args; ++args")} 1 { if (!mod && !main) print }' $1 > tmp
mv tmp ${1}
