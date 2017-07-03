
#include "StdAfx.h"

#include <AzTest/AzTest.h>

class Cubism3Test
    : public ::testing::Test
{
protected:
    void SetUp() override
    {

    }

    void TearDown() override
    {

    }
};

TEST_F(Cubism3Test, ExampleTest)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
