#!/bin/bash
# This script will output text from stdin with dzen2

# screen dimensions
screen_width=1280
screen_height=1024

# message styling
x=$[screen_width / 2 - width / 2]
y=$[screen_height / 2 - height / 2]
bg=blue
fg=white
#bg=black
#fg=#aba7ff
seconds=60
height=30
width=300

# output the message
killall dzen2
dzen2 -bg $bg -fg $fg -p $seconds -x $x -y $y -w $width -h $height
