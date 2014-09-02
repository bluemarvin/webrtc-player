#!/bin/sh
for file in $1/lib/*.so ; do
  echo $2/bin/paxctl -m $file
  $2/bin/paxctl -m $file
done
echo $2/bin/paxctl -m $1/$3
$2/bin/paxctl -m $1/$3
