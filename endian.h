#ifndef _endian_h_
#define _endian_h_


typedef unsigned int uint;

template<typename T>
T EndianConvert(T Value)
{
    const uint BitsPerByte = 8;
    const uint SizeInBytes = sizeof(T);
    const uint InitialByteOffset = BitsPerByte * (SizeInBytes - 1);

    T LowByteMask = 0xFF;
    T HighByteMask = 0xFF << InitialByteOffset;

    uint LowByteOffset = 0;
    uint HighByteOffset = InitialByteOffset;

    for (uint Counter = 0; Counter < SizeInBytes / 2; Counter++)
    {
        T LowByte = Value & LowByteMask;
        T HighByte = Value & HighByteMask;

        T ToggleMask = ~(LowByteMask | HighByteMask);
        Value &= ToggleMask;

        uint ByteOffset = HighByteOffset - LowByteOffset;
        Value |= LowByte << ByteOffset;
        Value |= HighByte >> ByteOffset;

        LowByteOffset += BitsPerByte;
        HighByteOffset -= BitsPerByte;

        LowByteMask <<= BitsPerByte;
        HighByteMask >>= BitsPerByte;
    }

    return Value;
}


#endif
