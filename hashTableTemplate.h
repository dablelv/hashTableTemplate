/************************************************************
*@DESCRIPTION: hash table template
*@BIRTH:created by anonymous person
*@REVISION:last modified by dablelv on 20180326
*************************************************************/

#ifndef HASH_TABLE_TEMPLATE_H
#define HASH_TABLE_TEMPLATE_H

#include <stdio.h>
#include <time.h>
#include <math.h>
#include <sys/shm.h>
#include <stdint.h>
#include <string.h>
#ifndef DDWORD
    #define DDWORD uint64_t
#endif
#ifndef DWORD
    #define DWORD uint32_t
#endif
#ifndef WORD
    #define WORD uint16_t
#endif
#ifndef BYTE 
    #define BYTE uint8_t
#endif

/************************************
*@brief:determine whether a number is a prime number
*************************************/
static int HashT_IsPrimeNum(uint32_t n)
{
	if(1==n)
	{
		return 0;
	}
    int i=0;
    double k = sqrt((double)n);
    for(i=2; i<= k; i++)
    {
        if(n%i==0)
        {
            return 0;
        }
    }
    return 1;
}

/**************************************************
*@brief:create or get shared memory segment
*@ret:return address of shared memory segment
***************************************************/
static char* HashT_GetShm(key_t iKey, size_t Size, int iFlag)
{
    int iShmID;
    char* sShm;
    char sErrMsg[128]={0};

    if((iShmID=shmget(iKey,Size,iFlag))<0)
	{
        snprintf(sErrMsg,sizeof(sErrMsg),"%s %s %u shmget key 0x%x %zu",__FILE__,__FUNCTION__,__LINE__,iKey,Size);
        perror(sErrMsg);
        return NULL;
    }
	
    if ((sShm = (char*)shmat(iShmID, NULL ,0)) == (char *) -1)
	{
		snprintf(sErrMsg,sizeof(sErrMsg),"%s %s %u shmat shmid %d",__FILE__,__FUNCTION__,__LINE__,iShmID);
        perror(sErrMsg);
        return NULL;
    }
    return sShm;
}

/***************************
*@brief:create or get shared memory segment version 2
*@ret:0 is returned on success, -1 on error
****************************/
static int HashT_GetShm2(void **pstShm, key_t iShmKey, size_t Size, int iFlag)
{
    char* sShm;
	
	//获取现有共享内存失败
    if(!(sShm = HashT_GetShm(iShmKey, Size, iFlag & (~IPC_CREAT))))
	{
		//不是创建标识，返回错误
        if(!(iFlag & IPC_CREAT)) return -1;
		
		//创建新共享内存
        if (!(sShm = HashT_GetShm(iShmKey, Size, iFlag))) return -1;
        memset(sShm, 0, Size);
    }
    *pstShm = sShm;
    return 0;
}

/************************************
*@brief:hash table template
*@param:Element_T:元素类型;Key_T:元素键值类型;nHashLen:hash表长度;nHashTime:hash表数量
*@author:anonymous person
*@date:unknown
*@revison:
************************************/
template<class Element_T,class Key_T,int nHashLen,int nHashTime>
class CHashTable
{
public:
#pragma pack(1)
    typedef Element_T (* TableP_T)[nHashLen];
#pragma pack()
    typedef int (* ElementEmptyFunc_T)(Element_T* pE,void* pExt);
    typedef int (* ElementSameFunc_T)(Element_T* pE,Key_T key,void* pExt);
    typedef int (* ElementDoFunc_T)(Element_T* pE,void* pExt);
    
    CHashTable();
    ~CHashTable(){}
    int Initialize(int nHashKey,ElementSameFunc_T IfElementSameFunc=NULL,ElementEmptyFunc_T IfElementEmptyFunc=NULL);
    int Initialize(int nHashKey,DWORD ElementExpiredSec);
    int Initialize(void* pMem,size_t MemSize,ElementSameFunc_T IfElementSameFunc=NULL,ElementEmptyFunc_T IfElementEmptyFunc=NULL);
    int Initialize(void* pMem,size_t MemSize,DWORD ElementExpiredSec);
    size_t GetRequiredMemSize()
    {
        return nHashLen*nHashTime*sizeof(Element_T)+(size_t)32*sizeof(DWORD);
    }
    int AttachToMem(void* pMem, size_t MemSize);
    int Reset()
    {
        memset(m_TableP,0,nHashLen*nHashTime*sizeof(Element_T));
        return 0;
    }
    Element_T* GetElement(Key_T key,void* pExt=NULL);
    Element_T* GetNewSpace(Key_T key,void* pExt=NULL);
    int DoForEach(int* piBeginTimePos,int* piBeginLenPos,int iScanNodes,ElementDoFunc_T ElementDoFunc,void* pExt=NULL);
    
    TableP_T GetTableP()
    {
        return m_TableP;
    }
    int GetStat(DWORD* pdwStat)
    {
        *pdwStat=*m_pStat;
        return 0;
    }
    int SetStat(DWORD dwStat)
    {
        *m_pStat=dwStat;
        return 0;
    }

private:
    int Initialize(int nHashKey,void* pMem,size_t MemSize,ElementSameFunc_T IfElementSameFunc,ElementEmptyFunc_T IfElementEmptyFunc,DWORD ElementExpiredSec);

private:
    TableP_T m_TableP;
    ElementSameFunc_T m_IfElementSameFunc;
    ElementEmptyFunc_T m_IfElementEmptyFunc;
    DWORD m_ElementExpiredSec;
    int m_HashKey;
    DWORD m_HashBase[nHashTime];
    DWORD* m_pStat;
};

//构造函数
template<class Element_T,class Key_T,int nHashLen,int nHashTime> CHashTable<Element_T,Key_T,nHashLen,nHashTime>::CHashTable()
{
    m_IfElementSameFunc=NULL;
    m_IfElementEmptyFunc=NULL;
    m_HashKey=0;
    m_ElementExpiredSec=0;
}

//@brief:初始化Hash表
//@param:nHashKey：shmkey;ElementExpiredSec:生命周期
template<class Element_T,class Key_T,int nHashLen,int nHashTime> int CHashTable<Element_T,Key_T,nHashLen,nHashTime>::Initialize(int nHashKey,DWORD ElementExpiredSec)
{
    return Initialize(nHashKey,NULL,0,NULL,NULL,ElementExpiredSec);
}

//@brief:初始化Hash表
//@param:nHashKey:shmkey;IfElementSameFunc:操纵符元素是否相等;IfElementEmptyFunc:操纵符元素是否为空
template<class Element_T,class Key_T,int nHashLen,int nHashTime>
int CHashTable<Element_T,Key_T,nHashLen,nHashTime>::Initialize(int nHashKey,ElementSameFunc_T IfElementSameFunc,ElementEmptyFunc_T IfElementEmptyFunc)
{
    return  Initialize(nHashKey,NULL,0,IfElementSameFunc,IfElementEmptyFunc,0);
}

//@brief:初始化Hash表
//@param:pMem:内存地址;MemSize:内存空间大小;ElementExpiredSec:生命周期
template<class Element_T,class Key_T,int nHashLen,int nHashTime>
int CHashTable<Element_T,Key_T,nHashLen,nHashTime>::Initialize(void* pMem,size_t MemSize,DWORD ElementExpiredSec)
{
    return Initialize(0,pMem,MemSize,NULL,NULL,ElementExpiredSec);
}

//@brief:初始化Hash表
//@param:pMem:内存地址;MemSize:内存空间大小;IfElementSameFunc:操纵符元素是否相等;IfElementEmptyFunc:操纵符元素是否为空
template<class Element_T,class Key_T,int nHashLen,int nHashTime>
int CHashTable<Element_T,Key_T,nHashLen,nHashTime>::Initialize(void* pMem,size_t MemSize,ElementSameFunc_T IfElementSameFunc,ElementEmptyFunc_T IfElementEmptyFunc)
{
    return  Initialize(0,pMem,MemSize,IfElementSameFunc,IfElementEmptyFunc,0);
}

//@brief:初始化Hash表
//@param:nHashKey:shmkey;pMem:内存地址;MemSize:内存区域大小;IfElementSameFunc:操纵符元素是否相等;IfElementEmptyFunc:操纵符元素是否为空;ElementExpiredSec:生命周期
template<class Element_T,class Key_T,int nHashLen,int nHashTime>
int CHashTable<Element_T,Key_T,nHashLen,nHashTime>::Initialize(int nHashKey,void* pMem,size_t MemSize,ElementSameFunc_T IfElementSameFunc,ElementEmptyFunc_T IfElementEmptyFunc,DWORD ElementExpiredSec)
{
    int nRet=0;
    if (nHashKey<=0 && (NULL==pMem || 0==MemSize)) return -1;
    if (nHashKey>0 && (NULL!=pMem && MemSize>0)) return -1;
    size_t RequiredMemSize=GetRequiredMemSize();

    m_HashKey=nHashKey;
    m_IfElementSameFunc=IfElementSameFunc;
    m_IfElementEmptyFunc=IfElementEmptyFunc;
    m_ElementExpiredSec=ElementExpiredSec;
    if (nHashKey>0)
    {
        if( HashT_GetShm2((void**)&m_TableP, m_HashKey, RequiredMemSize, 0600) < 0 )
        {
            if( HashT_GetShm2((void**)&m_TableP, m_HashKey, RequiredMemSize, 0600|IPC_CREAT) < 0 )
            {
                printf("GetShm2 %x Error\n",m_HashKey);
                return -1;
            }
            else
            {
                nRet=1;//first allocate the mem;
            }
        }
        m_pStat=(DWORD*)( (char*)m_TableP + nHashLen*nHashTime*sizeof(Element_T) );
    }
    else if (NULL != pMem && MemSize > 0)
    {
        if (MemSize < RequiredMemSize)
        {
            printf("hash_template init err: MemSize[%zu] < RequriedMemSize[%zu]\n", MemSize, RequiredMemSize);
            return -1;
        }
        m_TableP=(TableP_T)pMem;
        m_pStat=(DWORD*)( (char*)m_TableP + nHashLen*nHashTime*sizeof(Element_T) );
    }
    else
    {
        return -1;
    }
	
	//采用大质数作为hash表的模
    uint64_t ulStep=(nHashLen & 1)?nHashLen:(nHashLen-1);
    int nNum = nHashTime;
    for(; ; ulStep -=2)
    {
        if(nNum == 0)break;
        if(HashT_IsPrimeNum(ulStep))
        {
            m_HashBase[nHashTime-nNum]=ulStep;
            nNum--;
        }   
    }
    return nRet;
}

//@brief:获取元素
//@param:key:元素key;pExt:扩展参数
template<class Element_T,class Key_T,int nHashLen,int nHashTime>
Element_T* CHashTable<Element_T,Key_T,nHashLen,nHashTime>::GetElement(Key_T key,void* pExt)
{
    int i, hash;
    Element_T* pE = NULL;

    for(i=0; i<nHashTime; ++i)
    {
        hash = key % m_HashBase[i];
        pE = &(m_TableP[i][hash]);
        if(m_IfElementSameFunc != NULL)
        {
            if(m_IfElementSameFunc(pE,key,pExt))return pE;
        }
        else
        {
            DWORD dwCurTime=time(NULL);
            Key_T* pcurkey=(Key_T*)pE;
            DWORD* pdwStartTime=(DWORD*)((char*)pE+sizeof(Key_T));
            if(*pcurkey == key && (m_ElementExpiredSec==0 || *pdwStartTime > dwCurTime - m_ElementExpiredSec))return pE;
        }
    }
    return NULL;
}

//@brief:获取新元素空间
//@param:key:元素key;pExt:扩展参数
template<class Element_T,class Key_T,int nHashLen,int nHashTime>
Element_T* CHashTable<Element_T,Key_T,nHashLen,nHashTime>::GetNewSpace(Key_T key,void* pExt)
{
    int i, hash;
    Element_T *pE = NULL;

    for(i=0; i<nHashTime; ++i)
    {
        hash = key % m_HashBase[i];
        pE = &(m_TableP[i][hash]);
        if(m_IfElementEmptyFunc != NULL)
        {
            if(m_IfElementEmptyFunc(pE,pExt))return pE;
        }
        else
        {
            DWORD dwCurTime=time(NULL);
            Key_T* pcurkey=(Key_T*)pE;
            DWORD* pdwStartTime=(DWORD*)((char*)pE+sizeof(Key_T));
            if(*pcurkey == 0)return pE;
            if(m_ElementExpiredSec>0 && *pdwStartTime <= dwCurTime - m_ElementExpiredSec)return pE;
        }
    }

    return NULL;
}

//@brief:遍历指定的元素做相应的操作
//@param:piBeginTimePos:hash桶下标;piBeginLenPos:桶内元素下标;iScanNodes:扫描元素个数;ElementDoFunc:操纵符;pExt:扩展参数
template<class Element_T,class Key_T,int nHashLen,int nHashTime>
int CHashTable<Element_T,Key_T,nHashLen,nHashTime>::DoForEach(int* piBeginTimePos,int* piBeginLenPos,int iScanNodes,ElementDoFunc_T ElementDoFunc,void* pExt)
{
    Element_T *pE = NULL;
    if(piBeginTimePos==NULL || piBeginLenPos==NULL)return -1;
    if(*piBeginTimePos>=nHashTime || *piBeginLenPos>=nHashLen)return -2;
    if(ElementDoFunc==NULL)return -3;
    if(iScanNodes<=0)return -4;
    
    int iTimes=0;
    for( ; *piBeginTimePos < nHashTime; *piBeginTimePos=(*piBeginTimePos+1)%nHashTime )
    {
        for( ; *piBeginLenPos < nHashLen; ++(*piBeginLenPos) )
        {
            if ( ++iTimes > iScanNodes )
            {
                return 0;
            }
            pE = &(m_TableP[*piBeginTimePos][*piBeginLenPos]);
            ElementDoFunc(pE,pExt);
        }
        *piBeginLenPos=0;
    }
    return 0;
}
#endif  //HASH_TEMPLATE_H
