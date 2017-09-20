cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)

ifeq ($(AUDIO_INPUT_CAPLESSMODE),1)
CFLAGS_analog.o += -D_MIC_CAPMODE_
endif

ifeq ($(AUDIO_INPUT_LARGEGAIN),1)
CFLAGS_analog.o += -D_MIC_LARGEGAIN_
endif

ifeq ($(AUDIO_OUTPUT_DIFF),1)
CFLAGS_analog.o += -DAUDIO_OUTPUT_DIFF
endif
