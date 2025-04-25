#!/bin/bash

set -e

TFL_DIR="tflite-micro"
TFL_REPO="https://github.com/tensorflow/tflite-micro"
TFL_REV="b489b9973be22aefbe12d137032bc3a08abc0065"

OPENOCD_DIR="tools/openocd"
OPENOCD_REPO="https://git.code.sf.net/p/openocd/code"
OPENOCD_REV="v0.12.0"

prepare_tfl()
{
	if ! [[ -d $TFL_DIR ]] ; then
		echo "Get TFL"
		git clone $TFL_REPO $TFL_DIR
	fi

	( cd $TFL_DIR && git fetch && git checkout $TFL_REV )

	echo "Build TFL"
	( cd $TFL_DIR && make -f tensorflow/lite/micro/tools/make/Makefile TARGET="riscv32_mcu" microlite -j $(nproc) )

	echo "TFL done."
}

prepare_openocd()
{
	if ! [[ -d $OPENOCD_DIR ]] ; then
		echo "Get OpenOCD"
		git clone $OPENOCD_REPO $OPENOCD_DIR
	fi

	( cd $OPENOCD_DIR && git fetch && git checkout $OPENOCD_REV )

	echo "Build OpenOCD"
	( cd $OPENOCD_DIR && \
		./bootstrap && \
		./configure && \
		make -j $(nproc) \
	)

	echo "OpenOCD done."
}

prepare_tfl
prepare_openocd
