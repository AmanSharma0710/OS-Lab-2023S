answer=1
for line in $(rev $1)
do
    x=$answer
    y=$line
    while [ $y -ne 0 ] 
    do
        t=$x
        x=$y
        y=$((t%y))
    done
    answer=$((answer*(line/x)))
    echo $answer
done | tail -1