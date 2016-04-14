#include <iostream>
#include <stdio.h>
#include <assert.h>
#include "Message.h"

using namespace beyondy::Async;

#define EQUAL(a, b) do { if ((a)!=(b)) \
	std::cerr << "a(" << (a) << ") != b(" << (b) << ") at " << __FILE__ << ", " << __LINE__ << std::endl; } while(0)
int main(int argc, char **argv)
{
	{
		unsigned char buf[1]; Message msg(buf, sizeof buf); 
		int8_t a = 0x69, b = 1; 
		msg.writeInt8(a); msg.readInt8(b); EQUAL(a, b);
		a = 0x99, b = 1; 
		msg.reset();
		msg.writeInt8(a); msg.readInt8(b); EQUAL(a, b);
	}
	{
		unsigned char buf[1]; Message msg(buf, sizeof buf); 
		uint8_t a = 0x69, b = 1; 
		msg.writeUint8(a); EQUAL(msg.getWptr(), 1); msg.readUint8(b); EQUAL(msg.getRptr(), 1); EQUAL(a, b);
		a = 0x99, b = 1; 
		msg.reset();
		msg.writeUint8(a); EQUAL(msg.getWptr(), 1); msg.readUint8(b); EQUAL(msg.getRptr(), 1); EQUAL(a, b);
	}

	{
		unsigned char buf[2]; Message msg(buf, sizeof buf); 
		int16_t a = 0x69f9, b = 1; 
		msg.writeInt16(a); EQUAL(msg.getWptr(), 2); msg.readInt16(b); EQUAL(msg.getRptr(), 2); EQUAL(a, b);
		a = 0x99f3, b = 1;
		msg.reset();
		msg.writeInt16(a); EQUAL(msg.getWptr(), 2); msg.readInt16(b); EQUAL(msg.getRptr(), 2); EQUAL(a, b);
	}
	{
		unsigned char buf[2]; Message msg(buf, sizeof buf); 
		uint16_t a = 0x69f9, b = 1; 
		msg.writeUint16(a); EQUAL(msg.getWptr(), 2); msg.readUint16(b); EQUAL(msg.getRptr(), 2); EQUAL(a, b);
		a = 0x99fb, b = 1; 
		msg.reset();
		msg.writeUint16(a); EQUAL(msg.getWptr(), 2); msg.readUint16(b); EQUAL(msg.getRptr(), 2); EQUAL(a, b);
	}
	  
	{
		unsigned char buf[4]; Message msg(buf, sizeof buf); 
		int32_t a = 0x69f9abcd, b = 1; 
		msg.writeInt32(a); EQUAL(msg.getWptr(), 4); msg.readInt32(b); EQUAL(msg.getRptr(), 4); EQUAL(a, b);
		a = 0x99f3abcd, b = 1;
		msg.reset();
		msg.writeInt32(a); EQUAL(msg.getWptr(), 4); msg.readInt32(b); EQUAL(msg.getRptr(), 4); EQUAL(a, b);
	}
	{
		unsigned char buf[4]; Message msg(buf, sizeof buf); 
		uint32_t a = 0x69f9abcd, b = 1; 
		msg.writeUint32(a); EQUAL(msg.getWptr(), 4); msg.readUint32(b); EQUAL(msg.getRptr(), 4); EQUAL(a, b);
		a = 0x99fbdeff, b = 1; 
		msg.reset();
		msg.writeUint32(a); EQUAL(msg.getWptr(), 4); msg.readUint32(b); EQUAL(msg.getRptr(), 4); EQUAL(a, b);
	}

	{
		unsigned char buf[8]; Message msg(buf, sizeof buf); 
		int64_t a = 0x69f9abcdefababcdLL, b = 1; 
		msg.writeInt64(a); EQUAL(msg.getWptr(), 8); msg.readInt64(b); EQUAL(msg.getRptr(), 8); EQUAL(a, b);
		a = 0x99f3abcdcdefabcd, b = 1;
		msg.reset();
		msg.writeInt64(a); EQUAL(msg.getWptr(), 8); msg.readInt64(b); EQUAL(msg.getRptr(), 8); EQUAL(a, b);
	}
	{
		unsigned char buf[8]; Message msg(buf, sizeof buf); 
		uint64_t a = 0x69f9fdfeabcdfefeLL, b = 1; 
		msg.writeUint64(a); EQUAL(msg.getWptr(), 8); msg.readUint64(b); EQUAL(msg.getRptr(), 8); EQUAL(a, b);
		a = 0x99fbabcdfefefefeLL, b = 0x99fbabcdfefefefe;
		msg.reset();
		msg.writeUint64(a); EQUAL(msg.getWptr(), 8); msg.readUint64(b); EQUAL(msg.getRptr(), 8); EQUAL(a, b);
	}

	unsigned char buf[15] = { 0 };
	Message msg(buf, sizeof buf);

	int8_t a1, b1 = 0;
	int16_t a2, b2 = 0;
	int32_t a3, b3 = 0;
	int64_t a4, b4 = 0;

	a1 = 0x79;
	a2 = 0x7fff;
	a3 = 0x7fffffff;
	a4 = 0x7fffffffffffffffLL;

	msg.writeInt8(a1);
	msg.writeInt16(a2);
	msg.writeInt32(a3);
	msg.writeInt64(a4);
	EQUAL(msg.getWptr(), 15);

	msg.readInt8(b1); assert(a1 == b1);
	msg.readInt16(b2); assert(a2 == b2);
	msg.readInt32(b3); assert(a3 == b3);
	msg.readInt64(b4); assert(a4 == b4);
	EQUAL(msg.getRptr(), 15);

	return 0;
}
