mkdir -p $2
for file in $1/*;do
    for line in $(cat <$file);do
        echo $line| cat >> $2/${line:0:1}
    done
done