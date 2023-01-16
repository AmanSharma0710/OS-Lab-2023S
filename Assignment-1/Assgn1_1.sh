answer=1
rev < $1 | while read line; do
    if [ -z $line ]; then
        echo $answer; exit
    fi
    x=$answer
    y=$line
    while [ $y -gt 0 ];do
        t=$x
        x=$y
        y=$((t%y))
    done
    answer=$((answer*(line/x)))
done