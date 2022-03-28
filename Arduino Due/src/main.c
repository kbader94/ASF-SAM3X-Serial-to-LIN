/**
 * \file
 *
 * \brief USB to LIN slave publisher for SAM3X
 *
 * \summary
 * This program is a LIN slave publisher for SAM3X which takes
 * serial data input from the UART, caches the data, and responds appropriately
 * to LIN requests on USART0; essentially turning it into a USB to LIN adapter.
 * Data sent to the device must be formatted as an 8 byte LIN frame followed by a
 * new line character.
 *
 * Copyright (c) 2014-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#include <asf.h>
#include <stdio_serial.h>
#include <conf_board.h>
#include <main.h>

/************************************************************************/
/* \brief Interrupt handler for USART LIN communication                 */
/************************************************************************/
void USART0_Handler(void){

	usart_lin_handler(LIN_SLAVE_NODE_NUM);
	
}

/************************************************************************/
/* \brief Interrupt handler for UART serial communication               */
/************************************************************************/
void console_uart_irq_handler(void)
{

	/* Get UART status and check if PDC receive buffer is full */
	if ((uart_get_status(CONSOLE_UART) & UART_SR_RXBUFF) == UART_SR_RXBUFF) {
		puts("buffer full");
		/* Configure PDC for data transfer (RX and TX) */
		//memcpy(g_uc_pdc_buffer, unprocessed_data, sizeof(g_uc_pdc_buffer));
		data_avail = true;
		pdc_rx_init(g_p_uart_pdc, &g_pdc_uart_packet, NULL);
		pdc_tx_init(g_p_uart_pdc, &g_pdc_uart_packet, NULL);
	}
}

/************************************************************************/
/* \brief Configure Peripheral DMA Controller for serial communication  */
/************************************************************************/
void config_pdc(void){
	
	/* Get pointer to UART PDC register base */
	g_p_uart_pdc = uart_get_pdc_base(CONSOLE_UART);

	/* Initialize PDC data packet for transfer */
	g_pdc_uart_packet.ul_addr = (uint32_t) g_uc_pdc_buffer;
	g_pdc_uart_packet.ul_size = BUFFER_SIZE;

	/* Initialize pdc data reception */
	pdc_rx_init(g_p_uart_pdc, &g_pdc_uart_packet, NULL);

	/* Enable PDC transfer */
	pdc_enable_transfer(g_p_uart_pdc, PERIPH_PTCR_RXTEN | PERIPH_PTCR_TXTEN);
	
}

/************************************************************************/
/*   \brief Configure UART0 for serial communication.                   */
/************************************************************************/
static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
		.paritytype = CONF_UART_PARITY
	};
	
	/* Configure console UART. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONF_UART, &uart_serial_options);
	
	config_pdc();
	
	/* Enable console UART IRQ SHOULD: move to configre_console*/
	uart_enable_interrupt(CONSOLE_UART, UART_IER_RXBUFF);
	NVIC_EnableIRQ(CONSOLE_UART_IRQn);
}

/************************************************************************/
/*	\brief Configure LIN on USART0 in slave mode                        */
/************************************************************************/
void config_lin(void){
	/* Enable the peripheral clock in the PMC. */
	pmc_enable_periph_clk(ID_USART0);
		
	/* Enable LIN transceiver */
	gpio_set_pin_high(PIN_USART0_CTS_IDX);
	gpio_set_pin_high(PIN_USART0_RTS_IDX);
	
	puts("-- Set LIN to Slave Publish mode\n\r");
	/* Node 0:  LIN_SLAVE_MODE */
	lin_init(USART0, false, LIN_SLAVE_NODE_NUM, 10400, sysclk_get_cpu_hz());
								
	/* Configure and enable interrupt of USART on LIN break detected. */
	NVIC_EnableIRQ(USART0_IRQn);
	usart_enable_interrupt(USART0, US_IER_LINID);
					
}

/************************************************************************/
/* \brief Process LIN data												*/
/* \return uint8_t with the LIN ID										*/
/************************************************************************/
uint8_t process_lin_data(uint8_t raw_data[10])
{
	
	puts("processing incoming lin frame");
	uint8_t id = raw_data[0];
	printf("Received LIN frame ID: %#x \r\n", id);
	for(int i = 1; i <= 8; i++){
		lin_data[id][i - 1] = raw_data[i];
		printf("%#x \r\n", raw_data[i]);
	}
	
	data_avail = false;
	return id;
}

/************************************************************************/
/* \brief register the LIN frame with the driver                        */
/************************************************************************/
void register_lin_response(uint8_t id){
	
	lin_descriptors[id].uc_id = id;
	lin_descriptors[id].uc_dlc = 8;
	lin_descriptors[id].lin_cmd = PUBLISH;
	lin_descriptors[id].uc_status = 0;
	lin_descriptors[id].uc_pt_data = *(lin_data + id);
	
	if(lin_register_descriptor(LIN_SLAVE_NODE_NUM, num_lin_responses, &lin_descriptors[id]) == PASS){
		num_lin_responses++;
	}
		
}

int main(void)
{
	/* Initialize the board */
	sysclk_init();
	board_init();

	/* Initialize the UART console */
	configure_console();

	/* Initialize LIN as a slave */
	config_lin();
	
	puts(STRING_HEADER);	

	while (1) {
		
		if(data_avail == true){
					
			uint8_t id = process_lin_data(g_uc_pdc_buffer);
			register_lin_response(id);
			
		}

	}
}

