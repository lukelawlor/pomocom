#!/bin/sh
# Generic script for sending a message to anything and playing a sound
# usage: ./msg.sh <sound name> <message>
# example: ./msg.sh square hello

config_dir=~/.config/pomocom

# command to use to send the message
message(){
	echo "$1" | $config_dir/msg_dzen.sh &
}

# command to use to play a sound
sound(){
	aplay "$1" -q &
}

# play the sound
case $1 in
	square) sound $config_dir/square.wav ;;
	snare) sound $config_dir/snare.wav ;;
	* ) echo "unknown sound" ;;
esac

# send the message
message "$2" &>/dev/null
