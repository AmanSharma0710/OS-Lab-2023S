cat $1 | sed -e "/$2/ s/\(.*\)/\L\1/" | sed "/$2/ s/[[:lower:]][^[:alpha:]]*\([[:alpha:]]\|$\)/\u&/g"