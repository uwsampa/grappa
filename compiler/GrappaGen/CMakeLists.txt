add_llvm_loadable_module( LLVMGrappaGen GrappaGen.cpp )
set_target_properties( LLVMGrappaGen PROPERTIES COMPILE_FLAGS "-fno-rtti" )
set_property(TARGET LLVMGrappaGen PROPERTY FOLDER "Compiler")
