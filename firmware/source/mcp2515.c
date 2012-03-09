/********************************************************************
 File: mcp2515.c

 Description:
 This file contains the MCP2515 interface functions.

 Authors and Copyright:
 (c) 2012, Thomas Fischl <tfischl@gmx.de>

 Device: PIC18F14K50
 Compiler: HI-TECH C PRO for the PIC18 MCU Family (Lite)  V9.65

 License:
 This file is open source. You can use it or parts of it in own
 open source projects. For closed or commercial projects you have to
 contact the authors listed above to get the permission for using
 this source code.

 ********************************************************************/

#include <htc.h>
#include "mcp2515.h"

/**
 * \brief Transmit one byte over SPI bus
 *
 * \param c Character to send
 * \return Received byte
 */
unsigned char spi_transmit(unsigned char c) {
    SSPBUF = c;
    while (!SSPSTATbits.BF) {};
    return SSPBUF;
}


/**
 * \brief Write to given register
 *
 * \param address Register address
 * \param data Value to write to given register
 */
void mcp2515_write_register(unsigned char address, unsigned char data) {

    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_WRITE);
    spi_transmit(address);
    spi_transmit(data);
   
    // release SS
    MCP2515_SS = 1;
}


/**
 * \brief Read from given register
 *
 * \param address Register address
 * \return register value
 */
unsigned char mcp2515_read_register(unsigned char address) {

    unsigned char data;
   
    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_READ);
    spi_transmit(address);   
    data = spi_transmit(0xff); 
   
    // release SS
    MCP2515_SS = 1;
   
    return data;
}


/**
 * \brief Modify bit of given register
 *
 * \param address Register address
 * \param mask Mask of bits to set
 * \param data Values to set
 *
 * This function works only on a few registers. Please check the datasheet!
 */
void mcp2515_bit_modify(unsigned char address, unsigned char mask, unsigned char data) {

    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_BIT_MODIFY);
    spi_transmit(address);
    spi_transmit(mask);
    spi_transmit(data);
   
    // release SS
    MCP2515_SS = 1;
}


/**
 * \brief Initialize spi interface, reset the MCP2515 and activate clock output signal
 */
void mcp2515_init() {

    unsigned char dummy;

    // init SPI
    SSPSTAT = 0x40; // CKE=1
    SSPCON1 = 0x22;
    dummy = SSPBUF; // dummy read to clear BF
    dummy = 0;

    TRISBbits.TRISB4 = 1; // set TRIS of SDI
    TRISCbits.TRISC6 = 0; // clear TRIS of SS
    TRISCbits.TRISC7 = 0; // clear TRIS of SDO
    TRISBbits.TRISB6 = 0; // clear TRIS of SCK
    LATCbits.LATC6 = 1; // SS

    while (++dummy) {};

    // reset device
    LATCbits.LATC6 = 0; // SS
    spi_transmit(0xC0); // reset device
    LATCbits.LATC6 = 1; // SS

    while (++dummy) {};

    mcp2515_write_register(MCP2515_REG_CANCTRL, 0x85); // set config mode, clock prescaling 1:2 and clock output

    // TODO: if mask/filters are used, change these both lines:
    mcp2515_write_register(MCP2515_REG_RXB0CTRL, 0x60); // turn mask/filters off; receive any message
    mcp2515_write_register(MCP2515_REG_RXB1CTRL, 0x60); // turn mask/filters off; receive any message

    mcp2515_write_register(MCP2515_REG_RXM0SIDH, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM0SIDL, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM0EID8, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM0EID0, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM1SIDH, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM1SIDL, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM1EID8, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM1EID0 , 0x00);

    mcp2515_write_register(MCP2515_REG_CANINTE, 0x03); // RX interrupt

}


/**
 * \brief Set bit timing registers
 *
 * \param cnf1 Configuration register 1
 * \param cnf2 Configuration register 2
 * \param cnf3 Configuration register 3
 *
 * This function has only affect if mcp2515 is in configuration mode
 */
void mcp2515_set_bittiming(unsigned char cnf1, unsigned char cnf2, unsigned char cnf3) {

    mcp2515_write_register(MCP2515_REG_CNF1, cnf1);
    mcp2515_write_register(MCP2515_REG_CNF2, cnf2);
    mcp2515_write_register(MCP2515_REG_CNF3, cnf3);

}

/**
 * \brief Read status byte of MCP2515
 *
 * \return status byte of MCP2515
 */
unsigned char mcp2515_read_status() {

    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_READ_STATUS);
    unsigned char status = spi_transmit(0xff);
   
    // release SS
    MCP2515_SS = 1;

    return status;
}


/**
 * \brief Read RX status byte of MCP2515
 *
 * \return RX status byte of MCP2515
 */
unsigned char mcp2515_rx_status() {

    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_RX_STATUS);
    unsigned char status = spi_transmit(0xff);
   
    // release SS
    MCP2515_SS = 1;

    return status;
}


/**
 * \brief Send given CAN message
 *
 * \ p_canmsg Pointer to can message to send
 * \return 1 if transmitted successfully to MCP2515 transmit buffer, 0 on error (= no free buffer available)
 */
unsigned char mcp2515_send_message(canmsg_t * p_canmsg) {

    unsigned char status = mcp2515_read_status();
    unsigned char address;

    // check length
    if (p_canmsg->length > 8) return 0;

    // get offest address of next free tx buffer
    if ((status & 0x04) == 0) {
        address = 0x00;
    } else if ((status & 0x10) == 0) {
        address = 0x02;
    } else if ((status & 0x40) == 0) {
        address = 0x04;
    } else {
        // no free transmit buffer
        return 0;
    }

    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_LOAD_TX | address);

    if (p_canmsg->flags.extended) {
        spi_transmit(p_canmsg->id >> 21);
        spi_transmit(((p_canmsg->id >> 13) & 0xe0) | ((p_canmsg->id >> 16) & 0x03) | 0x08);
        spi_transmit(p_canmsg->id >> 8);
        spi_transmit(p_canmsg->id);
    } else {
        spi_transmit(p_canmsg->id >> 3);
        spi_transmit(p_canmsg->id << 5);
        spi_transmit(0);
        spi_transmit(0);
    }

    // length and data
    if (p_canmsg->flags.rtr) {
        spi_transmit(p_canmsg->length | 0x40);
    } else {
        spi_transmit(p_canmsg->length);
        unsigned char i;
        for (i = 0; i < p_canmsg->length; i++) {
            spi_transmit(p_canmsg->data[i]);
        }
    }
   
    // release SS
    MCP2515_SS = 1;

    _delay(1);

    // pull SS to low level
    MCP2515_SS = 0;
    if (address == 0) address = 1;
    spi_transmit(MCP2515_CMD_RTS | address);
    // release SS
    MCP2515_SS = 1;

    return 1;
}

/*
 * \brief Read out one can message from MCP2515
 *
 * \param p_canmsg Pointer to can message structure to fill
 * \return 1 on success, 0 if there is no message to read
 */
unsigned char mcp2515_receive_message(canmsg_t * p_canmsg) {

    unsigned char status = mcp2515_rx_status();
    unsigned char address;

    if (status & 0x40) {
        address = 0x00;
    } else if (status & 0x80) {
        address = 0x04;
    } else {
        // no message in receive buffer
        return 0;
    }

    p_canmsg->flags.rtr = (status >> 3) & 0x01;
    p_canmsg->flags.extended = (status >> 4) & 0x01;

    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_READ_RX | address);

    if (p_canmsg->flags.extended) {
	p_canmsg->id =  (unsigned long) spi_transmit(0xff) << 21;
        unsigned long temp = spi_transmit(0xff);
        p_canmsg->id |= (temp & 0xe0) << 13;
        p_canmsg->id |= (temp & 0x03) << 16;
        p_canmsg->id |= (unsigned long) spi_transmit(0xff) << 8;
        p_canmsg->id |= (unsigned long) spi_transmit(0xff);
    } else {
        p_canmsg->id =  (unsigned long) spi_transmit(0xff) << 3;
        p_canmsg->id |= (unsigned long) spi_transmit(0xff) >> 5;
        spi_transmit(0xff);
        spi_transmit(0xff);
    }

    // get length and data
    p_canmsg->length = spi_transmit(0xff) & 0x0f;
    if (!p_canmsg->flags.rtr) {
        unsigned char i;
        for (i = 0; i < p_canmsg->length; i++) {
            p_canmsg->data[i] = spi_transmit(0xff);
        }
    }

    // release SS
    MCP2515_SS = 1;

    if (address == 0) address = 1;
    else address = 2;
    mcp2515_bit_modify(MCP2515_REG_CANINTF, address, 0);

    return 1;
}
