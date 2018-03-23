#! /bin/bash

BASEDIR=$(dirname "$0")

# initialize ingredients
pump=1;
cat $BASEDIR/ingredients.txt | while read uuid ; do
	echo $pump - $uuid;
	curl -X PUT -d $uuid http://localhost:9002/ingredients/$pump/;
       	echo;
	curl http://localhost:9002/ingredients/$pump/;
       	echo;
       	pump=$[$pump+1]; 
done
echo

# send recipe
curl -v -S -s -f --no-keepalive -X PUT -d @$BASEDIR/testrecipe.json http://localhost:9002/program/0/
exit $?
