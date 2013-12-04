cmd=$1

declare -a arr

while read line 
do
    arr+=($line)
done

for h in "${arr[@]}"
do
    ssh $h $cmd
done
