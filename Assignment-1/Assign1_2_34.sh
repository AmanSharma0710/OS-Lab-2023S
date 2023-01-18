for i in `cat fruits.txt`
do
    sed -i "s/${i}/\&/gI" $1 
done

awk '{print (/^[a-zA-Z]+[0-9]+[a-zA-Z0-9]*$/ && length($0)>4 && length($0)<21)?"YES":"NO"}' $1 > validation_results.txt

