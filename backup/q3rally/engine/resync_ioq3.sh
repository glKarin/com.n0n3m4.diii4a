#!/bin/sh

OLD_COMMIT=`grep "IOQ3_REVISION =" "Makefile" | cut -f3 -d' '`
NEW_COMMIT="ioquake3/main"

DRY_RUN=0

if [ $DRY_RUN -eq 0 ]; then
	if [ `git diff | wc -l` -ne 0 -o `git diff --staged | wc -l` -ne 0 ]; then
		echo "Cannot merge ioq3 changes while there are local changes."
		echo "Maybe 'git stash' and then afterward run 'git stash pop'?"
		echo "See 'git diff' and/or 'git diff --staged' for changes."
		exit 0
	fi
fi

git remote add ioquake3 https://github.com/ioquake/ioq3.git
git fetch ioquake3

# Resolve commit from name to abbreviated hash
OLD_COMMIT=`git log -1 --pretty=format:%h $OLD_COMMIT`
NEW_COMMIT=`git log -1 --pretty=format:%h $NEW_COMMIT`

echo "ioquake3 resync to commit $NEW_COMMIT from $OLD_COMMIT"
echo

PATCH=resync_${OLD_COMMIT}_to_${NEW_COMMIT}.patch
LOG=resync_${OLD_COMMIT}_to_${NEW_COMMIT}.log

if [ ! -f "$PATCH" ]; then
	git diff $OLD_COMMIT..$NEW_COMMIT |
	    sed "s|^+++ b/autoupdater-readme.txt$|+++ b/docs/autoupdater-readme.txt|g;
	         s|^+++ b/ChangeLog$|+++ b/docs/ChangeLog|g;
	         s|^+++ b/COPYING.txt$|+++ b/docs/COPYING.txt|g;
	         s|^+++ b/id-readme.txt$|+++ b/docs/id-readme.txt|g;
	         s|^+++ b/md4-readme.txt$|+++ b/docs/md4-readme.txt|g;
	         s|^+++ b/opengl2-readme.md$|+++ b/docs/opengl2-readme.md|g;
	         s|^+++ b/README.md$|+++ b/docs/README.md|g;
	         s|^+++ b/TODO$|+++ b/docs/TODO|g;
	         s|^+++ b/voip-readme.txt$|+++ b/docs/voip-readme.txt|g;" > "$PATCH"
fi

if [ ! -f "$LOG" ]; then
	echo "ioquake3 resync to commit $NEW_COMMIT from $OLD_COMMIT" > "$LOG"
	echo "" >> "$LOG"
	git log --pretty=format:%s $OLD_COMMIT..$NEW_COMMIT | tac >> "$LOG"
fi


# Added/changed binary files
BINARY_FILES=`grep -a -e "^Binary files /dev/null and b/.* differ$" -e "^Binary files a/.* and b/.* differ$" "$PATCH"`
if [ "x$BINARY_FILES" != "x" ]; then
	BINARY_FILES=`echo "$BINARY_FILES" | sed 's|^Binary files .* and b/\(.*\) differ$|\1|g' | sort`

	echo "Added/changed binary files:"
	echo "$BINARY_FILES"
	echo
	echo
fi

# Removed binary files
REMOVED_BINARY_FILES=`grep -a -e "^Binary files a/.* and /dev/null differ$" "$PATCH"`
if [ "x$REMOVED_BINARY_FILES" != "x" ]; then
	REMOVED_BINARY_FILES=`echo "$REMOVED_BINARY_FILES" | sed 's|^Binary files a/\(.*\) and /dev/null differ$|\1|g' | sort`

	echo "Removed binary files:"
	echo "$REMOVED_BINARY_FILES"
	echo
	echo
fi

#
# Match the following but only print the filename (e.g. example/file.c).
#
# --- /dev/null
# +++ b/example/file.c
#
# TODO?: Parse the file in a loop here instead of this sed nightmare?
#
ADDED_FILES=`cat "$PATCH" | sed -n '{N; /^--- \/dev\/null\n+++ b\/.*$/{p}; D}' | sed 's|^+++ b/\(.*\)$|\1|g' | grep -v "/dev/null" | sort`
if [ "x$ADDED_FILES" != "x" ]; then
	echo "Added text files:"
	echo "$ADDED_FILES"
	echo
	echo
fi

#
# Match the following but only print the filename (e.g. example/file.c).
#
# --- a/example/file.c
# +++ /dev/null
#
REMOVED_FILES=`cat "$PATCH" | sed -n '{N; /^--- a\/.*\n+++ \/dev\/null$/{p}; D}' | sed 's|^--- a/\(.*\)$|\1|g' | grep -v "/dev/null" | sort`
if [ "x$REMOVED_FILES" != "x" ]; then
	echo "Removed text files:"
	echo "$REMOVED_FILES"
	echo
	echo
fi

# This is changed and renamed files.
CHANGED_FILES=`cat "$PATCH" | sed -n '{N; /^--- a\/.*\n+++ b\/.*$/{p}; D}' | sed 's|^+++ b/\(.*\)$|\1|g' | grep -v "^--- a/" | sort`
if [ "x$CHANGED_FILES" != "x" ]; then
	echo "Changed text files:"
	echo "$CHANGED_FILES"
	echo
	echo
fi

if [ $DRY_RUN -eq 0 ]; then
	# --force disables asking about missing files.
	patch --force --merge --no-backup-if-mismatch -p1 -i "$PATCH"

	# Use xargs -n1 to run each command separate so git doesn't skip all if one file is missing.

	if [ "x$BINARY_FILES" != "x" ]; then
		echo $BINARY_FILES | xargs -n1 git checkout $NEW_COMMIT --
		echo $BINARY_FILES | xargs -n1 git add --
	fi

	if [ "x$REMOVED_BINARY_FILES" != "x" ]; then
		echo $REMOVED_BINARY_FILES | xargs -n1 git rm --
	fi

	if [ "x$ADDED_FILES" != "x" ]; then
		echo $ADDED_FILES | xargs -n1 git add --
	fi

	# Renamed files need to be added.
	if [ "x$CHANGED_FILES" != "x" ]; then
		echo $CHANGED_FILES | xargs -n1 git add --
	fi

	if [ "x$REMOVED_FILES" != "x" ]; then
		echo $REMOVED_FILES | xargs -n1 git rm --
	fi

	# Update ioq3 revision in Makefile.
	sed "s/IOQ3_REVISION = $OLD_COMMIT/IOQ3_REVISION = $NEW_COMMIT/" Makefile > Makefile.temp
	mv Makefile.temp Makefile

	git commit -a --file="$LOG"

	# Report files that failed to merge.
	UNMERGED_FILES=`git grep -l "<<<<<<<" | grep -v "resync_ioq3.sh"`
	if [ "x$UNMERGED_FILES" != "x" ]; then
		echo
		echo
		echo "Some files failed to merge automatically and contain:"
		echo
		echo "<<<<<<<"
		echo "  q3rally text"
		echo "======="
		echo "  ioquake3 text"
		echo ">>>>>>>"
		echo
		echo "Edit and manually merge the following files and then run 'git commit -a --amend'"
		echo
		echo "$UNMERGED_FILES"
		echo
	else
		echo
		echo
		echo "All files merged automatically."
	fi
else
	echo
	echo "dry-run: no changes made"
fi

