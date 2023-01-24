answer=1
for line in $(rev $1)
do
    x=$answer
    y=$line
    while [ $y -ne 0 ] 
    do
        t=$x
        x=$y
        let y=$t%$y
    done
    let answer=$line/$x*$answer
    echo $answer
done | tail -1