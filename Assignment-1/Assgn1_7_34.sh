mkdir -p $2
touch $2/{a..z}
for line in $(sort $1/*)
do
    echo $line >> $2/${line:0:1}
done