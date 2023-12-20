dir student-distrib
!./mp3_debug &
target remote localhost:1234
file student-distrib/bootimg
layout src
b entry
