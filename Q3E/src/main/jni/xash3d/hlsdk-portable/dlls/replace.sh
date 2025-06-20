f="effects.h *.cpp"
for m in SetThink SetTouch SetUse SetBlocked SetMoveDone; do
m2=`echo $m|sed -e s/Set/Reset/`
sed -e s/$m[[:space:]]\*\([[:space:]]\*/$m\(/g -e s/$m\([[:space:]]\*\\\&/$m\(/g -e s/$m\([[:space:]]\*NULL[[:space:]]\)/$m2\(\)/g -e s/$m\([[:space:]]*/$m\(\ \\\&/g -e s/$m2\(\)/$m\(\ NULL\ \)/g -i $f
done
