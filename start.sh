#!/bin/bash

LOG=$(pwd)/log_$(date +%F.%H-%M-%S).txt
echo "Log will be written to $LOG."

./server -server MyServer -world world -password password -log $LOG
