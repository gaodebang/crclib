#include <stdio.h>
/*ref 36-212 v8.6.0 , pp 8-9 */
/* the highest degree is set by default */
// 38.212 5.1中定义了CRC24A,CRC24B,CRC24C,CRC16,CRC11,CRC6等6种生成多项式。
// 36.212 5.1.1定义了CRC24A,CRC24B,CRC16,CRC8共4中生成多项式
// 25.212 4.2.1.1定义了CRC24,CRC16,CRC12,CRC8共4中生成多项式
// 为方便计算，生成多项式已经去掉了最高位，所得到的即为一位除法的余数，例如：poly24a=0x100000000^0x1864cfb00
uint32_t poly24a = 0x864cfb00; // 1000 0110 0100 1100 1111 1011
                                   // D^24 + D^23 + D^18 + D^17 + D^14 + D^11 + D^10 + D^7 + D^6 + D^5 + D^4 + D^3 + D + 1
uint32_t poly24b = 0x80006300; // 1000 0000 0000 0000 0110 0011
                                   // D^24 + D^23 + D^6 + D^5 + D + 1
uint32_t poly24c = 0xb2b11700; // 1011 0010 1011 0001 0001 0111
                                   // D^24+D^23+D^21+D^20+D^17+D^15+D^13+D^12+D^8+D^4+D^2+D+1

uint32_t poly16 = 0x10210000; // 0001 0000 0010 0001            D^16 + D^12 + D^5 + 1
uint32_t poly12 = 0x80F00000; // 1000 0000 1111                 D^12 + D^11 + D^3 + D^2 + D + 1
uint32_t poly8 = 0x07000000;  // 1001 1011                      D^8  + D^7  + D^4 + D^3 + D + 1
uint32_t poly6 = 0x84000000;      // 10000100000... -> D^6+D^5+1
uint32_t poly11 = 0xc4200000;     // 11000100001000... -> D^11+D^10+D^9+D^5+1

/*********************************************************

For initialization && verification purposes,
   bit by bit implementation with any polynomial

The first bit is in the MSB of each byte

*********************************************************/
uint32_t crcbit(uint8_t *inputptr, // crc按位计算实现
                    int octetlen,
                    uint32_t poly)
{
    uint32_t i, crc = 0, c; // i，循环变量; crc，用来存储上一位计算的余数; c，是从外部输入的需要生成校验码的数据。

    while (octetlen-- > 0)
    {                            // octetlen表示需要计算的输入数据的个数，因为是1, 所以这个循环只执行一次，也就是每次计算一个数据的crc
        c = (*inputptr++) << 24; // 因为生成多项式最大是24位的，因此输入数据左移24位，补零

        for (i = 8; i != 0; i--)
        {                                // 计算8位数据的crc，每次计算1bit
            if (((uint32_t)1 << 31) & (c ^ crc))   // 如果c的最高位，也就是当前计算的数据位和crc的最高位，也就是上一位计算的余数的最高位其中之一为1
                crc = (crc << 1) ^ poly; // 上一位计算的余数乘2(crc<<1)再加上当前位计算的余数(poly)（模2加法，即异或）
            else
                crc <<= 1; // 如果不满足上述条件，crc直接左移一位，进入下一位计算

            c <<= 1; // 输入数据左移一位，准备计算下一位
        }
    }

    return crc;
}

/*********************************************************

crc table initialization

*********************************************************/
static uint32_t crc24aTable[256]; // 所有crc table的大小都定义为256,也就是2^8,说明这是一个8位数据的table
static uint32_t crc24bTable[256]; // 里面保存了所有8位数据的CRC结果
static uint32_t crc24cTable[256];
static uint16_t crc16Table[256];
static uint16_t crc12Table[256];
static uint8_t crc8Table[256];

void crcTableInit(void)
{
    uint8_t c = 0; // 定义一个无符号字符变量，因为占用1个字节，因此它的最大值只能是255

    do
    {
        crc24aTable[c] = crcbit(&c, 1, poly24a);
        crc24bTable[c] = crcbit(&c, 1, poly24b);
        crc24cTable[c] = crcbit(&c, 1, poly24c);
        crc16Table[c] = (uint16_t)(crcbit(&c, 1, poly16) >> 16);
        crc12Table[c] = (uint16_t)(crcbit(&c, 1, poly12) >> 16);
        crc8Table[c] = (uint8_t)(crcbit(&c, 1, poly8) >> 24);
    } while (++c); // 从0到255循环，为每一个crc table填写00000000～11111111的crc校验码
}

static uint8_t crc8_table[256];

//注意，低位先传送时（比如串口是先传送的低位，所以反序计算是必要的），生成多项式应反转(低位与高位互换)。如CRC16-CCITT为0x1021，反转后为0x8408
void crc8_table_r_init(void)//计算反序列crc table
{
    uint32_t i, j;
    uint8_t c;
    uint8_t poly = CRC8_POLY;

    for (i = 0; i < 256; i++)
    {
        c = i;

        for (j = 0; j < 8; j++)
        {
            if (c & 0x01)
                c = poly ^ (c >> 1);
            else
                c >>= 1;
        }

        crc8_table[i] = c;
    }
}