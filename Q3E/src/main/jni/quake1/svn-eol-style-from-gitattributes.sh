for F in *; do
	exec 3<.gitattributes
	while read <&3 -r L S; do
		if [ -z "${F##$L}" ]; then
			s=$S
		fi
	done
	case "$s" in
		'-diff -crlf')
			svn propdel svn:eol-style "$F"
			;;
		'-crlf')
			svn propdel svn:eol-style "$F"
			;;
		'crlf=input')
			svn propset svn:eol-style native "$F"
			;;
		*)
			echo "UNKNOWN: $s"
			;;
	esac
done
