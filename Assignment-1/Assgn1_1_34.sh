answer=1
for line in $(rev <$1)
do
    x=$answer
    y=$line
    while true
    do((y!=0))||break
        t=$x
        x=$y
        y=$((t%y))
    done
    answer=$((answer*(line/x)))
    echo $answer
done | tail -1