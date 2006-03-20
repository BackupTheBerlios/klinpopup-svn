#!/bin/sh
RECEIVED=`date +"%s_%N"`
POPUP_DIR=/var/lib/klinpopup
POPUP_FILE=$POPUP_DIR/$RECEIVED
# please DON'T change the time format here!
TIME=`date --iso-8601=seconds`
MESSAGE=`cat $4`

# create message
echo -e "$1\n$2\n$3\n$TIME\n$MESSAGE" > $POPUP_DIR/$RECEIVED

# delete message file from samba
rm -f $4
