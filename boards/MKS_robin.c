#include "driver.h"

#if defined(BOARD_MKS_ROBIN_NANO_31) 

#if TRINAMIC_SPI_ENABLE

#include "trinamic/common.h"

#define spi_get_byte() sw_spi_xfer(0)
#define spi_put_byte(d) sw_spi_xfer(d)

static struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} cs[TMC_N_MOTORS_MAX];

// XXXXX replace with something better...
inline static void delay (void)
{
    volatile uint32_t dly = 8;  //10

    while(--dly)
        __ASM volatile ("nop");
}

static uint8_t sw_spi_xfer (uint8_t byte)
{
  uint_fast8_t msk = 0x80, res = 0;

  DIGITAL_OUT(TRINAMIC_SCK_PORT, TRINAMIC_SCK_PIN, 0);

  do {
    DIGITAL_OUT(TRINAMIC_MOSI_PORT, TRINAMIC_MOSI_PIN, (byte & msk) != 0);
    msk >>= 1;
    delay();
    res = (res << 1) | DIGITAL_IN(TRINAMIC_MISO_PORT, TRINAMIC_MISO_PIN);
    DIGITAL_OUT(TRINAMIC_SCK_PORT, TRINAMIC_SCK_PIN, 1);
    delay();
    if (msk)
      DIGITAL_OUT(TRINAMIC_SCK_PORT, TRINAMIC_SCK_PIN, 0);
  } while (msk);

  return (uint8_t)res;
}

TMC_spi_status_t tmc_spi_read (trinamic_motor_t driver, TMC_spi_datagram_t *datagram)
{
  TMC_spi_status_t status;

  DIGITAL_OUT(cs[driver.id].port, cs[driver.id].pin, 0);

  datagram->payload.value = 0;

  datagram->addr.write = 0;
  spi_put_byte(datagram->addr.value);
  spi_put_byte(0);
  spi_put_byte(0);
  spi_put_byte(0);
  spi_put_byte(0);

  DIGITAL_OUT(cs[driver.id].port, cs[driver.id].pin, 1);
  delay();
  DIGITAL_OUT(cs[driver.id].port, cs[driver.id].pin, 0);

  status = spi_put_byte(datagram->addr.value);
  datagram->payload.data[3] = spi_get_byte();
  datagram->payload.data[2] = spi_get_byte();
  datagram->payload.data[1] = spi_get_byte();
  datagram->payload.data[0] = spi_get_byte();

  DIGITAL_OUT(cs[driver.id].port, cs[driver.id].pin, 1);

  return status;
}

TMC_spi_status_t tmc_spi_write (trinamic_motor_t driver, TMC_spi_datagram_t *datagram)
{
  TMC_spi_status_t status;

  DIGITAL_OUT(cs[driver.id].port, cs[driver.id].pin, 0);

  datagram->addr.write = 1;
  status = spi_put_byte(datagram->addr.value);
  spi_put_byte(datagram->payload.data[3]);
  spi_put_byte(datagram->payload.data[2]);
  spi_put_byte(datagram->payload.data[1]);
  spi_put_byte(datagram->payload.data[0]);

  DIGITAL_OUT(cs[driver.id].port, cs[driver.id].pin, 1);

  return status;
}

static void add_cs_pin (xbar_t *gpio, void *data)
{
  if (gpio->group == PinGroup_MotorChipSelect) {
    switch (gpio->function) {

    case Output_MotorChipSelectX:
      cs[X_AXIS].port = (GPIO_TypeDef *)gpio->port;
      cs[X_AXIS].pin = gpio->pin;
      break;

    case Output_MotorChipSelectY:
      cs[Y_AXIS].port = (GPIO_TypeDef *)gpio->port;
      cs[Y_AXIS].pin = gpio->pin;
      break;

    case Output_MotorChipSelectZ:
      cs[Z_AXIS].port = (GPIO_TypeDef *)gpio->port;
      cs[Z_AXIS].pin = gpio->pin;
      break;

    case Output_MotorChipSelectM3:
      cs[3].port = (GPIO_TypeDef *)gpio->port;
      cs[3].pin = gpio->pin;
      break;

    case Output_MotorChipSelectM4:
      cs[4].port = (GPIO_TypeDef *)gpio->port;
      cs[4].pin = gpio->pin;
      break;

    default:
      break;
    }
  }
}

static void if_init(uint8_t motors, axes_signals_t enabled)
{
  static bool init_ok = false;

  UNUSED(motors);

  if (!init_ok) {

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Set all output pins: push-pull, no pull-up, slow
    GPIO_InitStruct.Pin = 1 << TRINAMIC_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TRINAMIC_MOSI_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = 1 << TRINAMIC_SCK_PIN;
    HAL_GPIO_Init(TRINAMIC_SCK_PORT, &GPIO_InitStruct);
    DIGITAL_OUT(TRINAMIC_SCK_PORT, TRINAMIC_SCK_PIN, 1);

    // Set the input pin: input with pull-up
    GPIO_InitStruct.Pin = 1 << TRINAMIC_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(TRINAMIC_MISO_PORT, &GPIO_InitStruct);

    hal.enumerate_pins(true, add_cs_pin, NULL);
  }
}

void board_init (void)
{
  static trinamic_driver_if_t driver_if = {.on_drivers_init = if_init};
  trinamic_if_init(&driver_if);
}

#endif  //TRINAMIC_SPI_ENABLE

#if TRINAMIC_UART_ENABLE

void board_init (void)
{
    extern void tmc_uart_init (void);
    tmc_uart_init();
}

#endif

#endif  //BOARD_MKS_ROBIN_NANO_31