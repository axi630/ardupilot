#ifndef _USART_H
#define _USART_H

#include <hal_types.h>
#include "ring_buffer.h"

/*
 * Devices
 */

#ifndef USART_RX_BUF_SIZE
#define USART_RX_BUF_SIZE               512
#endif

#ifndef USART_TX_BUF_SIZE
#define USART_TX_BUF_SIZE               1024
#endif

typedef void (* usart_cb)();

typedef struct usart_state {
    Handler callback;

    uint8_t txbusy;

    uint8_t rx_buf[USART_RX_BUF_SIZE];
    uint8_t tx_buf[USART_TX_BUF_SIZE];

    uint8_t is_used; // flag that USART is used

} usart_state;

/** USART device type */
typedef struct usart_dev {
    USART_TypeDef* regs;             // registers
    uint32_t clk;
    IRQn_Type irq;
    uint8_t rx_pin;
    uint8_t tx_pin;
    uint8_t gpio_af;
    usart_state *state;
    ring_buffer *rxrb;                 // RX ring buffer
    ring_buffer *txrb;                 // TX ring buffer 
} usart_dev;


extern const usart_dev * const UARTS[];

#ifdef __cplusplus
  extern "C" {
#endif

extern const usart_dev * const _USART1;
extern const usart_dev * const _USART2; 
extern const usart_dev * const _USART3;
extern const usart_dev * const _UART4;
extern const usart_dev * const _UART5;
extern const usart_dev * const _USART6;

#define USART_F_RXNE 0x20
#define USART_F_TXE  0x80
#define USART_F_ORE  0x8

#define USART_BIT_IDLEIE 0x10
#define USART_BIT_RXNEIE 0x20
#define USART_BIT_TCEIE  0x40
#define USART_BIT_TXEIE  0x80
#define USART_BIT_PEIE   0x100

#define USART_BIT_LBDIE  0x40

#define USART_BIT_CTSIE  0x400
#define USART_BIT_EIE    0x1

#define UART_Mode_Rx      (0x4)
#define UART_Mode_Tx      (0x8)

#define UART_FlowControl_None         (0<<8)
#define UART_FlowControl_RTS          (1<<8)
#define UART_FlowControl_CTS          (2<<8)
#define UART_FlowControl_RTS_CTS      (3<<8)

#define UART_Word_8b                  (0<<12)
#define UART_Word_9b                  (1<<12)

#define USART_Clock_Disable           (0<<11)
#define USART_Clock_Enable            (1<<11)

#define USART_CPOL_Low                (1<<10)
#define USART_CPOL_High               (1<<10)

#define USART_CPHA_1Edge              (0<<9)
#define USART_CPHA_2Edge              (1<<9)

#define USART_BIT_CTS                 (1<<9)
#define USART_BIT_LBD                 (1<<8)
#define USART_BIT_TXE                 (1<<7)
#define USART_BIT_TC                  (1<<6)
#define USART_BIT_RXNE                (1<<5)
#define USART_BIT_IDLE                (1<<4)
#define USART_BIT_ORE                 (1<<3)
#define USART_BIT_NE                  (1<<2)
#define USART_BIT_FE                  (1<<1)
#define USART_BIT_PE                  (1<<0)

#define USART_LastBit_Disable         (0x0000)
#define USART_LastBit_Enable          (0x0100)

/**
 * @brief Initialize a serial port.
 * @param dev         Serial port to be initialized
 */
void usart_init(const usart_dev *dev);

/**
 * @brief Configure a serial port's baud rate.
 *
 * @param dev        	Serial port to be configured
 * @param baudRate      Baud rate for transmit/receive.
 * @param wordLength  			Specifies the number of data bits transmitted or received in a frame. This parameter can be a value of USART_Word_Length 
 * @param stopBits				Specifies the number of stop bits transmitted. This parameter can be a value of USART_Stop_Bits 
 * @param parity				Specifies the parity mode. This parameter can be a value of USART_Parity 
 * @param mode					Specifies wether the Receive or Transmit mode is enabled or disabled. This parameter can be a value of USART_Mode 
 * @param hardwareFlowControl	Specifies wether the hardware flow control mode is enabled or disabled. This parameter can be a value of USART_Hardware_Flow_Control
 */
void usart_setup(const usart_dev *dev, 
				 uint32_t  baudRate, 
				 uint16_t  wordLength, 
				 uint16_t  stopBits, 
				 uint16_t  parity, 
				 uint16_t  mode, 
				 uint16_t  hardwareFlowControl);

/**
 * @brief Enable a serial port.
 *
 * USART is enabled in single buffer transmission mode, multibuffer
 * receiver mode, 8n1.
 *
 * Serial port must have a baud rate configured to work properly.
 *
 * @param dev Serial port to enable.
 * @see usart_set_baud_rate()
 */
static inline void usart_enable(const usart_dev *dev)
{
    dev->state->is_used=true;

    /* Enable USART */    
    dev->regs->CR1 |= USART_CR1_UE; /* Enable the selected USART by setting the UE bit in the CR1 register */

}


static inline uint8_t usart_is_used(const usart_dev *dev)
{
    return dev->state->is_used;
}


/**
 * @brief Turn off a serial port.
 * @param dev Serial port to be disabled
 */
static inline void usart_disable(const usart_dev *dev){
    /* Disable the selected USART by clearing the UE bit in the CR1 register */
    dev->regs->CR1 &= (uint16_t)~(USART_CR1_UE);
    
    /* Clean up buffer */
    dev->state->is_used=false;
}

/**
 *  @brief Call a function on each USART.
 *  @param fn Function to call.
 */
void usart_foreach(void (*fn)(const usart_dev*));


static inline uint32_t usart_txfifo_nbytes(const usart_dev *dev) {
	return rb_full_count(dev->txrb);
}
static inline uint32_t usart_txfifo_freebytes(const usart_dev *dev) {
        uint32_t used = rb_full_count(dev->txrb);
        if(used >= USART_TX_BUF_SIZE/2) return 0; // leave half for dirty writes without check - thanks s_s
	return (USART_TX_BUF_SIZE/2-used);
}
/**
 * @brief Disable all serial ports.
 */
static inline void usart_disable_all(void) {
    usart_foreach(usart_disable);
}

/**
 * @brief Nonblocking USART transmit
 * @param dev Serial port to transmit over
 * @param buf Buffer to transmit
 * @param len Maximum number of bytes to transmit
 * @return Number of bytes transmitted
 */
uint32_t usart_tx(const usart_dev *dev, const uint8_t *buf, uint32_t len);

/**
 * @brief Transmit an unsigned integer to the specified serial port in
 *        decimal format.
 *
 * This function blocks until the integer's digits have been
 * completely transmitted.
 *
 * @param dev Serial port to send on
 * @param val Number to print
 */
void usart_putudec(const usart_dev *dev, uint32_t val);

/**
 * @brief Transmit one character on a serial port.
 *
 * This function blocks until the character has been successfully
 * transmitted.
 *
 * @param dev Serial port to send on.
 * @param byte Byte to transmit.
 */
static inline uint32_t usart_putc(const usart_dev* dev, uint8_t bt) {
    return usart_tx(dev, &bt, 1);
}

/**
 * @brief Transmit a character string on a serial port.
 *
 * This function blocks until str is completely transmitted.
 *
 * @param dev Serial port to send on
 * @param str String to send
    used to inform about exception 

 */


static inline void usart_putstr(const usart_dev *dev, const char* str) {
    uint32_t i = 0;
    uint8_t c;
    while ( (c=str[i++]) )  usart_putc(dev, c);
}

/**
 * @brief Read one character from a serial port.
 *
 * It's not safe to call this function if the serial port has no data
 * available.
 *
 * @param dev Serial port to read from
 * @return byte read
 * @see usart_data_available()
 */
static inline uint8_t usart_getc(const usart_dev *dev) {
    return rb_remove(dev->rxrb);
}

/**
 * @brief Return the amount of data available in a serial port's RX buffer.
 * @param dev Serial port to check
 * @return Number of bytes in dev's RX buffer.
 */
static inline uint32_t usart_data_available(const usart_dev *dev) {
    return rb_full_count(dev->rxrb);
}


/**
 * @brief Discard the contents of a serial port's RX buffer.
 * @param dev Serial port whose buffer to empty.
 */
static inline void usart_reset_rx(const usart_dev *dev) {
    rb_reset(dev->rxrb);
}

static inline void usart_reset_tx(const usart_dev *dev) {
    rb_reset(dev->txrb);
    dev->state->txbusy = 0;
}

static inline void usart_set_callback(const usart_dev *dev, Handler cb)
{
    dev->state->callback = cb;
}


void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
void USART3_IRQHandler(void);
void USART4_IRQHandler(void);
void USART5_IRQHandler(void);
void USART6_IRQHandler(void);

void UART4_IRQHandler(void);
void UART5_IRQHandler(void);


#ifdef __cplusplus
  }
#endif

#endif
