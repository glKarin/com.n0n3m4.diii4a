#!/bin/sh

outfile=$1; shift
hosts=$1; shift

on()
{
	case "$1" in
		localhost)
			shift
			exec "$@"
			;;
		*)
			exec ssh "$@"
			;;
	esac
}

pids=
mainpid=$$
trap 'kill $pids' EXIT
trap 'exit 1' INT USR1

n=0
for h in $hosts; do
	nn=`on "$h" cat /proc/cpuinfo | grep -c '^processor[ 	:]'`
	n=$(($nn + $n))
done

rm -f bruteforce-*
i=0
for h in $hosts; do
	nn=`on "$h" cat /proc/cpuinfo | grep -c '^processor[ 	:]'`
	ii=$(($nn + $i))
	while [ $i -lt $ii ]; do
		i=$(($i+1))
		(
			on "$h" ./crypto-keygen-standalone -n $n -o /dev/stdout "$@" > bruteforce-$i &
			pid=$!
			trap 'kill $pid' TERM
			wait
			if [ -s "bruteforce-$i" ]; then
				trap - TERM
				mv "bruteforce-$i" "$outfile"
				kill -USR1 $mainpid
			else
				rm -f "bruteforce-$i"
			fi
		) &
		pids="$pids $!"
	done
done
wait
