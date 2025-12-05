/*
 * Copyright 2025 WinKernelLite Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "include/Resource.h"

/* Global Resource state variables - single instance across all translation units */

/* Critical region counter (protected by its own critical section) */
LONG g_WinKernelLite_KernelApcDisableCount = 0;
CRITICAL_SECTION g_WinKernelLite_KernelApcDisableLock;
BOOLEAN g_WinKernelLite_KernelApcDisableLockInitialized = FALSE;

/* Global system resources list */
LIST_ENTRY g_WinKernelLite_SystemResourcesList;
CRITICAL_SECTION g_WinKernelLite_SystemResourcesLock;
BOOLEAN g_WinKernelLite_SystemResourcesInitialized = FALSE;
