Import("env")

env.Replace(
    UPLOADER="./tools/openocd/src/openocd -f ./tools/gd32vf103_jlink.cfg",
    UPLOADCMD="$UPLOADER -c 'flash write_image erase unlock $BUILD_DIR/${PROGNAME}.elf; shutdown'"
)
