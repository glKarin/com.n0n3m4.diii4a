#/bin/bash

# jpeg2ppm
if [[ $1 == "jpeg2ppm" ]]; then
./djpeg -dct int -ppm -outfile testout.ppm testorig.jpg && cmp testimg.ppm testout.ppm

# jpeg2gif
elif [[ $1 == "jpeg2gif" ]]; then
./djpeg -dct int -gif -outfile testout.gif testorig.jpg && cmp testimg.gif testout.gif

# jpeg2bmp
elif [[ $1 == "jpeg2bmp" ]]; then
./djpeg -dct int -bmp -colors 256 -outfile testout.bmp testorig.jpg && cmp testimg.bmp testout.bmp

#ppm2jpg
elif [[ $1 == "ppm2jpg" ]]; then
./cjpeg -dct int -outfile testout.jpg testimg.ppm && cmp testimg.jpg testout.jpg

#progressive2ppm
elif [[ $1 == "progressive2ppm" ]]; then
./djpeg -dct int -ppm -outfile testoutp.ppm testprog.jpg && cmp testimg.ppm testoutp.ppm

#ppm2progressive
elif [[ $1 == "ppm2progressive" ]]; then
./cjpeg -dct int -progressive -opt -outfile testoutp.jpg testimg.ppm && cmp testimgp.jpg testoutp.jpg

#progressive2baseline
elif [[ $1 == "progressive2baseline" ]]; then
./jpegtran -outfile testoutt.jpg testprog.jpg && cmp testorig.jpg testoutt.jpg
fi
