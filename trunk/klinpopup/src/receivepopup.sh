#!/bin/sh
RECEIVED=`date +"%s_%N"`
POPUP_DIR=/var/lib/klinpopup
POPUP_FILE=$POPUP_DIR/$RECEIVED

# create message
#sender
echo $1 >> $POPUP_FILE
#sender machine
echo $2 >> $POPUP_FILE
#sender IP
echo $3 >> $POPUP_FILE
# please DON'T change the time format here!
echo `date --iso-8601=seconds` >> $POPUP_FILE
#message file
cat $4 >> $POPUP_FILE

# delete message file from samba
rm -f $4
