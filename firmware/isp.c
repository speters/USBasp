/*
 * isp.c - part of USBasp
 *
 * Autor..........: Thomas Fischl <tfischl@gmx.de>
 * Description....: Provides functions for communication/programming
 *                  over ISP interface
 * Licence........: GNU GPL v2 (see Readme.txt)
 * Creation Date..: 2005-02-23
 * Last change....: 2009-02-28
 * AVR芯片已经支持
 * AT89S系列完美支持
 * 就差LGT芯片
 */

#include <avr>
#include "isp.h"
#include "clock.h"
#include "usbasp.h"

#define spiHWdisable() UCSR0B=0x00

uchar sck_sw_delay;//实际上软件SPI已经不存在
uchar sck_ubrrh;
uchar sck_ubrrl;//SPI速率
uchar chip_type;//芯片型号 0代表AVR,1代表S5x,2代表LGT8F系列
void spiHWenable() {
	UBRR0H=sck_ubrrh;
	UBRR0L=sck_ubrrl;
	UCSR0B= _BV(TXEN) | _BV(RXEN);//允许发送和接收
	UCSR0C= _BV(UMSEL1) | _BV(UMSEL0);//在CLK的上升沿接收、下降沿输出,SPI主机模式
}

void ispSetSCKOption(uchar option) {

	if (option == USBASP_ISP_SCK_AUTO)
		option = USBASP_ISP_SCK_375;

	//if (option >= USBASP_ISP_SCK_93_75) {
	//	ispTransmit = ispTransmit_hw;
		sck_sw_delay = 3;
		switch (option) {
		//大改……
		case USBASP_ISP_SCK_6000:
			sck_ubrrh=0;
			sck_ubrrl=0x00;//6Mbps
		case USBASP_ISP_SCK_3000:
			sck_ubrrh=0;
			sck_ubrrl=0x01;//3Mbps		
			break;
		case USBASP_ISP_SCK_1500:
			/* enable SPI, master, 1.5MHz, XTAL/8 */
			sck_ubrrh=0;
			sck_ubrrl=0x03;//1.5Mbps
			break;
		case USBASP_ISP_SCK_750:
			/* enable SPI, master, 750kHz, XTAL/16 */
			sck_ubrrh=0;
			sck_ubrrl=0x07;
			break;
		case USBASP_ISP_SCK_375:
		default:
			/* enable SPI, master, 375kHz, XTAL/32 (default) */
			sck_ubrrh=0;
			sck_ubrrl=15;//375K
			break;
		case USBASP_ISP_SCK_187_5:
			/* enable SPI, master, 187.5kHz XTAL/64 */
			sck_ubrrh=0;
			sck_ubrrl=31;
			break;
		case USBASP_ISP_SCK_93_75:
			/* enable SPI, master, 93.75kHz XTAL/128 */
			sck_ubrrh=0;
			sck_ubrrl=63;
			break;

		case USBASP_ISP_SCK_32:
			sck_ubrrh=0;
			sck_ubrrl=0xb9;

			break;
		case USBASP_ISP_SCK_16:
			sck_ubrrh=1;
			sck_ubrrl=0x72;

			break;
		case USBASP_ISP_SCK_8:
			sck_ubrrh=2;
			sck_ubrrl=0xe4;

			break;
		case USBASP_ISP_SCK_4:
			sck_ubrrh=5;
			sck_ubrrl=0xc8;

			break;
		case USBASP_ISP_SCK_2:
			sck_ubrrh=0xb;
			sck_ubrrl=0x90;

			break;
		case USBASP_ISP_SCK_1:
			sck_ubrrh=0x17;
			sck_ubrrl=0x20;

			break;
		case USBASP_ISP_SCK_0_5:
			sck_ubrrh=0x2e;
			sck_ubrrl=0x40;

			break;
		}
}

void ispDelay() {

	uint8_t starttime = TIMERVALUE;
	if(chip_type=CHIP_S5x)
	{
		while ((uint8_t) (TIMERVALUE - starttime) < 4*sck_sw_delay) {
		}
	}
	else
	{
		while ((uint8_t) (TIMERVALUE - starttime) < 2*sck_sw_delay) {
		}
	}	
}

void ispReset()
{

	if(chip_type == CHIP_AVR)
	{
		ISP_OUT &= ~(1 << ISP_RST); //AVR负脉冲复位

	}
	else if(chip_type == CHIP_S5x)
	{
		ISP_OUT |= (1 << ISP_RST); /* RST high */
	}
	ispDelay();
	ispDelay();
	ispDelay();
	ispDelay();//等待让芯片内部复位
}
void ispConnect() {
	
	/* all ISP pins are inputs before */
	/* now set output pins */
	//SPI_DDR &= ~(1<< ISP_MISO);
	CK_OUT &= ~(1<< ISP_SCK);
	ISP_DDR |= (1 << ISP_RST) ;
	CK_DDR  |= (1 << ISP_SCK);
	SPI_DDR |= (1<< ISP_MOSI);
	//if (ispTransmit == ispTransmit_hw) {
		spiHWenable();//SPI开
	//}
	/* reset device */
	CK_OUT &= ~(1 << ISP_SCK); /* SCK low */
	ISP_OUT &= ~(1 << ISP_RST); /* RST low */

	/* positive reset pulse > 2 SCK (target) */
	ispDelay();
	ISP_OUT |= (1 << ISP_RST); /* RST high */
	
	ispReset();
}


void ispDisconnect() {
	spiHWdisable();
	/* set all ISP pins inputs */
	ISP_DDR &= ~(1 << ISP_RST);// | (1 << ISP_SCK) | (1 << ISP_MOSI));
	SPI_DDR &= ~(1 << ISP_MOSI) ;
	SPI_DDR &= ~(1 << ISP_MISO);//关闭输出
	CK_DDR &= ~(1 << ISP_SCK);
	/* switch pullups off */
	ISP_OUT &= ~(1 << ISP_RST);// | (1 << ISP_SCK) | (1 << ISP_MOSI));
	SPI_OUT &= ~(1 << ISP_MOSI);
	SPI_OUT &= ~(1 << ISP_MISO);
	CK_OUT &= ~(1 << ISP_SCK);//关闭上拉
	/* disable hardware SPI */

}

uchar ispTransmit_hw(uchar send_byte) {
	UDR0 = send_byte;

	while (!(UCSR0A & _BV(UDRE)));//等待数据寄存器空
	//while (!(UCSR0A & _BV(TXC)));
	while (!(UCSR0A & _BV(RXC)));//等待接收完成
	return UDR0;
}

uchar checkOut()
{
	uchar check,check1;
	uchar count ;
	count=16;//尝试16次
	ispReset();
	while (count--) {
		ispTransmit(0xAC);
		ispTransmit(0x53);
		check = ispTransmit(0);
		check1= ispTransmit(0);//89S52

		if (check == 0x53) {
			return 0; //三字节0x53是AVR
		}
		if(check1 == 0x69 && chip_type == CHIP_S5x){
			return 0;//四字节0x69是89S52
		}    
		spiHWdisable();

		/* pulse SCK */
		ISP_OUT |= (1 << ISP_SCK); /* SCK high */
		ispDelay();
		ISP_OUT &= ~(1 << ISP_SCK); /* SCK low */
		ispDelay();

		//if (ispTransmit == ispTransmit_hw) {
			spiHWenable();
		//}

	}
	return 1;
}
uchar ispEnterProgrammingMode() {

	
	chip_type=CHIP_AVR;//先用AVR试验
	
	if(checkOut() ==0) return 0;
	chip_type=CHIP_S5x;
	if(checkOut() ==0) return 0;
	return 1;

}
uchar ispReadFlash(unsigned long address) {
	ispTransmit(0x20 | ((address & 1) << 3));
	ispTransmit(address >> 9);
	ispTransmit(address >> 1);
	return ispTransmit(0);
}

uchar ispWriteFlash(unsigned long address, uchar data, uchar pollmode) {
	uchar retries ;
	/* 0xFF is value after chip erase, so skip programming
	 if (data == 0xFF) {
	 return 0;
	 }
	 */

	ispTransmit(0x40 | ((address & 1) << 3));
	ispTransmit(address >> 9);
	ispTransmit(address >> 1);
	ispTransmit(data);

	if (pollmode == 0)
		return 0;

	if (data == 0x7F) {
		clockWait(15); /* wait 4,8 ms */
		return 0;
	} else {

		/* polling flash */
		if(chip_type==CHIP_AVR) retries=30;
		if(chip_type == CHIP_S5x) retries=50;
		uint8_t starttime = TIMERVALUE;
		while (retries != 0) {
			if (ispReadFlash(address) != 0x7F) {
				return 0;
			};

			if ((uint8_t) (TIMERVALUE - starttime) > CLOCK_T_320us) {
				starttime = TIMERVALUE;
				retries--;
			}

		}
		return 1; /* error */
	}

}

uchar ispFlushPage(unsigned long address, uchar pollvalue) {
	ispTransmit(0x4C);
	ispTransmit(address >> 9);
	ispTransmit(address >> 1);
	ispTransmit(0);

	if (pollvalue == 0xFF) {
		clockWait(15);
		return 0;
	} else {

		/* polling flash */
		uchar retries = 30;
		uint8_t starttime = TIMERVALUE;

		while (retries != 0) {
			if (ispReadFlash(address) != 0xFF) {
				return 0;
			};

			if ((uint8_t) (TIMERVALUE - starttime) > CLOCK_T_320us) {
				starttime = TIMERVALUE;
				retries--;
			}

		}

		return 1; /* error */
	}

}

uchar ispReadEEPROM(unsigned int address) {
	ispTransmit(0xA0);
	ispTransmit(address >> 8);
	ispTransmit(address);
	return ispTransmit(0);
}

uchar ispWriteEEPROM(unsigned int address, uchar data) 
{

	ispTransmit(0xC0);
	ispTransmit(address >> 8);
	ispTransmit(address);
	ispTransmit(data);

	clockWait(30); // wait 9,6 ms

	return 0;
}
</avr></tfischl@gmx.de>
