/*
 *  Copyright (C) 2006-2016, Arm Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of Mbed TLS (https://tls.mbed.org)
 */

/*
 * Workaround to mbed-mbedtls dependency on mbed-filesystem when
 * MBEDTLS_FS_IO is enabled
 *
 * On enabling MBEDTLS_FS_IO, mbed-mbedtls will become dependent on
 * mbed-filesystem, but this dependency is not specified in mbedtls
 * CMakeLists.txt. This dependency becomes to compile error when
 * platform.stdio-minimal-console-only is also enabled for small memory
 * footprint (like this project). Work around the issue by disabling
 * MBEDTLS_FS_IO.
 *
 */
#if defined(MBEDTLS_FS_IO)
#undef MBEDTLS_FS_IO
#endif
