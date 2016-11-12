# /bin/bash

VID_FILE=$1

cat $VID_FILE | ag -o "https://www.youtube.*" >> ytdl_vk.txt
cat $VID_FILE | ag -o "https://vk.com.*" > players.txt

while read x;
	do curl $x | ag -o "https://cs[0-9a-z-]*.[vkcdn-]*.[ment]*/[0-9a-z\/]*/[0-9a-z]*\.[0-9]*.[0-9a-z]*\?extra=[0-9a-zA-Z-_]*" | head -n1 >> links.txt;
done < players.txt
