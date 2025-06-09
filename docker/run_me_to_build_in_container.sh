#!/bin/sh
SDIR=dirname $0
cd $SDIR
cmake -DCMAKE_INSTALL_PREFIX=../install ../src
make -j 10 && make install

echo 'Build finished'
echo "Binaries should be in $SDIR/../install/bin"
echo "Now create your bentokey.bin file and copy it into $DB_KEYFILE location"
echo "Or can use the following command to create it:"
echo "echo your_key_here | ccrypt -e --key databento > $DB_KEYFILE"
echo "Good luck! :)
