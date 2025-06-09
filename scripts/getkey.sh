#!/bin/sh
# This script assumes your databento key is in an encrypted file
# the location of which is specified by the DB_KEYFILE environment variable.
# It decrypts the key using ccrypt and outputs it to stdout
# and applications call it to get the key. If you don't want anything encrypt,
# just replace the below with "echo your_key_here".
# Otherwise, do "echo your_key_here | ccrypt -e --key databento > /path/to/keyfile"
# to generate a keyfile of yours or figure a better way to store it.
if [ -z "$DB_KEYFILE" ]; then
  echo "Error: DB_KEYFILE environment variable is not set."
  exit 1
fi
cat $DB_KEYFILE | ccrypt -d --key databento
