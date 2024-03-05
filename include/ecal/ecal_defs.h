/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2019 Continental Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================= eCAL LICENSE =================================
*/

/**
 * @file   ecal_defs.h
 * @brief  eCAL core defines (version numbers)
**/

#ifndef ecal_defs_h_included
#define ecal_defs_h_included
#define ECAL_VERSION_MAJOR (5)
#define ECAL_VERSION_MINOR (12)
#define ECAL_VERSION_PATCH (1)
#define ECAL_VERSION "v5.12.1"
#define ECAL_DATE "27.10.2023"
#define ECAL_PLATFORMTOOLSET "v142"

#define ECAL_VERSION_INTEGER                          ECAL_VERSION_CALCULATE(ECAL_VERSION_MAJOR, ECAL_VERSION_MINOR, ECAL_VERSION_PATCH)
#define ECAL_VERSION_CALCULATE(major, minor, patch)   (((major)<<16)|((minor)<<8)|(patch))

#define ECAL_INSTALL_APP_DIR     "bin"
#define ECAL_INSTALL_SAMPLES_DIR "samples/bin/"
#define ECAL_INSTALL_LIB_DIR     "lib"
#define ECAL_INSTALL_CONFIG_DIR  "cfg"
#define ECAL_INSTALL_INCLUDE_DIR "include"
#define ECAL_INSTALL_PREFIX      "C:/Program Files/HDF_Group/HDF5/1.10.10"

#endif // ecal_defs_h_included
