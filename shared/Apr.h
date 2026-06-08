#ifndef APR_H
#define APR_H
#pragma once
#include "windows.h"
#define BUFFER_SIZE 20000

class CApr
{
public:
    CApr(void);
    ~CApr(void);
    void AprInitialize(DWORD key[]);
    void Cryption(BYTE* packet, int& len, bool flag, BYTE* out);
    //void passEncrypt(const char* pass,char* buffer);
    void KeyExpansion(DWORD in[], DWORD out[], bool flag);
    DWORD swap(DWORD ul);
    DWORD GenerateIV();

    void swapArray(DWORD expandedKey[]);
    void xorArray(DWORD expandedKey[]);

    void FillPacket(BYTE* inPacket, int& pSize, BYTE* outPacket);
    void XorKey(BYTE* packet, BYTE* xKey);
    void Encryption(DWORD expandedKey[], BYTE* xorKey);
    void Decryption(DWORD expandedKey[], BYTE* xorKey);
    void removeJunkPart(BYTE* packet, int& len);

    DWORD private_key[4];
    BYTE send_counter;
    DWORD m_iv;
};
#endif