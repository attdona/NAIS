/*
 * Copyright (C) 2015 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     cpu_cc3200
 * @{
 *
 * @file
 * @brief       Low-level SPI driver implementation
 *
 * @author      Attilio Dona' <@attiliodona>
 *
 * @}
 */

#include "cpu.h"
#include "mutex.h"
#include "periph/spi.h"
#include "periph_conf.h"

/* guard this file in case no SPI device is defined */
#if SPI_NUMOF

static unsigned long bitrate[] = {
  [SPI_CLK_100KHZ] = 100000,
  [SPI_CLK_400KHZ] = 400000,
  [SPI_CLK_1MHZ] = 1000000,
  [SPI_CLK_5MHZ] = 5000000,
  [SPI_CLK_10MHZ] = 10000000
};

#if 0
/**
 * @brief   array holding one pre-initialized mutex for each SPI device
 */
static mutex_t locks[] =  {
#if SPI_0_EN
    [SPI_0] = MUTEX_INIT,
#endif
#if SPI_1_EN
    [SPI_1] = MUTEX_INIT,
#endif
};
#endif

static mutex_t lock = MUTEX_INIT;

void spi_init(spi_t bus)
{
	// cc3200 has only one SPI for external use
    assert(bus < SPI_NUMOF);

    MAP_PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);

    //
    // Configure PIN_05 for SPI0 GSPI_CLK
    //
    MAP_PinTypeSPI(digital_pin_to_pin_num[SPI_0_PIN_SCK], PIN_MODE_7);

    //
    // Configure PIN_06 for SPI0 GSPI_MISO
    //
    MAP_PinTypeSPI(digital_pin_to_pin_num[SPI_0_PIN_MISO], PIN_MODE_7);

    //
    // Configure PIN_07 for SPI0 GSPI_MOSI
    //
    MAP_PinTypeSPI(digital_pin_to_pin_num[SPI_0_PIN_MOSI], PIN_MODE_7);

    //
    // Configure PIN_08 for SPI0 GSPI_CS
    //
    //MAP_PinTypeSPI(digital_pin_to_pin_num[SPI_0_PIN_CS], PIN_MODE_7);

    //
    // Reset SPI
    //
    MAP_SPIReset(GSPI_BASE);

    //
    // Configure SPI interface
    //
    // see:
    //  e2e.ti.com/support/wireless_connectivity/f/968/p/359727/1265934#1265934
    //
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
            bitrate[SPI_0_SPEED], SPI_MODE_MASTER, SPI_0_MODE,
                     (SPI_HW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVELOW |
                     SPI_WL_8));

    //
    // Enable SPI for communication
    //
    MAP_SPIEnable(GSPI_BASE);

    /* configure SPI mode */


}

int spi_conf_pins(spi_t dev)
{
    if (dev >= SPI_NUMOF) {
        return -1;
    }

    //
    // Configure PIN_06 for SPI0 GSPI_MISO
    //
    MAP_PinTypeSPI(digital_pin_to_pin_num[SPI_0_PIN_MISO], PIN_MODE_7);

    return 0;
}


int spi_acquire(spi_t bus, spi_cs_t cs, spi_mode_t mode, spi_clk_t clk)
{
    assert(bus < SPI_NUMOF);
    mutex_lock(&lock);
    return SPI_OK;
}

void spi_release(spi_t bus)
{
    assert(bus < SPI_NUMOF);
    mutex_unlock(&lock);
}

uint8_t spi_transfer_byte(spi_t bus, spi_cs_t cs, bool cont, uint8_t out)
{
    uint8_t in;
    spi_transfer_bytes(bus, cs, false, &out, &in, 1);
    return in;
}

void spi_transfer_bytes(spi_t bus, spi_cs_t cs, bool cont,
        const void *out, void *in, size_t len)
{
    assert(bus < SPI_NUMOF);

    MAP_SPITransfer(GSPI_BASE, (unsigned char*) out, (unsigned char*) in, len,
            0);

}

uint8_t spi_transfer_reg(spi_t bus, spi_cs_t cs, uint8_t reg, uint8_t out)
{
    uint8_t in;
    assert(bus < SPI_NUMOF);

    MAP_SPITransfer(GSPI_BASE, &reg, 0, 1, 0);

    if(MAP_SPITransfer(GSPI_BASE, (unsigned char*) &out, (unsigned char*) &in,
            1, 0)) {
        assert(0);
    }
    return in; // success transfer
}

void spi_transfer_regs(spi_t bus, spi_cs_t cs, uint8_t reg,
        const void *out, void *in, size_t len)
{
    assert(bus < SPI_NUMOF);

    MAP_SPITransfer(GSPI_BASE, &reg, 0, 1, 0);
    if(MAP_SPITransfer(GSPI_BASE, (unsigned char*) out, (unsigned char*) in,
            len, 0)) {
        assert(0);
    }
}

void spi_transmission_begin(spi_t dev, char reset_val)
{
    /* spi slave is not implemented */
}


#endif /* SPI_NUMOF */
