#include "mcp3008.h"

#include <avrtos/avrtos.h>
#include <avrtos/drivers/gpio.h>
#include <avrtos/drivers/spi.h>
#include <avrtos/logging.h>

#define LOG_LEVEL CONFIG_MCP3008_LOG_LEVEL

// MCP3008 CS in PB0
#define MCP3008_CS_DEVICE GPIOB_DEVICE
#define MCP3008_CS_PIN    PINn0

#define MCP3008_SGL_DIFF MCP3008_SINGLE_ENDED

static struct spi_regs mcp3008_spi_regs;

void mcp3008_init(void)
{
    const struct spi_config config = {
        .mode        = SPI_MODE_MASTER,
        .polarity    = SPI_CLOCK_POLARITY_RISING,
        .phase       = SPI_CLOCK_PHASE_SAMPLE,
        .prescaler   = SPI_PRESCALER_X32,
        .irq_enabled = 0u,
    };
    mcp3008_spi_regs = spi_config_into_regs(config);

    gpiol_pin_init(MCP3008_CS_DEVICE, MCP3008_CS_PIN, GPIO_OUTPUT, GPIO_HIGH);
}

uint16_t mcp3008_read(uint8_t channel)
{
    gpiol_pin_write_state(MCP3008_CS_DEVICE, MCP3008_CS_PIN, GPIO_LOW);

    spi_transceive(0x01); /* 7 leading zeros + start bit */

    const uint8_t msb = spi_transceive((MCP3008_SGL_DIFF << 7u) | (channel << 4u));
    const uint8_t lsb = spi_transceive(0x0);

    gpiol_pin_write_state(MCP3008_CS_DEVICE, MCP3008_CS_PIN, GPIO_HIGH);

    return ((msb & 0x7u) << 8u) | lsb;
}

int mcp3008_read_all(uint16_t *values)
{
    /* Arduino SPI driver is used for the MCP2515 CAN controller, so we need to
     * save and restore the SPI registers.
     * Moreover we need to make sure the SPI is not used by another device
     * during this function call, the use of cooperative threads garantees
     * this.
     */

    struct spi_regs saved_regs;
    spi_regs_save(&saved_regs);
    spi_regs_restore(&mcp3008_spi_regs);

    for (uint8_t i = 0u; i < 8u; i++) {
        values[i] = mcp3008_read(i);
    }

    spi_regs_restore(&saved_regs);

    return 0;
}