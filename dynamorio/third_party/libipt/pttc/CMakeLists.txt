# Copyright (c) 2013-2022, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#  * Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#  * Neither the name of Intel Corporation nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

include_directories(
  include
  ../libipt/internal/include
)

set(PTTC_FILES
  src/errcode.c
  src/file.c
  src/parse.c
  src/pttc.c
  src/util.c
  src/yasm.c
  ../libipt/src/pt_cpu.c
)

if (CMAKE_HOST_UNIX)
  set(PTTC_FILES
    ${PTTC_FILES}
    src/posix/util.c
  )
endif (CMAKE_HOST_UNIX)

if (CMAKE_HOST_WIN32)
  set(PTTC_FILES
    ${PTTC_FILES}
    src/windows/util.c
  )
endif (CMAKE_HOST_WIN32)

add_executable(pttc
  ${PTTC_FILES}

  src/main.c
)

target_link_libraries(pttc libipt)

if (PEVENT)
  target_link_libraries(pttc pevent)
endif (PEVENT)
