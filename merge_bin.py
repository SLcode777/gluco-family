Import("env")
import os

# boot_app0.bin initialises the `otadata` partition so the bootloader knows to
# boot ota_0. The partition scheme here has no `factory` partition, so without
# it the bootloader has nothing to start -> black screen. A normal
# `pio run -t upload` flashes it automatically; a merged image must include it.
FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
BOOT_APP0 = os.path.join(FRAMEWORK_DIR, "tools", "partitions", "boot_app0.bin")

# Build a single flashable image (bootloader + partitions + otadata + app) with
# `esptool merge_bin` rather than a raw byte-copy: tools that flash the image
# "as-is" (ESP Web Tools / the web flasher button) do NOT patch the bootloader
# header at flash time the way a normal `pio run -t upload` does.
#
# We pass `keep` for the flash params so the *native* headers are preserved
# untouched. On ESP32-S3 the bootloader is built in DIO mode (even though the
# app runs in QIO) — forcing `--flash_mode qio` here rewrites the bootloader
# header to QIO and the board no longer boots (black screen). `keep` avoids that.
env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.bin",
    env.VerboseAction(" ".join([
        '"$PYTHONEXE" "$OBJCOPY"',
        "--chip", "esp32s3", "merge_bin",
        "-o", "$BUILD_DIR/${PROGNAME}_merged.bin",
        "--flash_mode", "keep",
        "--flash_freq", "keep",
        "--flash_size", "keep",
        "0x0", "$BUILD_DIR/bootloader.bin",
        "0x8000", "$BUILD_DIR/partitions.bin",
        "0xe000", BOOT_APP0,
        "0x10000", "$BUILD_DIR/${PROGNAME}.bin",
    ]), "Building merged firmware for ESP Web Tools"),
)
