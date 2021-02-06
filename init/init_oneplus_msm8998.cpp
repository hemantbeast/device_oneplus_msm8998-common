/*
   Copyright (C) 2007, The Android Open Source Project
   Copyright (c) 2016, The CyanogenMod Project
   Copyright (c) 2017, The LineageOS Project

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#include <sys/sysinfo.h>

#include "vendor_init.h"

static constexpr const char* kDcDimmingBrightnessPath =
        "/proc/flicker_free/min_brightness";
static constexpr const char* kProjectNamePath =
        "/sys/project_info/project_name";
static constexpr const char* kSensorVersionPath =
        "/sys/devices/soc/soc:fingerprint_detect/sensor_version";

using android::base::Trim;
using android::base::GetProperty;
using android::base::ReadFileToString;
using android::base::WriteStringToFile;

void property_override(char const prop[], char const value[])
{
    prop_info *pi;

    pi = (prop_info*) __system_property_find(prop);
    if (pi)
        __system_property_update(pi, value, strlen(value));
    else
        __system_property_add(prop, strlen(prop), value, strlen(value));
}

void init_target_properties()
{
    std::string device;
    bool unknownDevice = true;

    if (ReadFileToString(kProjectNamePath, &device)) {
        LOG(INFO) << "Device info: " << device;

        if (!strncmp(device.c_str(), "16859", 5)) {
            // Oneplus 5
            property_override("ro.display.series", "OnePlus 5");
            WriteStringToFile("66", kDcDimmingBrightnessPath);
            unknownDevice = false;
        }
        else if (!strncmp(device.c_str(), "17801", 5)) {
            // Oneplus 5T
            property_override("ro.display.series", "OnePlus 5T");
            WriteStringToFile("370", kDcDimmingBrightnessPath);
            unknownDevice = false;
        }

        property_override("vendor.boot.project_name", device.c_str());
    }
    else {
        LOG(ERROR) << "Unable to read device info from " << kProjectNamePath;
    }

    if (unknownDevice) {
        property_override("ro.display.series", "UNKNOWN");
    }
}

void init_fingerprint_properties()
{
    std::string sensor_version;

    if (ReadFileToString(kSensorVersionPath, &sensor_version)) {
        LOG(INFO) << "Loading Fingerprint HAL for sensor version " << sensor_version;
        if (Trim(sensor_version) == "1" || Trim(sensor_version) == "2") {
            property_override("ro.hardware.fingerprint", "fpc");
        }
        else if (Trim(sensor_version) == "3") {
            property_override("ro.hardware.fingerprint", "goodix");
        }
        else {
            LOG(ERROR) << "Unsupported fingerprint sensor: " << sensor_version;
        }
    }
    else {
        LOG(ERROR) << "Failed to detect sensor version";
    }
}

void init_dalvik_vm_properties()
{
    struct sysinfo sys;

    sysinfo(&sys);
    if (sys.totalram < 7168ull * 1024 * 1024) {
        // 6GB RAM
        property_override("dalvik.vm.heapstartsize", "8m");
        property_override("dalvik.vm.heapgrowthlimit", "192m");
        property_override("dalvik.vm.heapsize", "512m");
        property_override("dalvik.vm.heaptargetutilization", "0.6");
        property_override("dalvik.vm.heapminfree", "8m");
        property_override("dalvik.vm.heapmaxfree", "16m");
    }
    else {
        // 8GB RAM
        property_override("dalvik.vm.heapstartsize", "24m");
        property_override("dalvik.vm.heapgrowthlimit", "256m");
        property_override("dalvik.vm.heapsize", "512m");
        property_override("dalvik.vm.heaptargetutilization", "0.46");
        property_override("dalvik.vm.heapminfree", "8m");
        property_override("dalvik.vm.heapmaxfree", "48m");
    }
}

void vendor_load_properties() {
    LOG(INFO) << "Loading vendor specific properties";
    init_target_properties();
    init_fingerprint_properties();
    init_dalvik_vm_properties();
}
