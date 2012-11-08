#!/usr/bin/env bash
DIR=$(dirname $0)
DIR="${DIR}";
DETACHED=$($DIR/../aeternum start -p $DIR/test.pid -- sleep 100)
if [ $? != 0 ]; then
  echo 'FAILED TO SPAWN DETACHED';
  exit 1;
fi
# give it time to start
sleep 1
CHILD=$(printf <test.pid)
kill $CHILD
sleep 1
if [ $CHILD == $(printf <test.pid) ]; then
  echo 'FAILED TO RESTART CHILD';
  exit 1;
fi
kill $DETACHED
if [ $? != 0 ]; then
  echo 'FAILED TO KILL DETACHED';
  exit 1;
fi
