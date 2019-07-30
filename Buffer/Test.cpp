#include "buffer.h"

#include <iostream>
#include "gtest/gtest.h"

#ifdef _DEBUG  
#pragma comment(lib, "gtestd.lib")  
#pragma comment(lib, "gtest_maind.lib")  
#else  
#pragma comment(lib, "gtest.lib")  
#pragma comment(lib, "gtest_main.lib")   
#endif


TEST(TestRW, RWLen)
{
	char r[255]{ 0 };
	Buffer a("123");

	// TestWrite
	a.write("123", 3);
	a.write("ÈËÃñ±Ò", 4);
	EXPECT_EQ(10, a.len());
	EXPECT_EQ(10, a.size());

	// TestRead
	EXPECT_EQ(3, a.read(r, 3));
	EXPECT_EQ(7, a.len());
	EXPECT_EQ(7, a.size());
	a.write("123", 3);

	// TestMove
	Buffer b(std::move(a));
	EXPECT_EQ(10, b.len());
	EXPECT_EQ(10, b.size());
	EXPECT_EQ(0, a.len());
	EXPECT_EQ(0, a.size());

	// TestFindSucess
	EXPECT_EQ(3, b.find("ÈË", 1));

	// TestRead
	EXPECT_EQ(10, b.read(r, 20));


	// TestWrite
	a.write(r, 3);

	// TestWrite Object
	b.write(a);
	EXPECT_EQ(3, a.len());
	EXPECT_EQ(3, a.size());
	EXPECT_EQ(3, b.len());
	EXPECT_EQ(3, b.size());

	// TestFindFailed
	EXPECT_EQ(-1, b.find("ÈË", 1));

	// TestMove
	Buffer c;
	c = std::move(b);
	EXPECT_EQ(0, b.len());
	EXPECT_EQ(0, b.size());
	EXPECT_EQ(3, c.len());
	EXPECT_EQ(3, c.size());

	// TestContent
	memset(r, 0, 255);
	c.read(r, c.len());
	EXPECT_EQ(0, strcmp(r, "123"));
}

int main(int argc, char* argv[]) {

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
