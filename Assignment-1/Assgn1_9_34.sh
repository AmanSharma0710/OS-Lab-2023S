fun(){
    cat $1 | awk $3 | sort | uniq $2
}
fun $1 -c {'print$2'} | sort -nk1,1r | awk {'print$2" "$1'}
fun $1 -d {'print$1'}
fun $1 -u {'print$1'} | wc -l