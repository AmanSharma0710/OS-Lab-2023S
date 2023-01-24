dir_path=$1
dest_folder=$2
attribute_names=("${@:3}")

for file in "$dir_path"/*.jsonl; do
    csv_file="$dest_folder/$(basename "$file" .jsonl).csv"
    attributes=$(IFS=,; echo "${attribute_names[*]}")
    echo "${attributes}" > "$csv_file"
    while read -r line; do
        values=""
        for attribute in "${attribute_names[@]}"; do
        value=$(echo "$line" | jq -j ".$attribute")
        if [[ $value == *,* ]] || [[ $value == *\"* ]] || [[ $value == *$'\n'* ]]; then
            value=${value//\"/\"\"}
            value="\"$value\""
        fi
        values="$values$value,"
        done
        values=${values::-1}
         echo "$values" >> "$csv_file"
    done < "$file"
done
