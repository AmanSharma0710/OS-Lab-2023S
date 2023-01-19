w=main.csv
t=t.csv
v=Date,Category,Amount,Name


while getopts c:n:s:h z                               
do 
    case "${z}" in
        c) c=${OPTARG}
           
           f[$y]=$z 
           let y++
           ;;
        n) n=${OPTARG}
            f[$y]=$z 
           let y++
           ;;
        s) s=${OPTARG}
            f[$y]=$z 
           let y++
           ;;
        h)  f[$y]=$z 
           let y++
           ;;
    esac
done
shift "$((OPTIND-1))"                                   

if [ "$#" -ne 4 ]; then
    exit 1
fi

if [ ! -f $w ]; then                             
    touch $w
fi

if [ ! -s $w ]; then                             
    echo $v > $w
fi

echo "$1,$2,$3,$4" >> $w
echo "Inserted $1,$2,$3,$4 into $w"
awk -F'[-/]' '{printf "%02d-%02d-%02d\t%s\n", $3,$2,$1, $0}' $w | sort | cut -f2- > $t   
mv $t $w

for i in "${f[@]}"
do 
    case "${i}" in
        c)
           echo "$header"
           while IFS="," read -r j k l m
           do
                if [ "$c" = "$k" ]; then
                    
                    let p=$p+$l
                fi
           done < <(tail -n +2 $w)                           
           echo $p
           ;;
        n) 
           while IFS="," read -r j k l m 
           do
                if [ "$n" = "$m" ]; then
                    let q=$q+$l
                fi
           done < <(tail -n +2 $w)                            
           echo $q
           ;;
        s) 
            case $s in 
                category) echo $v > $t
                        tail -n +2 $w | sort -t"," -k2 >> $t 
                        mv $t $w
                        ;;
                amount) echo $v > $t
                        tail -n +2 $w  | sort -t"," -k3 >> $t
                        mv $t $w 
                        ;;
                name)   echo $v > $t
                        tail -n +2 $w  | sort -t"," -k4 >> $t
                        mv $t $w
                        ;;
            esac
            ;;
        h) echo "Name: Expenses"
           echo "Usage: ./q8.sh [-c expenditure_item_category] [-n expenditure_for_name] [-s sort_column_name] [-h help] Args 1-4"
                
           echo "Description: Adds argument as entry in main.csv and sorts the file chronologically."
           echo "-c : Gives expenditure by item category"
           echo "-n : Gives expenditure by name"
           echo "-s : Sorts columnwise"
           echo "-h : Display help"
           ;;
    esac
done
