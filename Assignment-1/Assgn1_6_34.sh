typeset -a a
typeset i j
> output.txt

a[1]=""
for (( i = 2; i <= 1000000; i++ )); do
  a[$i]=$i
done
for (( i = 2; i * i <= 1000000; i++ )); do
  if [[ ! -z ${a[$i]} ]]; then
    for (( j = i * i; j <= 1000000; j += i )); do
      a[$j]=""
    done
  fi
done

for k in `tr -d '\r' < input.txt`
do   
  d=()
  for (( i = 1; i * i <= k; i++ )); 
  do
    if [ $((k / i * i)) == $k ]; then
      if [[ ! -z ${a[$i]} ]]; then
        d+=($i) 
      fi
      if [[ ! -z ${a[$((k/i))]} ]]; then
        d+=($((k/i)))
      fi
    fi
  done
  d_sorted=($(for dd in "${d[@]}"; do echo $dd; done | sort -n)) 
  echo ${d_sorted[*]} >> output.txt && d=()
done 
