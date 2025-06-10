#!/bin/sh
SDIR=$(dirname $0)
cd $SDIR
cmake -DCMAKE_INSTALL_PREFIX=../install ../src
make -j 10 && make install

if [ $? -eq 0 ]; then
    echo 'Build finished'
    echo "Binaries should be in ${SDIR}/../install/bin"
    echo "Now create your bentokey.bin file and copy it into \$DB_KEYFILE location ${DB_KEYFILE}"
    echo "Or can use the following command to create it:"
    echo "echo your_key_here | ccrypt -e --key databento > \$DB_KEYFILE"
    echo 
    echo "To fetch some data, try:"
    echo "cd ../install; ./bin/bentohistchains -s spy -n 3 -d 2025-05-02 -l info"
    echo "The client is designed to load symbols in paralles. So in the above command"
    echo "try putting spy,qqq,amd,aapl,googl,msft,nflx,tsla,amzn instead of just spy."
    echo
    echo "Good luck! :)"
else
    echo "Build or install failed!"
    exit 1
fi
