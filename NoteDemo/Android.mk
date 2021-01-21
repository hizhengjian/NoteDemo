#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SDK_VERSION := system_current
LOCAL_MODULE_TAGS := optional


LOCAL_SRC_FILES := $(call all-subdir-java-files)

# use appcompat/support lib from the tree, so improvements/
# regressions are reflected in test data
#LOCAL_RESOURCE_DIR := \
#    $(LOCAL_PATH)/res \


LOCAL_STATIC_ANDROID_LIBRARIES := \
        android-support-v4 

LOCAL_PACKAGE_NAME := NoteDemo
LOCAL_CERTIFICATE := platform

LOCAL_PROGUARD_ENABLED:= disabled

#LOCAL_JNI_SHARED_LIBRARIES := libpaintworker

include $(BUILD_PACKAGE)
