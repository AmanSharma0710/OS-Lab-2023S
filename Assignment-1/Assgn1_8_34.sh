declare -a flags                                         # array to store flags in order
count=0
while getopts c:n:s:h flag                               # getopts to get flags, : indicates that the flag takes an argument
do 
    case "${flag}" in
        c) category=${OPTARG}
           flags[$count]=${flag}
           let count++
           ;;
        n) name=${OPTARG}
              flags[$count]=${flag}
              let count++
           ;;
        s) sortcol=${OPTARG}
                flags[$count]=${flag}
                let count++
            ;;
        h) flags[$count]=${flag}
              let count++
           ;;
    esac
done
shift "$((OPTIND-1))"                                   # shift to get the arguments

if [ "$#" -ne 4 ]; then
    echo "Illegal number of parameters."
    exit 1
fi

if [ ! -f main.csv ]; then                             # if file doesn't exist, create it
    touch main.csv
fi

if [ ! -s main.csv ]; then                             # if file is empty, add header
    echo "Date,Category,Amount,Name" > main.csv
fi

line="$1,$2,$3,$4"
echo $line >> main.csv
echo "Inserted $1,$2,$3,$4 into main.csv"
awk -F'[-/]' '{printf "%02d-%02d-%02d\t%s\n", $3,$2,$1, $0}' main.csv | sort | cut -f2- > temp.csv   # sort by date by default, awk needed cuz date in dd/mm/yy format
mv temp.csv main.csv

for i in "${flags[@]}"
do 
    case "${i}" in
        c)
           answer=0
           echo "$header"
           while IFS="," read -r col1 col2 col3 col4
           do
                if [ "$category" = "$col2" ]; then
                    
                    let answer=$answer+$col3
                fi
           done < <(tail -n +2 main.csv)                            # tail -n +2 to skip header
           echo $answer
           ;;
        n) echo "Name: $name"
           answer=0;
           while IFS="," read -r col1 col2 col3 col4
           do
                if [ "$name" = "$col4" ]; then
                    let answer=$answer+$col3
                fi
           done < <(tail -n +2 main.csv)                            # tail -n +2 to skip header
           echo $answer
           ;;
        s) echo "Sort: $sortcol"
            case $sortcol in 
                date)  ;;                                           # already sorted by date
                category) head -n1 main.csv && tail -n+2 main.csv | sort -t"," -k2 > temp.csv # ensure that header itself is not sorted
                          mv temp.csv > main.csv
                        ;;
                amount) head -n1 main.csv && tail -n+2 main.csv | sort -t"," -k3 > temp.csv
                        mv temp.csv main.csv 
                        ;;
                name) head -n1 main.csv && tail -n+2 main.csv | sort -t"," -k4 > temp.csv
                        mv temp.csv > main.csv
                        ;;
            esac
            ;;
        h) echo "Name: Expenses_Info"
           echo "Usage: ./q8.sh [-c expenditure_item_category] [-n expenditure_for_name] [-s sort_column_name] [-h help] Args 1-4"
                
           echo "Description: Adds argument as entry in main.csv and sorts the file chronologically."
           echo "-c : Gives expenditure by item category"
           echo "-n : Gives expenditure by name"
           echo "-s : Sorts columnwise"
           echo "-h : Display help"
           ;;
    esac
done