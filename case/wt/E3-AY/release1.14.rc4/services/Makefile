obj-y := audioflinger/ bt_profiles/ fs/ bt_app/ overlay/ resources/ ../utils/crc32/

ifeq ($(BLE),1)
obj-y += bluedroid/ bridge/
endif

ifneq ($(FPGA),1)
obj-y += nvrecord/
endif

ifneq ($(filter 1, $(HW_FIR_EQ_PROCESS) $(SW_IIR_EQ_PROCESS)),)
obj-y += audio_eq/
endif

ifeq ($(VOICE_RECOGNITION),1)
obj-y += voice_recognition/
endif

ifeq ($(PCTOOLS_SUPPT),1)
obj-y += rt_rwburn/
endif
