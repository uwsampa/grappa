input=$1
skips=$2
if [ -z "$2" ]
then
    skips=1
fi
tailoffset=`expr $skips + 1`
grep pipeline $input | grep -v group | tail -n+$tailoffset | awk '{gsub(/ +/, " ");print}' | cut -d ':' -f6 | cut -d ' ' -f 2
