mkdir -p $2
while read -n1 letter;
do
    touch $2/$letter
done < <(echo -n "abcdefghijklmnopqrstuvwxyz")
echo ../7input/* | xargs awk '{print $1}' | sort