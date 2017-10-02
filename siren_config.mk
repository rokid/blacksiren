
# have android confi
ifneq ($(NDK_TOOLCHAIN_VERSION)$(ANDROID_TOOLCHAIN)$(ANDROID_BUILD_PATHS),)
  CONFIG_ANDROID_LOG=y
  CONFIG_BACKUP_FILE_PATH=/system/etc/blacksiren.json
  CONFIG_STORE_FILE_PATH=/data/blacksiren/
  # enable debug
  #CONFIG_NO_STDOUT_DEBUG=y

  # use legacy siren test
  CONFIG_LEGACY_SIREN_TEST=y
else
  CONFIG_BACKUP_FILE_PATH=/etc/blacksiren.json
  CONFIG_STORE_FILE_PATH=/opt/blacksiren/
  CONFIG_LEGACY_SIREN_TEST=
endif

# use FIFO priority
#CONFIG_USE_FIFO=y

# debug channel
#CONFIG_DEBUG_CHANNEL=y

CONFIG_REMOTE_CONFIG_HOSTNAME=config.open.rokid.com

CONFIG_REMOTE_CONFIG_FILE_URL=https://config.open.rokid.com/openconfig/blacksiren.json

#CONFIG_USE_AD1=y

#CONFIG_USE_AD2=y
#CONFIG_TARGET_PRODUCT=mini

#CONFIG_BF_MVDR=y
