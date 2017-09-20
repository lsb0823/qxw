export DEBUG                                            ?= 1
# enable:1
# disable:0

WATCHER_DOG                                            ?= 1
# enable:1
# disable:0

export MBED                                             ?= 1
# enable:1
# disable:0

export RTOS                                             ?= 1
# enable:1
# disable:0

export PC_CMD_UART							?= 0
# enable:1
# disable:0

export AUDIO_CODEC_ASYNC_CLOSE             ?= 1
# enable:1
# disable:0

export HW_FIR_EQ_PROCESS                         ?= 0
# enable:1
# disable:0

export SW_IIR_EQ_PROCESS                         ?= 1
# enable:1
# disable:0

export AUDIO_RESAMPLE                         ?= 0
# enable:1
# disable:0

export SPEECH_ECHO_CANCEL                      ?= 1
# enable:1
# disable:0

export SPEECH_NOISE_SUPPRESS                  ?= 1
# enable:1
# disable:0

export SPEAKER_NOISE_SUPPRESS 				  ?= 0
# enable:1
# disable:0

export SPEECH_PACKET_LOSS_CONCEALMENT                  ?= 1
# enable:1
# disable:0

export SPEECH_WEIGHTING_FILTER_SUPPRESS 				  ?= 1
# enable:1
# disable:0

export SPEAKER_WEIGHTING_FILTER_SUPPRESS 				  ?= 0
# enable:1
# disable:0

export VOICE_DETECT                                 ?= 0
# enable:1
# disable:0

export VOICE_PROMPT                                 ?= 1
# enable:1
# disable:0

export VOICE_RECOGNITION                                 ?= 0
# enable:1
# disable:0

export LED_STATUS_INDICATION                   ?= 0
# enable:1
# disable:0

export BLE                                                 ?= 0
# enable:1
# disable:0

export BTADDR_GEN                                    ?= 1
# enable:1
# disable:0

export FACTORY_MODE                                 ?= 1
# enable:1
# disable:0

export ENGINEER_MODE                                 ?= 1
# enable:1
# disable:0

export AUDIO_SCO_BTPCM_CHANNEL               ?= 1
# enable:1
# disable:0

include $(KBUILD_ROOT)/config/$(T)/customize.mk
include $(KBUILD_ROOT)/config/$(T)/hardware.mk
include $(KBUILD_ROOT)/config/convert.mk