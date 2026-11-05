/*! @file
  @brief SPI class for ESP32 / ESP-IDF and mruby/c.

  Common peripheral I/O API, aligned with the STM32 reference implementation:
    SPI.new(unit:, frequency:, mode:, first_bit:, mosi_pin:, miso_pin:, clk_pin:)
    spi.setmode(frequency:, mode:, first_bit:)
    spi.read(read_bytes) -> String
    spi.write(data...) -> nil
    spi.transfer(out_data, additional_read_bytes = 0) -> String

  Notes:
  - This class owns only the SPI bus. Chip-select is intentionally not managed.
    Use GPIO around read/write/transfer, or use a device-specific class such as
    LCDSPI or SDSPI.
  - The first member of spi_data_t is the SPI host. This preserves compatibility
    with existing LCDSPI/SDSPI code that reads instance->data as
    spi_host_device_t.
*/

#include "mrbc_esp32_spi.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "soc/soc_caps.h"

static const char *TAG = "SPI";

#define SPI_DEFAULT_MOSI_PIN   23
#define SPI_DEFAULT_MISO_PIN   18
#define SPI_DEFAULT_CLK_PIN    14
#define SPI_DEFAULT_FREQUENCY  656000
#define SPI_DEFAULT_MODE       0
#define SPI_MAX_TRANSFER_BYTES 65535

#define SPI_FIRST_BIT_MSB 0
#define SPI_FIRST_BIT_LSB 1

/*
 * host MUST remain the first member for compatibility with LCDSPI and SDSPI.
 */
typedef struct {
  spi_host_device_t host;
  int32_t frequency;
  int mosi_pin;
  int miso_pin;
  int clk_pin;
  uint8_t mode;
  uint8_t first_bit;
  bool initialized;
} spi_data_t;

uint8_t *make_output_buffer(mrb_vm *vm, mrb_value v[], int argc,
                            int start_idx, int *ret_bufsiz);

static spi_data_t *spi_get_data(mrbc_value v[])
{
  if (v[0].instance == NULL) return NULL;
  return (spi_data_t *)v[0].instance->data;
}

static bool spi_valid_host(spi_host_device_t host)
{
  if (host == SPI2_HOST) return true;
#if SOC_SPI_PERIPH_NUM > 2
  if (host == SPI3_HOST) return true;
#endif
  return false;
}

static bool spi_unit_to_host(int value, spi_host_device_t *host)
{
  switch (value) {
    case 2:
      *host = SPI2_HOST;
      return true;
#if SOC_SPI_PERIPH_NUM > 2
    case 3:
      *host = SPI3_HOST;
      return true;
#endif
    default:
      return false;
  }
}

static bool spi_validate_settings(int32_t frequency, int mode, int first_bit)
{
  return frequency > 0 &&
         mode >= 0 && mode <= 3 &&
         (first_bit == SPI_FIRST_BIT_MSB || first_bit == SPI_FIRST_BIT_LSB);
}

static esp_err_t spi_add_temporary_device(spi_data_t *spi,
                                           spi_device_handle_t *device)
{
  if (spi == NULL || !spi->initialized || !spi_valid_host(spi->host)) {
    return ESP_ERR_INVALID_STATE;
  }

  spi_device_interface_config_t config = {
    .command_bits = 0,
    .address_bits = 0,
    .dummy_bits = 0,
    .mode = spi->mode,
    .clock_speed_hz = spi->frequency,
    .spics_io_num = -1,
    .flags = SPI_DEVICE_NO_DUMMY |
             (spi->first_bit == SPI_FIRST_BIT_LSB
                ? SPI_DEVICE_BIT_LSBFIRST
                : 0),
    .queue_size = 1,
  };

  return spi_bus_add_device(spi->host, &config, device);
}

static void spi_raise_esp_error(mrbc_vm *vm, const char *operation, esp_err_t err)
{
  ESP_LOGE(TAG, "%s failed: %s", operation, esp_err_to_name(err));
  mrbc_raisef(vm, 0, "%s failed (%s)", operation, esp_err_to_name(err));
}

/* SPI constructor. */
static void c_spi_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(spi_data_t));
  mrbc_instance_call_initialize(vm, v, argc);
}

/*
 * SPI.new / initialize
 *
 * Defaults preserve the old ESP32 class pins and host while adopting the
 * common API defaults of mode 0, 656 kHz and MSB first.
 */
static void c_spi_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(unit, frequency, freq, mode, first_bit,
              mosi_pin, miso_pin, clk_pin);

  spi_data_t *spi = spi_get_data(v);
  if (spi == NULL) {
    mrbc_raise(vm, MRBC_CLASS(RuntimeError), "SPI instance data is unavailable");
    return;
  }

  memset(spi, 0, sizeof(*spi));

#if SOC_SPI_PERIPH_NUM > 2
  spi->host = SPI3_HOST;  /* Preserve the previous implementation's default. */
#else
  spi->host = SPI2_HOST;
#endif
  spi->frequency = SPI_DEFAULT_FREQUENCY;
  spi->mode = SPI_DEFAULT_MODE;
  spi->first_bit = SPI_FIRST_BIT_MSB;
  spi->mosi_pin = SPI_DEFAULT_MOSI_PIN;
  spi->miso_pin = SPI_DEFAULT_MISO_PIN;
  spi->clk_pin = SPI_DEFAULT_CLK_PIN;
  spi->initialized = false;

  if (MRBC_ISNUMERIC(unit)) {
    if (!spi_unit_to_host(MRBC_TO_INT(unit), &spi->host)) {
      mrbc_raise(vm, MRBC_CLASS(ArgumentError), "unit must be 2 or 3");
      return;
    }
  }

  if (MRBC_ISNUMERIC(frequency)) spi->frequency = MRBC_TO_INT(frequency);
  if (MRBC_ISNUMERIC(freq))      spi->frequency = MRBC_TO_INT(freq);
  if (MRBC_ISNUMERIC(mode))      spi->mode = MRBC_TO_INT(mode);
  if (MRBC_ISNUMERIC(first_bit)) spi->first_bit = MRBC_TO_INT(first_bit);
  if (MRBC_ISNUMERIC(mosi_pin))  spi->mosi_pin = MRBC_TO_INT(mosi_pin);
  if (MRBC_ISNUMERIC(miso_pin))  spi->miso_pin = MRBC_TO_INT(miso_pin);
  if (MRBC_ISNUMERIC(clk_pin))   spi->clk_pin = MRBC_TO_INT(clk_pin);

  if (!spi_validate_settings(spi->frequency, spi->mode, spi->first_bit)) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError),
               "invalid frequency, mode, or first_bit");
    return;
  }

  spi_bus_config_t bus_config = {
    .mosi_io_num = spi->mosi_pin,
    .miso_io_num = spi->miso_pin,
    .sclk_io_num = spi->clk_pin,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = SPI_MAX_TRANSFER_BYTES,
  };

  esp_err_t err = spi_bus_initialize(spi->host, &bus_config, SPI_DMA_CH_AUTO);
  if (err != ESP_OK) {
    spi_raise_esp_error(vm, "spi_bus_initialize", err);
    return;
  }

  spi->initialized = true;

  ESP_LOGI(TAG, "SPI bus initialized");
  ESP_LOGI(TAG, "unit: %d", (int)spi->host);
  ESP_LOGI(TAG, "mosi: %d", spi->mosi_pin);
  ESP_LOGI(TAG, "miso: %d", spi->miso_pin);
  ESP_LOGI(TAG, "clk:  %d", spi->clk_pin);
  ESP_LOGI(TAG, "frequency: %ld", (long)spi->frequency);
  ESP_LOGI(TAG, "mode: %u", (unsigned)spi->mode);
  ESP_LOGI(TAG, "first_bit: %s",
           spi->first_bit == SPI_FIRST_BIT_LSB ? "LSB" : "MSB");

  /* Do not overwrite v[0]. SPI.new must return the instance. */
}

/*
 * Change per-device transfer settings used by read/write/transfer.
 * The bus pins and host cannot be changed after bus initialization.
 */
static void c_spi_setmode(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(unit, frequency, freq, mode, first_bit);
  spi_data_t *spi = spi_get_data(v);
  if (spi == NULL || !spi->initialized) {
    mrbc_raise(vm, MRBC_CLASS(RuntimeError), "SPI bus is not initialized");
    return;
  }

  int32_t new_frequency = spi->frequency;
  int new_mode = spi->mode;
  int new_first_bit = spi->first_bit;

  if (MRBC_ISNUMERIC(unit)) {
    spi_host_device_t requested_host;
    if (!spi_unit_to_host(MRBC_TO_INT(unit), &requested_host) ||
        requested_host != spi->host) {
      mrbc_raise(vm, MRBC_CLASS(ArgumentError),
                 "unit cannot be changed after initialization");
      return;
    }
  }

  if (MRBC_ISNUMERIC(frequency)) new_frequency = MRBC_TO_INT(frequency);
  if (MRBC_ISNUMERIC(freq))      new_frequency = MRBC_TO_INT(freq);
  if (MRBC_ISNUMERIC(mode))      new_mode = MRBC_TO_INT(mode);
  if (MRBC_ISNUMERIC(first_bit)) new_first_bit = MRBC_TO_INT(first_bit);

  if (!spi_validate_settings(new_frequency, new_mode, new_first_bit)) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError),
               "invalid frequency, mode, or first_bit");
    return;
  }

  spi->frequency = new_frequency;
  spi->mode = (uint8_t)new_mode;
  spi->first_bit = (uint8_t)new_first_bit;
  SET_NIL_RETURN();
}

/* spi.read(read_bytes) -> String. Sends zero bytes while receiving. */
static void c_spi_read(mrbc_vm *vm, mrbc_value v[], int argc)
{
  if (argc < 1 || v[1].tt != MRBC_TT_INTEGER) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "read_bytes must be an Integer");
    return;
  }

  int read_bytes = mrbc_integer(v[1]);
  if (read_bytes < 0 || read_bytes > SPI_MAX_TRANSFER_BYTES) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "invalid read size");
    return;
  }

  mrbc_value ret = mrbc_string_new(vm, NULL, read_bytes);
  uint8_t *rx_buffer = (uint8_t *)mrbc_string_cstr(&ret);
  if (read_bytes == 0) {
    SET_RETURN(ret);
    return;
  }

  uint8_t *tx_buffer = (uint8_t *)mrbc_alloc(vm, read_bytes);
  if (tx_buffer == NULL) {
    mrbc_raise(vm, MRBC_CLASS(NoMemoryError), "SPI read buffer allocation failed");
    return;
  }
  memset(tx_buffer, 0, read_bytes);
  memset(rx_buffer, 0, read_bytes);

  spi_data_t *spi = spi_get_data(v);
  spi_device_handle_t device = NULL;
  esp_err_t err = spi_add_temporary_device(spi, &device);
  if (err == ESP_OK) {
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = read_bytes * 8;
    transaction.tx_buffer = tx_buffer;
    transaction.rx_buffer = rx_buffer;
    err = spi_device_transmit(device, &transaction);
  }

  esp_err_t remove_err = ESP_OK;
  if (device != NULL) remove_err = spi_bus_remove_device(device);
  mrbc_free(vm, tx_buffer);

  if (err != ESP_OK) {
    spi_raise_esp_error(vm, "SPI read", err);
    return;
  }
  if (remove_err != ESP_OK) {
    spi_raise_esp_error(vm, "spi_bus_remove_device", remove_err);
    return;
  }

  SET_RETURN(ret);
}

/*
 * spi.write("str")
 * spi.write(d1, d2, ...)
 * spi.write([d1, d2, ...])
 */
static void c_spi_write(mrbc_vm *vm, mrbc_value v[], int argc)
{
  int buffer_size = 0;
  uint8_t *buffer = make_output_buffer(vm, v, argc, 1, &buffer_size);
  if (buffer == NULL) return;

  spi_data_t *spi = spi_get_data(v);
  spi_device_handle_t device = NULL;
  esp_err_t err = spi_add_temporary_device(spi, &device);
  if (err == ESP_OK && buffer_size > 0) {
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = buffer_size * 8;
    transaction.tx_buffer = buffer;
    err = spi_device_transmit(device, &transaction);
  }

  esp_err_t remove_err = ESP_OK;
  if (device != NULL) remove_err = spi_bus_remove_device(device);
  mrbc_free(vm, buffer);

  if (err != ESP_OK) {
    spi_raise_esp_error(vm, "SPI write", err);
    return;
  }
  if (remove_err != ESP_OK) {
    spi_raise_esp_error(vm, "spi_bus_remove_device", remove_err);
    return;
  }

  SET_NIL_RETURN();
}

/*
 * spi.transfer(out_data, additional_read_bytes = 0) -> String
 *
 * The returned String contains received bytes for the entire transaction,
 * including the bytes received while out_data is transmitted.
 */
static void c_spi_transfer(mrbc_vm *vm, mrbc_value v[], int argc)
{
  if (argc < 1) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "transfer requires output data");
    return;
  }

  int tx_size = 0;
  uint8_t *tx_buffer = make_output_buffer(vm, v, 1, 1, &tx_size);
  if (tx_buffer == NULL) return;

  int additional_read_bytes = 0;
  if (argc >= 2) {
    if (v[2].tt != MRBC_TT_INTEGER) {
      mrbc_free(vm, tx_buffer);
      mrbc_raise(vm, MRBC_CLASS(ArgumentError),
                 "additional_read_bytes must be an Integer");
      return;
    }
    additional_read_bytes = mrbc_integer(v[2]);
    if (additional_read_bytes < 0) {
      mrbc_free(vm, tx_buffer);
      mrbc_raise(vm, MRBC_CLASS(ArgumentError),
                 "additional_read_bytes must not be negative");
      return;
    }
  }

  int transfer_size = tx_size + additional_read_bytes;
  if (transfer_size > SPI_MAX_TRANSFER_BYTES) {
    mrbc_free(vm, tx_buffer);
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "transfer is too large");
    return;
  }

  if (additional_read_bytes > 0) {
    uint8_t *expanded = (uint8_t *)mrbc_realloc(vm, tx_buffer, transfer_size);
    if (expanded == NULL) {
      mrbc_free(vm, tx_buffer);
      mrbc_raise(vm, MRBC_CLASS(NoMemoryError),
                 "SPI transfer buffer allocation failed");
      return;
    }
    tx_buffer = expanded;
    memset(tx_buffer + tx_size, 0, additional_read_bytes);
  }

  mrbc_value ret = mrbc_string_new(vm, NULL, transfer_size);
  uint8_t *rx_buffer = (uint8_t *)mrbc_string_cstr(&ret);
  memset(rx_buffer, 0, transfer_size);

  spi_data_t *spi = spi_get_data(v);
  spi_device_handle_t device = NULL;
  esp_err_t err = spi_add_temporary_device(spi, &device);
  if (err == ESP_OK && transfer_size > 0) {
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = transfer_size * 8;
    transaction.tx_buffer = tx_buffer;
    transaction.rx_buffer = rx_buffer;
    err = spi_device_transmit(device, &transaction);
  }

  esp_err_t remove_err = ESP_OK;
  if (device != NULL) remove_err = spi_bus_remove_device(device);
  mrbc_free(vm, tx_buffer);

  if (err != ESP_OK) {
    spi_raise_esp_error(vm, "SPI transfer", err);
    return;
  }
  if (remove_err != ESP_OK) {
    spi_raise_esp_error(vm, "spi_bus_remove_device", remove_err);
    return;
  }

  SET_RETURN(ret);
}

void mrbc_esp32_spi_gem_init(struct VM *vm)
{
  mrbc_class *spi_class = mrbc_define_class(vm, "SPI", mrbc_class_object);

  mrbc_define_method(vm, spi_class, "new",        c_spi_new);
  mrbc_define_method(vm, spi_class, "initialize", c_spi_initialize);
  mrbc_define_method(vm, spi_class, "setmode",    c_spi_setmode);
  mrbc_define_method(vm, spi_class, "read",       c_spi_read);
  mrbc_define_method(vm, spi_class, "write",      c_spi_write);
  mrbc_define_method(vm, spi_class, "transfer",   c_spi_transfer);

  mrbc_value msb_first = mrbc_integer_value(SPI_FIRST_BIT_MSB);
  mrbc_value lsb_first = mrbc_integer_value(SPI_FIRST_BIT_LSB);
  mrbc_set_class_const(spi_class, mrbc_str_to_symid("MSB_FIRST"), &msb_first);
  mrbc_set_class_const(spi_class, mrbc_str_to_symid("LSB_FIRST"), &lsb_first);
}
