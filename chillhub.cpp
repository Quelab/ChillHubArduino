#include "Arduino.h"
#include "chillhub.h"
#include "crc.h"
#include <string.h>
#include <stdint.h>

// define this macro if there is no debug UART port.
#define DebugUart_UartPutString(...)
#define DebugUart_SpiUartWriteTxData(...)
#ifndef MSB_OF_U16
  #define MSB_OF_U16(v) ((v>>8)&0x00ff)
#endif
#ifndef LSB_OF_U16
  #define LSB_OF_U16(v) (v&0x00ff)
#endif

#define STX 0xff
#define ESC 0xfe

static const char resIdKey[] = "resID";
static const char valKey[] = "val";

static const uint8_t sizeOfU32JsonField = 5;
static const uint8_t sizeOfU16JsonField = 3;
static const uint8_t sizeOfU8JsonField = 2;

//chInterface ChillHub;

const StateHandler_fp chInterface::StateHandlers[] = {
   StateHandler_WaitingForStx,
   StateHandler_WaitingForLength,
   StateHandler_WaitingForPacket,
   NULL
};

unsigned char chInterface::packetBuf[64] = {0};
RingBuffer chInterface::packetRB(&packetBuf[0], sizeof(packetBuf));
uint8_t chInterface::currentState = State_WaitingForStx;
unsigned char chInterface::recvBuf[64] = {0};
uint8_t chInterface::bufIndex;
uint8_t chInterface::payloadLen;
uint8_t chInterface::msgType;
uint8_t chInterface::dataType;
uint8_t chInterface::packetLen;
uint8_t chInterface::packetIndex;
chCbTableType* chInterface::callbackTable = NULL;

chInterface::chInterface(void) {
  Serial.begin(115200);
}

void chInterface::printU8(uint8_t val) {
  uint8_t digits[3];
  uint8_t i;

  for (i=0; i<sizeof(digits); i++) {
    digits[sizeof(digits)-i-1] = val % 10;
    val = val / 10;
  }

  for(i=0; (i<(sizeof(digits)-1))&&(digits[i] == 0); i++);

  for (; i<sizeof(digits); i++) {
    DebugUart_SpiUartWriteTxData(digits[i]+'0');
  }
}

void chInterface::printU16(uint16_t val) {
  uint8_t digits[5];
  uint8_t i;

  for (i=0; i<sizeof(digits); i++) {
    digits[sizeof(digits)-i-1] = val % 10;
    val = val / 10;
  }

  for(i=0; (i<(sizeof(digits)-1))&&(digits[i] == 0); i++);

  for (; i<sizeof(digits); i++) {
    DebugUart_SpiUartWriteTxData(digits[i]+'0');
  }
}

void chInterface::printI16(int16_t val) {
  uint8_t digits[5];
  uint8_t i;

  if (val < 0) {
    DebugUart_SpiUartWriteTxData('-');
    val = -val;
  }

  for (i=0; i<sizeof(digits); i++) {
    digits[sizeof(digits)-i-1] = val % 10;
    val = val / 10;
  }

  for(i=0; (i<(sizeof(digits)-1))&&(digits[i] == 0); i++);

  for (; i<sizeof(digits); i++) {
    DebugUart_SpiUartWriteTxData(digits[i]+'0');
  }
}

void chInterface::printU32(uint32_t val) {
  uint8_t digits[10];
  uint8_t i;

  for (i=0; i<sizeof(digits); i++) {
    digits[sizeof(digits)-i-1] = val % 10;
    val = val / 10;
  }

  for(i=0; (i<(sizeof(digits)-1))&&(digits[i] == 0); i++);

  for (; i<sizeof(digits); i++) {
    DebugUart_SpiUartWriteTxData(digits[i]+'0');
  }
}

void chInterface::printI32(int32_t val) {
  uint8_t digits[10];
  uint8_t i;

  if (val < 0) {
    DebugUart_SpiUartWriteTxData('-');
    val = -val;
  }

  for (i=0; i<sizeof(digits); i++) {
    digits[sizeof(digits)-i-1] = val % 10;
    val = val / 10;
  }

  for(i=0; (i<(sizeof(digits)-1))&&(digits[i] == 0); i++);

  for (; i<sizeof(digits); i++) {
    DebugUart_SpiUartWriteTxData(digits[i]+'0');
  }
}

void chInterface::sendU8Msg(unsigned char msgType, unsigned char payload) {
  uint8_t buf[16];
  uint8_t index=0;

  buf[index++] = 3;
  buf[index++] = msgType;
  buf[index++] = unsigned8DataType;
  buf[index++] = payload;
  sendPacket(buf, index);
}

void chInterface::sendI8Msg(unsigned char msgType, signed char payload) {
  uint8_t buf[16];
  uint8_t index=0;

  buf[index++] = 3;
  buf[index++] = msgType;
  buf[index++] = signed8DataType;
  buf[index++] = payload;
  sendPacket(buf, index);
}

void chInterface::sendU16Msg(unsigned char msgType, unsigned int payload) {
  uint8_t buf[16];
  uint8_t index=0;

  buf[index++] = 4;
  buf[index++] = msgType;
  buf[index++] = unsigned16DataType;
  buf[index++] = (payload >> 8) & 0xff;
  buf[index++] = payload & 0xff;
  sendPacket(buf, index);
}

void chInterface::sendI16Msg(unsigned char msgType, signed int payload) {
  uint8_t buf[16];
  uint8_t index=0;

  buf[index++] = 4;
  buf[index++] = msgType;
  buf[index++] = signed16DataType;
  buf[index++] = (payload >> 8) & 0xff;
  buf[index++] = payload & 0xff;
  sendPacket(buf, index);
}

void chInterface::sendBooleanMsg(unsigned char msgType, unsigned char payload) {
  uint8_t buf[16];
  uint8_t index=0;

  buf[index++] = 3;
  buf[index++] = msgType;
  buf[index++] = booleanDataType;
  buf[index++] = payload;
  sendPacket(buf, index);
}

void chInterface::setup(const char* name, const char *UUID) {
  uint8_t buf[128];
  uint8_t nameLen = strlen(name);
  uint8_t uuidLen = strlen(UUID);
  uint8_t index=0;

  memset(buf, 0, sizeof(buf));

  // Is the buf big enough to send the name?
  if ((nameLen + uuidLen) >= (sizeof(buf)-20)) {
    return;
  }

  // send header info
  buf[index++] = nameLen + uuidLen + 6; // length of the following message
  buf[index++] = deviceIdMsgType;
  buf[index++] = arrayDataType;
  buf[index++] = 2; // number of elements
  buf[index++] = stringDataType; // data type of elements

  // send device type
  buf[index++] = nameLen;
  strcat((char *)&buf[index], name);
  index += nameLen;

  // send UUID
  buf[index++] = uuidLen;
  strcat((char *)&buf[index], UUID);
  index += uuidLen;
  sendPacket(buf, index);
}

void chInterface::subscribe(unsigned char type, chillhubCallbackFunction callback) {
  storeCallbackEntry(type, CHILLHUB_CB_TYPE_FRIDGE, callback);
  sendU8Msg(subscribeMsgType, type);
}

void chInterface::unsubscribe(unsigned char type) {
  sendU8Msg(unsubscribeMsgType, type);
  callbackRemove(type, CHILLHUB_CB_TYPE_FRIDGE);
}

void chInterface::setAlarm(unsigned char ID, char* cronString, unsigned char strLength, chillhubCallbackFunction callback) {
  uint8_t buf[256];
  uint8_t index=0;

  storeCallbackEntry(ID, CHILLHUB_CB_TYPE_CRON, callback);

  buf[index++] = strLength + 4; // message length
  buf[index++] = setAlarmMsgType;
  buf[index++] = stringDataType;
  buf[index++] = strLength + 1; // string length
  buf[index++] = ID; // callback id... it's best to use a character here otherwise things don't work right
  strncat((char *)&buf[index], cronString, strLength);
  index += strLength;
  sendPacket(buf, index);
}

void chInterface::unsetAlarm(unsigned char ID) {
  sendU8Msg(unsetAlarmMsgType, ID);
  callbackRemove(ID, CHILLHUB_CB_TYPE_CRON);
}

void chInterface::getTime(chillhubCallbackFunction cb) {
  uint8_t buf[16];
  uint8_t index=0;

  storeCallbackEntry(0, CHILLHUB_CB_TYPE_TIME, cb);

  buf[index++] = 1;
  buf[index++] = getTimeMsgType;
  sendPacket(buf, index);
}

void chInterface::addCloudListener(unsigned char ID, chillhubCallbackFunction cb) {
  storeCallbackEntry(ID, CHILLHUB_CB_TYPE_CLOUD, cb);
}

uint8_t chInterface::appendJsonKey(uint8_t *pBuf, const char *key) {
  uint8_t keyLen = strlen(key);
  *pBuf = keyLen;
  pBuf++;
  strncpy((char *)pBuf, key, keyLen);
  return keyLen + 1;
}

uint8_t chInterface::appendJsonString(uint8_t *pBuf, const char *s) {
  uint8_t len = strlen(s);
  *pBuf++ = stringDataType;
  *pBuf++ = len;
  strcpy((char*)pBuf, s);
  return len + 2;
}

uint8_t chInterface::appendJsonU8(uint8_t *pBuf, uint8_t v) {
  *pBuf++ = unsigned8DataType;
  *pBuf++ = v;
  return 2;
}

uint8_t chInterface::appendJsonU16(uint8_t *pBuf, uint16_t v) {
  *pBuf++ = unsigned16DataType;
  *pBuf++ = MSB_OF_U16(v);
  *pBuf++ = LSB_OF_U16(v);
  return 3;
}

uint8_t chInterface::appendJsonU32(uint8_t *pBuf, uint32_t v) {
  *pBuf++ = unsigned32DataType;
  *pBuf++ = (((v) & 0xFF000000)>>32);
  *pBuf++ = (((v) & 0xFF0000)>>16);
  *pBuf++ = (((v) & 0xFF00)>>8);
  *pBuf++ = ((v) &  0xFF);

  return 5;
}

uint8_t chInterface::appendJsonI8(uint8_t *pBuf, int8_t v) {
  *pBuf++ = signed8DataType;
  *pBuf++ = v;
  return 2;
}

uint8_t chInterface::appendJsonI16(uint8_t *pBuf, int16_t v) {
  *pBuf++ = signed16DataType;
  *pBuf++ = MSB_OF_U16(v);
  *pBuf++ = LSB_OF_U16(v);
  return 3;
}

uint8_t chInterface::appendJsonI32(uint8_t *pBuf, int32_t v) {
  *pBuf++ = signed32DataType;
  *pBuf++ = (((v) & 0xFF000000)>>32);
  *pBuf++ = (((v) & 0xFF0000)>>16);
  *pBuf++ = (((v) & 0xFF00)>>8);
  *pBuf++ = ((v) &  0xFF);

  return 5;
}

void chInterface::createCloudResourceU16(const char *name, uint8_t resID, uint8_t canUpdate, uint16_t initVal) {
  uint8_t buf[256];
  uint8_t index=0;

  // set up message header and send
  index = 0;
  buf[index++] = 37 + strlen(name); // length
  buf[index++] = registerResourceType; // message type
  buf[index++] = jsonDataType; // message data type
  buf[index++] = 4; // JSON fields

  index += appendJsonKey(&buf[index], "name");
  index += appendJsonString(&buf[index], name);

  index += appendJsonKey(&buf[index], resIdKey);
  index += appendJsonU8(&buf[index], resID);

  index += appendJsonKey(&buf[index], "canUp");
  index += appendJsonU8(&buf[index], canUpdate);

  index += appendJsonKey(&buf[index], "initVal");
  index += appendJsonU16(&buf[index], initVal);

  sendPacket(buf, index);
}

void chInterface::createCloudResourceI16(const char *name, uint8_t resID, uint8_t canUpdate, int16_t initVal) {
  uint8_t buf[256];
  uint8_t index=0;

  // set up message header and send
  index = 0;
  buf[index++] = 37 + strlen(name); // length
  buf[index++] = registerResourceType; // message type
  buf[index++] = jsonDataType; // message data type
  buf[index++] = 4; // JSON fields

  index += appendJsonKey(&buf[index], "name");
  index += appendJsonString(&buf[index], name);

  index += appendJsonKey(&buf[index], resIdKey);
  index += appendJsonU8(&buf[index], resID);

  index += appendJsonKey(&buf[index], "canUp");
  index += appendJsonU8(&buf[index], canUpdate);

  index += appendJsonKey(&buf[index], "initVal");
  index += appendJsonI16(&buf[index], initVal);

  sendPacket(buf, index);
}


void chInterface::createCloudResourceU32(const char *name, uint8_t resID, uint8_t canUpdate, uint32_t initVal) {
  uint8_t buf[256];
  uint8_t index=0;

  // set up message header and send
  index = 0;
  buf[index++] = 37 + strlen(name); // length
  buf[index++] = registerResourceType; // message type
  buf[index++] = jsonDataType; // message data type
  buf[index++] = 4; // JSON fields

  index += appendJsonKey(&buf[index], "name");
  index += appendJsonString(&buf[index], name);

  index += appendJsonKey(&buf[index], resIdKey);
  index += appendJsonU8(&buf[index], resID);

  index += appendJsonKey(&buf[index], "canUp");
  index += appendJsonU8(&buf[index], canUpdate);

  index += appendJsonKey(&buf[index], "initVal");
  index += appendJsonU32(&buf[index], initVal);

  sendPacket(buf, index);
}

void chInterface::createCloudResourceI32(const char *name, uint8_t resID, uint8_t canUpdate, int32_t initVal) {
  uint8_t buf[256];
  uint8_t index=0;

  // set up message header and send
  index = 0;
  buf[index++] = 37 + strlen(name); // length
  buf[index++] = registerResourceType; // message type
  buf[index++] = jsonDataType; // message data type
  buf[index++] = 4; // JSON fields

  index += appendJsonKey(&buf[index], "name");
  index += appendJsonString(&buf[index], name);

  index += appendJsonKey(&buf[index], resIdKey);
  index += appendJsonU8(&buf[index], resID);

  index += appendJsonKey(&buf[index], "canUp");
  index += appendJsonU8(&buf[index], canUpdate);

  index += appendJsonKey(&buf[index], "initVal");
  index += appendJsonI32(&buf[index], initVal);

  sendPacket(buf, index);
}

uint8_t chInterface::sizeOfJsonKey(const char *key) {
  return strlen(key) + 1;
}

void chInterface::updateCloudResourceU16(uint8_t resID, uint16_t val) {
  uint8_t buf[64];
  uint8_t index = 0;

  buf[index++] = 3 +
    sizeOfJsonKey(resIdKey) + sizeOfU8JsonField +
    sizeOfJsonKey(valKey) + sizeOfU16JsonField;

  buf[index++] = updateResourceType;
  buf[index++] = jsonDataType;
  buf[index++] = 2; // number of json fields

  index += appendJsonKey(&buf[index], resIdKey);
  index += appendJsonU8(&buf[index], resID);
  index += appendJsonKey(&buf[index], valKey);
  index += appendJsonU16(&buf[index], val);

  sendPacket(buf, index);
}

void chInterface::updateCloudResourceI16(uint8_t resID, int16_t val) {
  uint8_t buf[64];
  uint8_t index = 0;

  buf[index++] = 3 +
    sizeOfJsonKey(resIdKey) + sizeOfU8JsonField +
    sizeOfJsonKey(valKey) + sizeOfU16JsonField;

  buf[index++] = updateResourceType;
  buf[index++] = jsonDataType;
  buf[index++] = 2; // number of json fields

  index += appendJsonKey(&buf[index], resIdKey);
  index += appendJsonU8(&buf[index], resID);
  index += appendJsonKey(&buf[index], valKey);
  index += appendJsonI16(&buf[index], val);

  sendPacket(buf, index);
}

void chInterface::updateCloudResourceU32(uint8_t resID, uint32_t val) {
  uint8_t buf[64];
  uint8_t index = 0;

  buf[index++] = 3 +
    sizeOfJsonKey(resIdKey) + sizeOfU8JsonField +
    sizeOfJsonKey(valKey) + sizeOfU32JsonField;

  buf[index++] = updateResourceType;
  buf[index++] = jsonDataType;
  buf[index++] = 2; // number of json fields

  index += appendJsonKey(&buf[index], resIdKey);
  index += appendJsonU8(&buf[index], resID);
  index += appendJsonKey(&buf[index], valKey);
  index += appendJsonU32(&buf[index], val);

  sendPacket(buf, index);
}

void chInterface::updateCloudResourceI32(uint8_t resID, int32_t val) {
  uint8_t buf[64];
  uint8_t index = 0;

  buf[index++] = 3 +
    sizeOfJsonKey(resIdKey) + sizeOfU8JsonField +
    sizeOfJsonKey(valKey) + sizeOfU32JsonField;

  buf[index++] = updateResourceType;
  buf[index++] = jsonDataType;
  buf[index++] = 2; // number of json fields

  index += appendJsonKey(&buf[index], resIdKey);
  index += appendJsonU8(&buf[index], resID);
  index += appendJsonKey(&buf[index], valKey);
  index += appendJsonI32(&buf[index], val);

  sendPacket(buf, index);
}

void chInterface::processChillhubMessagePayload(void) {
  chillhubCallbackFunction callback = NULL;

  // got the payload, process the message
  bufIndex = 0;
  payloadLen = recvBuf[bufIndex++];
  msgType = recvBuf[bufIndex++];
  dataType = recvBuf[bufIndex++];

  if ((msgType == alarmNotifyMsgType) || (msgType == timeResponseMsgType)) {
    // data is an array, don't care about data type or length
    bufIndex+=2;
    if (msgType == alarmNotifyMsgType) {
      DebugUart_UartPutString("Got an alarm notification.\r\n");
      callback = callbackLookup(recvBuf[bufIndex++], CHILLHUB_CB_TYPE_CRON);
    }
    else {
      DebugUart_UartPutString("Received a time response.\r\n");
      callback = callbackLookup(0, CHILLHUB_CB_TYPE_TIME);
    }

    if (callback) {
      unsigned char time[4];
      for (uint8_t j = 0; j < 4; j++)
      {
        time[j] = recvBuf[bufIndex++];
      }
      DebugUart_UartPutString("Calling time response/alarm callback.\r\n");
      ((chCbFcnTime)callback)(time); // <-- I don't think this works this way...

      if (msgType == timeResponseMsgType) {
        callbackRemove(0, CHILLHUB_CB_TYPE_TIME);
      }
    } else {
      DebugUart_UartPutString("No callback found.\r\n");
    }
  }
  else {
    DebugUart_UartPutString("Received a message: ");
    printU8(msgType);
    DebugUart_UartPutString("\r\n");
    callback = callbackLookup(msgType, (msgType <= CHILLHUB_RESV_MSG_MAX)?CHILLHUB_CB_TYPE_FRIDGE:CHILLHUB_CB_TYPE_CLOUD);

    if (callback) {
      DebugUart_UartPutString("Found a callback for this message, calling...\r\n");
      switch(dataType) {
        case stringDataType:
          DebugUart_UartPutString("Data type is a string.\r\n");
          ((chCbFcnStr)callback)((char *)&recvBuf[bufIndex]);
          break;
        case unsigned8DataType:
        case booleanDataType:
          ((chCbFcnU8)callback)(recvBuf[bufIndex++]);
          break;
        case unsigned16DataType: {
          unsigned int payload = 0;
          DebugUart_UartPutString("Data type is a U16.\r\n");
          payload |= (recvBuf[bufIndex++] << 8);
          payload |= recvBuf[bufIndex++];
          ((chCbFcnU16)callback)(payload);
          break;
        }
        case unsigned32DataType: {
          unsigned long payload = 0;
          DebugUart_UartPutString("Data type is a U32.\r\n");
          for (char j = 0; j < 4; j++) {
            payload = payload << 8;
            payload |= recvBuf[bufIndex++];
          }
          ((chCbFcnU32)callback)(payload);
          break;
        }
        default:
          DebugUart_UartPutString("Don't know what this data type is: ");
          printU8(dataType);
          DebugUart_UartPutString("\r\n");
      }
    } else {
      DebugUart_UartPutString("No callback for this message found.\r\n");
    }
  }
}

void chInterface::ReadFromSerialPort(void) {
  if (Serial.available() > 0) {
    // Get the payload length.  It is one less than the message length.
    if (packetRB.IsFull() == RING_BUFFER_IS_FULL) {
      DebugUart_UartPutString("Ringbuffer was full, removing a byte.\r\n");
      packetRB.Read();
    }
    packetRB.Write(Serial.read());
  }
}

void chInterface::CheckPacket(void) {
  uint8_t i;
  uint16_t crc = crc_init();
  uint16_t crcSent = (recvBuf[bufIndex-2]<<8) + recvBuf[bufIndex-1];
  bufIndex -= 2;

  for(i=0; i<bufIndex; i++) {
    crc = crc_update(crc, &recvBuf[i], 1);
  }

  crc = crc_finalize(crc);

  if (crc == crcSent) {
    DebugUart_UartPutString("Checksum checks!\r\n");
    processChillhubMessagePayload();
  } else {
    DebugUart_UartPutString("Checksum FAILED!\r\n");
    DebugUart_UartPutString("Checksum received: ");
    printU16(crcSent);
    DebugUart_UartPutString("\r\nChecksum calc'd: ");
    printU16(crc);
    DebugUart_UartPutString("\r\n");
  }
}

// state handlers
uint8_t chInterface::StateHandler_WaitingForStx(void) {
  ReadFromSerialPort();

  // process bytes in the buffer
  while(packetRB.IsEmpty() == RING_BUFFER_NOT_EMPTY) {
    if (packetRB.Read() == STX) {
      DebugUart_UartPutString("Got STX.\r\n");
      return State_WaitingForLength;
    }
  }

  return State_WaitingForStx;
}

uint8_t chInterface::StateHandler_WaitingForLength(void) {
  ReadFromSerialPort();

  if (packetRB.IsEmpty() == RING_BUFFER_NOT_EMPTY) {
    packetLen = packetRB.Peek(0);
    if (packetLen == ESC) {
      if (packetRB.BytesUsed() > 1) {
        packetRB.Read();
      } else {
        return State_WaitingForLength;
      }
    }
    packetLen = packetRB.Read();
    if (packetLen < sizeof(packetBuf)-2) {
      bufIndex = 0;
      payloadLen = 0;
      msgType = 0;
      dataType = 0;
      packetIndex = 0;
      DebugUart_UartPutString("Got length!\r\n");
      return State_WaitingForPacket;
    } else {
      DebugUart_UartPutString("Length is too long, aborting.\r\n");
      return State_WaitingForStx;
    }
  }

  return State_WaitingForLength;
}

uint8_t chInterface::StateHandler_WaitingForPacket(void) {
  uint8_t bytesUsed;
  uint8_t b;
  ReadFromSerialPort();

  bytesUsed = packetRB.BytesUsed();
  while (bytesUsed > packetIndex) {
    if (packetRB.Peek(packetIndex) == ESC) {
      if ((bytesUsed - packetIndex) > 1) {
        packetIndex++;
      } else {
        return State_WaitingForPacket;
      }
    }
    b = packetRB.Peek(packetIndex++);
    recvBuf[bufIndex++] =  b;
    DebugUart_UartPutString("Got a byte: ");
    printU8(b);
    DebugUart_UartPutString("\r\n");
    if (bufIndex >= packetLen + 2) {
      CheckPacket();
      return State_WaitingForStx;
    }
  }

  return State_WaitingForPacket;
}

void chInterface::loop(void) {
  if (currentState < State_Invalid) {
    if(StateHandlers[currentState] != NULL) {
      currentState = StateHandlers[currentState]();
    }
  }
}

void chInterface::storeCallbackEntry(unsigned char sym, unsigned char typ, chillhubCallbackFunction fcn) {
  chCbTableType* newEntry = new chCbTableType;
  newEntry->symbol = sym;
  newEntry->type = typ;
  newEntry->callback = fcn;
  newEntry->rest = callbackTable;
  callbackTable = newEntry;
}

chillhubCallbackFunction chInterface::callbackLookup(unsigned char sym, unsigned char typ) {
  chCbTableType* entry = callbackTable;
  while (entry) {
    if ((entry->type == typ) && (entry->symbol == sym))
      return (entry->callback);
    else
      entry = entry->rest;
  }
  return NULL;
}

void chInterface::callbackRemove(unsigned char sym, unsigned char typ) {
  chCbTableType* prev = callbackTable;
  chCbTableType* entry;
  if (prev)
    entry = prev->rest;

  while (entry) {
    if ((entry->type == typ) && (entry->symbol == sym)) {
      prev->rest = entry->rest;
      delete entry;
    }
    else {
      prev = entry;
      entry = entry->rest;
    }
  }
}

uint8_t chInterface::isControlChar(uint8_t c) {
  switch(c) {
    case STX:
      return 1;
    case ESC:
      return 1;
    default:
      return 0;
  }
}

void chInterface::outputChar(uint8_t c) {
  uint8_t buf[2];
  uint8_t index=0;

  if (isControlChar(c)) {
    buf[index++] = ESC;
  }
  buf[index++] = c;

  Serial.write(buf, index);
}

void chInterface::sendPacket(uint8_t *pBuf, uint8_t len){
  uint16_t crc = crc_init();
  uint8_t buf[1];
  uint8_t i;

  // send STX
  buf[0] = STX;
  Serial.write(buf, 1);
  // send packet length
  outputChar(len);

  // send packet
  for(i=0; i<len; i++) {
    crc = crc_update(crc, &pBuf[i], 1);
    outputChar(pBuf[i]);
  }

  // send CS
  outputChar(MSB_OF_U16(crc));
  outputChar(LSB_OF_U16(crc));
}



