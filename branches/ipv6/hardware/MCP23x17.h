#ifndef MCP23x17_H
#define MCP23x17_H


// MCP23x17 Registers

#define	MCP23x17_IODIRA		0x00
#define	MCP23x17_IPOLA		0x02
#define	MCP23x17_GPINTENA	0x04
#define	MCP23x17_DEFVALA	0x06
#define	MCP23x17_INTCONA	0x08
#define	MCP23x17_IOCON		0x0A
#define	MCP23x17_GPPUA		0x0C
#define	MCP23x17_INTFA		0x0E
#define	MCP23x17_INTCAPA	0x10
#define	MCP23x17_GPIOA		0x12
#define	MCP23x17_OLATA		0x14

#define	MCP23x17_IODIRB		0x01
#define	MCP23x17_IPOLB		0x03
#define	MCP23x17_GPINTENB	0x05
#define	MCP23x17_DEFVALB	0x07
#define	MCP23x17_INTCONB	0x09
#define	MCP23x17_IOCONB		0x0B
#define	MCP23x17_GPPUB		0x0D
#define	MCP23x17_INTFB		0x0F
#define	MCP23x17_INTCAPB	0x11
#define	MCP23x17_GPIOB		0x13
#define	MCP23x17_OLATB		0x15

// Bits in the IOCON register

#define	IOCON_UNUSED	0x01
#define	IOCON_INTPOL	0x02
#define	IOCON_ODR		0x04
#define	IOCON_HAEN		0x08
#define	IOCON_DISSLW	0x10
#define	IOCON_SEQOP	    0x20
#define	IOCON_MIRROR	0x40
#define	IOCON_BANK_MODE	0x80

// Default initialisation mode

#define	IOCON_INIT	(IOCON_SEQOP)

// SPI Command codes

#define	CMD_WRITE	0x40
#define CMD_READ	0x41

#endif
