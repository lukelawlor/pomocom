#!/bin/sh
# Generic script for sending a message to anything and playing a sound
# usage: ./msg.sh <sound name> <message>
# example: ./msg.sh square hello

# command to use to send the message
message(){
	echo "$1" | ~/.config/pomocom/msg_dzen.sh &
}

# command to use to play a sound
sound(){
	aplay "$1" -q &
}

# play the sound
case $1 in
	square) sound ~/misc/snd/square.wav ;;
	snare) sound ~/misc/snd/snare.wav ;;
	* ) echo "unknown sound" ;;
esac

# send the message
message "$2" &>/dev/null
