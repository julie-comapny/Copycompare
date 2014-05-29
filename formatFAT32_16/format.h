
#include <windows.h>
#include <stdio.h>


int SectorPerCluster = 0;
int FATbits = 0;
unsigned int TotalSecotr = 0;
const int MB = 1024 * 1024;
const int BU = 8192;
const int RDE = 512;

const int SECTOR_SZ = 512;
const int BYTE_SZ = 8;

#pragma pack(1)
//MBR
struct MBR{
	BYTE BootInd;
	BYTE StrHead;
	BYTE StrSetCyl[2];
	BYTE SystemId;
	BYTE Endhead;
	BYTE EndSetCyl[2];
	BYTE RelSector[4];
	BYTE TotalSecotr[4];
}mbr;


//BPB FAT32
struct BPB_FAT32{
	BYTE BS_jmpBoot[3];
	BYTE BS_OEMName[8];
	BYTE BPB_BytsPerSec[2];
	BYTE BPB_SecPerClus;
	BYTE BPB_RsvdSecCnt[2];
	BYTE BPB_NumFATs;
	BYTE BPB_RootEntCnt[2];
	BYTE BPB_TotSec16[2];
	BYTE BPB_Media;
	BYTE BPB_FATSz16[2];
	BYTE BPB_SecPerTrk[2];
	BYTE BPB_NumHeads[2];
	BYTE BPB_HiddSec[4];
	BYTE BPB_TotSec32[4];
	//Extend
	BYTE BPB_FATSz32[4];
	BYTE BPB_ExtFlags[2];
	BYTE BPB_FSVer[2];
	BYTE BPB_RootClus[4];
	BYTE BPB_FSInfo[2];
	BYTE BPB_BkBootSe[2];
	BYTE BPB_Reserved[12];
	BYTE BS_DrvNum;
	BYTE BS_Reserved1;
	BYTE BS_BootSig;
	BYTE BS_VolID[4];
	BYTE BS_VolLab[11];
	BYTE BS_FilSysType[8];
	BYTE ReservedFAT32[420];
	BYTE Signature_word[2];
} bpb32 = { { 0XEB, 0X00, 0X90 }, { 0x20 }, { 0X00, 0X02 }, 0X40, { 0X00 } \
, 0X02, { 0 }, { 0 }, 0XF8, { 0 }, { 0 }, { 0 }, { 0 } \
, { 0 }, { 0 }, { 0 }, { 0 }, { 0X02, 0X00, 0X00, 0X00 } \
, { 0X01, 0X00 }, { 0X06, 0X00 }, { 0 } \
, 0X80, 0, 0X29, { rand() % 256 }, { 0X4A, 0X55, 0X4C, 0X49, 0X45, 0X20, 0X4C, 0X49, 0X4E, 0X20, 0X20 }\
, { 0X46, 0X41, 0X54, 0X33, 0X32, 0X20, 0X20, 0X20 }, { 0 }, { 0X55, 0XAA } };


//BPB FAT16
struct BPB_FAT16{
	BYTE BS_jmpBoot[3];
	BYTE BS_OEMName[8];
	BYTE BPB_BytsPerSec[2];
	BYTE BPB_SecPerClus;
	BYTE BPB_RsvdSecCnt[2];
	BYTE BPB_NumFATs;
	BYTE BPB_RootEntCnt[2];
	BYTE BPB_TotSec16[2];
	BYTE BPB_Media;
	BYTE BPB_FATSz16[2];
	BYTE BPB_SecPerTrk[2];
	BYTE BPB_NumHeads[2];
	BYTE BPB_HiddSec[4];
	BYTE BPB_TotSec32[4];
	//Extend
	BYTE BS_DrvNum;
	BYTE BS_Reserved1;
	BYTE BS_BootSig;
	BYTE BS_VolID[4];
	BYTE BS_VolLab[11];
	BYTE BS_FilSysType[8];
	BYTE ReservedFAT16[448];
	BYTE Signature_word[2];
} bpb16 = { { 0XEB, 0X00, 0X90 }, { 0x00 }, { 0X00, 0X02 }, 0X00, { 0X00 } \
, 0X02, { 0X00, 0X02 }, { 0 }, 0XF8, { 0 }, { 0 }, { 0 }, { 0 } \
, { 0 }, 0X80, 0, 0X29, { 0 }, { 0X4A, 0X55, 0X4C, 0X49, 0X45, 0X20, 0X4C, 0X49, 0X4E, 0X20, 0X20 }\
, { 0X46, 0X41, 0X54, 0X31, 0X36, 0X20, 0X20, 0X20 }, { 0 }, { 0X55, 0XAA } };



//FS Info in FAT32
struct FSInfo{
	BYTE FSI_LeadSig[4];
	BYTE FSI_Reserved1[480];
	BYTE FSI_StrucSig[4];
	BYTE FSI_Free_Count[4];
	BYTE FSI_Nxt_Free[4];
	BYTE FSI_Reserved2[14];
	BYTE FSI_TrailSig[2];
}fsinfo = { { 0X52, 0X52, 0X61, 0X41 }, { 0 }, { 0X72, 0X72, 0X41, 0X61 } \
, { 0XFF, 0XFF, 0XFF, 0XFF }, { 0XFF, 0XFF, 0XFF, 0XFF }, { 0 }, { 0X55, 0XAA } };

//DirEntry
struct DirEntry{
	BYTE DIR_Name[10];
	BYTE DIR_Attr;
	BYTE DIR_NTRes;
	BYTE DIR_CrtTime[2];
	BYTE DIR_CrtDate[2];
	BYTE DIR_LstAccDate[2];
	BYTE DIR_FstClusHI[2];
	BYTE DIR_WrtTime[2];
	BYTE DIR_WrtDate[2];
	BYTE DIR_FstClusLO[2];
	BYTE DIR_FileSize[4];
};


#pragma pack()

int setBytesCombine(BYTE arrpart[], const int start, const int end);
void setIntDiv(BYTE *ptr, const int value, const int part);
void printfData(BYTE* ptr, int size);
BOOL LockVolume(HANDLE &hDevice);
BOOL UnLockVolume(HANDLE &hDevice);
int setParma(double &TotalSecotr);

void ReadMbr(HANDLE hDevice);
void CalSFFat32(unsigned int &TS, int &SC, int &SF, int &RSC);
void CalSFFat16(unsigned int &TS, int &SC, int &SF, int &RSC);
void initBPB_FAT32(int &RSC, int &SF);
void initBPB_FAT16(int &RSC, int &SF, int &SC);
void initFATable_32(BYTE* fat, int &SF);
void initFATable_16(BYTE* fat, int &SF);


inline int setBytesCombine(BYTE arrpart[], const int start, const int end){
	int ret = (int)arrpart[start];
	for (int i = 1; i < end - start + 1; i++){
		ret |= arrpart[i] << i*BYTE_SZ;
	}
	return ret;
}
inline void setIntDiv(BYTE *ptr, const int value, const int part){
	//printf("SetInt Value: %d\n", value);
	ptr[0] = value;
	//printf("ptr:%02X", *ptr);
	for (int i = 1; i < part; i++)
	{
		ptr[i] = value >> i*BYTE_SZ;
		//printf("\n %02X ", ptr[i]);
	}

}
inline void printfData(BYTE* ptr, int size){
	for (int i = 0; i < size; i++){
		if (i % 16 == 0) printf("\n");
		printf("%02X ", *(ptr + i));

	}

}

inline BOOL LockVolume(HANDLE &hDevice){
	DWORD ret = 0;
	return DeviceIoControl(
		hDevice,
		FSCTL_LOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&ret,
		NULL);

}
inline BOOL UnLockVolume(HANDLE &hDevice){
	DWORD ret = 0;
	return DeviceIoControl(
		hDevice,
		FSCTL_UNLOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&ret,
		NULL);
}
