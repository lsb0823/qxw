cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y +=  \
    -Iservices/bt_profiles/inc \
    -Iservices/bt_profiles \
    -Iservices/bt_app \
    -Iservices/audioflinger \
    -Iservices/nvrecord \
    -Iservices/overlay \
    -Iservices/resources \
    -Iapps/apptester \
    -Iapps/factory \
    -Iutils/crc32 \
    -Iplatform/drivers/bt \
    -Iapps/audioplayers/rbplay

ifeq ($(APP_TEST_AUDIO),1)
CFLAGS_apps_tester.o += -DAPP_TEST_AUDIO
endif
