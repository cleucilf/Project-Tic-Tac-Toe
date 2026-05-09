#!/bin/bash

while true
do
  move=$(( (RANDOM % 9) + 1 ))
  mosquitto_pub -h localhost -t tictactoe/player2 -m "$move"
  echo "Sent random move: $move"
  sleep 3
done
