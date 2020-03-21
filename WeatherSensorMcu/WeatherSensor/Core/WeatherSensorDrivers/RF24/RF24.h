/*
 * nrf24l01.h
 *
 *  Created on: 1 авг. 2019 г.
 *      Author: dima
 */

#ifndef NRF24L01_H_
#define NRF24L01_H_

#include "main.h"
#include "string.h"
#include "stdio.h"
#include "stdbool.h"


///////////////////////////////////////////////////////////////////////////////

typedef void (send_text_t(char *text));

typedef enum
{
	RF24_PA_MIN = 0,
	RF24_PA_LOW,
	RF24_PA_HIGH,
	RF24_PA_MAX,
	RF24_PA_ERROR
} rf24_pa_dbm_e;

typedef enum
{
	RF24_1MBPS = 0,
	RF24_2MBPS,
	RF24_250KBPS
} rf24_datarate_e;

typedef enum
{
	RF24_CRC_DISABLED = 0,
	RF24_CRC_8,
	RF24_CRC_16
} rf24_crclength_e;


uint8_t nrf_init(SPI_HandleTypeDef* spi, GPIO_TypeDef* cs_port, uint16_t cs_pin, GPIO_TypeDef* ce_port, uint16_t ce_pin);
bool nrf_isChipConnected();
void nrf_startListening(void);
void nrf_stopListening(void);
bool nrf_availableMy(void);
void nrf_read(void* buf, uint8_t len);
bool nrf_write(const void* buf, uint8_t len);
bool nrf_available(uint8_t* pipe_num);
uint8_t nrf_spiTrans(uint8_t cmd);
void nrf_powerDown(void);
void nrf_powerUp(void);
void nrf_writeAckPayload(uint8_t pipe, const void* buf, uint8_t len);
bool nrf_isAckPayloadAvailable(void);
uint8_t nrf_whatHappened();
void nrf_startFastWrite(const void* buf, uint8_t len, const bool multicast, bool startTx);
uint8_t nrf_flush_tx(void);
void nrf_closeReadingPipe(uint8_t pipe);
void nrf_setAddressWidth(uint8_t a_width);
void nrf_setRetries(uint8_t delay, uint8_t count);
void nrf_setChannel(uint8_t channel);
uint8_t nrf_getChannel(void);
void nrf_setPayloadSize(uint8_t size);
uint8_t nrf_getPayloadSize(void);
uint8_t nrf_getDynamicPayloadSize(void);
void nrf_enableAckPayload(void);
void nrf_enableDynamicPayloads(void);
void nrf_disableDynamicPayloads(void);
void nrf_enableDynamicAck();
bool nrf_isPVariant(void);
void nrf_setAutoAck(bool enable);
void nrf_setAutoAckPipe(uint8_t pipe, bool enable);
void nrf_setPALevel(uint8_t level);
uint8_t nrf_getPALevel(void);
bool nrf_setDataRate(rf24_datarate_e speed);
rf24_datarate_e nrf_getDataRate(void);
void nrf_setCRCLength(rf24_crclength_e length);
rf24_crclength_e nrf_getCRCLength(void);
void nrf_disableCRC(void);
void nrf_maskIRQ(bool tx_ok,bool tx_fail,bool rx_ready);
void nrf_openReadingPipe(uint8_t number, uint64_t address);
void nrf_openWritingPipe(uint64_t address);
uint8_t nrf_flush_rx(void);
void nrf_csn(uint8_t mode);
void nrf_ce(uint8_t level);
uint8_t nrf_read_register(uint8_t reg);
uint8_t nrf_write_registerMy(uint8_t reg, const uint8_t* buf, uint8_t len);
uint8_t nrf_write_register(uint8_t reg, uint8_t value);
uint8_t nrf_write_payload(const void* buf, uint8_t len, const uint8_t writeType);
uint8_t nrf_read_payload(void* buf, uint8_t len);
uint8_t nrf_get_status(void);
void nrf_toggle_features(void);

/////////////////////////////////////////////////////////////////////////////
void nrf_print_details(send_text_t sender);


#endif /* NRF24L01_H_ */
