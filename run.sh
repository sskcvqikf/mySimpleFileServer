# /bin/sh

START_DIR=$(pwd)
INCLUDE_DIR=$START_DIR/include
LIB_DIR=$START_DIR/lib
CACHE_DIR=$START_DIR/.cache

BOOST_CACHE_DIR=$CACHE_DIR/boost
BOOST_DIR=$LIB_DIR/boost
BOOST_FINAL_FILES=$CACHE_DIR/boost-installation

mkdir -p $INCLUDE_DIR
mkdir -p $LIB_DIR
mkdir -p $CACHE_DIR
mkdir -p $BOOST_FINAL_FILES

git clone --recursive https://github.com/boostorg/boost.git $BOOST_CACHE_DIR

mkdir -p $BOOST_DIR

cd $BOOST_CACHE_DIR
./bootstrap.sh --prefix=$BOOST_FINAL_FILES
./b2 headers
./b2 install

cp -r $BOOST_FINAL_FILES/include/boost $INCLUDE_DIR/
cp -r $BOOST_FINAL_FILES/lib/boost $LIB_DIR/

cd $START_DIR

rm -rf $CACHE_DIR

make

./main
