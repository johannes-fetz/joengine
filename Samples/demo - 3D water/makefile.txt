JO_COMPILE_WITH_VIDEO_MODULE = 0
JO_COMPILE_WITH_TGA_MODULE = 1
JO_COMPILE_WITH_AUDIO_MODULE = 0
JO_COMPILE_WITH_3D_MODULE = 1
JO_COMPILE_WITH_EFFECTS_MODULE = 0
JO_DEBUG = 0
# (30 images per second on a NTSC console, 25 on a PAL one: the water and the crates in 2 fields)
JO_FRAMERATE = 2
SRCS=main.c
JO_ENGINE_SRC_DIR=../../jo_engine
COMPILER_DIR=../../Compiler
include $(COMPILER_DIR)/COMMON/jo_engine_makefile
