#!/usr/bin/env ruby
##########################################################
# Script that post-processes LLVM IR to help us debug it
# 
# Cleanup procedures include putting metadata comments 
# inline and converting 'addrspace(100)' back to 
# 'global'
#
# Usage example:
# > grappaclang basic.cpp -emit-llvm -S -o basic.ll
# > ir_debug basic.ll
##########################################################

ir = ARGF.read

# scan for !\d+ metadata ""
metamap = {}
ir.scan(/!(\d+) = metadata !{metadata !"(.*)"}/){ metamap[$~[1]] = $~[2] }
# /\!(\d+) = metadata \!\{metadata !"hello world"}/

# require 'awesome_print'; ap metamap

# replace metadata refs !\d+ with string (in comment)
# replace addrspace(100) -> grappa_global
ir.each_line do |line|
  line.gsub!(/!(\d+)(?! = metadata)\s*$/){ ";@'#{metamap[$~[1]]}'" }
  line.gsub!(/addrspace\(100\)/,'global')
  line.gsub!(/addrspace\(200\)/,'symmetric')
  
  puts line
end
