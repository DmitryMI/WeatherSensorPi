/*
 * nrf24l01.c
 *
 *  Created on: 1 авг. 2019 г.
 *      Author: dima
 */
#include "RF24.h"

#include "../Delay/delay.h"

#define rf24_max(a,b) (a>b?a:b)
#define rf24_min(a,b) (a<b?a:b)

/* Memory Map */
#define NRF_CONFIG      0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define NRF_STATUS  0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0A
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
#define FIFO_STATUS 0x17
#define DYNPD	    0x1C
#define FEATURE	    0x1D

/* Bit Mnemonics */
#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0
#define ENAA_P5     5
#define ENAA_P4     4
#define ENAA_P3     3
#define ENAA_P2     2
#define ENAA_P1     1
#define ENAA_P0     0
#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0
#define AW          0
#define ARD         4
#define ARC         0
#define PLL_LOCK    4
#define RF_DR       3
#define RF_PWR      6
#define RX_DR       6
#define TX_DS       5
#define MAX_RT      4
#define RX_P_NO     1
#define TX_FULL     0
#define PLOS_CNT    4
#define ARC_CNT     0
#define TX_REUSE    6
#define FIFO_FULL   5
#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0
#define DPL_P5	    5
#define DPL_P4	    4
#define DPL_P3	    3
#define DPL_P2	    2
#define DPL_P1	    1
#define DPL_P0	    0
#define EN_DPL	    2
#define EN_ACK_PAY  1
#define EN_DYN_ACK  0

/* Instruction Mnemonics */
#define R_REGISTER    0x00
#define W_REGISTER    0x20
#define REGISTER_MASK 0x1F
#define ACTIVATE      0x50
#define R_RX_PL_WID   0x60
#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD  0xA0
#define W_ACK_PAYLOAD 0xA8
#define FLUSH_TX      0xE1
#define FLUSH_RX      0xE2
#define REUSE_TX_PL   0xE3
#define NOP           0xFF

/* Non-P omissions */
#define LNA_HCURR   0

/* P model memory Map */
#define RPD         0x09
#define W_TX_PAYLOAD_NO_ACK  0xB0

/* P model bit Mnemonics */
#define RF_DR_LOW   5
#define RF_DR_HIGH  3
#define RF_PWR_LOW  1
#define RF_PWR_HIGH 2

#define HIGH 1
#define LOW  0

static SPI_HandleTypeDef *hspi;

static GPIO_TypeDef* rf_ce_port;
static uint8_t rf_ce_pin;

static GPIO_TypeDef* rf_csn_port;
static uint8_t rf_csn_pin;

bool p_variant; /** False for RF24L01 and true for RF24L01P */
uint8_t payload_size = 0; /**< Fixed size of payloads */
bool dynamic_payloads_enabled; /**< Whether dynamic payloads are enabled. */
uint8_t pipe0_reading_address[5] = {0,}; /**< Last address set on pipe 0 for reading. */
uint8_t addr_width = 0; /**< The address width to use - 3,4 or 5 bytes. */
uint8_t txDelay = 0;


static void nrf_delay_us(uint32_t us) // DelayMicro
{
    delay_us(us);
}

void nrf_csn(uint8_t level)
{
	HAL_GPIO_WritePin(rf_csn_port, rf_csn_pin, level);
	nrf_delay_us(5);
}

void nrf_ce(uint8_t level)
{
	HAL_GPIO_WritePin(rf_ce_port, rf_ce_pin, level);
}

uint8_t nrf_read_register(uint8_t reg)
{
	uint8_t addr = R_REGISTER | (REGISTER_MASK & reg);
	uint8_t dt = 0;

	nrf_csn(LOW);
	HAL_SPI_TransmitReceive(hspi, &addr, &dt, 1, 1000);
	HAL_SPI_TransmitReceive(hspi, (uint8_t*)0xff, &dt, 1, 1000);
	nrf_csn(HIGH);
	return dt;
}

uint8_t nrf_write_registerMy(uint8_t reg, const uint8_t* buf, uint8_t len)
{
	uint8_t status = 0;
	uint8_t addr = W_REGISTER | (REGISTER_MASK & reg);

	nrf_csn(LOW);
	HAL_SPI_TransmitReceive(hspi, &addr, &status, 1, 100);
	HAL_SPI_Transmit(hspi, (uint8_t*)buf, len, 100);
	nrf_csn(HIGH);
	return status;
}

uint8_t nrf_write_register(uint8_t reg, uint8_t value)
{
	uint8_t status = 0;
	uint8_t addr = W_REGISTER | (REGISTER_MASK & reg);
	nrf_csn(LOW);
	HAL_SPI_TransmitReceive(hspi, &addr, &status, 1, 100);
	HAL_SPI_Transmit(hspi, &value, 1, 1000);
	nrf_csn(HIGH);
	return status;
}

uint8_t nrf_write_payload(const void* buf, uint8_t data_len, const uint8_t writeType)
{
	uint8_t status = 0;
	const uint8_t* current = (const uint8_t*)buf;
	uint8_t addr = writeType;

	data_len = rf24_min(data_len, payload_size);
	uint8_t blank_len = dynamic_payloads_enabled ? 0 : payload_size - data_len;

	nrf_csn(LOW);
	HAL_SPI_TransmitReceive(hspi, &addr, &status, 1, 100);
	HAL_SPI_Transmit(hspi, (uint8_t*)current, data_len, 100);

	while(blank_len--)
	{
		uint8_t empt = 0;
		HAL_SPI_Transmit(hspi, &empt, 1, 100);
	}

	nrf_csn(HIGH);
	return status;
}

uint8_t nrf_read_payload(void* buf, uint8_t data_len)
{
	uint8_t status = 0;
	uint8_t* current = (uint8_t*)buf;

	if(data_len > payload_size)
	{
		data_len = payload_size;
	}

	uint8_t blank_len = dynamic_payloads_enabled ? 0 : payload_size - data_len;

	uint8_t addr = R_RX_PAYLOAD;
	nrf_csn(LOW);
	HAL_SPI_Transmit(hspi, &addr, 1, 100);
	HAL_SPI_Receive(hspi, (uint8_t*)current, data_len, 100);

	while(blank_len--)
	{
		uint8_t empt = 0;
		HAL_SPI_Receive(hspi, &empt, 1, 100);
	}

	nrf_csn(HIGH);
	return status;
}

uint8_t nrf_flush_rx(void)
{
	return nrf_spiTrans(FLUSH_RX);
}

uint8_t nrf_flush_tx(void)
{
	return nrf_spiTrans(FLUSH_TX);
}

uint8_t nrf_spiTrans(uint8_t cmd)
{
	uint8_t status = 0;
	nrf_csn(LOW);
	HAL_SPI_TransmitReceive(hspi, &cmd, &status, 1, 1000);
	nrf_csn(HIGH);
	return status;
}

uint8_t nrf_get_status(void)
{
	return nrf_spiTrans(NOP);
}

void nrf_setChannel(uint8_t channel)
{
	nrf_write_register(RF_CH, channel);
}

uint8_t nrf_getChannel()
{
	return nrf_read_register(RF_CH);
}

void nrf_setPayloadSize(uint8_t size)
{
	payload_size = rf24_min(size, 32);
}

uint8_t nrf_getPayloadSize(void)
{
	return payload_size;
}

uint8_t nrf_init(SPI_HandleTypeDef* spi, GPIO_TypeDef* cs_port, uint16_t cs_pin, GPIO_TypeDef* ce_port, uint16_t ce_pin)
{
	hspi = spi;
	rf_csn_port = cs_port;
	rf_csn_pin = cs_pin;
	rf_ce_port = ce_port;
	rf_ce_pin = ce_pin;

	uint8_t setup = 0;
	p_variant = false;
	payload_size = 32;
	dynamic_payloads_enabled = false;
	addr_width = 5;
	pipe0_reading_address[0] = 0;

	nrf_ce(LOW);
	nrf_csn(HIGH);
	HAL_Delay(5);

	nrf_write_register(NRF_CONFIG, 0x0C); // Reset NRF_CONFIG and enable 16-bit CRC.
	nrf_setRetries(5, 15);
	nrf_setPALevel(RF24_PA_MAX); // Reset value is MAX

	if(nrf_setDataRate(RF24_250KBPS)) // check for connected module and if this is a p nRF24l01 variant
	{
		p_variant = true;
	}

	setup = nrf_read_register(RF_SETUP);
	nrf_setDataRate(RF24_1MBPS); // Then set the data rate to the slowest (and most reliable) speed supported by all hardware.

	// Disable dynamic payloads, to match dynamic_payloads_enabled setting - Reset value is 0
	nrf_toggle_features();
	nrf_write_register(FEATURE, 0);
	nrf_write_register(DYNPD, 0);
	dynamic_payloads_enabled = false;

	// Reset current status. Notice reset and flush is the last thing we do
	nrf_write_register(NRF_STATUS, (1 << RX_DR) | (1 << TX_DS) | (1 << MAX_RT));
	nrf_setChannel(76);
	nrf_flush_rx();
	nrf_flush_tx();
	nrf_powerUp(); //Power up by default when begin() is called
	nrf_write_register(NRF_CONFIG, (nrf_read_register(NRF_CONFIG)) & ~(1 << PRIM_RX));
	return (setup != 0 && setup != 0xff);
}

bool nrf_isChipConnected()
{
	uint8_t setup = nrf_read_register(SETUP_AW);

	if(setup >= 1 && setup <= 3)
	{
		return true;
	}

	return false;
}

void nrf_startListening(void)
{
	nrf_powerUp();

	nrf_write_register(NRF_CONFIG, nrf_read_register(NRF_CONFIG) | (1 << PRIM_RX));
	nrf_write_register(NRF_STATUS, (1 << RX_DR) | (1 << TX_DS) | (1 << MAX_RT));
	nrf_ce(HIGH);
	// Restore the pipe0 adddress, if exists
	if(pipe0_reading_address[0] > 0)
	{
		nrf_write_registerMy(RX_ADDR_P0, pipe0_reading_address, addr_width);
	}
	else
	{
		nrf_closeReadingPipe(0);
	}

	if(nrf_read_register(FEATURE) & (1 << EN_ACK_PAY))
	{
		nrf_flush_tx();
	}
}


static const uint8_t child_pipe_enable[] = {ERX_P0, ERX_P1, ERX_P2, ERX_P3, ERX_P4, ERX_P5};

void nrf_stopListening(void)
{
	nrf_ce(LOW);
	nrf_delay_us(txDelay);

	if(nrf_read_register(FEATURE) & (1 << EN_ACK_PAY))
	{
		nrf_delay_us(txDelay); //200
		nrf_flush_tx();
	}

	nrf_write_register(NRF_CONFIG, (nrf_read_register(NRF_CONFIG)) & ~(1 << PRIM_RX));
	nrf_write_register(EN_RXADDR, nrf_read_register(EN_RXADDR) | (1 << child_pipe_enable[0])); // Enable RX on pipe0
}

void nrf_powerDown(void)
{
	nrf_ce(LOW); // Guarantee nrf_ce is low on powerDown

	//nrf_write_register(NRF_CONFIG, nrf_read_register(NRF_CONFIG) & ~(1 << PWR_UP));
	uint8_t cfg = nrf_read_register(NRF_CONFIG);
		// if not powered up then power up and wait for the radio to initialize
	if(cfg & (1 << PWR_UP))
	{
		nrf_write_register(NRF_CONFIG, cfg & ~(1 << PWR_UP));
		HAL_Delay(5);
		cfg = nrf_read_register(NRF_CONFIG);
	}
}

//Power up now. Radio will not power down unless instructed by MCU for config changes etc.
void nrf_powerUp(void)
{
	uint8_t cfg = nrf_read_register(NRF_CONFIG);
	// if not powered up then power up and wait for the radio to initialize
	if(!(cfg & (1 << PWR_UP)))
	{
		nrf_write_register(NRF_CONFIG, cfg | (1 << PWR_UP));
		HAL_Delay(5);
	}
}


//Similar to the previous write, clears the interrupt flags
bool nrf_write(const void* buf, uint8_t len)
{
	nrf_startFastWrite(buf, len, 1, 1);

	uint8_t status;

	/*
	while(1)
	{
		status = nrf_get_status();
		int data_transmitted = status & (1 << TX_DS);
		int max_retries_reached = status & (1 << MAX_RT);
		if(data_transmitted || max_retries_reached)
		{
			break;
		}
	}
	*/

	while(!(nrf_get_status() & ((1 << TX_DS) | (1 << MAX_RT))))
	{}

	nrf_ce(LOW);
	status = nrf_write_register(NRF_STATUS, (1 << RX_DR) | (1 << TX_DS) | (1 << MAX_RT));

	if(status & (1 << MAX_RT))
	{
		nrf_flush_tx(); //Only going to be 1 packet int the FIFO at a time using this method, so just flush
		return 0;
	}

	//TX OK 1 or 0
	return 1;
}

void nrf_startFastWrite(const void* buf, uint8_t len, const bool multicast, bool startTx)
{
	nrf_write_payload(buf, len, multicast ? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD);

	if(startTx)
	{
		nrf_ce(HIGH);
	}
}

void nrf_maskIRQ(bool tx, bool fail, bool rx)
{
	uint8_t config = nrf_read_register(NRF_CONFIG);
	config &= ~(1 << MASK_MAX_RT | 1 << MASK_TX_DS | 1 << MASK_RX_DR); //clear the interrupt flags
	config |= fail << MASK_MAX_RT | tx << MASK_TX_DS | rx << MASK_RX_DR; // set the specified interrupt flags
	nrf_write_register(NRF_CONFIG, config);
}

uint8_t nrf_getDynamicPayloadSize(void)
{
	uint8_t result = 0, addr;
	nrf_csn(LOW);
	addr = R_RX_PL_WID;
	HAL_SPI_TransmitReceive(hspi, &addr, &result, 1, 1000);
	HAL_SPI_TransmitReceive(hspi, (uint8_t*)0xff, &result, 1, 1000);
	nrf_csn(HIGH);

	if(result > 32)
	{
		nrf_flush_rx();
		HAL_Delay(2);
		return 0;
	}

	return result;
}

bool nrf_availableMy(void)
{
	return nrf_available(NULL);
}

bool nrf_available(uint8_t* pipe_num)
{
	if(!(nrf_read_register(FIFO_STATUS) & (1 << RX_EMPTY)))
	{
		if(pipe_num) // If the caller wants the pipe number, include that
		{
			uint8_t status = nrf_get_status();
			*pipe_num = (status >> RX_P_NO) & 0x07;
		}

		return 1;
	}

	return 0;
}

void nrf_read(void* buf, uint8_t len)
{
	nrf_read_payload(buf, len);
	nrf_write_register(NRF_STATUS, (1 << RX_DR) | (1 << MAX_RT) | (1 << TX_DS));
}


uint8_t nrf_whatHappened()
{
	uint8_t status = nrf_write_register(NRF_STATUS, (1 << RX_DR) | (1 << TX_DS) | (1 << MAX_RT));
	/*uint8_t tx_ok = status & (1 << TX_DS);
	uint8_t tx_fail = status & (1 << MAX_RT);
	uint8_t rx_ready = status & (1 << RX_DR);*/
	return status;
}

void nrf_openWritingPipe(uint64_t value)
{
	nrf_write_registerMy(RX_ADDR_P0, (uint8_t*)&value, addr_width);
	nrf_write_registerMy(TX_ADDR, (uint8_t*)&value, addr_width);
	nrf_write_register(RX_PW_P0, payload_size);
}


static const uint8_t child_pipe[] = {RX_ADDR_P0, RX_ADDR_P1, RX_ADDR_P2, RX_ADDR_P3, RX_ADDR_P4, RX_ADDR_P5};

static const uint8_t child_payload_size[] = {RX_PW_P0, RX_PW_P1, RX_PW_P2, RX_PW_P3, RX_PW_P4, RX_PW_P5};


void nrf_openReadingPipe(uint8_t child, uint64_t address)
{
	if(child == 0)
	{
		memcpy(pipe0_reading_address, &address, addr_width);
	}

	if(child <= 6)
	{
		// For pipes 2-5, only write the LSB
		if(child < 2)
			nrf_write_registerMy(child_pipe[child], (const uint8_t*)&address, addr_width);
		else
			nrf_write_registerMy(child_pipe[child], (const uint8_t*)&address, 1);

		nrf_write_register(child_payload_size[child], payload_size);
		nrf_write_register(EN_RXADDR, nrf_read_register(EN_RXADDR) | (1 << child_pipe_enable[child]));
	}
}

void nrf_setAddressWidth(uint8_t a_width)
{
	if(a_width -= 2)
	{
		nrf_write_register(SETUP_AW, a_width%4);
		addr_width = (a_width%4) + 2;
	}
	else
	{
        nrf_write_register(SETUP_AW, 0);
        addr_width = 2;
    }
}

void nrf_closeReadingPipe(uint8_t pipe)
{
	nrf_write_register(EN_RXADDR, nrf_read_register(EN_RXADDR) & ~(1 << child_pipe_enable[pipe]));
}

void nrf_toggle_features(void)
{
	uint8_t addr = ACTIVATE;
	nrf_csn(LOW);
	HAL_SPI_Transmit(hspi, &addr, 1, 1000);
	HAL_SPI_Transmit(hspi, (uint8_t*)0x73, 1, 1000);
	nrf_csn(HIGH);
}

void nrf_enableDynamicPayloads(void)
{
	nrf_write_register(FEATURE, nrf_read_register(FEATURE) | (1 << EN_DPL));
	nrf_write_register(DYNPD, nrf_read_register(DYNPD) | (1 << DPL_P5) | (1 << DPL_P4) | (1 << DPL_P3) | (1 << DPL_P2) | (1 << DPL_P1) | (1 << DPL_P0));
	dynamic_payloads_enabled = true;
}

void nrf_disableDynamicPayloads(void)
{
	nrf_write_register(FEATURE, 0);
	nrf_write_register(DYNPD, 0);
	dynamic_payloads_enabled = false;
}

void nrf_enableAckPayload(void)
{
	nrf_write_register(FEATURE, nrf_read_register(FEATURE) | (1 << EN_ACK_PAY) | (1 << EN_DPL));
	nrf_write_register(DYNPD, nrf_read_register(DYNPD) | (1 << DPL_P1) | (1 << DPL_P0));
	dynamic_payloads_enabled = true;
}

void nrf_enableDynamicAck(void)
{
    nrf_write_register(FEATURE, nrf_read_register(FEATURE) | (1 << EN_DYN_ACK));
}

void nrf_writeAckPayload(uint8_t pipe, const void* buf, uint8_t len)
{
	const uint8_t* current = (const uint8_t*)buf;
	uint8_t data_len = rf24_min(len, 32);
	uint8_t addr = W_ACK_PAYLOAD | (pipe & 0x07);
	nrf_csn(LOW);
	HAL_SPI_Transmit(hspi, &addr, 1, 1000);
	HAL_SPI_Transmit(hspi, (uint8_t*)current, data_len, 1000);
	nrf_csn(HIGH);
}

bool nrf_isAckPayloadAvailable(void)
{
	return !(nrf_read_register(FIFO_STATUS) & (1 << RX_EMPTY));
}

bool nrf_isPVariant(void)
{
	return p_variant;
}

void nrf_setAutoAck(bool enable)
{
	if(enable)
		nrf_write_register(EN_AA, 0x3F);
	else
		nrf_write_register(EN_AA, 0);
}

void nrf_setAutoAckPipe(uint8_t pipe, bool enable)
{
	if(pipe <= 6)
	{
		uint8_t en_aa = nrf_read_register(EN_AA);

		if(enable)
		{
			en_aa |= (1 << pipe);
		}
		else
		{
			en_aa &= ~(1 << pipe);
		}

		nrf_write_register(EN_AA, en_aa);
	}
}

void nrf_setPALevel(uint8_t level)
{
  uint8_t setup = nrf_read_register(RF_SETUP) & 0xF8;

  if(level > 3) // If invalid level, go to max PA
  {
	  level = (RF24_PA_MAX << 1) + 1;		// +1 to support the SI24R1 chip extra bit
  }
  else
  {
	  level = (level << 1) + 1;	 		// Else set level as requested
  }

  nrf_write_register(RF_SETUP, setup |= level);	// Write it to the chip
}

uint8_t nrf_getPALevel(void)
{
	return (nrf_read_register(RF_SETUP) & ((1 << RF_PWR_LOW) | (1 << RF_PWR_HIGH))) >> 1;
}

bool nrf_setDataRate(rf24_datarate_e speed)
{
	bool result = false;
	uint8_t setup = nrf_read_register(RF_SETUP);
	setup &= ~((1 << RF_DR_LOW) | (1 << RF_DR_HIGH));
	txDelay = 85;

	if(speed == RF24_250KBPS)
	{
		setup |= (1 << RF_DR_LOW);
		txDelay = 155;
	}
	else
	{
		if(speed == RF24_2MBPS)
		{
			setup |= (1 << RF_DR_HIGH);
			txDelay = 65;
		}
	}

	nrf_write_register(RF_SETUP, setup);
	uint8_t ggg = nrf_read_register(RF_SETUP);

	if(ggg == setup)
	{
		result = true;
	}

	return result;
}

rf24_datarate_e nrf_getDataRate(void)
{
	rf24_datarate_e result ;
	uint8_t dr = nrf_read_register(RF_SETUP) & ((1 << RF_DR_LOW) | (1 << RF_DR_HIGH));

	// switch uses RAM (evil!)
	// Order matters in our case below
	if(dr == (1 << RF_DR_LOW))
	{
		result = RF24_250KBPS;
	}
	else if(dr == (1 << RF_DR_HIGH))
	{
		result = RF24_2MBPS;
	}
	else
	{
		result = RF24_1MBPS;
	}

	return result;
}

void nrf_setCRCLength(rf24_crclength_e length)
{
	uint8_t config = nrf_read_register(NRF_CONFIG) & ~((1 << CRCO) | (1 << EN_CRC));

	if(length == RF24_CRC_DISABLED)
	{
		// Do nothing, we turned it off above.
	}
	else if(length == RF24_CRC_8)
	{
		config |= (1 << EN_CRC);
	}
	else
	{
		config |= (1 << EN_CRC);
		config |= (1 << CRCO);
	}

	nrf_write_register(NRF_CONFIG, config);
}

rf24_crclength_e nrf_getCRCLength(void)
{
	rf24_crclength_e result = RF24_CRC_DISABLED;

	uint8_t config = nrf_read_register(NRF_CONFIG) & ((1 << CRCO) | (1 << EN_CRC));
	uint8_t AA = nrf_read_register(EN_AA);

	if(config & (1 << EN_CRC) || AA)
	{
		if(config & (1 << CRCO))
		  result = RF24_CRC_16;
		else
		  result = RF24_CRC_8;
	}

	return result;
}

void nrf_disableCRC(void)
{
	uint8_t disable = nrf_read_register(NRF_CONFIG) & ~(1 << EN_CRC);
	nrf_write_register(NRF_CONFIG, disable);
}

void nrf_setRetries(uint8_t delay, uint8_t count)
{
	nrf_write_register(SETUP_RETR, (delay&0xf)<<ARD | (count&0xf)<<ARC);
}

void nrf_print_details(send_text_t sender)
{

	char str[64] = {0};
	uint8_t status = nrf_get_status();
	snprintf(str, 64, "get_status: 0x%02x\n", status);
	sender(str);

	status = nrf_getPALevel();
	snprintf(str, 64, "getPALevel: 0x%02x  ", status);
	sender(str);

	if(status == 0x00)
	{
		sender("RF24_PA_MIN\n");
	}
	else if(status == 0x01)
	{
		sender("RF24_PA_LOW\n");
	}
	else if(status == 0x02)
	{
		sender("RF24_PA_HIGH\n");
	}
	else if(status == 0x03)
	{
		sender("RF24_PA_MAX\n");
	}

	status = nrf_getChannel();
	snprintf(str, 64, "getChannel: 0x%02x № %d\n", status, status);
	sender(str);

	status = nrf_getDataRate();
	snprintf(str, 64, "getDataRate: 0x%02x", status);
	 sender(str);

	if(status == 0x02)
	{
		sender("RF24_250KBPS\n");
	}
	else if(status == 0x01)
	{
		sender("RF24_2MBPS\n");
	}
	else
	{
		sender("RF24_1MBPS\n");
	}

	status = nrf_getPayloadSize();
	snprintf(str, 64, "getPayloadSize: %d\n", status);
	sender(str);

	status = nrf_getCRCLength();
	snprintf(str, 64, "getCRCLength: 0x%02x", status);
	sender(str);

	if(status == 0x00)
	{
		sender("RF24_CRC_DISABLED\n");
	}
	else if(status == 0x01)
	{
		sender("RF24_CRC_8\n");
	}
	else if(status == 0x02)
	{
		sender("RF24_CRC_16\n");
	}
}



