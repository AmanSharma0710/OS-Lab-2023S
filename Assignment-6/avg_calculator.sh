for (( i=0; i<10; i++))
do
    ./mergesort
done
# awk '{s+=$1}END{printf("Average time taken:%f\n",s/10.0)}' time.txt
# awk '{s+=$1}END{printf("Average memory used:%f\n",s/10.0)}' mem.txt
# awk '{s+=$1}END{printf("Average number of blocks used:%f\n",s/10.0)}' nodes.txt
# uncomment below for without freeList
awk '{s+=$1}END{printf("Average time taken:%f\n",s/20.0)}' time_without.txt
awk '{s+=$1}END{printf("Average memory used:%f\n",s/20.0)}' mem_without.txt
awk '{s+=$1}END{printf("Average number of blocks used:%f\n",s/10.0)}' nodes.txt