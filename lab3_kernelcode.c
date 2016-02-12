
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
//#include <linux/asm/mmap.h>

#include <linux/errno.h>
#include "lab3_def.h"

#define pci_vendor_ids_CCORSI 0x1234
#define PCI_DEVICE_ID_CCORCI_KYOUKO3 0x1113
#define Device_RAM 0x0020

MODULE_LICENSE("GPL");//("Proprietary");
MODULE_AUTHOR("Shreya Deodhar");
long kyouko3_ioctl(struct file *fp,unsigned int cmd, unsigned long arg);
void fifo_flush(void);

//struct cdev mychar_dev;
struct cdev kyouko3_cdev;
struct FIFO_entry
{
	u32 command;
	u32 value;
};

struct fifo_struct
{
	u64 p_base;
	struct FIFO_entry *k_base;
	u32 head;
	u32 tail_cache;
};

struct kyouko3{

	unsigned long p_control_base;
	unsigned int  *k_control_base;
	unsigned long p_card_ram;
	unsigned int  *k_card_ram_base;
	unsigned long len_ctrl;
	unsigned long len_ram;
	struct pci_dev *dev;
	struct fifo_struct fifo;
	unsigned int graphics_on;
}kyouko3;

unsigned int K_READ_REG( unsigned int reg)
{
	unsigned int value;
//	udelay(1);
	rmb();
	value = *(kyouko3.k_control_base + (reg>>2));
	return(value);
}

void K_WRITE_REG(unsigned int reg,unsigned int val)
{
//	udelay(1);
	*(kyouko3.k_control_base+(reg>>2)) = val;
//	return 0;
}

unsigned int init_fifo(void)
{
	kyouko3.fifo.k_base = pci_alloc_consistent(kyouko3.dev,8192u,(dma_addr_t*)&kyouko3.fifo.p_base);
	//load FIFOStart with kyouko3.fifo.p_base
	K_WRITE_REG(FifoStart,kyouko3.fifo.p_base); 
	//load FIFOEnd with kyouko3.fifo.p_base +8192
	K_WRITE_REG(FifoEnd,(kyouko3.fifo.p_base) + 8192u);

	kyouko3.fifo.head = 0;
	kyouko3.fifo.tail_cache = 0;

//pause: till fifo init
//	while( K_READ_REG(FIFOTail)!=0)	
//	schedule();

	return 0;
}

int kyouko3_open(struct inode *inode, struct file *fp)
{
	unsigned size_ram;
//	printk(KERN_ALERT " kyouko3 : Opening");
	printk(" kyouko3 : Opening");
		
	kyouko3.k_control_base = ioremap(kyouko3.p_control_base,65536);//kyouko3.len_ctrl );
	size_ram = K_READ_REG(Device_RAM);
	size_ram = size_ram * 1024* 1024;
	kyouko3.k_card_ram_base = ioremap(kyouko3.p_card_ram,size_ram);
	init_fifo();
	return 0;
}

int kyouko3_release( struct inode *inode, struct file *fp)
{
	//free_irq(kyouko3.dev->irq,&kyouko3);
	//pci_disable_msi(kyouko3.dev);
	kyouko3_ioctl(fp,VMODE,GRAPHICS_OFF);
	fifo_flush();
	iounmap(kyouko3.k_control_base);
	iounmap(kyouko3.k_card_ram_base);	
    	pci_free_consistent(kyouko3.dev,8192u,kyouko3.fifo.k_base,*((dma_addr_t*)&kyouko3.fifo.p_base));
	//pci_clear_master(kyouko3.dev);
	//pci_disable_device(kyouko3.dev);
	printk(KERN_ALERT " kyouko3 : BUUH BYE\n");	
	return 0;
}

int kyouko3_mmap(struct file *fp,struct vm_area_struct *vma)
{
	int ret=0;
	switch(vma->vm_pgoff<<PAGE_SHIFT)
	{
		case 0: //control
		ret = io_remap_pfn_range(vma, vma->vm_start, kyouko3.p_control_base>>PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot);
		break;

		case 0x80000000: 
			// Custom offset provided by user; must be above memory capacity of system to be acknokedged as IO addr
		ret = io_remap_pfn_range(vma, vma->vm_start, kyouko3.p_card_ram>>PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot);
		
		break;

		default:
		printk(KERN_EMERG "Malfunction: mmap default called");
		break;
	}
		return(ret);
	
}

/*struct file_operations kyouko3_fops = 
{
	.open = kyouko3_open,
	.release = kyouko3_release,
	.owner = THIS_MODULE
};*/

//fifo_write

void fifo_write(unsigned int reg, unsigned int value)
{
	kyouko3.fifo.k_base[kyouko3.fifo.head].command=reg;
	kyouko3.fifo.k_base[kyouko3.fifo.head].value=value;
	kyouko3.fifo.head++;
	if(kyouko3.fifo.head >= 1024)
	{
		kyouko3.fifo.head = 0;
	}
}
//fifo write float
void fifo_write_F(unsigned int reg, float value)
{
	kyouko3.fifo.k_base[kyouko3.fifo.head].command=reg;
	kyouko3.fifo.k_base[kyouko3.fifo.head].value=*(unsigned int *)&value;
	kyouko3.fifo.head++;
	if(kyouko3.fifo.head >= 1024)
	{
		kyouko3.fifo.head = 0;
	}
}

/*unsigned int wait_on_fifo(void)
{
//	K_WRITE_REG(FIFOHead,kyouko3.fifo.head);

	while(kyouko3.fifo.tail_cache != kyouko3.fifo.head)
	{
		kyouko3.fifo.tail_cache = K_READ_REG(FIFOTail);
		schedule();
	}
	// You may wanna make it a separate function -done
	return 1;
}*/

void fifo_flush(void)
{
	K_WRITE_REG(FIFOHead,kyouko3.fifo.head);
//	wait_on_fifo(); 
	while(kyouko3.fifo.tail_cache != kyouko3.fifo.head)
	{
		kyouko3.fifo.tail_cache = K_READ_REG(FIFOTail);
		schedule();
	}
	// You may wanna make it a separate function -done
	return 1;
//	FIFO_FLUSH: Mirror the fifo in in driver. 
};

// FIFO_QUEUE :
/*
void fifo_queue(unsigned int arg)
{
//Pull the fifo entry from user space and add to the driver buffer.
// pull the entry from user space and add to 
// driver buffer
//User ioctl

int ret;
ret = copy_from_user((void *)&kyouko3.fifo.k_base, (struct FIFO_entry *) arg, sizeof(Fifo_entry));
if(ret)
	printk(KERN_EMERG "fifo_queue copy_from_user malfunction!");
fifo_write(Fifo_entry.command, Fifo_entry.value);
}
*/


long kyouko3_ioctl(struct file *fp,unsigned int cmd, unsigned long arg)
{
//	float var = 1.0f;
//	unsigned int int_var = 0,ret_val =0;
	int ret;
	struct FIFO_entry cur_entry;
	
	switch(cmd)
	{
		case VMODE:	
			if(arg == GRAPHICS_ON)
			{
				printk(KERN_ALERT "IN graphics on\n");
				K_WRITE_REG(0x8000+ _FColumns,1024);
				K_WRITE_REG(0x8000+_FRows,768);
				K_WRITE_REG(0x8000+_FRowPitch,1024*4);
				K_WRITE_REG(0x8000+_FFormat,0xf888);
				K_WRITE_REG(0x8000+_FAddress,0);

				K_WRITE_REG(0x9000+_DWidth,1024);
				K_WRITE_REG(0x9000+_DHeight,768);
				K_WRITE_REG(0x9000+_DVirtX,0);
				K_WRITE_REG(0x9000+_DVirtY,0);
				K_WRITE_REG(0x9000+_DFrame,0);
				K_WRITE_REG(Accl,0x40000000);
				
				
				K_WRITE_REG(ModeSet,0);
				udelay(10);
				fifo_write(Clear_Color,0x3F000000);//load clear color
				fifo_write(Clear_Color+4,0x3F000000);
				fifo_write(Clear_Color+8,0x3F000000);
				fifo_write(Clear_Color+12,0x3F800000);
				fifo_write(Clear_Buffer,0x03);

				fifo_write(Flush,0x00);
				udelay(10);
				fifo_flush();
				
				printk(KERN_ALERT "IN graphics on debug2\n");
				kyouko3.graphics_on = 1;
			}
			else
			{
				kyouko3.graphics_on = 0;
				//K_WRITE_REG(Flush,0x00);
				//fifo_write(Flush,0x00);
				fifo_flush();
				//K_WRITE_REG(Reboot,1);
				K_WRITE_REG(Accl,0x80000000);
				K_WRITE_REG(ModeSet,0);
			}
			break;
		case FIFO_QUEUE:
			//fifo_queue(arg);
//			int ret;
			ret = copy_from_user((void*)&cur_entry,(struct FIFO_entry*) arg,sizeof(struct FIFO_entry));
			if(ret)
				printk(KERN_EMERG "fifo_queue copy_from_user malfunction!");
			fifo_write(cur_entry.command, cur_entry.value);
		break;

		case FIFO_FLUSH:
			fifo_flush();
		break;

		default:
		break;
	}
	return 0;
}

int kyouko3_probe(struct pci_dev *pci_dev, const struct pci_device_id *pci_id)
{
	kyouko3.p_control_base = pci_resource_start(pci_dev,1);
	kyouko3.p_card_ram = pci_resource_start(pci_dev,2);

	kyouko3.len_ctrl = pci_resource_len(pci_dev,1);
	kyouko3.len_ram = pci_resource_len(pci_dev,2);
	kyouko3.dev = pci_dev;
	/*if(pci_enable_device(pci_dev))
	{
		printk(KERN_ALERT "Error probe\n");
	}
	pci_set_master(pci_dev);*/

	return 0;
}

struct file_operations kyouko3_fops = {
	.open = kyouko3_open,
	.release = kyouko3_release,
	.unlocked_ioctl = kyouko3_ioctl,
	.mmap = kyouko3_mmap,
	.owner = THIS_MODULE
};

struct pci_device_id kyouko3_dev_ids[]={
{

	PCI_DEVICE(pci_vendor_ids_CCORSI,PCI_DEVICE_ID_CCORCI_KYOUKO3)
},{0}
};
 
void kyouko3_remove(struct pci_dev* pci_dev)
{
	pci_disable_device(pci_dev);
}

struct pci_driver kyouko3_pci_dev ={
	.name = "kyouko3",
	.id_table = kyouko3_dev_ids,
	.probe = kyouko3_probe,
	.remove = kyouko3_remove
};

int __init kyouko_init(void)
{
	cdev_init( &kyouko3_cdev, &kyouko3_fops);
	cdev_add(  &kyouko3_cdev, MKDEV(500,127), 1);
	kyouko3_cdev.owner = THIS_MODULE;

	if(pci_register_driver(&kyouko3_pci_dev))
	{
		printk(KERN_ALERT "Error\n");
	}

	pci_enable_device(kyouko3.dev);
	pci_set_master(kyouko3.dev);
	
//	cdev_init( &mychar_dev, &kyouko3_fops);
//	cdev_add(  &mychar_dev, MKDEV(500,127), 1);
	printk(KERN_ALERT "Kyouko3  Initialized");
	return 0;
}

void __exit kyouko_exit(void)
{
	pci_unregister_driver(&kyouko3_pci_dev);
	cdev_del(&kyouko3_cdev);	//mychar_dev);
	printk(KERN_ALERT "Kyouko3 Exiting");
//	return 0;
}

module_exit(kyouko_exit);
module_init(kyouko_init);


