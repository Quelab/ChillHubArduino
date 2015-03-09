#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string>
#include <cstring>
//#include <iostream>

using namespace std;

#include "ringbuf.h"

static uint8_t buf[10];

TEST_GROUP(ringBufTests)
{
   void fillBuffer(RingBuffer *rb) 
   {
      uint8_t i;

      for (i=0; i<sizeof(buf); i++) 
      {
         rb->Write(i);
      }
   }

   void setup()
   {
   }

   void teardown()
   {
   }
};

TEST(ringBufTests, compile)
{
}

TEST(ringBufTests, ringBufInitCompile)
{
   RingBuffer rb(&buf[0], sizeof(buf));
}

TEST(ringBufTests, checkpBufNullReturnsZero)
{
   RingBuffer rb(NULL, sizeof(buf));
   BYTES_EQUAL(RING_BUFFER_BAD, rb.Write(1));
}

TEST(ringBufTests, checkSizeGreaterThanZero)
{
   RingBuffer rb(&buf[0], 0);
   BYTES_EQUAL(RING_BUFFER_BAD, rb.Write(1));
}

TEST(ringBufTests, compileRingBufferWrite)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   rb.Write(42);
}

TEST(ringBufTests, checkByteAddedToBuffer)
{
   uint8_t i;

   memset(&buf[0], 42, sizeof(buf));
   RingBuffer rb(&buf[0], sizeof(buf));

   for (i=0; i<sizeof(buf); i++) {
      BYTES_EQUAL(RING_BUFFER_ADD_SUCCESS, rb.Write(i));
      BYTES_EQUAL(i, buf[i]);
      BYTES_EQUAL(i+1, rb.BytesUsed());
   }
}

TEST(ringBufTests, checkBufferFull)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   fillBuffer(&rb);
   BYTES_EQUAL(RING_BUFFER_IS_FULL, rb.IsFull());
}

TEST(ringBufTests, checkThatAddToFullBufferFails)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   fillBuffer(&rb);
   BYTES_EQUAL(RING_BUFFER_ADD_FAILURE, rb.Write(42));
}

TEST(ringBufTests, compileRead)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   rb.Read();
}

TEST (ringBufTests, readFromEmptyBufferReturnsFF)
{
   RingBuffer rb(&buf[0], sizeof(buf));

   BYTES_EQUAL(0xff, rb.Read());
}

TEST (ringBufTests, checkThatReadReturnsValue)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   rb.Write(42);
   BYTES_EQUAL(42, rb.Read());
}

TEST (ringBufTests, checkThatReadDecrementsBytesUsed)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   fillBuffer(&rb);
   rb.Read();
   BYTES_EQUAL(9, rb.BytesUsed());
}

TEST (ringBufTests, checkReadOfFullBufferContents)
{
   uint8_t i;
   RingBuffer rb(&buf[0], sizeof(buf));

   fillBuffer(&rb);

   for(i=0; i<sizeof(buf); i++) {
      BYTES_EQUAL(i, rb.Read());
   }
}

TEST (ringBufTests, checkReadFromFullBufferResultsInNotFull)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   fillBuffer(&rb);

   rb.Read();
   BYTES_EQUAL(RING_BUFFER_NOT_FULL, rb.IsFull());
}

TEST(ringBufTests, afterReadingLastByteBufferIsEmpty)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   rb.Write(42);
   rb.Read();
   BYTES_EQUAL(RING_BUFFER_IS_EMPTY, rb.IsEmpty());
}

TEST(ringBufTests, compileRingBufferIsEmpty)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   rb.IsEmpty();
}

TEST(ringBufTests, isEmptyReturnsBadPointer)
{
   RingBuffer rb(&buf[0], 0);
   BYTES_EQUAL(RING_BUFFER_BAD, rb.IsEmpty());
}

TEST(ringBufTests, emptyRingBufferReturnsEmpty)
{
   RingBuffer rb(&buf[0], sizeof(buf));

   BYTES_EQUAL(RING_BUFFER_IS_EMPTY, rb.IsEmpty());
}

TEST(ringBufTests, notEmptyRingBufferReturnsNotEmpty)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   rb.Write(42);

   BYTES_EQUAL(RING_BUFFER_NOT_EMPTY, rb.IsEmpty());
}

TEST(ringBufTests, compileRingBufferFull)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   rb.IsFull();
}

TEST(ringBufTests, isFullReturnsBadPointer)
{
   RingBuffer rb(&buf[0], 0);
   BYTES_EQUAL(RING_BUFFER_BAD, rb.IsFull());
}

TEST(ringBufTests, isFullReturnsFullWhenFull) {
   RingBuffer rb(&buf[0], sizeof(buf));
   fillBuffer(&rb);
   BYTES_EQUAL(RING_BUFFER_IS_FULL, rb.IsFull());
}

TEST(ringBufTests, isFullReturnsNotFullWhenNotFull) {
   RingBuffer rb(&buf[0], sizeof(buf));
   BYTES_EQUAL(RING_BUFFER_NOT_FULL, rb.IsFull());
}

TEST(ringBufTests, compilePeek)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   rb.Peek(0);
}

TEST(ringBufTests, peekNullPointerCheck)
{
   RingBuffer rb(NULL, sizeof(buf));
   BYTES_EQUAL(0xff, rb.Peek(0));
}

TEST(ringBufTests, peekBadPositionReturnsFail)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   BYTES_EQUAL(0xff, rb.Peek(sizeof(buf) + 1));
}

TEST(ringBufTests, peekFromEmptyBufReturnsFail)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   BYTES_EQUAL(0xff, rb.Peek(1));
}

TEST(ringBufTests, peekAttemptToReadBeyondAvailableFails)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   rb.Write(42);
   BYTES_EQUAL(0xff, rb.Peek(1));
}

TEST(ringBufTests, peekCheckPeekNormal)
{
   uint8_t i;
   RingBuffer rb(&buf[0], sizeof(buf));

   fillBuffer(&rb);

   for(i=0; i<sizeof(buf); i++) {
      BYTES_EQUAL(i, rb.Peek(i));
   }
}

TEST(ringBufTests, peekCheckWrapCase)
{
   uint8_t i;
   uint8_t retVal;
   RingBuffer rb(&buf[0], sizeof(buf));

   fillBuffer(&rb);

   for (i=0; i<4; i++) {
      rb.Read();
   }

   i = 10;
   do {
      retVal = rb.Write(i++);
   } while(retVal != RING_BUFFER_ADD_FAILURE);

   BYTES_EQUAL(13, rb.Peek(8));
}


TEST(ringBufTests, compileBytesUsed)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   rb.BytesUsed();
}

TEST(ringBufTests, bytesUsedCheckNullPointer)
{
   RingBuffer rb(NULL, sizeof(buf));
   BYTES_EQUAL(0xff, rb.BytesUsed());
}

TEST(ringBufTests, bytesUsedReturns0WhenBufEmpty)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   BYTES_EQUAL(0, rb.BytesUsed());
}

TEST(ringBufTests, bytesUsedReturnsSizeOfWhenFull)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   fillBuffer(&rb);

   BYTES_EQUAL(sizeof(buf), rb.BytesUsed());
}

TEST(ringBufTests, compileBytesAvailable)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   rb.BytesAvailable();
}

TEST(ringBufTests, bytesAvailableNullPointerCheck)
{
   RingBuffer rb(NULL, sizeof(buf));
   BYTES_EQUAL(0xff, rb.BytesAvailable());
}

TEST(ringBufTests, bytesAvailableReturnsBufSizeWhenEmpty)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   BYTES_EQUAL(sizeof(buf), rb.BytesAvailable());
}

TEST(ringBufTests, bytesAvailableReturns0WhenFull)
{
   RingBuffer rb(&buf[0], sizeof(buf));
   fillBuffer(&rb);
   BYTES_EQUAL(0, rb.BytesAvailable());
}


