#!/usr/bin/env ruby
require 'fileutils'; require 'ostruct'; require 'optparse'
include FileUtils
opt = OpenStruct.new
opt.prefix = "#{Dir.pwd}/install"

OptionParser.new {|p|
  p.on("--prefix=/install/path"){|_| opt.prefix = _ }
}.parse!(ARGV)

system "git clone http://llvm.org/git/llvm.git"
system "git clone http://llvm.org/git/clang.git llvm/tools/clang"
system "git clone http://llvm.org/git/lldb.git llvm/tools/lldb"
# system "git clone http://llvm.org/git/clang-tools-extra.git llvm/tools/clang/tools/extra"
# system "git clone http://llvm.org/git/libcxx.git llvm/projects/libcxx"

# find prefix for current GCC so we can have clang use its stdlib headers, etc
gcc_dir = `which gcc`[/(.*)\/bin\/gcc/,1]

cd(*mkdir_p("llvm/build")) {
  # system "cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=#{opt.prefix}"
  
  # build LLVM & Clang using autoconf/make so we can use '--with-gcc-toolchain' to setup include directories
  system "../configure --enable-optimized --enable-debug-symbols --with-gcc-toolchain=#{gcc_dir} --prefix=#{opt.prefix}"
  system "make -j12"
  system "make install"
  
  # install cmake files so we can use find_package(LLVM)
  cd(*mkdir_p("cmake")) {
    system "cmake -DCMAKE_INSTALL_PREFIX=#{opt.prefix} ../.."
    system "cmake -P cmake/modules/cmake_install.cmake"
  }
}
