# Pessimistically assume shared cache
/cache size/ {kb=$4; n++}
END {print (n==0||kb<1024*n ? 1 : int(kb/(1024*n)))}
