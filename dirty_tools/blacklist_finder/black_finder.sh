#! /bin/mksh

black_list="../list.txt"

[ ! -d c_$1 ] && ./blacklist_finder $1 2>&1
cd c_$1

ls -1 | while read line
do
	grepped="$(grep --file=$black_list $line)"
	if [ -n "$grepped" ]
	then
		head -n1 $line
		echo "$grepped"
		echo -e "~~~\n"
	fi
done

exit 0
