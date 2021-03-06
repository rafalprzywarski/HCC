#!/bin/bash
set -e

if [ "${RPI_ADDRESS}" == "" ]; then
  echo "error: RPI_ADDRESS not defined"
  exit 1
fi

DEST_DIR=/home/pi/remote_builds/HCC

echo "synchronising..."
ssh ${RPI_ADDRESS} "mkdir -p ${DEST_DIR}"
rsync -av --delete --exclude xcode --exclude Debug --exclude Release --exclude "*.pgm" * ${RPI_ADDRESS}:${DEST_DIR}/
