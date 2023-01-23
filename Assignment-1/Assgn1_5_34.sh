#!/bin/bash

# this bash script will take a directory, iterate through all the subdirectories and python files in them and print their path and name
# it will also go through each python file and print the comments and their line numbers

# Extract directory path 
dir_path=$1

# Iterate over all subdirectories in the directory path
surf(){
    path=$1
    # echo "path is: " "$path"
    for file in "$path"/*; do
        # echo "file is: " "$file"
        # if its a python file then print its path and name
        if [[ $file == *.py ]]; then
            echo "$file" 
            lineno=1
            # Iterate over each line in the python file
            while read -r line; do
                # if the line contains a comment then print the line number in the file and the line
                # if # is part of a string in double quotes then ignore it
                if [[ $line == *\"*#*\"* ]]; then
                    continue
                fi
                if [[ $line == *#* ]]; then
                    echo "Line number is $lineno"
                    echo "$line"
                    echo $'\n'
                fi
                
                # if its a multi line comment then print onlt the starting line number in the file and the entire comment
                if [[ $line == \"\"\"* ]]; then
                    echo "Line number is $lineno"
                    echo "$line"
                    # iterate through all the lines starting from this line until the end of the comment
                    # this loop should start from the next line 
                    while read -r line; do
                        echo "$line"
                        if [[ $line == *\"\"\" ]]; then
                            break
                        fi
                        lineno=$((lineno+1))
                    done < "$file"
                    echo $'\n'
                fi
                lineno=$((lineno+1))
            done < "$file"
            echo $'\n' $'\n'
        fi

        # if its a directory other than the current one or the previous one then iterate through all the files in it
        if [[ -d $file ]]; then
            surf "$file"
        fi
    done
}

surf "$dir_path" > output5.txt