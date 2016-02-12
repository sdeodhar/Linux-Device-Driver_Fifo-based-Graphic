#define VMODE           _IOW(0xcc,0,unsigned long)
#define BLIND_DMA    _IOW(0xcc,1,unsigned long)
#define UNBLIND_DMA    _IOW(0xcc,5,unsigned long)
#define START_DMA    _IOWR(0xcc,2,unsigned long)
#define FIFO_QUEUE   _IOWR(0xcc,3,unsigned long)
#define FIFO_FLUSH    _IO(0xcc,4)

#define Major_Version	0x0000
#define Minor_Version	0x0004
#define Vendor		0x0008
#define Supported_Features 0x000C
#define Manufacture_Week 0x0010
#define Device_RAM	 0x0020
#define Number_of_Frames 0x0024
#define Number_of_DACs	 0x0028
#define Internal_FIFO_Size 0x002c

#define Reboot 		0x1000
#define Flags		0x1004
#define ModeSet		0x1008
#define IsrSet		0x100c
#define Accl		0x1010
#define FifoStart	0x1020
#define FifoEnd		0x1024

#define BufferA_Addr	0x2000
#define BufferA_Config	0x2008

#define Clear_Buffer	0x3008
#define Flush		0x3ffc

#define Status		0x4008
#define FIFOHead	0x4010
#define FIFOTail	0x4014

#define Vertex_Coordinate 0x5000
#define Vertex_Color    0x5010
#define Clear_Color	0x5100
#define Frame_Objects	0x8000
#define DAC_Objects	0x9000
#define Primitive	0x3000
#define Vertex_Emit	0x3004

#define GRAPHICS_ON 1
#define GRAPHICS_OFF 0

enum{
	_FColumns	= 0x0000,
	_FRows		= 0x0004,
	_FRowPitch	= 0x0008,
	_FFormat	= 0x000C,
	_FAddress	= 0x0010,

	_DWidth		= 0x0000,
	_DHeight	= 0x0004,
	_DVirtX		= 0x0008,
	_DVirtY		= 0x000C,
	_DFrame		= 0x0010,
};
