#include "CanInterface.h"
#define CAN_FRAME 0

// #define DEBUG_CAN

CanInterface::CanInterface(SPIClass *spi, uint8_t csPin, uint8_t intPin) {
    pinMode(intPin, INPUT);
    _intPin = intPin;
    _CAN = new mcp2515_can(csPin);
    _CAN->setSPI(spi);
}

CanInterface::~CanInterface() {
    for (auto const& pair : _delegates) {
        delete pair.second;
    }
}

void CanInterface::begin() {
    _CAN->begin(CAN_500KBPS,MCP_8MHz);
}

void CanInterface::handle() {
    if(!digitalRead(_intPin)){
        while(_CAN->checkReceive() == CAN_MSGAVAIL){
            CanMessage message = CAN_MESSAGE_NULL;
            _CAN->readMsgBuf(&message.dataLength, message.data);
            message.id = _CAN->getCanId();

            #ifdef DEBUG_CAN 
                DEBUG_SERIAL_LN("-----------------------------");
                DEBUG_SERIAL_F("CAN MESSAGE RECEIVED - ID: 0x%X\n", message.id);

                for (int i = 0; i < message.dataLength; i++) { // print the data
                    DEBUG_SERIAL_F("0x%X\t", message.data[i]);
                }
                DEBUG_SERIAL_LN();
            #endif

            // check if we're listening for id, continue if we're not
            if(_delegates.find(message.id) == _delegates.end()){
                continue;
            }

            _delegates[message.id]->execute((CommandArgs)&message);
        }
    }
}

void CanInterface::addMessageListen(uint16_t id, Command* canListenerDelegate) {
    _delegates[id] = canListenerDelegate;
}

void CanInterface::sendMessage(CanMessage message) {
    _CAN->sendMsgBuf(message.id, CAN_FRAME, message.dataLength, message.data );   
}
