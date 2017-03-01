#!/bin/bash
COUNT=`git rev-list --all | wc -l`
echo -n ${COUNT}
