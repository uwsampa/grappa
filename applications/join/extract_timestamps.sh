input=$1
#schema
#process-id event-type timestamp
echo "stream type time" >$input.trace
grep timestamp $input | awk '{gsub(/ +/, " ");print}' | cut -d ' ' -f 7,8,9 >>$input.trace
