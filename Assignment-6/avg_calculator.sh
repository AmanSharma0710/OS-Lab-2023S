for (( i=0; i<100; i++))
do
    ./mergesort
done
awk '{s+=$1}END{printf("Average time taken:%f\n",s/100.0)}' time.txt
awk '{s+=$1}END{printf("Average memory used:%f\n",s/100.0)}' mem.txt
# uncomment below for without freeList
# awk '{s+=$1}END{printf("Average time taken:%f\n",s/100.0)}' time_without.txt
# awk '{s+=$1}END{printf("Average memory used:%f\n",s/100.0)}' mem_without.txt