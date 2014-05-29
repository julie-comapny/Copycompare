/*
Author:		Julie Lin
Purpose:	using 1.logical device 'src'(ex:G:) to find physical device number X ("\\.\PhysicalDriveX")
and 2.find physical device size
3.do format
State:		(2014.0409) find physical device number success
			(2014.0411) try do format it (do format FAT32 first)
			(2014.0414) read MBR success
			(2014.0417) Acheive FSinfo BPB FAtable format FAT32
			(2014.0420) Using Struct to format FAT16/FAT32 OK!
			(2014.0427) modify code test not yet...
Problem:
*/
#include "format.h"

#include <string>
#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <math.h>

using namespace std;

//// ============================================================================
////								MBR PART
//// ============================================================================
void ReadMbr(HANDLE hDevice){

	BYTE* buffer = new BYTE[SECTOR_SZ];
	DWORD dwRead = 0;
	if (!ReadFile(hDevice, buffer, SECTOR_SZ, &dwRead, 0)){
		cerr << "Read MBR fail" << endl;
		delete[] buffer;
		return;
	}

	mbr.BootInd			= *(buffer + 446);
	mbr.StrHead			= *(buffer + 447);
	mbr.StrSetCyl[0]	= *(buffer + 448);
	mbr.StrSetCyl[1]	= *(buffer + 449);
	mbr.SystemId		= *(buffer + 450);
	mbr.Endhead			= *(buffer + 451);
	mbr.EndSetCyl[0]	= *(buffer + 452);
	mbr.EndSetCyl[1]	= *(buffer + 453);
	mbr.RelSector[0]	= *(buffer + 454);
	mbr.RelSector[1]	= *(buffer + 455);
	mbr.RelSector[2]	= *(buffer + 456);
	mbr.RelSector[3]	= *(buffer + 457);
	mbr.TotalSecotr[0]	= *(buffer + 458);
	mbr.TotalSecotr[1]	= *(buffer + 459);
	mbr.TotalSecotr[2]	= *(buffer + 460);
	mbr.TotalSecotr[3]	= *(buffer + 461);
	
	delete[] buffer;
}
//// ============================================================================
////						BPB PRAT (BIOS Parameter Block)
//// ============================================================================
void CalSFFat32(unsigned int &TS, int &SC, int &SF, int &RSC){
	double dTS = TS;
	double dSF = SF;
	double dSC = SC;
	double dRSC = RSC;
	double NOM = BU;
	double SSA, MAX; 
	double dSF2 = 0;
	BOOL SFloop = false;

	dSF = ceil((TS / SectorPerCluster)*FATbits / (SECTOR_SZ << 3));
	do{
		if (SFloop) dSF--;

		dRSC = BU - 2 * dSF;
		if (dRSC < 9) dRSC += BU;
		SSA = dRSC + 2 * dSF;

		do{
			if (dSF2 > dSF)
			{
				SSA += BU;
				dRSC += BU;
			}
			MAX = (int)((dTS - NOM - SSA) / SC) + 1;
			dSF2 = ceil((2 + MAX - 1)*FATbits / (SECTOR_SZ << 3));
		} while (dSF2 > dSF);
		SFloop = true;
	} while (dSF2 != dSF);
	dSF++;
	dRSC = dRSC - 2;
	
	TS = static_cast<int>(dTS);
	SF = static_cast<int>(dSF);
	SC = static_cast<int>(dSC);
	RSC = static_cast<int>(dRSC);

}
void CalSFFat16(unsigned int &TS, int &SC, int &SF, int &RSC){

	BOOL SFloop = false;
	double RDE = 512;
	double dTS = TS;
	double dSF = SF;
	double dSC = SC;
	double dRSC = RSC;
	double SSA, NOM, MAX;
	double dSF2 = 0;
	dRSC = 1;
	dSF = ceil((dTS / SC) *FATbits/ (SECTOR_SZ << 3));

	do{
		if (SFloop) dSF--;
		SSA = dRSC + 2 * dSF + ceil((32 * RDE / SECTOR_SZ));
		NOM = BU - SSA;
		if (BU != NOM) NOM += BU;
		do{
			MAX = (int)((dTS - NOM - SSA) / SC) + 1;
			dSF2 = ceil((2 + (MAX - 1))*FATbits / (SECTOR_SZ << 3));
			if (dSF2 > dSF){
				NOM += BU;
			}
		} while (dSF2 > dSF);
		SFloop = true;
	} while (dSF2 != dSF);
	dSF++;
	TS = static_cast<int>(dTS);
	SF = static_cast<int>(dSF);
	SC = static_cast<int>(dSC);
	RSC = static_cast<int>(dRSC);
}

void initBPB_FAT32(int &RSC, int &SF)
{
	for (int i = 0; i < 8; i++){
		bpb32.BS_OEMName[i] = 0X20;
	}
	bpb32.BPB_RsvdSecCnt[0] = SF;
	bpb32.BPB_RsvdSecCnt[1] = RSC >> 1*BYTE_SZ ;
	bpb32.BPB_TotSec32[0] = TotalSecotr;
	bpb32.BPB_TotSec32[1] = TotalSecotr >> 1 * BYTE_SZ;
	bpb32.BPB_TotSec32[2] = TotalSecotr >> 2 * BYTE_SZ;
	bpb32.BPB_TotSec32[3] = TotalSecotr >> 3 * BYTE_SZ;
	
	////BPB FAT32 Extend
	bpb32.BPB_FATSz32[0] = SF;
	bpb32.BPB_FATSz32[1] = SF >> 1 * BYTE_SZ;
	bpb32.BPB_FATSz32[2] = SF >> 2 * BYTE_SZ;
	bpb32.BPB_FATSz32[3] = SF >> 3 * BYTE_SZ;

	bpb32.BS_VolID[0] = rand()%256;
	bpb32.BS_VolID[1] = rand()%256;
	bpb32.BS_VolID[2] = rand()%256;
	bpb32.BS_VolID[3] = rand()%256;

}
void initBPB_FAT16(int &RSC, int &SF,int &SC){
	memset(&bpb16.BS_OEMName[0], 0X20, 8);

	bpb16.BPB_RsvdSecCnt[0] = RSC;
	bpb16.BPB_RsvdSecCnt[1] = RSC >> 1 * BYTE_SZ;
	bpb16.BPB_FATSz16[0] = SF;
	bpb16.BPB_FATSz16[1] = SF >> 1 * BYTE_SZ;
	bpb16.BPB_SecPerClus = SC;
	bpb16.BPB_TotSec32[0] = TotalSecotr;
	bpb16.BPB_TotSec32[1] = TotalSecotr >> 1 * BYTE_SZ;
	bpb16.BPB_TotSec32[2] = TotalSecotr >> 2 * BYTE_SZ;
	bpb16.BPB_TotSec32[3] = TotalSecotr >> 3 * BYTE_SZ;

	//BPB FAT16 Extend	   
	bpb16.BS_VolID[0] = rand() % 256;
	bpb16.BS_VolID[1] = rand() % 256;
	bpb16.BS_VolID[2] = rand() % 256;
	bpb16.BS_VolID[3] = rand() % 256;

}

//// ============================================================================
////								FAT Table PART 
//// ============================================================================
void initFATable_32(BYTE* fat,int &SF){

	memset(&fat[0], 0X00, SF*SECTOR_SZ);
	memset(&fat[0], 0XFF, 12);

	fat[0] = 0XF8;
	fat[3] = 0XF0;
	fat[7] = 0XF0;
	fat[11] = 0XF0;

}
void initFATable_16(BYTE* fat, int &SF){
	memset(&fat[0], 0X00, SF*SECTOR_SZ);
	fat[0] = 0XF8;
	fat[1] = 0XFF;
	fat[2] = 0XFF;
	fat[3] = 0XFF;
}


int setParma(double &TotalSecotr){
	TotalSecotr = TotalSecotr*SECTOR_SZ;
	if (TotalSecotr > 256 )
	{
		setIntDiv(bpb16.BPB_SecPerTrk, 63, 2);
		if (TotalSecotr> 504  && TotalSecotr < 1008 * MB) setIntDiv(bpb16.BPB_NumHeads, 32, 2);
		else setIntDiv(bpb16.BPB_NumHeads, 16, 2);
		if (TotalSecotr >2016 ) setIntDiv(bpb16.BPB_NumHeads, 128, 2);
		else setIntDiv(bpb16.BPB_NumHeads, 64, 2);
		
	}
	else {
		setIntDiv(bpb16.BPB_SecPerTrk, 32, 2);
		if (TotalSecotr<32  && TotalSecotr>16 )  setIntDiv(bpb16.BPB_NumHeads, 4, 2);
		else setIntDiv(bpb16.BPB_NumHeads, 2, 2);
		if (TotalSecotr > 128 ) setIntDiv(bpb16.BPB_NumHeads, 16, 2);
		else setIntDiv(bpb16.BPB_NumHeads, 8, 2);
	}
	if (TotalSecotr < 2 ) {
		setIntDiv(bpb16.BPB_NumHeads, 2, 2);
		setIntDiv(bpb16.BPB_SecPerTrk, 16, 2);
	}
	

	if (TotalSecotr< 4 ) setIntDiv(bpb16.BPB_HiddSec, 27, 2);
	if (TotalSecotr >= 4 && TotalSecotr<8) setIntDiv(bpb16.BPB_HiddSec, 25, 2);
	if (TotalSecotr >= 8 && TotalSecotr<16) setIntDiv(bpb16.BPB_HiddSec, 57, 2);
	if (TotalSecotr >= 16 && TotalSecotr<32 ) setIntDiv(bpb16.BPB_HiddSec, 51, 2);
	if (TotalSecotr >= 32 && TotalSecotr<64 ) setIntDiv(bpb16.BPB_HiddSec, 39, 2);
	if (TotalSecotr >= 64 && TotalSecotr<256 ) setIntDiv(bpb16.BPB_HiddSec, 95, 2);
	if (TotalSecotr >= 256  && TotalSecotr<512 ) setIntDiv(bpb16.BPB_HiddSec, 255, 2);
	if (TotalSecotr >= 512  && TotalSecotr<2048 ) setIntDiv(bpb16.BPB_HiddSec, 227, 2);

	return 0;
}

int cleanCluster(int ClusNum, HANDLE &hDevice, int &SF, int &RSC){
	//int BuffSz = SECTOR_SZ*SectorPerCluster;
	DWORD dwWrite = 0;
	int LogicAddr = (ClusNum-1)*SECTOR_SZ*SectorPerCluster + SF * 2 + RSC;
	SetFilePointer(hDevice, LogicAddr, NULL, FILE_BEGIN);
	BYTE* buffer = new BYTE[SECTOR_SZ];
	memset(buffer, 0, sizeof(buffer));
	if (!WriteFile(hDevice, buffer, SECTOR_SZ, &dwWrite, 0)){
		cerr << "WriteFile fail" << endl;
		return -1;
		delete[]buffer;
	}
	delete[]buffer;
	return 0;
}

int main(int argc, char* argv[]){

	VOLUME_DISK_EXTENTS volumeDisk;
	DISK_GEOMETRY disk_geo;
	DWORD retBytes;
	HANDLE hDevice, hDevice1;

	char src1[10];
	char src2[MAX_PATH];
	cout << "Please enter the device: ";
	cin >> src1;
	
	::CharUpperBuff(&src1[0], 1);
	if (src1[0] < ('D') || src1[0] > _T('K'))
	{		
		return FALSE;
	}

	StringCchPrintf(src2, MAX_PATH, "\\\\.\\%s", src1);
	hDevice = CreateFile(	src2,
							GENERIC_READ | GENERIC_WRITE,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING,
							0,
							NULL);


	if (!LockVolume(hDevice)){
		cerr << "LockVolume fail" << endl;
	}

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Create read File fail!\r\n" << std::endl;
		printf("GetLastError :%d\n", GetLastError());
	}

	if (!DeviceIoControl(hDevice,
		IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
		NULL,
		0,
		&volumeDisk,
		sizeof(VOLUME_DISK_EXTENTS),
		&retBytes,
		NULL)){
		cerr << "DeviceIoControl fail" << endl;
	}

	cout << "Find Physical device number: ";
	cout << "DiskNumber " << volumeDisk.Extents[0].DiskNumber << endl;

	char src[MAX_PATH];
	StringCchPrintf(src, MAX_PATH, "\\\\.\\PHYSICALDRIVE%d", volumeDisk.Extents[0].DiskNumber);

	hDevice1 = CreateFile(	src,
							GENERIC_READ | GENERIC_WRITE,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING,
							0,
							NULL);

	if (hDevice1 == INVALID_HANDLE_VALUE)
	{
		std::cout << "Create read File fail!\r\n" << std::endl;
	}


	if (!DeviceIoControl(hDevice1,
		IOCTL_DISK_GET_DRIVE_GEOMETRY,
		NULL,
		0,
		&disk_geo,
		sizeof(disk_geo),
		&retBytes,
		NULL)){
		cerr << "PHYSICALDRIVE DeviceIoControl fail" << endl;
	}


	cout << "Find Physical device size: ";
	cout << "BytesPerSector " << disk_geo.BytesPerSector << " Bytes" << endl;
	cout << "SECTOR_SZPerTrack " << disk_geo.SectorsPerTrack << " SECTOR_SZ" << endl;
	cout << "TracksPerCylinder " << disk_geo.TracksPerCylinder << " Tracks" << endl;
	cout << "Cylinders " << disk_geo.Cylinders.QuadPart << endl;
	cout << "Totally size is: " << disk_geo.Cylinders.QuadPart*disk_geo.TracksPerCylinder*disk_geo.SectorsPerTrack*disk_geo.BytesPerSector << " Bytes" << endl;
	cout << "Totally size is: " << disk_geo.Cylinders.QuadPart*disk_geo.TracksPerCylinder*disk_geo.SectorsPerTrack << " SECTOR_SZ" << endl << endl;
	
	//Start format
	int SF = 1;
	int RSC = 0;
	int	SecTrack = 0;
	int NHead = 0;
	int Nhid = 0;
	BOOL FAT16 = false;
	BOOL FAT32 = false;
	int BPBstart = 0;
	int FSinfostart = 0;
	int BPB2start = 0;
	int FSinfo2start = 0;
	int FAT1start = 0;
	int FAT2start = 0;
	int rootStart = 0;
	DWORD dwWrite = 0;
	
	ReadMbr(hDevice1);
	TotalSecotr = setBytesCombine(&mbr.TotalSecotr[0],0,3);
	double mbTotalSecotr = static_cast<double>(TotalSecotr) / (1024 * 1024);

	if (mbTotalSecotr*SECTOR_SZ < 2048) 
	{
		printf("\n[FAT16] Format Start...\n");
		FAT16 = true; 		
		SectorPerCluster = 32;
		if (mbTotalSecotr*SECTOR_SZ > 1024) SectorPerCluster = 64;
		FATbits = 16;
		setParma(mbTotalSecotr);		
		CalSFFat16(TotalSecotr, SectorPerCluster, SF, RSC);
		if (SF < 0) return -1;
		initBPB_FAT16(RSC, SF, SectorPerCluster);
		FAT1start = RSC*SECTOR_SZ;
		FAT2start = (RSC + SF)*SECTOR_SZ;
		rootStart = (SF * 2 + RSC)*SECTOR_SZ;
		//start format
		SetFilePointer(hDevice, 0, NULL, FILE_BEGIN);
		int TotalClus = ceil((double)(TotalSecotr - SF * 2 - RSC) / SectorPerCluster);

		BYTE* pBpb16 = &bpb16.BS_jmpBoot[0];
		SetFilePointer(hDevice, 0, NULL, FILE_BEGIN);

		if (!WriteFile(hDevice, pBpb16, SECTOR_SZ, &dwWrite, 0)){
			cerr << "WriteFile fail: " << GetLastError() << endl;
		}
		SetFilePointer(hDevice, FAT1start, NULL, FILE_BEGIN);
		BYTE* fat16 = new BYTE[SF*SECTOR_SZ];
		initFATable_16(fat16, SF);		
		if (!WriteFile(hDevice, fat16, SECTOR_SZ, &dwWrite, 0)){
			cerr << "WriteFile fail: " << GetLastError() << endl;
		}
		SetFilePointer(hDevice, FAT2start, NULL, FILE_BEGIN);
		if (!WriteFile(hDevice, fat16, SECTOR_SZ, &dwWrite, 0)){
			cerr << "WriteFile fail: " << GetLastError() << endl;
		}

		SetFilePointer(hDevice, rootStart, NULL, FILE_BEGIN);
		BYTE* buffer = new BYTE[SECTOR_SZ*SectorPerCluster];
		memset(buffer, 0, SECTOR_SZ*SectorPerCluster);
		if (!WriteFile(hDevice, buffer, SECTOR_SZ*SectorPerCluster, &dwWrite, 0)){
			cerr << "WriteFile fail: " << GetLastError() << endl;
		}
		delete[] fat16;
	}
	else 
	{
		if (mbTotalSecotr*SECTOR_SZ < 32768) 
		{
			cout << "\n[FAT32] Format Start...\n";
			FAT32 = true;
			SectorPerCluster = 64;
			FATbits = 32;
			setIntDiv(bpb32.BPB_SecPerTrk, 63, 2);
			if (mbTotalSecotr*SECTOR_SZ < 4032)
				setIntDiv(bpb32.BPB_NumHeads, 128, 2);
			else setIntDiv(bpb32.BPB_NumHeads, 255, 2);
			setIntDiv(bpb32.BPB_HiddSec, 8192, 4);
			CalSFFat32(TotalSecotr, SectorPerCluster, SF, RSC);
			if (SF < 0) return -1;
			initBPB_FAT32(RSC, SF);
			FSinfostart = setBytesCombine(bpb32.BPB_FSInfo, 0, 1)*SECTOR_SZ;
			BPB2start = setBytesCombine(bpb32.BPB_BkBootSe, 0, 1)*SECTOR_SZ;
			FSinfo2start = (setBytesCombine(bpb32.BPB_BkBootSe, 0, 1) + setBytesCombine(bpb32.BPB_FSInfo, 0, 1))*SECTOR_SZ;
			FAT1start = RSC*SECTOR_SZ;
			FAT2start = (RSC + SF)*SECTOR_SZ;
			rootStart = (SF * 2 + RSC)*SECTOR_SZ;
			cout << "[FSinfostart] " << FSinfostart << endl;
			cout << "[BPB2start] " << BPB2start << endl;
			cout << "[FSinfo2start] " << FSinfo2start << endl;
			cout << "[FAT1start] " << FAT1start << endl;
			cout << "[FAT2start] " << FAT2start << endl;
			cout << "[rootStart] " << rootStart << endl;

			//start format
			SetFilePointer(hDevice, 0, NULL, FILE_BEGIN);
			BYTE* pBpb32 = &bpb32.BS_jmpBoot[0];
			cout << "\n[BPB BLock]" << endl;
			printfData(pBpb32, 512);
			SetFilePointer(hDevice, BPB2start, NULL, FILE_BEGIN);
			BYTE* pFsinfo = &fsinfo.FSI_LeadSig[0];
			cout << "\n[pFsinfo]" << endl;
			printfData(pFsinfo, 512);
			SetFilePointer(hDevice, FSinfo2start, NULL, FILE_BEGIN);
			BYTE* AfterFsinfo = new BYTE[SECTOR_SZ];
			memset(AfterFsinfo, 0, SECTOR_SZ);
			AfterFsinfo[510] = 0X55;
			AfterFsinfo[511] = 0XAA;
			BYTE* fat32 = new BYTE[SF*SECTOR_SZ];
			initFATable_32(fat32, SF);
			SetFilePointer(hDevice, FAT1start, NULL, FILE_BEGIN);
			cout << "\n[fat32]" << endl;
			printfData(fat32, 512);
			SetFilePointer(hDevice, FAT2start, NULL, FILE_BEGIN);
			delete[] fat32;
			delete[] AfterFsinfo;
		}
		else {
			cerr << "Exceed Max format Size: ";
			cin.get();
			return -1;
			
		}
	}
	printf("\nEnd format :)");
	UnLockVolume(hDevice);
	CloseHandle(hDevice);
	CloseHandle(hDevice1);

	system("pause");
	return 0;
}