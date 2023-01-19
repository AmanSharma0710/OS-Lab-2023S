mkdir -p $2
touch $2/{a..z}
while read line
do
    echo $line >> $2/${line:0:1}
done < <(sort $1/*)