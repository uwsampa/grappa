awk '{ gsub(/[ \t]+/, " ");print }' | cut -d ' ' -f $1
