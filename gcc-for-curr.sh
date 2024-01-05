#
#   File name   : setenv.sh
#   Author      : Chenghua Xu
#   Time        : 2017-10-16
#   Description :
#
#   Usage       :
#
#   Bug report  : xuchenghua@loongson.cn
#
#   Log :
#
#
###################################################################
#!/bin/sh
export CROSS_COMPILE=loongarch32r-linux-gnusf-
export ARCH=loongarch32r
export PATH=/home/yangzhongkai/loongarch32r-linux-gnusf-2022-05-20/bin/:$PATH
export LD_LIBRARY_PATH=/home/yangzhongkai/loongarch32r-linux-gnusf-2022-05-20/lib/:$PATH

