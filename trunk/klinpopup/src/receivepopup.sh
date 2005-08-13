#!/bin/sh
RECEIVED=`date +"%s_%N"`
POPUP_DIR=/var/lib/klinpopup
POPUP_FILE=$POPUP_DIR/$RECEIVED
TIME=`date --iso-8601=seconds`
MESSAGE=`cat $4`

# create message
#sender
echo -e "$1\n$2\n$3\n$TIME\n$MESSAGE" > /tmp/$RECEIVED
mv /tmp/$RECEIVED $POPUP_DIR/
#sender machine
#echo $2 >> $POPUP_FILE
#sender IP
#echo $3 >> $POPUP_FILE
# please DON'T change the time format here!
#TIME=`date --iso-8601=seconds` >> $POPUP_FILE
#message file
#cat $4 >> $POPUP_FILE

# delete message file from samba
rm -f $4
