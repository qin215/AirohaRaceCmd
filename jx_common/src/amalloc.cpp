#include "Afxtempl.h" 
#include "ServerCmd.h"
#include "qcloud_iot_export_log.h"
#include "mywin.h"
#include <vector>
#include <algorithm>


using namespace std;


/*
 * 地址分配算法，created by qinjiangwei @2018/11/27
 * 基本思想
 * 简单一维数组记录地址的分配使用信息
 * (A, LEN)
 * A -- 地址
 * LEN -- 连续地址长度
 * 用列表标示整个地址的分配
 */

#define ADDR_USED			1
#define ADDR_NOT_USED		1

#define ADDR_BEGIN_ADDR		0
#define ADDR_NULL_ADDR		0



class AddrItem
{
public:
	kal_uint32 addr;		// 地址
	kal_uint32 len;			// 长度
	kal_uint32 flag;		// 标志
	kal_uint32 id;			// database记录id

	AddrItem()
	{
		addr = 0;
		len = 0;
		flag = ADDR_USED;
		id = 0;
	}

	AddrItem(kal_uint32 a, kal_uint32 Len, kal_uint32 Flag)
	{
		addr = a;
		len = Len;
		flag = Flag;
		id = 0;
	}

	bool check_addr_overlapped(const AddrItem& item) const
	{
		bool ok = true;

		if (addr > item.addr)
		{
			if (item.addr + item.len > addr)
			{
				Log_e("address is overlapped:begin(%d, %d), end(%d, %d)", addr, len, item.addr, item.len);
				ok = false;
			}
		}
		else if (addr < item.addr)
		{
			if (addr + len > item.addr)
			{
				Log_e("address is overlapped:begin(%d, %d), end(%d, %d)", addr, len, item.addr, item.len);
				ok = false;
			}
		}
		else
		{
			Log_d("address left(%d, %d) is equal to right(%d, %d)", addr, len, item.addr, item.len);
			ok = false;
		}

		return ok;
	}

	bool operator < (const AddrItem& rhs) const
	{
		check_addr_overlapped(rhs);
		return (addr + len) < rhs.addr;
	}

	bool operator > (const AddrItem& rhs) const
	{
		check_addr_overlapped(rhs);
		return (addr + len) > rhs.addr;
	}

	void print()
	{
		Log_d("base:%d, len:%d, flag=%d, dbid=%d\n", addr, len, flag, id);
	}
};

typedef vector<AddrItem> AddrVector;

/*
 * 地址的分配列表
 */
class AddressList
{
public:
	UINT64 base_addr;
	kal_uint32 base_len;
	kal_uint32 seg_id;		// 数据库记录ID

	AddrVector list;
};

/*******************************************************************
*作用
       初始化地址列表
*参数
     UINT64 base		-	起始地址
	 kal_uint32 len		-	长度
*返回值
	无
*其它说明
	2018/11/27 by qinjiangwei
********************************************************************/
void * init_address_list(UINT64 base, kal_uint32 len, kal_uint32 seg_id)
{
		Log_d("base=%lld, len=%d, seg_id=%d", base, len, seg_id);
		AddressList *plist = new AddressList;
		if (!plist)
		{
			Log_e("no memory!\n");
			return NULL;
		}

		plist->base_addr = base;
		plist->base_len = len;
		plist->seg_id = seg_id;

		return (void *)plist;
}

/*******************************************************************
*作用
       释放地址列表
*参数
	void *handle		-	handle
*返回值
	无
*其它说明
	2018/11/27 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT void deinit_address_list(void *handle)
{
	delete handle;
}

/*******************************************************************
*作用
       将地址信息加入到列表中，并排序
*参数
     kal_uint32 addr	-	起始地址
	 kal_uint32 len		-	长度
	 kal_uint32 flag	-	使用标志
	 kal_uint32 id		-	数据库记录id
*返回值
	无
*其它说明
	2018/11/27 by qinjiangwei
********************************************************************/
void add_address_list(void *handle, kal_uint32 addr, kal_uint32 len, kal_uint32 flag, kal_uint32 id)
{
	AddressList *list = (AddressList *)handle;
	AddrItem item(addr, len, flag);

	Log_d("handle=%x, addr=%d, len=%d, flag=%d, id=%d", handle, addr, len, flag, id);

	if (id == 0)
	{
		item.id = db_insert_alloc_record(addr, len, flag, list->seg_id);
	}
	else
	{
		item.id = id;
	}

	list->list.push_back(item);
	sort(list->list.begin(), list->list.end());
}


/*******************************************************************
*作用
      分配一段连续的地址空间
*参数
	kal_uint32 len	-	连续的地址长度
*返回值
	0 :	无地址可分配
	> 0: 起始地址
*其它说明
	2018/11/27 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT UINT64 addr_alloc(void *handle, kal_uint32 len)
{
	int i;
	int n = 0;
	AddrItem item;
	UINT64 ret;
	AddressList *plist = (AddressList *)handle;

	Log_d("handle=%x, len=%d", handle, len);

	if (plist->base_addr == 0)
	{
		Log_e("please init the address item!\n");
		return ADDR_NULL_ADDR;
	}

	if (len > plist->base_len)
	{
		Log_e("req len(%d) is too long(%d)\n", len, plist->base_len);
		return ADDR_NULL_ADDR;
	}

	if (plist->list.size() == 0)
	{
		add_address_list((void *)plist, ADDR_BEGIN_ADDR, len, ADDR_USED, 0);
		Log_d("ret addr =%lld", plist->base_addr + ADDR_BEGIN_ADDR);

		return plist->base_addr + ADDR_BEGIN_ADDR;
	}

	item = plist->list[0];
	if (len < item.addr)			// 判断第一个位置是否满足空间要求
	{
		item.addr = 0;
		item.len = len;
		item.flag = ADDR_USED;

		item.id = db_insert_alloc_record(item.addr, item.len, item.flag, plist->seg_id);
		plist->list.insert(plist->list.begin(), item);
		Log_d("ret addr=%lld", plist->base_addr + ADDR_BEGIN_ADDR);
		return plist->base_addr + ADDR_BEGIN_ADDR;
	}
	else if (len == item.addr)
	{
		item.addr = 0;
		item.len += len;
		db_update_alloc_record(item.addr, item.len, item.flag, item.id);
		plist->list[0] = item;
		Log_d("ret addr=%lld", plist->base_addr + ADDR_BEGIN_ADDR);
		return plist->base_addr + ADDR_BEGIN_ADDR;
	}

	for (i = 0; i < plist->list.size() - 1; i++)
	{
		AddrItem next;
		AddrVector::iterator iter;
	
		item = plist->list[i];
		next = plist->list[i + 1];

		if (next.addr - (item.addr + item.len) >= len)			// 找到空闲大小
		{
			ret = item.addr + item.len;
			
			item.len += len;

			if (item.len + item.addr == next.addr)		// 合并长度，删除下一个记录
			{
				item.len += next.len;
				db_update_alloc_record(item.addr, item.len, item.flag, item.id);
				plist->list.erase(plist->list.begin() + i + 1);
				db_delete_alloc_record(next.id);
			}
			else
			{
				db_update_alloc_record(item.addr, item.len, item.flag, item.id);
			}
			
			plist->list[i] = item;
			
			Log_d("ret addr=%lld", ret + plist->base_addr);
			return ret + plist->base_addr;
		}
	}

	item = plist->list[plist->list.size() - 1];
	if (item.addr + item.len + len > plist->base_len)
	{
		Log_e("no free address: last addr(%d), requst(%d), total(%d)", item.addr + item.len, len, plist->base_len);
		return ADDR_NULL_ADDR;
	}

	ret = item.addr + item.len + plist->base_addr;
	item.len += len;
	db_update_alloc_record(item.addr, item.len, item.flag, item.id);
	plist->list[plist->list.size() - 1] = item;

	Log_d("ret addr=%lld", ret);
	return ret;
}

/*******************************************************************
*作用
      释放一段连续的地址空间
*参数
	UINT64 base_addr	-	地址
	int len				-	地址长度
*返回值
	TRUE :	释放成功
	FALSE:  释放失败
*其它说明
	2018/11/27 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT Boolean addr_free(void *handle, UINT64 base_addr, int len)
{
	UINT64 ret;
	AddressList *plist = (AddressList *)handle;
	int i;
	AddrItem item;
	kal_uint32 offset; 
	AddrVector::iterator iter;

	offset = base_addr - plist->base_addr;

	Log_d("handle=%x, base_addr=%lld, offset =%d, len=%d", handle, base_addr, offset, len);

	if (base_addr < plist->base_addr || base_addr > (plist->base_addr + plist->base_len))
	{
		Log_e("address(%lld) beyond, (%d, %d)", base_addr, plist->base_addr, plist->base_len);
		return FALSE;
	}

	for (i = 0; i < plist->list.size(); i++)
	{
		item = plist->list[i];

		if (offset >= item.addr && offset <= item.addr + item.len)
		{
			break;
		}
	}

	if (i == plist->list.size())
	{
		Log_e("not alloc(%d)", offset);
		return FALSE;
	}

	if (offset == item.addr) 
	{
		if (len == item.len)
		{
			iter = plist->list.begin() + i;
			plist->list.erase(iter);
			db_delete_alloc_record(item.id);

			return TRUE;
		}
		else if (len < item.len)
		{
			item.addr += len;
			item.len -= len;

			plist->list[i] = item;
			db_update_alloc_record(item.addr, item.len, item.flag, item.id);

			return TRUE;
		}
		else
		{
			Log_e("free len(%d) greater than record(%d)", len, item.len);
			return FALSE;
		}

	}
	else //if (offset > item.addr)
	{
		if (offset + len > item.addr + item.len)
		{
			Log_e("free len(%d, %d) greater than record(%d, %d)", offset, len, item.addr, item.len);
			return FALSE;
		}

		AddrItem nitem(item.addr, offset - item.addr, ADDR_USED);
		nitem.id = item.id;
		db_update_alloc_record(nitem.addr, nitem.len, nitem.flag, nitem.id);
		plist->list[i] = nitem;

		nitem.addr = offset + len;
		nitem.len = item.len - (offset - item.addr) - len;

		if (nitem.len > 0)
		{
			nitem.id = db_insert_alloc_record(nitem.addr, nitem.len, nitem.flag, plist->seg_id);
			plist->list.insert(plist->list.begin() + i + 1, nitem);
		}

		return TRUE;
	}
}


/*******************************************************************
*作用
      打印地址信息列表
*参数
	无
*返回值
	无
*其它说明
	2018/11/27 by qinjiangwei
********************************************************************/
void print_addr_list(void *handle)
{
	int i;
	AddrItem item;
	AddressList *plist = (AddressList *)handle;

	Log_d("addr list base=%lld, len=%d, alloc nr=%d", plist->base_addr, plist->base_len, plist->list.size());
	for (i = 0; i < plist->list.size(); i++)
	{
		item = plist->list[i];
		item.print();
	}
}

/*******************************************************************
*作用
      测试函数
*参数
	无
*返回值
	无
*其它说明
	2018/11/27 by qinjiangwei
********************************************************************/
FUNC_DLL_EXPORT void amalloc_test()
{
	void *handle;
	UINT64 addr1;
	UINT64 addr2;
	UINT64 addr3;

	handle = db_fetch_mac_allocate_list("abcde");


	addr1 = addr_alloc(handle, 10);
	printf("addr1=%lld\n", addr1);

	print_addr_list(handle);
	addr_free(handle, addr1 + 3, 7);
	print_addr_list(handle);
	addr2 = addr_alloc(handle, 10);
	printf("addr2=%lld\n", addr2);
#if 0
	addr_free(handle, addr2, 512);
	addr3 = addr_alloc(handle, 256);
	printf("addr3=%lld\n", addr3);
	print_addr_list(handle);
	addr_free(handle, addr3, 128);
	print_addr_list(handle);
#else
	
#endif
	deinit_address_list(handle);
}


