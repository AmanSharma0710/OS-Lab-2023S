s="$(seq 2 1e6|sort)"

for n in $(seq 2 1e6)
do
  s="$(comm -23 <(echo "$s") <(seq $((n * n)) $n 1e6|sort))"
done

for i in `tr -d '\r' < input.txt`
do   
  for j in $(echo "$s" | sort -n)
  do
    if [ $((i / j * j)) == $i ]
    then
    a+="$j "
    fi
  done
  echo ${a[*]} >> output.txt && a=""
done 




