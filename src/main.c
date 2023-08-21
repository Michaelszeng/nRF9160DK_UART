/*
 * THIS IS NOW BEING USED TO TEST UART RECEIVING
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/uart.h>

#define SLEEP_TIME_MS   1000

// #define UART uart1  // uart1: TX = P0.01, RX = P0.00.    TO BE USED FOR UART DEBUGGING
#define UART uart2  // uart2: TX = P0.16, RX = P0.15.    TO BE USED FOR UART COMMS WITH nRF9160

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(UART));

#define BUFF_SIZE 10  // IMPORTANT: RX and TX buffers must be the same size. This is bc UART_RX_RDY event only occurs when RX buffer is full.
static uint8_t* rx_buf;  // A buffer to store incoming UART data

#define MAGIC_NUMBER 0xFF  // Used to confirm that incoming data is valid


static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{

	switch (evt->type) {
	
	case UART_TX_DONE:
		// do something
		break;

	case UART_TX_ABORTED:
		// do something
		break;
		
	case UART_RX_RDY:
		printk("evt->data.rx.len: %d\n", evt->data.rx.len);
		// printk("	evt->data.rx.offset: %d\n", evt->data.rx.offset);

		// Sometimes, on boot, trash data is received, so this prevents us from trying to parse the trash data
		if (evt->data.rx.buf[evt->data.rx.offset] != MAGIC_NUMBER) {
			printk("Received data that did not start with the magic number. Ignoring it.\n");
			break;
		}
		if (evt->data.rx.len != BUFF_SIZE) {
			printk("Received data not correct length. Ignoring it.\n");
			break;
		}
		
		printk("data received: ");
		for (int i=0; i < evt->data.rx.len; i++) {
			printk("%02X ", evt->data.rx.buf[evt->data.rx.offset + i]);
		}
		printk("\n");
		break;

	case UART_RX_BUF_REQUEST:
		// printk("UART_RX_BUF_REQUEST\n");
		rx_buf = k_malloc(BUFF_SIZE * sizeof(uint8_t));
		if (rx_buf) {
			uart_rx_buf_rsp(uart, rx_buf, BUFF_SIZE);
		} else {
			printk("WARNING: Not able to allocate UART receive buffer");
		}
		break;

	case UART_RX_BUF_RELEASED:
		// printk("UART_RX_BUF_RELEASED\n");
		k_free(rx_buf);
		break;
		
	case UART_RX_DISABLED:
		printk("UART_RX_DISABLED\n");
		uart_rx_enable(uart, rx_buf, BUFF_SIZE, SYS_FOREVER_US);
		break;

	case UART_RX_STOPPED:
		printk("UART_RX_STOPPED\n");
		break;
		
	default:
		break;
	}
}


int main(void)
{
	k_msleep(1000);
	printk("Starting Program..\n");

	int ret;

	if (!device_is_ready(uart)) {
		printk("uart not ready. returning.\n");
		return -1;
	}

	ret = uart_callback_set(uart, uart_cb, NULL);
	if (ret) {
		return ret;
	}

	k_msleep(1000);
	ret = uart_rx_enable(uart, rx_buf, BUFF_SIZE, SYS_FOREVER_US);
	if (ret) {
		printk("uart_rx_enable faild with ret=%d\n", ret);
		return ret;
	}

	static char tx_buf[BUFF_SIZE] = {MAGIC_NUMBER, 0xFF, 0xFF, 0xFF, 0xFF, '1', '2', '3', '4', '5'};

	int ctr = 0;
	while (1) {

		printk("Looping... %d\n", ctr);
		ctr++;

		ret = uart_tx(uart, tx_buf, BUFF_SIZE, SYS_FOREVER_US);
		if (ret) {
			printk("uart_tx errored: %d.\n", ret);
		}

		k_msleep(SLEEP_TIME_MS);

	}
	return 0;
}
