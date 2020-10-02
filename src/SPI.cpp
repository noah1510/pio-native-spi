/* 
 SPI.cpp - SPI library for esp8266
 Copyright (c) 2015 Hristo Gochkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "SPI.h"
#include <iostream>
//#include "HardwareSerial.h"

#define SPI_PINS_HSPI			0 // Normal HSPI mode (MISO = GPIO12, MOSI = GPIO13, SCLK = GPIO14);
#define SPI_PINS_HSPI_OVERLAP	1 // HSPI Overllaped in spi0 pins (MISO = SD0, MOSI = SDD1, SCLK = CLK);

#define SPI_OVERLAP_SS 0

#define ESP8266_CLOCK 80000000UL

typedef union {
        uint32_t regValue;
        struct {
                unsigned regL :6;
                unsigned regH :6;
                unsigned regN :6;
                unsigned regPre :13;
                unsigned regEQU :1;
        };
} spiClk_t;

SPIClass::SPIClass() {
    useHwCs = false;
    pinSet = SPI_PINS_HSPI;
}

bool SPIClass::pins(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
{
    if (sck == 6 &&
        miso == 7 &&
        mosi == 8 &&
        ss == 0) {
        pinSet = SPI_PINS_HSPI_OVERLAP;
    } else if (sck == 14 &&
	           miso == 12 &&
               mosi == 13) {
        pinSet = SPI_PINS_HSPI;
    } else {
        return false;
    }

    return true;
}

void SPIClass::begin() {
    if(SPIClass::SPI_CONSOLE_OUTPUTS){
        switch (pinSet) {
        case SPI_PINS_HSPI_OVERLAP:
            std::cout << "calling setHwCs(true)" << std::endl;
            setHwCs(true);
            break;
        case SPI_PINS_HSPI:
        default:
            std::cout << "setting pinmode to SPECIAL for " << SCK << ", " << MISO << " and " << MOSI << std::endl;
            break;
        }
    }

    setFrequency(1000000); ///< 1MHz
}

void SPIClass::end() {
    if(SPIClass::SPI_CONSOLE_OUTPUTS){
        switch (pinSet) {
        case SPI_PINS_HSPI:
            std::cout << "setting pinmode to INPUT for " << SCK << ", " << MISO << " and " << MOSI << std::endl;
            if (useHwCs) {
                std::cout << "setting pinmode to INPUT for " << SS << std::endl;
            }
            break;
        case SPI_PINS_HSPI_OVERLAP:
            if (useHwCs) {
                std::cout << "setting pinmode to INPUT for " << SPI_OVERLAP_SS << std::endl;
            }
            break;
        }
    }
}

void SPIClass::setHwCs(bool use) {
    if(SPIClass::SPI_CONSOLE_OUTPUTS){
        switch (pinSet) {
        case SPI_PINS_HSPI:
            if (use) {
                std::cout << "setting pinmode to SPECIAL for " << SS << std::endl;
            } else {
                if (useHwCs) {
                    std::cout << "setting pinmode to INPUT for " << SS << std::endl;
                }
            }
            break;
        case SPI_PINS_HSPI_OVERLAP:
            if (use) {
                std::cout << "setting pinmode to FUNCTION_1 for " << SPI_OVERLAP_SS << std::endl;
            }
            else {
                if (useHwCs) {
                    std::cout << "setting pinmode to INPUT for " << SPI_OVERLAP_SS << std::endl;
                }
            }
            break;
        }
    }

    useHwCs = use;
}

void SPIClass::beginTransaction(SPISettings settings) {
    currentSPISettings = settings;
}

void SPIClass::endTransaction() {
}

void SPIClass::setDataMode(uint8_t dataMode) {
    currentSPISettings._dataMode = dataMode;
}

void SPIClass::setBitOrder(uint8_t bitOrder) {
    currentSPISettings._bitOrder = bitOrder;
}

/**
 * calculate the Frequency based on the register value
 * @param reg
 * @return
 */
static uint32_t ClkRegToFreq(spiClk_t * reg) {
    return (ESP8266_CLOCK / ((reg->regPre + 1) * (reg->regN + 1)));
}

void SPIClass::setFrequency(uint32_t freq) {
    currentSPISettings._clock = freq;
}

void SPIClass::setClockDivider(uint32_t clockDiv) {
    currentClockDivider = clockDiv;
}

inline void SPIClass::setDataBits(uint16_t bits) {
    currentDataBits = bits;
}

uint8_t SPIClass::transfer(uint8_t data) {
    // reset to 8Bit mode
    setDataBits(8);
    if(SPIClass::SPI_CONSOLE_OUTPUTS){
        std::cout << "transfering " << data << std::endl;
    }
    return data;
}

uint16_t SPIClass::transfer16(uint16_t data) {
    write16(data,currentSPISettings._bitOrder);
    return data;
}

void SPIClass::transfer(void *buf, uint16_t count) {
    uint8_t *cbuf = reinterpret_cast<uint8_t*>(buf);

    // cbuf may not be 32bits-aligned
    for (; (((unsigned long)cbuf) & 3) && count; cbuf++, count--)
        *cbuf = transfer(*cbuf);

    // cbuf is now aligned
    // count may not be a multiple of 4
    uint16_t count4 = count & ~3;
    transferBytes(cbuf, cbuf, count4);

    // finish the last <4 bytes
    cbuf += count4;
    count -= count4;
    for (; count; cbuf++, count--)
        *cbuf = transfer(*cbuf);
}

void SPIClass::write(uint8_t data) {
    transfer(data);
}

void SPIClass::write16(uint16_t data) {
    transfer16(data);
}

void SPIClass::write16(uint16_t data, bool msb) {
    uint16_t dat;
    if(msb) {
        // MSBFIRST Byte first
        dat = (data >> 8) | (data << 8);
    } else {
        // LSBFIRST Byte first
        dat = data;
    }

    transfer((dat >> 8) & 0xFF);
    transfer(dat & 0xFF);
}

void SPIClass::write32(uint32_t data) {
    write32(data, currentSPISettings._bitOrder == MSBFIRST);
}

void SPIClass::write32(uint32_t data, bool msb) {
    // Set to 32Bits transfer
    setDataBits(32);
    if(msb) {
        union {
                uint32_t l;
                uint8_t b[4];
        } data_;
        data_.l = data;
        // MSBFIRST Byte first
        data = (data_.b[3] | (data_.b[2] << 8) | (data_.b[1] << 16) | (data_.b[0] << 24));
    }
    write16((data >> 16) & 0xFFFF, msb);
    write16(data & 0xFFFF, msb);
}

/**
 * Note:
 *  data need to be aligned to 32Bit
 *  or you get an Fatal exception (9)
 * @param data uint8_t *
 * @param size uint32_t
 */
void SPIClass::writeBytes(const uint8_t * data, uint32_t size) {
    while(size) {
        if(size > 64) {
            writeBytes_(data, 64);
            size -= 64;
            data += 64;
        } else {
            writeBytes_(data, size);
            size = 0;
        }
    }
}

void SPIClass::writeBytes_(const uint8_t * data, uint8_t size) {
    // Set Bits to transfer
    setDataBits(size * 8);

    const uint32_t * dataPtr = (uint32_t*) data;
    uint32_t dataSize = ((size + 3) / 4);

    while(dataSize--) {
        write32(*dataPtr);
        dataPtr++;
    }
}

/**
 * @param data uint8_t *
 * @param size uint8_t  max for size is 64Byte
 * @param repeat uint32_t
 */
void SPIClass::writePattern(const uint8_t * data, uint8_t size, uint32_t repeat) {
    if(size > 64) return; //max Hardware FIFO

    if(data == nullptr && size != 0 ){
        return;
    }

    for(uint32_t i = 0;i < repeat; i++){
        for(uint8_t j = 0; j < size;j++){
            write(data[j]);
        }
    }

}

/**
 * @param out uint8_t *
 * @param in  uint8_t *
 * @param size uint32_t
 */
void SPIClass::transferBytes(const uint8_t * out, uint8_t * in, uint32_t size) {
    while(size) {
        if(size > 64) {
            transferBytes_(out, in, 64);
            size -= 64;
            if(out) out += 64;
            if(in) in += 64;
        } else {
            transferBytes_(out, in, size);
            size = 0;
        }
    }
}

/**
 * Note:
 *  in and out need to be aligned to 32Bit
 *  or you get an Fatal exception (9)
 * @param out uint8_t *
 * @param in  uint8_t *
 * @param size uint8_t (max 64)
 */
void SPIClass::transferBytesAligned_(const uint8_t * out, uint8_t * in, uint8_t size) {
    transferBytes_(out,in,size);
}


void SPIClass::transferBytes_(const uint8_t * out, uint8_t * in, uint8_t size) {
    if(out == nullptr || in == nullptr || size == 0){
        return;
    }

    for(uint8_t i = 0; i < size;i++){
        uint8_t dat = out[i];
        in[i] = transfer(dat);
    }
    
}


#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SPI)
SPIClass SPI;
#endif