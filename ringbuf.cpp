/*
 * Yet another ring buffer implementation for bytes.
 *
 * Author: Rob Bultman
 * email : rob@firstbuild.com
 *
 * Copyright (c) 2014 FirstBuild
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ringbuf.h"
#include <stdlib.h>

RingBuffer::RingBuffer(uint8_t *pBuffer, uint8_t bufSize) {
   if ((pBuffer == NULL) || (bufSize == 0))
   {
      pBuf = NULL;
      bufSize = 0;
      return;
   }

   head = 0;
   tail = 0;
   size = bufSize;
   pBuf = pBuffer;
   full = RING_BUFFER_NOT_FULL;
   empty = RING_BUFFER_IS_EMPTY;
   bytesUsed = 0;
}

uint8_t RingBuffer::Write(uint8_t val) {
   if (pBuf == NULL) {
      return RING_BUFFER_BAD;
   }
   if (full == RING_BUFFER_IS_FULL) {
      return RING_BUFFER_ADD_FAILURE;
   }

   pBuf[tail++] = val;
   bytesUsed++;
   empty = RING_BUFFER_NOT_EMPTY;
   if (tail >= size) {
      tail = 0;
   }
   if (head == tail) {
      full = RING_BUFFER_IS_FULL;
   }

   return RING_BUFFER_ADD_SUCCESS;
}

uint8_t RingBuffer::Read(void) {
   uint8_t retVal = 0xff;

   if (pBuf == NULL) {
      return retVal;
   }
   if (empty == RING_BUFFER_IS_EMPTY) {
      return retVal;
   }

   retVal = pBuf[head++];
   bytesUsed--;
   full = RING_BUFFER_NOT_FULL;
   if (head >= size) {
      head = 0;
   }
   if (head == tail) {
      empty = RING_BUFFER_IS_EMPTY;
   }

   return retVal;
}

uint8_t RingBuffer::IsEmpty(void) {
   if (pBuf == NULL) {
      return RING_BUFFER_BAD;
   }

   return empty;
}

uint8_t RingBuffer::IsFull(void) {
   if (pBuf == NULL) {
      return RING_BUFFER_BAD;
   }

   return full;
}

uint8_t RingBuffer::Peek(uint8_t pos) {
   if (pBuf == NULL) {
      return 0xff;
   }
   if (pos >= size) {
      return 0xff;
   }
   if (empty == RING_BUFFER_IS_EMPTY) {
      return 0xff;
   }
   if (pos >= bytesUsed) {
      return 0xff;
   }

   pos += head;
   if (pos >= size) {
      pos -= (size - 1);
   }

   return pBuf[pos];
}


uint8_t RingBuffer::BytesUsed(void) {
   (void) pBuf;

   if (pBuf == NULL) {
      return 0xff;
   }

   return bytesUsed;
}

uint8_t RingBuffer::BytesAvailable(void) {
   if (pBuf == NULL) {
      return 0xff;
   }
   
   return size - bytesUsed;
}


