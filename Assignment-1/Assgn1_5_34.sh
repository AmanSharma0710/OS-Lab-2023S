dir_path=$1
recur(){
    path=$1
    for file in "$path"/*; do
        if [[ $file == *.py ]]; then
            echo "File: " "$file"
            echo $'\n'"Comments: "
            awk '{
                if ($0 ~ /^([^\"]*\"[^\"]*\")*[^\"]*#[^\"]*$/) {
                    print "Line number is " NR; 
                    print $0;
                }
                else{
                    if ($0 ~ /^[ \t]*\"\"\"/){
                        if ($0 ~ /\"\"\"$/){
                            print "Line number is " NR;
                            print $0;
                        }
                        else{
                            print "Line number is " NR;
                            print $0;
                            while(getline){
                                print $0;
                                if ($0 ~ /\"\"\"/){
                                    break;
                                }
                            }
                        }
                    } 
                }
            }' "$file"
            echo "-------------------------------End of file-----------------------------" $'\n'
        fi

        if [[ -d $file ]]; then
            recur "$file"
        fi
    done        
}

recur "$dir_path"