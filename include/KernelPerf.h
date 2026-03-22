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

#ifndef WINKERNEL_KERNELPERF_H_
#define WINKERNEL_KERNELPERF_H_

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

__forceinline
LARGE_INTEGER
KeQueryPerformanceCounter(
    _Out_opt_ PLARGE_INTEGER PerformanceFrequency
)
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    if (PerformanceFrequency) {
        QueryPerformanceFrequency(PerformanceFrequency);
    }
    return counter;
}

#ifdef __cplusplus
}
#endif

#endif  /* WINKERNEL_KERNELPERF_H_ */
