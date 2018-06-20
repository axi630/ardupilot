#pragma once 

#include "Config.h"

class F4Light::_parser { // universal parser interface
public:
    _parser() {};
    virtual ~_parser() {};

    virtual void init(uint8_t ch) = 0;
    virtual void late_init(uint8_t b) {}

    virtual uint64_t get_last_signal()    const { noInterrupts(); uint64_t t= _last_signal; interrupts(); return t; }
    virtual uint64_t get_last_change()    const { noInterrupts(); uint64_t t= _last_change; interrupts(); return t; }
    virtual uint8_t  get_valid_channels() const {                 uint8_t  t= _channels;                  return t; }
    virtual uint16_t get_val(uint8_t ch)  const {                 uint16_t t= _val[ch];                   return t; }

    virtual bool bind(int dsmMode) const  { return true; }
    virtual bool is_failsafe() const { return false; }

protected:
    volatile uint64_t _last_signal;
    volatile uint16_t _val[F4Light_RC_INPUT_NUM_CHANNELS];
    uint64_t          _last_change;
    volatile uint8_t  _channels;

};
