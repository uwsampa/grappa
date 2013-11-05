add_llvm_loadable_module( LLVMGrappaGen GrappaGen.cpp )
set_target_properties( LLVMGrappaGen PROPERTIES COMPILE_FLAGS "-fno-rtti" )
