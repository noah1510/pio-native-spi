#include "SPI.h"
#include "unity.h"

bool spi_begin(uint8_t bitOrder, uint8_t dataMode){
    try{
        SPI.setBitOrder(MSBFIRST);
        SPI.setDataMode(SPI_MODE0);
        SPI.begin();
        SPI.end();
    }catch (std::exception){
        return false;
    }

    return true;
}

void begin_test(){
    auto bitOrders = {MSBFIRST, LSBFIRST};
    auto dataModes = {SPI_MODE0,SPI_MODE1,SPI_MODE2,SPI_MODE3};

    for(auto order : bitOrders){
        for (auto mode : dataModes){
            TEST_ASSERT_TRUE(spi_begin(order,mode));
        }
    }
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    RUN_TEST(begin_test);

    UNITY_END();

    return 0;
}
