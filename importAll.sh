#!/usr/bin/env bash

device="pi@192.168.195.111"      ## TELEMETRY pwd telemetrypi

FOLDERNAME="$(date +"%d-%b-%Y__%H-%M-%S")"
SOURCEPATH="~/logs/"
DESTPATH="/home/filippo/Desktop/CANDUMP_DEFAULT_FOLDER/"

echo $FOLDERNAME

ssh $device "
cd ~/telemetry/python &&
python3 zip_logs.py" &&

scp $device:~/logs/logs.zip $DESTPATH &&
unzip $DESTPATH'logs.zip' -d $DESTPATH'logs' &&
rm $DESTPATH'logs.zip'
