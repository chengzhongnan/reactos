/*
 *  FreeLoader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Based on XenoLinux drivers/xen/console/console.c
 * Copyright (c) 2002-2004, K A Fraser.
 * 
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "freeldr.h"
#include "machxen.h"

#include <rosxen.h>
#include <xen.h>
#include <ctrl_if.h>
#include <evtchn.h>

/*
 * Extra ring macros to sync a consumer index up to the public producer index.
 * Generally UNSAFE, but we use it for recovery and shutdown in some cases.
 */
#define RING_DROP_PENDING_RESPONSES(_r)                                 \
    do {                                                                \
        (_r)->rsp_cons = (_r)->sring->rsp_prod;                         \
    } while (0)

#define OUTPUT_BUFFER_SIZE sizeof(((ctrl_msg_t *) NULL)->msg)
static char OutputBuffer[OUTPUT_BUFFER_SIZE];
static unsigned OutputPtr = 0;

VOID
XenConsFlush()
{
  ctrl_msg_t Msg;

  if (0 == OutputPtr)
    {
      return;
    }

  Msg.type = CMSG_CONSOLE;
  Msg.subtype = CMSG_CONSOLE_DATA;
  Msg.length = OutputPtr;
  memcpy(Msg.msg, OutputBuffer, OutputPtr);
  Msg.id      = 0xff;

  XenCtrlIfSendMessageBlock(&Msg);

  OutputPtr = 0;
}

VOID
XenConsFlushWait()
{
  ctrl_msg_t Msg;

  XenEvtchnDisableEvents();

  while (0 != OutputPtr || ! XenCtrlIfTransmitterEmpty())
    {
      XenCtrlIfDiscardResponses();

      if (0 == OutputPtr)
        {
          continue;
        }

      Msg.type = CMSG_CONSOLE;
      Msg.subtype = CMSG_CONSOLE_DATA;
      Msg.length = OutputPtr;
      memcpy(Msg.msg, OutputBuffer, OutputPtr);
      Msg.id      = 0xff;

      if (XenCtrlIfSendMessageNoblock(&Msg))
        {
          OutputPtr = 0;
        }
    }

  XenEvtchnEnableEvents();
}

static VOID
PutCharInBuffer(int Ch)
{
  if (OUTPUT_BUFFER_SIZE <= OutputPtr)
    {
      XenConsFlush();
    }
  OutputBuffer[OutputPtr++] = Ch;
}

VOID
XenConsPutChar(int Ch)
{
  if ('\n' == Ch)
    {
      PutCharInBuffer('\r');
      PutCharInBuffer('\n');
      XenConsFlush();
    }
  else
    {
      PutCharInBuffer(Ch);
    }
}

/* EOF */
