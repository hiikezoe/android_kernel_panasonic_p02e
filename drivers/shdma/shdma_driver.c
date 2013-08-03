/**
 * @file  shdma_driver.c
 * @brief MM放送用DMAドライバ制御関数
 */
/*----------------------------------------------------------------------------
  Debug
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Include File
----------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/ion.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/semaphore.h>

#include <mach/hardware.h>

#include "shdma_driver.h"

#include <linux/dma-mapping.h>	/* DMA_TO_DEVICE */
/*----------------------------------------------------------------------------
  Macro Definition
----------------------------------------------------------------------------*/
/**
 * @brief ドライバ名
 */
#define D_SHDMA_DRV_NAME           "shdma" /**< driver name */

/*----------------------------------------------------------------------------
  Type Definition
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Prototype Declaration
----------------------------------------------------------------------------*/
static int shdma_open(struct inode *, struct file  *);
static ssize_t shdma_write( struct file *file, const char __user *buffer, size_t count, loff_t *f_pos );
static int shdma_close(struct inode *, struct file  *);
static void shdma_complete_func( struct msm_dmov_cmd *_cmd,
				unsigned int result,
				struct msm_dmov_errdata *err);
/*----------------------------------------------------------------------------
  Global Variable
----------------------------------------------------------------------------*/
/**
 * @brief ドライバメジャー番号
 */
int    shdma_major;

/**
 * @brief fops
 */
const struct file_operations shdma_fops = {
	.owner =	THIS_MODULE,	/**< モジュール */
	.write =	shdma_write,	/**< write関数 */
	.open =		shdma_open,	/**< open関数 */
	.release =	shdma_close	/**< release関数 */
};
/**
 * @brief DMA転送情報構造体
 */
struct shdma_tiler_copy_info_t tci[1];

/*----------------------------------------------------------------------------
  Static Variable
----------------------------------------------------------------------------*/
/**
 * @brief ドライバ属性
 */
static DEVICE_ATTR(name, S_IRUGO, NULL, NULL);

/**
 * @brief クラス変数
 */
static struct class  *shdma_dev_class;

/**
 * @brief デバイス変数
 */
static struct device *shdma_dev_device;

/**
 * @brief DMA転送完了資源
 */
atomic_t atomic_shdma;		/* include/linux/types.h */

/**
 * @brief wait queue変数
 */
wait_queue_head_t wq;

/**
 * @brief ドライバopen用資源
 */
static struct semaphore open_sem;

/**
 * @brief ドライバwrite用資源
 */
static struct semaphore write_sem;
/*----------------------------------------------------------------------------
  External Reference
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Global Function
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Static Function
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Public Function
----------------------------------------------------------------------------*/
/**
 * @brief ドライバオープン関数
 *
 * @param[in] *inode : 識別番号
 * @param[in] *file  : ファイル
 * @retval D_SHDMA_RET_OK : 正常終了
 * @retval D_SHDMA_RET_NG : 異常終了
 * @retval EBUSY          : ドライバ使用中
 * @exception なし
 * @see       なし
 */
static int shdma_open(
	struct inode *inode,
	struct file  *file )
{
	int ret = D_SHDMA_RET_OK;

	/** <ol><li>処理開始 */
	SHDMA_DEBUG_MSG_ENTER(0, 0, 0);

	/** <li>inode,file NULLチェック */
	if( inode == NULL ){
		printk("***ERROR: SHDMA open inode = NULL\n");
		return D_SHDMA_RET_NG;
	}
	if( file == NULL ){
		printk("***ERROR: SHDMA open file = NULL\n");
		return D_SHDMA_RET_NG;
	}

	/** <li>ドライバopen用セマフォ獲得 */
	ret = down_trylock( &open_sem );
	if( ret != 0 ){
		printk("***ERROR: Acquisition falut open semaphore\n");
		return -EBUSY;
	}

	/** <li>wait event識別変数を初期化する */
	init_waitqueue_head( &wq );

	SHDMA_DEBUG_MSG_EXIT();
	/** <li>処理終了</ol> */

	return D_SHDMA_RET_OK;
}
/**
 * @brief ドライバクローズ関数
 *
 * @param[in] *inode : 識別番号
 * @param[in] *file  : ファイル
 * @retval D_SHDMA_RET_OK : 正常終了
 * @retval D_SHDMA_RET_NG : 異常終了
 * @exception なし
 * @see       なし
 */
static int shdma_close(
	struct inode *inode,
	struct file  *file )
{
	/** <ol><li>処理開始 */
	SHDMA_DEBUG_MSG_ENTER(0, 0, 0);

	/** <li>inode,file NULLチェック */
	if( inode == NULL ){
		printk("***ERROR: SHDMA close inode = NULL\n");
		return D_SHDMA_RET_NG;
	}
	if( file == NULL ){
		printk("***ERROR: SHDMA close file = NULL\n");
		return D_SHDMA_RET_NG;
	}

	/** <li>ドライバopenセマフォ解放 */
	up( &open_sem );

	SHDMA_DEBUG_MSG_EXIT();
	/** <li>処理終了</ol> */

	return D_SHDMA_RET_OK;
}

/**
 * @brief DMA転送完了通知関数
 *
 * @param[in] _cmd   : DMA転送制御構造体
 * @param[in] result : DMA転送結果
 * @param[in] err    : DMA転送エラー情報構造体
 * @retval なし
 * @exception なし
 * @see       なし
 */
static void shdma_complete_func(
	struct msm_dmov_cmd *_cmd,
	unsigned int result,
	struct msm_dmov_errdata *err)
{
	struct shdma_dmov_exec_cmdptr_cmd *cmd = container_of(_cmd, struct shdma_dmov_exec_cmdptr_cmd, dmov_cmd);

	/** <ol><li>処理開始 */
	SHDMA_DEBUG_MSG_ENTER(0, 0, 0);

	/** <li>DMA転送結果をDMA転送制御構造体へコピー */
	cmd->result = result;

	/** <li>DMA転送結果確認 */
	/** <ol><li>DMA転送エラー構造体にデータがある。かつ、DMA転送結果がエラーの場合、エラー情報をDMA転送制御構造体へコピー */
	if(( result != D_SHDMA_DMOV_RESULT_OK ) && err ){
		printk("***ERROR: SHDMA transfer NG  result = 0x%08x\n", result );
		memcpy( &cmd->err, err, sizeof( struct msm_dmov_errdata ));
	}
	/** </ol> */
	/** <li>DMA転送完了資源を-1する。その結果 DMA転送完了資源が0の場合 wake_up する。0ではない場合はなにもしない */
	if( atomic_dec_and_test( &atomic_shdma )){
		wake_up( &wq );
	} else {
		SHDMA_DEBUG_MSG_INFO("***exist atomic_shdma = 0x%x\n", atomic_shdma.counter );
	}

	SHDMA_DEBUG_MSG_EXIT();
	/** <li>処理終了</ol>*/
}

/**
 * @brief ドライバwrite関数
 *
 * @param[in] *file   : ファイル
 * @param[in] *buffer : 書き込みパラメータ
 * @param[in] count   : 書き込みサイズ
 * @param[in] *f_pos  : ファイルのRW位置
 * @retval write_size     : 正常終了 書き込みサイズ
 * @retval EFAULT         : 異常終了 アドレス不正
 * @retval ENOENT         : 異常終了 ファイルorディレクトリがない
 * @retval ENOSPC         : 異常終了 デバイスに空領域がない
 * @retval EINVAL         : 異常終了 引数なし
 * @retval D_SHDMA_RET_NG : 異常終了 エラー
 * @exception なし
 * @see       なし
 */
static ssize_t shdma_write(
    struct file    *file,
    const char __user    *buffer,
    size_t         count,
    loff_t         *f_pos )
{
	int ret = D_SHDMA_RET_OK;
	unsigned int i,j;
	int err = D_SHDMA_RET_OK;
	int result_chk = D_SHDMA_RET_OK;
	struct vm_area_struct *vma;
	unsigned long pfn = 0;
	ion_phys_addr_t src_phys = 0;
	unsigned long dst_phys = 0;
	size_t src_len;
	unsigned long trans_size = 0;
	unsigned long shdma_trans_num_rows = 0;
	unsigned long dma_trans_num_rows = 0;
	unsigned long dma_trans_num_rows_rem = 0;
	unsigned addr_offset = 0;
	struct ion_handle *shdma_src_handle;
	struct shdma_dmov_exec_cmdptr_cmd cmd[3];
	struct shdma_command_t shdma_cmd[D_SHDMA_CHANNEL_MAX];
	unsigned int id[D_SHDMA_CHANNEL_MAX] = { DMOV_SHDMA_CH1, DMOV_SHDMA_CH2, DMOV_SHDMA_CH3 };
	unsigned long width_yuv = 0;
	unsigned long height_y = 0;
	unsigned long height_uv = 0;
	unsigned long ysize_align = 0;
	unsigned long uvsize_align = 0;
	int ion_ret = 0;


	/** <ol><li>処理開始 */
	SHDMA_DEBUG_MSG_ENTER(0, 0, 0);

	/** <li> ドライバwriteセマフォ獲得*/
	down( &write_sem );

	/** <li>初期化処理 */
	/** <ol><li>引数NULLチェック */
	if( file == NULL || buffer == NULL || count <= 0 || f_pos == NULL ){
		printk("***ERROR: argument NULL    file = %p  buffer = %p  count = 0x%x  f_pos = %p\n", file, buffer, count, f_pos );
		up( &write_sem );
		return -EINVAL;
	}

	/** <li>上位からのパラメータをコピーする */
	if (copy_from_user(&tci, buffer, sizeof(tci))){
		printk("***ERROR: fault copy write data parameter.\n" );
		up( &write_sem );
		return -EFAULT;
	}

	/** <li>転送元、転送先アドレスNULLチェック */
	if( tci[0].dst_handle == NULL || tci[0].src_handle == NULL ){
		printk("***ERROR: fault transfer address NULL   src = %p  dst = %p\n", tci[0].src_handle, tci[0].dst_handle );
		up( &write_sem );
		return -EINVAL;
	}

	/** <li>転送幅、高さチェック */
	if(( tci[0].height < D_SHDMA_CHANNEL_MAX ) || ( tci[0].src_stride == 0  )){
		printk("***ERROR: argument ERROR   height = %d  width = %ld\n", tci[0].height, tci[0].src_stride );
		up( &write_sem );		
		return -EINVAL;
	}
	if(( tci[0].src_stride % D_SHDMA_ODD_CHECK ) != 0 ){	/* widthが奇数の場合はありえないためNGを返す */
		printk("***ERROR: argument ERROR width is odd number   width = %ld\n", tci[0].src_stride );
		up( &write_sem );
		return -EINVAL;
	}

	/** <li>内部変数の初期化をする */
	memset( &cmd, 0, sizeof(struct shdma_dmov_exec_cmdptr_cmd) * 3 );
	for( i = 0; i < D_SHDMA_CHANNEL_MAX; i++ ){
		memset( &shdma_cmd[i], 0, sizeof(struct shdma_command_t));
	}
	/** </ol>*/

	/** <li>物理アドレス取得 */
	/** <ol><li>転送元物理アドレス取得 */
	shdma_src_handle = (struct ion_handle *)tci[0].src_handle;
	ion_ret = ion_phys( shdma_src_handle->client, shdma_src_handle, &src_phys, &src_len);
	if( src_phys == 0 || src_len < 1 || ion_ret < 0 ){
		printk("***ERROR: get src_phys falut.\n");
		up( &write_sem );
		return -EFAULT;
	}

	/** <li>転送先物理アドレス取得 */
	vma = find_vma( current->mm, (unsigned int )tci[0].dst_handle );
	if( vma == NULL ){
		printk("***ERROR: get vma falut.\n");
		up( &write_sem );
		return -ENOENT;
	}
	follow_pfn( vma, (unsigned int)tci[0].dst_handle, &pfn );
	dst_phys = __pfn_to_phys( pfn );
	/** </ol> */

	/** <li>DMA転送用パラメータバッファ獲得 */
	for( i = 0; i < D_SHDMA_CHANNEL_MAX; i++ ){	/** <ol><li>DMAチャネル分獲得する */
		/** <ol><li>DMA転送用パラメータ領域獲得 */
		shdma_cmd[i].cmd_ptr = kmalloc(sizeof(dmov_box), GFP_KERNEL | __GFP_DMA);
		if( shdma_cmd[i].cmd_ptr == NULL ){
			printk("***ERROR: falut allocate buffer cmd_ptr  num = 0x%x .\n" , i);
			if( i != 0 ){
				for( j = 0; j < (i - 1); j++ ){
					kfree(shdma_cmd[j].cmd_ptr);
				}
			}
			up( &write_sem );
			return -ENOSPC;
		}
	}
	for( i = 0; i < D_SHDMA_CHANNEL_MAX; i++ ){
		/** <li>DMA転送用パラメータ先頭アドレス領域獲得 */
		shdma_cmd[i].cmd_ptr_ptr = kmalloc(sizeof(u32), GFP_KERNEL | __GFP_DMA);
		if( shdma_cmd[i].cmd_ptr_ptr == NULL ){
			printk("***ERROR: falut allocate buffer cmd_ptr_ptr  num = 0x%x .\n" , i);
			if( i != 0 ){
				for( j = 0; j < (i - 1); j++ ){
					kfree(shdma_cmd[j].cmd_ptr_ptr);
				}
			}
			for( j = 0; j < D_SHDMA_CHANNEL_MAX; j++ ){
				kfree(shdma_cmd[j].cmd_ptr);
			}
			up( &write_sem );
			return -ENOSPC;
		}
	}
	/** </ol></ol> */

	/** <li>転送サイズ計算 */
	/** <li>アライメント調整 */
	if(( tci[0].src_stride % D_SHDMA_ALIGN_128 ) != 0 ){	/*Y領域、UV領域幅アライメント調整*/
		width_yuv = ((( tci[0].src_stride /
				D_SHDMA_ALIGN_128 ) +
				D_SHDMA_ALIGN_ADJUST ) *
				D_SHDMA_ALIGN_128 );		/*128バイトでアライメント*/
	} else {
		width_yuv = tci[0].src_stride;
	}

	if(( tci[0].height % D_SHDMA_ALIGN_32 ) != 0 ){		/*Y領域高さアライメント調整*/
		height_y = ((( tci[0].height /
				D_SHDMA_ALIGN_32 ) +
				D_SHDMA_ALIGN_ADJUST ) *
				D_SHDMA_ALIGN_32 );		/*32バイトでアライメント*/
	} else {
		height_y = tci[0].height;
	}

	if((( tci[0].height / D_SHDMA_ALIGN_HEIGHT_UV ) %
			D_SHDMA_ALIGN_32 ) != 0 ){		/*UV領域高さアライメント調整*/
		height_uv = (((( tci[0].height /
				D_SHDMA_ALIGN_HEIGHT_UV ) /
				D_SHDMA_ALIGN_32 ) +
				D_SHDMA_ALIGN_ADJUST ) *
				D_SHDMA_ALIGN_32 );		/*32バイトでアライメント*/
	} else {
		height_uv = tci[0].height / D_SHDMA_ALIGN_HEIGHT_UV;
	}

	if(( width_yuv * height_y ) % D_SHDMA_ALIGN_8192 ){	/*Y領域のアライメント調整*/
		ysize_align = ((( width_yuv * height_y /
				D_SHDMA_ALIGN_8192 ) +
				D_SHDMA_ALIGN_ADJUST ) *
				D_SHDMA_ALIGN_8192 );		/*8Kバイトでアライメント*/
	} else {
		ysize_align = width_yuv * height_y;
	}

	if(( width_yuv * height_uv ) % D_SHDMA_ALIGN_8192 ){	/*YU領域のアライメント調整*/
		uvsize_align = ((( width_yuv * height_uv /
				D_SHDMA_ALIGN_8192 ) +
				D_SHDMA_ALIGN_ADJUST ) *
				D_SHDMA_ALIGN_8192 );		/*8Kバイトでアライメント*/
	} else {
		uvsize_align = width_yuv * height_uv;
	}

	shdma_trans_num_rows = (( ysize_align + uvsize_align ) /
					D_SHDMA_ALIGN_8192 );		/** <li>DMAbox転送回数はYUV領域を8Kで割った値*/
	trans_size = D_SHDMA_ALIGN_8192;				/** <li>DMA転送1boxサイズは8Kサイズを指定*/
	dma_trans_num_rows = shdma_trans_num_rows / D_SHDMA_CHANNEL_MAX;	/** <li>DMA1面あたりのbox転送回数算出 */
	dma_trans_num_rows_rem = shdma_trans_num_rows % D_SHDMA_CHANNEL_MAX;	/** <li>DMA1面あたりのbox転送回数の余り算出 */
	if( trans_size > D_SHDMA_TRANS_MAX_SIZE ){	/** <li>DMA転送1boxサイズが65535より大きい場合はハード制約で転送できないため、NGを返す */
		printk("***ERROR: Size over for DMA transfer.\n");
		for( i = 0; i < D_SHDMA_CHANNEL_MAX; i++ ){
			kfree(shdma_cmd[i].cmd_ptr);
			kfree(shdma_cmd[i].cmd_ptr_ptr);
		}
		up( &write_sem );
		return -EINVAL;
	}
	/** </ol> */

	/** <li>DMA転送用パラメータ設定 */
	for( i = 0; i < D_SHDMA_CHANNEL_MAX; i++ ){	/** <ol><li>DMAチャネル分設定する */
		if( i == D_SHDMA_CHANNEL_MAX - 1){	/** <ol><li>最後のDMAチャネルの場合転送box数の余りも転送する */
			dma_trans_num_rows += dma_trans_num_rows_rem;
		}
		shdma_cmd[i].cmd_ptr->cmd = CMD_PTR_LP | CMD_MODE_BOX;	/** <li>boxモード転送 */
		shdma_cmd[i].cmd_ptr->src_row_addr = (unsigned int)src_phys + addr_offset;	/** <li>転送元アドレス設定 */
		shdma_cmd[i].cmd_ptr->dst_row_addr = (unsigned int)dst_phys + addr_offset;	/** <li>転送先アドレス設定 */
		shdma_cmd[i].cmd_ptr->src_dst_len =			/** <li>1box転送サイズ設定 */
				(( trans_size & D_SHDMA_PARAM_MASK ) << D_SHDMA_SRC_PARAM_SHIFT ) |
				( trans_size & D_SHDMA_PARAM_MASK );
		shdma_cmd[i].cmd_ptr->num_rows =			/** <li>転送box数設定 */
				(( dma_trans_num_rows & D_SHDMA_PARAM_MASK ) << D_SHDMA_SRC_PARAM_SHIFT ) |
				( dma_trans_num_rows & D_SHDMA_PARAM_MASK );
		shdma_cmd[i].cmd_ptr->row_offset =			/** <li>転送オフセット設定 */
				(( trans_size & D_SHDMA_PARAM_MASK ) << D_SHDMA_SRC_PARAM_SHIFT ) |
				( trans_size & D_SHDMA_PARAM_MASK );
		/** <li>転送アドレスオフセット加算 */
		addr_offset += trans_size * dma_trans_num_rows;
	}
	/** </ol></ol> */

	/** <li>DMA転送用パラメータをマッピングする */
	for( i = 0; i < D_SHDMA_CHANNEL_MAX; i++ ){	/** <ol><li>DMAチャネル分マッピングする */
		/** <ol><li>DMA転送用パラメータ領域の物理アドレスを獲得する */
		shdma_cmd[i].map_cmd = dma_map_single( NULL, shdma_cmd[i].cmd_ptr,
					sizeof(*shdma_cmd[i].cmd_ptr), DMA_TO_DEVICE );
		if( shdma_cmd[i].map_cmd == 0 ){
			printk("***ERROR: falut cmd_ptr mapping.  num = 0x%x\n", i);
			if( i != 0 ){
				for( j = 0; j < (i - 1); j++ ){
					dma_unmap_single( NULL, shdma_cmd[j].map_cmd,
						sizeof(*shdma_cmd[j].cmd_ptr), DMA_TO_DEVICE );
				}
			}
			for( j = 0; j < D_SHDMA_CHANNEL_MAX; j++ ){
				kfree(shdma_cmd[j].cmd_ptr_ptr);
				kfree(shdma_cmd[j].cmd_ptr);
			}
			up( &write_sem );
			return -EFAULT;
		}
		/** <li>DMA転送用パラメータの物理アドレスをDMA転送用パラメータ先頭領域に格納する */
		*shdma_cmd[i].cmd_ptr_ptr = CMD_PTR_ADDR(shdma_cmd[i].map_cmd) | CMD_PTR_LP;
	}
	for( i = 0; i < D_SHDMA_CHANNEL_MAX; i++ ){
		/** <li>DMA転送用パラメータ先頭領域の物理アドレスを獲得する */
		err = shdma_cmd[i].map_cmd_ptr = dma_map_single( NULL, shdma_cmd[i].cmd_ptr_ptr,
					sizeof(*shdma_cmd[i].cmd_ptr_ptr), DMA_TO_DEVICE );
		if( err == 0 ){
			printk("***ERROR: falut cmd_ptr_ptr mapping.  num = 0x%x\n", i);
			if( i != 0 ){
				for( j = 0; j < (i - 1); j++ ){
					dma_unmap_single( NULL, shdma_cmd[j].map_cmd_ptr,
						sizeof(*shdma_cmd[j].cmd_ptr_ptr), DMA_TO_DEVICE );
				}
			}
			for( j = 0; j < D_SHDMA_CHANNEL_MAX; j++ ){
				dma_unmap_single( NULL, shdma_cmd[j].map_cmd,
					sizeof(*shdma_cmd[j].cmd_ptr), DMA_TO_DEVICE );
				kfree(shdma_cmd[j].cmd_ptr_ptr);
				kfree(shdma_cmd[j].cmd_ptr);
			}
			up( &write_sem );
			return -EFAULT;
		}
	}
	/** </ol></ol> */

	/** <li>DMA転送構造体にパラメータを設定する */
	for( i = 0; i < D_SHDMA_CHANNEL_MAX; i++ ){	/** <ol><li>DMAチャネル分設定する */
		cmd[i].dmov_cmd.cmdptr = DMOV_CMD_PTR_LIST | DMOV_CMD_ADDR(shdma_cmd[i].map_cmd_ptr);
		cmd[i].dmov_cmd.complete_func = shdma_complete_func;
		cmd[i].dmov_cmd.exec_func = NULL;
		cmd[i].id = id[i];
		cmd[i].result = 0;
	}
	/** </ol> */

	/** <li>DMA転送完了資源をチャネル数分に設定する */
	atomic_set( &atomic_shdma, D_SHDMA_CHANNEL_MAX );

	/** <li>DMA転送開始関数をコールする */
	for( i = 0; i < D_SHDMA_CHANNEL_MAX; i++ ){
		msm_dmov_enqueue_cmd( cmd[i].id, &cmd[i].dmov_cmd );
	}

	/** <li>DMA転送完了資源が0以下になるまでWaitする */
	wait_event( wq, ( atomic_read( &atomic_shdma ) <= 0 ));

	/** <li>DMA転送結果を確認する*/
	for( i = 0; i < D_SHDMA_CHANNEL_MAX; i++){	/** <ol><li>DMAチャネル分確認する */
		if( cmd[i].result != D_SHDMA_DMOV_RESULT_OK ){	/** <li>DMA転送結果がNGの場合、ログを出力する */
			result_chk = D_SHDMA_RET_NG;
			printk("***ERROR: dma id:%d result:0x%08x \n***flush: 0x%08x 0x%08x 0x%08x 0x%08x\n",
					id[i], cmd[i].result, cmd[i].err.flush[0],
					cmd[i].err.flush[1], cmd[i].err.flush[2], cmd[i].err.flush[3]);
		}
	}
	/** </ol>*/

	/** <li>獲得したメモリを解放する */
	for( i = 0; i < D_SHDMA_CHANNEL_MAX; i++ ){
		dma_unmap_single( NULL, (dma_addr_t)shdma_cmd[i].map_cmd_ptr,
					sizeof(*shdma_cmd[i].cmd_ptr_ptr), DMA_TO_DEVICE );
		dma_unmap_single( NULL, shdma_cmd[i].map_cmd,
					sizeof(*shdma_cmd[i].cmd_ptr), DMA_TO_DEVICE );
		kfree(shdma_cmd[i].cmd_ptr_ptr);
		kfree(shdma_cmd[i].cmd_ptr);
	}

	/** <li>DMA転送結果を返す */
	if( result_chk == 0 ){
		ret = count;
	} else {
		ret = result_chk;
	}

	/** <li>ドライバwriteセマフォ解放 */
	up( &write_sem );

	SHDMA_DEBUG_MSG_EXIT();
	/** <li>処理終了</ol>*/

	return ret;
}

/**
 * @brief ドライバ初期化関数
 *
 * @param[in] なし
 * @retval D_SHDMA_RET_OK : 正常終了
 * @retval EIO            : ドライバ初期化エラー
 * @retval D_SHDMA_RET_NG : ドライバ名取得失敗
 * @exception なし
 * @see       なし
 */
int shdma_init_module(void)
{
	int ret = 0;

	/** <ol><li>処理開始*/
	SHDMA_DEBUG_MSG_ENTER(0, 0, 0);

	/** <li>ドライバのアサイン */
	ret = register_chrdev(0,                     /* auto asign mode */
			D_SHDMA_DRV_NAME,
			&shdma_fops);

	/** <ol><li>アサイン失敗時はエラー終了 */
	if( ret < 0 ){
		printk("***ERROR: asign falut.\n" );
		return -1;
	}
	/** </ol> */
	shdma_major = ret;

	/** <li>クラス作成 */
	shdma_dev_class = class_create(THIS_MODULE, D_SHDMA_DRV_NAME);
	/** <ol><li>クラス作成失敗時はエラー終了 */
	if (IS_ERR(shdma_dev_class))
	{
		unregister_chrdev( shdma_major, D_SHDMA_DRV_NAME );
		printk("***ERROR: fault class create.\n" );
		return -EIO;
	}
	/** </ol> */

	/** <li>ドライバ作成 */
	shdma_dev_device = device_create(shdma_dev_class, NULL, MKDEV(shdma_major, 0), NULL, D_SHDMA_DRV_NAME);
	/** <ol><li>ドライバ作成失敗時はエラー終了 */
	if(IS_ERR(shdma_dev_device))
	{
		class_destroy( shdma_dev_class );
		unregister_chrdev( shdma_major, D_SHDMA_DRV_NAME );
		printk("***ERROR: fault device create.\n" );
		return -EIO;
	}
	/** </ol> */
	/** <li>ドライバファイル作成 */
	ret = device_create_file(shdma_dev_device, &dev_attr_name);
	/** <ol><li>ドライバファイル作成失敗時はエラー終了 */
	if( ret )
	{
		device_destroy( shdma_dev_class, MKDEV(shdma_major, 0));
		class_destroy( shdma_dev_class );
		unregister_chrdev( shdma_major, D_SHDMA_DRV_NAME );
		printk("***ERROR: fault device create file.\n" );
		return -EIO;
	}
	/** </ol>*/

	/** <li>セマフォ初期化 */
	sema_init( &open_sem, 1 );
	sema_init( &write_sem, 1 );

	SHDMA_DEBUG_MSG_EXIT();
	/** <li>処理終了</ol>*/

	return 0;
}

/**
 * @brief ドライバ削除関数
 *
 * @param[in] なし
 * @retval    なし
 * @exception なし
 * @see       なし
 */
void shdma_cleanup_module(void)
{
	/** <ol><li>処理開始*/
	SHDMA_DEBUG_MSG_ENTER(0, 0, 0);

	/** <li>ドライバファイル削除 */
	device_remove_file( shdma_dev_device, &dev_attr_name );
	/** <li>ドライバ削除 */
	device_destroy( shdma_dev_class, MKDEV(shdma_major, 0));
	/** <li>ドライバクラス削除 */
	class_destroy( shdma_dev_class );
	/** <li>ドライバアサイン解除 */
	unregister_chrdev( shdma_major, D_SHDMA_DRV_NAME );
	shdma_major = 0;

	SHDMA_DEBUG_MSG_EXIT();
	/** <li>処理終了</ol>*/
}

module_init(shdma_init_module);
module_exit(shdma_cleanup_module);
MODULE_DESCRIPTION("shdma driver");
MODULE_LICENSE("GPL");
