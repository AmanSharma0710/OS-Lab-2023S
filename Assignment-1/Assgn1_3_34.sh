#!/bin/bash

# Extract directory path, destination folder, and attribute names from command-line arguments
dir_path=$1
dest_folder=$2
attribute_names=("${@:3}")

# Iterate over all JSONL files in the directory path
for file in "$dir_path"/*.jsonl; do
    # Create a new CSV file with the same name in the destination folder
    csv_file="$dest_folder/$(basename "$file" .jsonl).csv"

    #create a new string which contains attributes separated by commas
    attributes=$(IFS=,; echo "${attribute_names[*]}")

    # Write the attribute names as the first row in the CSV file
    echo "${attributes}" > "$csv_file"

    # Iterate over each line in the JSONL file
    while read -r line; do
        # Initialize an empty string to store the values for the current JSON object
        values=""

        # Iterate over the attribute names
        for attribute in "${attribute_names[@]}"; do
        # Extract the value for the current attribute from the JSON object
        value=$(echo "$line" | jq -j --argjson value 128 ".$attribute")
        # Append the value to the string, followed by a comma
        # If the value contains a comma, surround it with double quotes
        if [[ $value == *,* ]] || [[ $value == *\"* ]] || [[ $value == *$'\n'* ]]; then
            # replace double quotes with pair of double quotes
            value=${value//\"/\"\"}
            value="\"$value\""
        fi
        values="$values$value,"
        #   echo $line
        done
        # remove last comma
        values=${values::-1}
        # echo $values
        # Append the values string to the CSV file
        echo "$values" >> "$csv_file"
    done < "$file"
done
