/* -*- mode: c; c-basic-offset: 8; -*-
 * AN30183A SubPMIC core driver
 * Copyright(C) 2012 Panasonic Mobile Communications R&D Lab. Co.,Ltd.
 * 
 *
 * This driver provides the core functions of AN30183A.
 * Implemented as mfd(Multi Function Driver).
 */
#if 0
#define PMC_SUBPMIC_DEBUG
#endif

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c/subpmic-core.h>

#ifdef PMC_SUBPMIC_DEBUG
#define DEBUG_PRINT(arg, ...)    printk(KERN_INFO "[SubPMIC]:" arg, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(arg, ...)
#endif

#define PROC_DEVICE "subpmic-i2c"
#define DRIVER_NAME  "subpmic_i2c_driver"

static DEFINE_MUTEX(subpmic_lock);
static struct i2c_client *subpmic_client;

/** 
* @brief  I2C�o�R�f�[�^�������ݏ���
* @param  *sub_addr �T�u�A�h���X(�������݃��W�X�^�A�h���X)
* @param  *value    �ݒ�l
* @param  num_bytes �ݒ�l�f�[�^��
* @retval 0 �������ݐ���
* @retval 0�ȉ� �������ݎ��s
*/
int subpmic_i2c_write(u8 sub_addr, u8 *value, unsigned num_bytes)
{
	int ret;
	struct i2c_msg msg[1];
	u8 *buff;

	/* Parameter null check */
	if( !value )
	{
		DEBUG_PRINT("%s  value is null\n", __FUNCTION__ );
		return -EIO;
	}

	buff = kzalloc( (sizeof(u8)*(num_bytes+1)), GFP_KERNEL );
	if( !buff )
	{
		return -EIO;
	}

  if(subpmic_client == NULL ){
		kfree(buff);
		DEBUG_PRINT("%s: subpmic_client is NULL.\n",__func__);
		return -EIO;
	}

    /* slave addr */
	/** @brief 1-1 �X���[�u�A�h���X�ݒ� - 0x6E�Œ� */
	msg[0].addr = subpmic_client->addr;
	/* ���M�o�C�g�� */
	/** @brief 1-2 �ݒ�l�o�C�g���ݒ� 1�Œ�*/
	msg[0].len =  num_bytes+1;
	/* �ǂݍ��݃t���O OFF */
	/** @brief 1-3 �ǂݍ��݃t���O�ݒ� OFF */
	msg[0].flags = 0;
	/* subaddress ���䃌�W�X�^�A�h���X�ݒ� */
	/** @brief 1-4 �T�u�A�h���X(���W�X�^�A�h���X)�ݒ� */
	msg[0].buf = buff;

	buff[0] = sub_addr;
	memcpy( buff+1, value, num_bytes );

	/* over write the first byte of buffer with the register address */
	/** @brief 3 i2c_transfer�ɂ��i2c�o�R�������� */
	ret = i2c_transfer(subpmic_client->adapter, msg, 1);

	kfree(buff);
	/* i2c_transfer returns number of messages transferred */
	if (ret != 1) {
		DEBUG_PRINT("%s: i2c_write failed to transfer all messages\n",
			DRIVER_NAME);
		if (ret < 0)
			return ret;
		else
			return -EIO;
	} else {
		return 0;
	}
}
EXPORT_SYMBOL(subpmic_i2c_write);

/** 
* @brief  I2C�o�R�f�[�^�ǂݍ��ݏ���
* @param  *sub_addr �T�u�A�h���X(�������݃��W�X�^�A�h���X)
* @param  *value    �ǂݏo���l
* @param  num_bytes �ǂݏo���l�f�[�^��
* @retval 0 �ǂݍ��ݐ���
* @retval 0�ȉ� �ǂݍ��ݎ��s
*/
int subpmic_i2c_read(u8 sub_addr, u8 *value, unsigned num_bytes)
{
	int ret;
	struct i2c_msg msg[2];

	DEBUG_PRINT("%s: >> \n",__func__);

    if( subpmic_client == NULL){
	    DEBUG_PRINT("%s: subpmic_client is NULL.\n",__func__);
		return -EIO;
	}


    /* �T�u�A�h���X���M */
	msg[0].addr = subpmic_client->addr;
	msg[0].len = 1;
	msg[0].flags = 0;			/* Write the register value */
	msg[0].buf = &sub_addr;
	
	/* �f�[�^��M�ݒ� */
	msg[1].addr = subpmic_client->addr;
	msg[1].flags = I2C_M_RD;	/* Read the register value */
	msg[1].len = num_bytes;	/* only n bytes */
	msg[1].buf = value;

	ret = i2c_transfer(subpmic_client->adapter, msg, 2);

	/* i2c_transfer returns number of messages transferred */
	if (ret != 2) {
		DEBUG_PRINT("%s: i2c_read failed to transfer all messages\n",
			DRIVER_NAME);
		if (ret < 0){
	        DEBUG_PRINT("%s: return ret << \n",__func__);
			return ret;
		}else{
	        DEBUG_PRINT("%s: return -EIO << \n",__func__);
			return -EIO;
		}
	} else {
	    DEBUG_PRINT("%s: return 0 << \n",__func__);
		return 0;
	}
}
EXPORT_SYMBOL(subpmic_i2c_read);

/** 
* @brief  I2C�o�R�f�[�^�r�b�g�ݒ菈��
* @param  addr      �T�u�A�h���X(����Ώۃ��W�X�^�A�h���X)
* @param  value     �ݒ�r�b�g�f�[�^
* @param  mode      �r�b�gON/OFF�ݒ胂�[�h
* @retval 0 �r�b�g�ݒ萬��
* @retval 0�ȉ� �r�b�g�ݒ莸�s
*/
int subpmic_i2c_bitset_u8(u8 addr, u8 value, u8 mode)
{
    int ret;
    u8 data;

    if((mode < 0) || (mode >= SUBPMIC_BITSET_NUM)){
        return -EINVAL;
    }

    /* �r������̂��߂�mutex_lock���� */
    mutex_lock(&subpmic_lock);

    ret = subpmic_i2c_read( addr, &data, 1 );
    if(ret == 0){
        if( mode == SUBPMIC_BITSET_ON ){
            data = data | value;
        }else{
            data = data & ~value;
        }

        ret = subpmic_i2c_write( addr, &data, 1 );
        if( ret != 0 ){
            DEBUG_PRINT("%s: subpmic_i2c_bitset_u8 : i2c_write failed.\n", DRIVER_NAME);
        }
    }else{
            DEBUG_PRINT("%s: subpmic_i2c_bitset_u8 : i2c_read failed.\n", DRIVER_NAME);
    }
 
	mutex_unlock(&subpmic_lock);
    return ret;
}
EXPORT_SYMBOL_GPL(subpmic_i2c_bitset_u8);

/** 
* @brief  SubPMIC probe����
* @param  *client I2C�X���[�u�f�o�C�X���
* @param  *id     �f�o�C�X�쐬�p�e���v���[�g���
* @retval 0 ���폈��
* @retval 0�ȊO �ُ폈��
*/
static int subpmic_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
#if 0
    struct i2c_adapter *adapter;

    DEBUG_PRINT("%s: >>\n",__func__);

    adapter = i2c_get_adapter(7);
	if(adapter == NULL){
	    DEBUG_PRINT("can't get i2c adapter\n");
		return -ENODEV;
	}
	DEBUG_PRINT("%s i2c_add_driver OK. adapter[0x%08x].\n",__func__,(int)adapter);

	if (!client) {
		dev_dbg(&client->dev, "client is null.\n");
        DEBUG_PRINT("%s: -EINVAL <<\n",__func__);
		return -EINVAL;
	}

#else
    int ret = 0;

    DEBUG_PRINT("%s: >>\n",__func__);
    if(i2c_check_functionality(client->adapter, I2C_FUNC_I2C)){
	    subpmic_client = client;
        DEBUG_PRINT("%s: client->addr = %08x <<\n",__func__,client->addr);
	}else{
	    DEBUG_PRINT("%s: i2c_check_functionality() false\n",__func__);
		ret = -ENOTSUPP;
	}

#endif

#if 0
    DEBUG_PRINT("%s: client->addr = %08x <<\n",__func__,client->addr);

    client->adapter = adapter;
	client->addr = 0x6E;
    client->adapter->timeout = 0;
	client->adapter->retries = 5;

	subpmic_client = client;
    DEBUG_PRINT("%s: <<\n",__func__);
	return 0;
#else

    DEBUG_PRINT("%s: <<\n",__func__);
    return ret;

#endif
}

/** 
* @brief  SubPMIC remove����
* @param  *client I2C�X���[�u�f�o�C�X���
* @retval 0 ���폈��
* @retval 0�ȊO �ُ폈��
*/
static int subpmic_i2c_remove(struct i2c_client *client)
{
	i2c_unregister_device(client);
	return 0;
}

static struct i2c_device_id subpmic_id[] = {
	{ "subpmic", 0 },
	{ },
};

static struct i2c_driver subpmic_driver = {
	.driver.name	= "subpmic",
	.probe		= subpmic_i2c_probe,
	.remove		= subpmic_i2c_remove,
	.id_table	= subpmic_id,
};
/** 
* @brief  SubPMIC init����
* @retval 0 ���폈��
* @retval 0�ȊO �ُ폈��
*/
static int __init subpmic_init(void)
{
    DEBUG_PRINT("%s: >>\n",__func__);
	return i2c_add_driver(&subpmic_driver);
}
//postcore_initcall(subpmic_init);

/** 
* @brief  SubPMIC exit����
* @retval 0 ���폈��
* @retval 0�ȊO �ُ폈��
*/
static void __exit subpmic_exit(void)
{
	i2c_del_driver(&subpmic_driver);
}
//module_exit(subpmic_exit);

subsys_initcall(subpmic_init);
module_exit(subpmic_exit);
MODULE_LICENSE("GPL");

//MODULE_LICENSE("GPL v2");
//MODULE_DESCRIPTION("SubPMIC core driver");
//MODULE_VERSION("1.0");

