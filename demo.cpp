#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <iostream>
using namespace std;

#include "hashTableTemplate.h"

#define UIN_HASH_SHMKEY			0x7ab90327
#define UIN_HASH_LEN			500000
#define UIN_HASH_TIME			20
#define UIN_HASH_CLEAR_TIME		(3600*2)

struct STUINElmt
{
	uint32_t	dwUIN;			//QQ
	uint32_t    dwStartTime;    //记录时间，注意，位置不能变，必须在key之后
	uint32_t    dwFlag;			//UIN标识
	uint32_t    dwReqCnt;       //请求的次数
};

typedef CHashTable<STUINElmt,uint32_t,UIN_HASH_LEN,UIN_HASH_TIME> UINHashTable_T;
UINHashTable_T objUinHashTab;

int main(int argc,char* argv[])
{
	//初始化Hash表
	int iRet = objUinHashTab.Initialize(UIN_HASH_SHMKEY,UIN_HASH_CLEAR_TIME);
    if (iRet < 0)
    {
        printf("UinHashTable Initialize Error %d\n", iRet);
		return -1;
    }
	
	//获取指定UIN对应的元素
	uint32_t dwUIN=1589276509U;
	STUINElmt *pUINElmt = objUinHashTab.GetElement(dwUIN);
	if (NULL == pUINElmt)
	{
		//元素不存在，则从Hash表中获取空闲空间
		pUINElmt = objUinHashTab.GetNewSpace(dwUIN);
		if (NULL != pUINElmt)
        {
            memset(pUINElmt,0,sizeof(STUINElmt));
            pUINElmt->dwUIN = dwUIN;
            pUINElmt->dwStartTime = time(NULL);
			pUINElmt->dwFlag = 0x0;
        }
	}
	
	if(NULL==pUINElmt)
	{
		printf("get element key %u failed\n",dwUIN);
		return -1;
	}
	
	//每一次执行程序，
	++pUINElmt->dwReqCnt;
	
	//再次从Hash表获取指定key元素
	STUINElmt *pUINElmt1=objUinHashTab.GetElement(dwUIN);
	printf("dwUIN %u startT %u flag 0x%x reqCnt %u \n",pUINElmt1->dwUIN,pUINElmt1->dwStartTime,pUINElmt1->dwFlag,pUINElmt1->dwReqCnt);
}