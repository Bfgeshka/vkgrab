# /bin/bash

TOKEN=$1

while read x;
	do vkgrab -t $TOKEN $x;
done < ~/docs/vk__todl_names.txt
