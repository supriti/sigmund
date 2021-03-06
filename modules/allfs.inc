#!/bin/bash

########################## TEST HELPERS ##################

function space_used
{
    df -k $TEST_DIR | xargs | awk '{ print $(NF-3)}'
}

function write_small_file
{
    file_arg=$1
    dd if=/dev/zero "of=$file_arg" bs=1M count=10 || error "ERROR writing 1OMB to $file_arg"
}

function write_small_file_random
{
    file_arg=$1
    dd if=/dev/urandom "of=$file_arg" bs=1M count=10 || error "ERROR writing 1OMB to $file_arg"
}

function write_med_file_random
{
    file_arg=$1
    dd if=/dev/urandom "of=$file_arg" bs=10M count=10 || error "ERROR writing 1O0MB to $file_arg"
}

function append_file
{
    file_arg=$1
    dd if=/dev/zero "of=$file_arg" bs=1M count=10  conv=notrunc oflag=append || error "ERROR appending 1OMB to $file_arg"
}

function get_test_user
{
    # get the first user which is not in group 0 (root)
    # and who is allowed to login
    grep -v nologin /etc/passwd | grep -v ':0:' | head -1 | cut -d ':' -f 1
}

function create_tree
{
    local ROOT=$1
    local DEPTH=$2
    local WIDTH=$3

    [ -n "$ROOT" ] || error "invalid arg to $0: $1"
    [ -n "$DEPTH" ] || error "invalid arg to $0: $2"
    [ -n "$WIDTH" ] || error "invalid arg to $0: $3"

    if (($DEPTH > 0)); then
        # create a subdir
        ((nb_file=$WIDTH - 1))
        mkdir -p $ROOT/subdir
        create_tree $ROOT/subdir $(($DEPTH - 1)) $WIDTH
    else
        nb_file=$WIDTH
    fi

    for f in $(seq 1 $nb_file); do
        touch $ROOT/file.$f
    done
}

############################ TEST FUNCTIONS ############################

### test1m: check that a user can copy 444 file
SPEED[test1m]=fast
TEST_TYPE[test1m]=dev,admin,prod
NEEDS_ROOT[test1m]=no
function test1m
{
    file="test_file"

    # create a file as user in this dir
    dd if=/dev/zero of=$file bs=1M count=1 || error "error writing $file"

    # set file 444
    chmod 444 "$file" || error "error chmod 444 $file"

    # copy the file as user
    cp $file $file.copy || error "error copying $file to $file.copy"

    ls -l $file.copy || error "target file $file.copy not found"
}

### test1r: check if altgroups are correctly managed by the FS
SPEED[test1g]=fast
TEST_TYPE[test1g]=dev,admin,prod
NEEDS_ROOT[test1g]=yes
PRE_TEST[test1g]=pre_test1g
function test1g
{
    cd $GROUP1/$GROUP2 || error "error when traversing tree with different group owners"
}

function pre_test1g
{
    mkdir -p  -m 700 $GROUP1
    chown  $TEST_USER:$GROUP1 $GROUP1

    cd  $GROUP1
    mkdir -p -m 700 $GROUP2
    chown  $TEST_USER:$GROUP2 $GROUP2
}



### test1w: write a 1GB large file
SPEED[test1w]=slow
TEST_TYPE[test1w]=dev,admin
NEEDS_ROOT[test1w]=no
function test1w
{
    file="test_file"

    # create a file as user in this dir
    dd if=/dev/zero of=$file bs=1M count=1024 || error "error writing $file"
}


### test2r: check recursive removal of large directories
SPEED[test2r]=very_slow
TEST_TYPE[test2r]=dev
NEEDS_ROOT[test2r]=no
function test2r
{
    dir="tree"

    mkdir -p $dir

    echo "creating wide namespace..."
    create_tree $dir 2 10000

    count=$(find $dir | wc -l)
    [ $count = 30003 ] || error "unexpected entry count in $dir"

    rm -r $dir || error "couldn't remove $dir"
}

### test2s: does a very small write io and check size
SPEED[test2s]=fast
TEST_TYPE[test2s]=dev,admin,prod
NEEDS_ROOT[test2s]=no
function test2s
{
    # Write 5 bytes (4 numerical characters and a CR at the end)
    echo "1234" > small_file.$$
    mysize=`stat --format="%s" small_file.$$`

    [ $mysize = 5 ] || error "Size is $mysize, should be 5"
}

### test3b : cthon04's basic tests
SPEED[test3b]=medium
TEST_TYPE[test3b]=dev,admin,prod
NEEDS_ROOT[test3b]=no
function test3b
{
   export NFSTESTDIR=$(pwd)/cthon-b

   cd $CTHON04_DIR/basic

   timeout -s KILL 5m ./runtests || error "ERROR while running cthon04's basic tests"
}

### test3g : cthon04's general tests
SPEED[test3g]=medium
TEST_TYPE[test3g]=dev,admin,prod
NEEDS_ROOT[test3g]=no
function test3g
{
   export NFSTESTDIR=$(pwd)/cthon-g

   cd $CTHON04_DIR/general

   timeout -s KILL 5m ./runtests || error "ERROR while running cthon04's general tests"
}

### test3s : cthon04's special tests
SPEED[test3s]=medium
#We skip, waiting an updated 9p client with mmap
#TEST_TYPE[test3s]=dev,admin,prod
TEST_TYPE[test3s]=dev,admin,prod,skip
NEEDS_ROOT[test3s]=no
function test3s
{
   export NFSTESTDIR=$(pwd)/cthon-s
   cd $CTHON04_DIR/special

   timeout -s KILL 5m ./runtests || error "ERROR while running cthon04's special tests"
}

### test3l : cthon04's lock tests
SPEED[test3l]=medium
#We skip, waiting an updated 9p client with mmap
#TEST_TYPE[test3l]=dev,admin
TEST_TYPE[test3l]=dev,admin,skip
NEEDS_ROOT[test3l]=no
function test3l
{
   export NFSTESTDIR=$(pwd)/cthon-l
   cd $CTHON04_DIR/lock

   timeout -s KILL 5m ./runtests || error "ERROR while running cthon04's lock tests"
}

### test4g : git clone a local repository
SPEED[test4g]=medium
TEST_TYPE[test4g]=dev,admin
NEEDS_ROOT[test4g]=no
function test4g
{
   git clone $GIT_CLONE_URL || error "ERROR while cloning a git repository"
}


### test4s : force a utimes on a symlink via a tar 
SPEED[test4s]=fast
TEST_TYPE[test4s]=dev,admin,prod
NEEDS_ROOT[test4s]=no
function test4s
{
  
   # build the tar
   mkdir -p testln
   touch testln/file
   ln -s file testln/link
   tar czf testln.tar.gz testln
   rm -rf testln

   # Untar the tarball
   tar xf testln.tar.gz  || error "ERROR while forcing a utimes on symbolic link via tar"
}

### test4k : untar a kernel tarball and compiles kernel
SPEED[test4k]=very_slow
TEST_TYPE[test4k]=dev
NEEDS_ROOT[test4k]=no
function test4k
{
  
   kdir=`basename $KERNEL_TAR | sed -e 's/\.tar.*$//g'`
   
   tar xvf $KERNEL_TAR || error "ERROR while decompresssing kernel archive"
   cd $kdir

   ( make defconfig && make -j 17 && make -j 17 modules ) ||  error "ERROR while compiling kernel"
}

### test5r: open and read a single file multiple times in parallel
SPEED[test5r]=fast
TEST_TYPE[test5r]=dev,admin,prod
NEEDS_ROOT[test5r]=no
function test5r
{
   dd if=/dev/urandom of=input bs=1k count=3
   seq 1 1000 | xargs -n 1 -P 25 -I{} cat input > /dev/null
}

### test5m: open and read multiple files in parallel
SPEED[test5m]=medium
TEST_TYPE[test5m]=dev,admin
NEEDS_ROOT[test5m]=no
function test5m
{
   for file in {01..50}; do touch f.${file}; done
   seq 1 1000 | xargs -n 1 -P 25 -I{} cat f.* > /dev/null
}

# syntax: ONLY=2,3 ./run_test.sh [-j] <test_dir>

######################## DEFINE TEST LIST HERE ####################

function run_allfs
{
  export -f create_tree

  run_test test1m  "copy file with 444 mode"
  run_test test1g  "traverse 700 dirs with altgroups"
  run_test test1w  "writes a 1GB file"
  run_test test2r  "rm -rf of wide namespace"
  run_test test2s  "writes 5 bytes and check file size"
  run_test test3b  "cthon04's basic tests"
  run_test test3g  "cthon04's general tests"
  run_test test3s  "cthon04's special tests"
  run_test test3l  "cthon04's lock tests"
  run_test test4g  "git clone a local repository"
  run_test test4s  "Tar calls utimes on symlink"
  run_test test4k  "decompress kernel tar + compilation"
  run_test test5r  "parallel open/read of a single file"
  run_test test5m  "parallel open/read of a many files"
}

