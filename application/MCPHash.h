#pragma once
#include <stdint.h>
#include <string.h>
#include "ida_macros.h"

class MCPHash
{
public:
    // �޸Ĳ������ͣ�ȷ��64λ������
    static uint32_t StringID(const char* a1, int32_t a2)
    {
        // ʹ�ñ�׼�̶��������ͣ��������������
        uint32_t eax = 0;
        uint32_t ebx = 0;
        uint32_t ecx = 0;
        uint32_t edx = 0;
        uint32_t esi = 0;
        uint32_t edi = 0;

        uint32_t ebp_1c = 0;
        uint32_t ebp_18 = 0;
        uint32_t ebp_14 = 0;
        uint32_t ebp_10 = 0;
        uint32_t ebp_c = 0;
        uint32_t ebp_8 = 0;
        uint32_t ebp_4 = 0;

        // ��ָ��ͳ��ȼ�飬���ӱ߽籣��
        if (a1 == nullptr || a2 <= 0)
            return 0;

        // ��ʼ��
        ebp_14 = ebx;
        ebp_18 = esi;
        // 64λָ�밲ȫת����ֻȡ��32λ������ԭ�߼�������ضϾ���
        ecx = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(a1) & 0xFFFFFFFF);
        esi = static_cast<uint32_t>(a2);
        eax = 0xF4FA8928;
        ebp_10 = eax;
        ebx = 0x37A8470E;
        ebp_1c = edi;
        edi = 0x7758B42B;
        ebp_c = edi;
        uint64_t result;
        uint32_t temp_ecx;
        if (esi != 0)
        {
            while (true)
            {
                // ��ȫ��ȡ�ֽڣ�����Խ��
                eax = static_cast<uint8_t>(*(reinterpret_cast<const uint8_t*>(a1) + (ecx - static_cast<uint32_t>(reinterpret_cast<uintptr_t>(a1) & 0xFFFFFFFF))));
                ecx++;
                ebp_c = eax;
                esi--;
                if (esi == 0) goto LABEL_21;

                // �޸��ֽڶ�ȡ����λ�߼�
                uint8_t temp_byte = static_cast<uint8_t>(*(reinterpret_cast<const uint8_t*>(a1) + (ecx - static_cast<uint32_t>(reinterpret_cast<uintptr_t>(a1) & 0xFFFFFFFF))));
                edx = static_cast<uint32_t>(temp_byte);
                eax = ecx + 1;
                edx = edx & 0xFF; // ȷ��ֻ������8λ
                if (esi != 0) ecx = eax;
                edx <<= 8;
                edx |= (ebp_c & 0xFF); // ֻ����ebp_c�ĵ�8λ
                esi = esi - 1; // �滻0xFFFFFFFF�ӷ������������չ

                if (esi != 0)
                {
                    temp_byte = static_cast<uint8_t>(*(reinterpret_cast<const uint8_t*>(a1) + (ecx - static_cast<uint32_t>(reinterpret_cast<uintptr_t>(a1) & 0xFFFFFFFF))));
                    ((uint8_t*)&ebp_4)[3] = temp_byte;
                }
                else
                {
                    ((uint8_t*)&ebp_4)[3] = 0x00;
                }

                eax = ecx + 1;
                if (esi != 0)
                {
                    ecx = eax;
                }

                eax = static_cast<uint32_t>(((uint8_t*)&ebp_4)[3]);
                eax <<= 16;
                eax |= (edx & 0xFFFF); // ֻ����edx��16λ
                edx = esi - 1;
                ebp_c = eax;

                if (esi == 0)
                {
                    esi = static_cast<uint16_t>(eax); // ֻ������16λ
                    goto LABEL_19;
                }

                if (esi == 1)
                {
                    ((uint8_t*)&ebp_4)[3] = static_cast<uint8_t>(edx);
                }
                else
                {
                    temp_byte = static_cast<uint8_t>(*(reinterpret_cast<const uint8_t*>(a1) + (ecx - static_cast<uint32_t>(reinterpret_cast<uintptr_t>(a1) & 0xFFFFFFFF))));
                    eax = (eax & 0xFFFFFF00) | static_cast<uint32_t>(temp_byte);
                    ecx++;
                    ((uint8_t*)&ebp_4)[3] = static_cast<uint8_t>(eax);
                }

                esi = static_cast<uint32_t>(((uint8_t*)&ebp_4)[3]);
                eax = edx;
                esi <<= 24;
                edx--;
                esi |= (ebp_c & 0xFFFFFF); // ֻ������24λ
                a2 = static_cast<int32_t>(edx);

                if (eax == 0) goto LABEL_19;

                // ��ϣ���ļ����߼��޸�
                eax = ebp_10;
                edi ^= esi;
                edx = eax;
                ebp_c = edi;
                eax = (eax << 1) | (eax >> 31); // ��ȫ������+��λ
                ebx ^= esi;
                edi = eax;
                ebp_10 = eax;
                eax = ebp_c;
                edi ^= 0x267B0B11;
                eax += edi;
                ebp_8 = ebx;
                eax &= 0xBFEF7FDF;
                eax |= 0x2040801;

                // 64λ�˷���ȫ����
                uint64_t result = static_cast<uint64_t>(eax) * static_cast<uint64_t>(ebx);
                eax = static_cast<uint32_t>(result & 0xFFFFFFFF);
                edx = static_cast<uint32_t>(result >> 32);

                // �޸���Ԫ��������ȼ�����
                ebx = (edx != 0) ? 1 : 0;
                esi = 0;
                ebx += eax;
                esi += ((ebx < eax) ? 1 : 0); // �޸����ȼ�����������

                eax = ebp_8;
                ebx += edx;
                esi += ((ebx < edx) ? 1 : 0); // �޸����ȼ�
                eax += edi;
                eax &= 0x7D7EBBDE;
                esi += ebx;
                eax |= 0x804021;
                ebp_8 = esi;

                // 64λ�˷���ȫ����
                result = static_cast<uint64_t>(eax) * static_cast<uint64_t>(ebp_c);
                eax = static_cast<uint32_t>(result & 0xFFFFFFFF);
                edx = static_cast<uint32_t>(result >> 32);

                ebx = ebp_8;
                edi = 0;
                edi = shld(edi, edx, 1); // ʹ���޸����shld

                esi = eax;
                eax = 0;
                edx += edx;
                edi += edx;
                eax += ((edi < edx) ? 1 : 0); // �޸����ȼ�
                edi += esi;
                eax += ((edi < esi) ? 1 : 0); // �޸����ȼ�
                esi = static_cast<uint32_t>(a2);

                edi = edi + (eax * 2);
                ebp_c = edi;

                if (esi == 0) goto LABEL_20;
            }

        LABEL_21:
            esi = static_cast<uint8_t>(eax); // ֻ������8λ
        LABEL_19:
            eax = ebp_10;
            ebx ^= esi;
            ecx = eax;
            ebp_8 = ebx;
            eax = (eax << 1) | (eax >> 31); // ��ȫ����
            ecx >>= 31;
            eax |= ecx;
            edi ^= esi;
            esi = eax;
            ebp_10 = eax;
            esi ^= 0x267B0B11;
            eax = esi + edi;
            eax &= 0xBFEF7FDF;
            eax |= 0x2040801;

            // 64λ�˷���ȫ����
            result = static_cast<uint64_t>(eax) * static_cast<uint64_t>(ebx);
            eax = static_cast<uint32_t>(result & 0xFFFFFFFF);
            edx = static_cast<uint32_t>(result >> 32);

            // �޸��߼���������ȼ�����
            ebx = 0;
            temp_ecx = (edx != 0) ? 1 : 0;
            temp_ecx = 0;
            temp_ecx += eax;
            temp_ecx += ((ebx < eax) ? 1 : 0); // �޸����ȼ�
            eax = ebp_8;
            ebx += edx;
            temp_ecx += ((ebx < edx) ? 1 : 0); // �޸����ȼ�
            eax += esi;
            eax &= 0x7D7EBBDE;
            temp_ecx += ebx;
            eax |= 0x804021;
            ebp_8 = temp_ecx;

            // 64λ�˷���ȫ����
            result = static_cast<uint64_t>(eax) * static_cast<uint64_t>(edi);
            eax = static_cast<uint32_t>(result & 0xFFFFFFFF);
            edx = static_cast<uint32_t>(result >> 32);

            ebx = ebp_8;
            esi = 0;
            esi = shld(esi, edx, 1); // ʹ���޸����shld
            ecx = eax;
            eax = 0;
            edx += edx;
            esi += edx;
            eax += ((esi < edx) ? 1 : 0); // �޸����ȼ�
            esi += ecx;
            eax += ((esi < ecx) ? 1 : 0); // �޸����ȼ�
            eax = esi + (eax * 2);
            ebp_c = eax;

        LABEL_20:
            eax = ebp_10;
        }

        // ���չ�ϣ����
        edi = eax;
        ebx ^= 0x9BE74448;
        eax = (eax << 1) | (eax >> 31); // ��ȫ����
        edi >>= 31;
        edi |= eax;
        ebp_8 = ebx;
        eax = ebp_c;
        eax ^= 0x9BE74448;
        esi = edi;
        esi ^= 0x267B0B11;
        ebp_c = eax;
        uint32_t ecx_final = 0;
        eax += esi;
        eax &= 0xBFEF7FDF;
        eax |= 0x2040801;

        // 64λ�˷���ȫ����
        result = static_cast<uint64_t>(eax) * static_cast<uint64_t>(ebx);
        eax = static_cast<uint32_t>(result & 0xFFFFFFFF);
        edx = static_cast<uint32_t>(result >> 32);

        // �޸���Ԫ��������ȼ�
        ecx_final = (edx != 0) ? 1 : 0;
        ebx = 0;
        ecx_final += eax;
        ebx += ((ecx_final < eax) ? 1 : 0); // �޸����ȼ�
        eax = ebp_8;
        ecx_final += edx;
        ebx += ((ecx_final < edx) ? 1 : 0); // �޸����ȼ�
        eax += esi;
        eax &= 0x7D7EBBDE;
        esi = 0;
        eax |= 0x804021;
        ebx += ecx_final;

        // 64λ�˷���ȫ����
        result = static_cast<uint64_t>(eax) * static_cast<uint64_t>(ebp_c);
        eax = static_cast<uint32_t>(result & 0xFFFFFFFF);
        edx = static_cast<uint32_t>(result >> 32);

        esi = shld(esi, edx, 1); // ʹ���޸����shld
        ecx_final = eax;
        eax = 0;
        edx += edx;
        esi += edx;
        eax += ((esi < edx) ? 1 : 0); // �޸����ȼ�
        esi += ecx_final;
        eax += ((esi < ecx_final) ? 1 : 0); // �޸����ȼ�
        edi = (edi << 1) | (edi >> 31); // �޸�ѭ����λ
        edi ^= 0x267B0B11;
        ebx ^= 0x66F42C48;
        ecx_final = 0;
        ebp_10 = edi;
        esi = esi + (eax * 2);
        esi ^= 0x66F42C48;
        eax = edi + esi;
        eax &= 0xBFEF7FDF;
        eax |= 0x2040801;

        // 64λ�˷���ȫ����
        result = static_cast<uint64_t>(eax) * static_cast<uint64_t>(ebx);
        eax = static_cast<uint32_t>(result & 0xFFFFFFFF);
        edx = static_cast<uint32_t>(result >> 32);

        // �޸���Ԫ��������ȼ�
        ecx_final = (edx != 0) ? 1 : 0;
        edi = 0;
        ecx_final += eax;
        edi += ((ecx_final < eax) ? 1 : 0); // �޸����ȼ�
        eax = ebp_10;
        ecx_final += edx;
        edi += ((ecx_final < edx) ? 1 : 0); // �޸����ȼ�
        eax += ebx;
        eax &= 0x7D7EBBDE;
        edi += ecx_final;
        eax |= 0x804021;

        // 64λ�˷���ȫ����
        result = static_cast<uint64_t>(eax) * static_cast<uint64_t>(esi);
        eax = static_cast<uint32_t>(result & 0xFFFFFFFF);
        edx = static_cast<uint32_t>(result >> 32);

        esi = 0;
        esi = shld(esi, edx, 1); // ʹ���޸����shld
        ecx_final = eax;
        eax = 0;
        edx += edx;
        esi += edx;
        eax += ((esi < edx) ? 1 : 0); // �޸����ȼ�
        esi += ecx_final;
        eax += ((esi < ecx_final) ? 1 : 0); // �޸����ȼ�
        eax = esi + (eax * 2);
        eax ^= edi;

        return eax;
    }

private:
    // �޸�SHLDָ��ʵ�֣������߽����
    static uint32_t shld(uint32_t dest, uint32_t src, int count)
    {
        // ȷ����λ�����ںϷ���Χ��
        count = count & 0x1F; // 32λ��λ����
        if (count == 0)
            return dest;
        if (count >= 32)
            return src;
        // ��ȷ��SHLDʵ�֣���src�ĸ�λ�Ƶ�dest�ĵ�λ
        return (dest << count) | (src >> (32 - count));
    }
};

class NeoXHash {
public:
    static uint32_t StringIDLegacy(const char* str, int len) {
        if (!str || len < 1) return 0;

        uint32_t A = 0xF4FA8928u;
        uint32_t B = 0x37A8470Eu;
        uint32_t C = 0x7758B42Bu;

        const uint8_t* p = reinterpret_cast<const uint8_t*>(str);

        int i = 0;
        for (; i + 4 <= len; i += 4)
            round(A, B, C, load_le32(p + i));

        if (i < len)
            round(A, B, C, load_tail(p + i, len - i));

        finalize_round(A, B, C, 0x9BE74448u, 0x66F42C48u);
        finalize_round(A, B, C, 0u, 0u);

        return B ^ C;
    }

private:
    static uint32_t rol32(uint32_t x, int n) {
        return (x << n) | (x >> (32 - n));
    }

    static uint32_t load_le32(const uint8_t* p) {
        return uint32_t(p[0]) | uint32_t(p[1]) << 8
            | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
    }

    static uint32_t load_tail(const uint8_t* p, int n) {
        uint32_t v = 0;
        for (int i = 0; i < n; ++i)
            v |= uint32_t(p[i]) << (i * 8);
        return v;
    }

    static uint32_t reduce_B(uint64_t prod) {
        const uint32_t lo = uint32_t(prod);
        const uint32_t hi = uint32_t(prod >> 32);
        const uint32_t r0 = lo + (hi ? 1u : 0u);
        const uint32_t c0 = r0 < lo ? 1u : 0u;
        const uint32_t r1 = r0 + hi;
        const uint32_t c1 = r1 < hi ? 1u : 0u;
        return r1 + c0 + c1;
    }

    static uint32_t reduce_C(uint64_t prod) {
        const uint32_t lo = uint32_t(prod);
        const uint32_t hi = uint32_t(prod >> 32);
        const uint32_t rhi = rol32(hi, 1);
        const uint32_t r = rhi + lo;
        return r + (r < lo ? 2u : 0u);
    }

    static void round(uint32_t& A, uint32_t& B, uint32_t& C, uint32_t chunk) {
        A = rol32(A, 1);
        const uint32_t salt = A ^ 0x267B0B11u;
        const uint32_t xB = B ^ chunk;
        const uint32_t xC = C ^ chunk;
        const uint32_t m1 = ((xC + salt) & 0xBDEB77DEu) | 0x02040801u;
        const uint32_t m2 = ((xB + salt) & 0x7D7EBBDEu) | 0x00804021u;
        B = reduce_B(uint64_t(m1) * xB);
        C = reduce_C(uint64_t(m2) * xC);
    }

    static void finalize_round(uint32_t& A, uint32_t& B, uint32_t& C,
        uint32_t xor_in, uint32_t xor_out) {
        A = rol32(A, 1);
        const uint32_t salt = A ^ 0x267B0B11u;
        const uint32_t xB = B ^ xor_in;
        const uint32_t xC = C ^ xor_in;
        const uint32_t m1 = ((xC + salt) & 0xBDEB77DEu) | 0x02040801u;
        const uint32_t m2 = ((xB + salt) & 0x7D7EBBDEu) | 0x00804021u;
        B = reduce_B(uint64_t(m1) * xB) ^ xor_out;
        C = reduce_C(uint64_t(m2) * xC) ^ xor_out;
    }
};