#! /bin/mksh

black_list="../list.txt"
cur_date=$(date +%s)

#about a year
# too_old_delta=31500000
too_old_delta=21500000

[ ! -d c_$1 ] && ./blacklist_finder $1 2>&1
cd c_$1

ls -1 | while read line
do
	grepped="$(grep --file=$black_list $line)"
	g_ltime=$(grep "^:ls" $line | sed -e 's/:ls://')
	let delta_time=$cur_date-$g_ltime
	#ltime=$(echo "$grepped" | grep "^:ls" | sed -e 's/:ls://')
	if [ -n "$grepped" ] || [ $delta_time -gt $too_old_delta ]
	then
		head -n1 $line
		date --date=@$g_ltime

		if [ $delta_time -gt $too_old_delta ]
		then
			echo "USER LAST_TIME IS TOO BIG, DEAD"
		fi

		echo ":::"
		echo "$grepped"
		echo -e "~~~\n"
	fi
done

exit 0
