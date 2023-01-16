#The script takes a text file as input. The txt file contains the name and the major of the
#student in each line and the shell script should print the
#Major names and their count separated by a space, sorted by count descending
#the number of students. If the count is the same, sort in ascending alphabetical
#order of the major name.
cat $1 | cut -d " " -f 2 | sort | uniq -c | sort -nr | awk '{print $2, $1}'