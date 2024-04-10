#ifndef _BYTEBUFFER_H_
#define _BYTEBUFFER_H_
#define DEFAULT_SIZE 4096
#define BB_UTILITY

#include <cstdlib>
#include <cstring>
#include <vector>

#ifdef BB_UTILITY
#include <iostream>
#include <stdio.h>
#endif

typedef unsigned char byte;

class ByteBuffer {
private:
	unsigned int rpos, wpos;
	std::vector<byte> buf;

#ifdef BB_UTILITY
	std::string name;
#endif

    template <typename T> T read() {
		T data = read<T>(rpos);
		rpos += sizeof(T);
		return data;
	}
	
	template <typename T> T read(unsigned int index) const {
		if(index + sizeof(T) <= buf.size())
			return *((T*)&buf[index]);
		return 0;
	}

	template <typename T> void append(T data) {
		unsigned int s = sizeof(data);

		if (size() < (wpos + s))
			buf.resize(wpos + s);
		memcpy(&buf[wpos], (byte*)&data, s);

		wpos += s;
	}
	
	template <typename T> void insert(T data, unsigned int index) {
		if ((index + sizeof(data)) > size()) {
			buf.resize(size() + (index + sizeof(data)));
		}

		memcpy(&buf[index], (byte*)&data, sizeof(data));
		wpos = index+sizeof(data);
	}

public:
	ByteBuffer(unsigned int size = DEFAULT_SIZE);
	ByteBuffer(byte* arr, unsigned int size);
	virtual ~ByteBuffer();

	unsigned int bytesRemaining(); 
	void clear(); 
	ByteBuffer* clone(); 
	bool equals(ByteBuffer* other); 
	void resize(unsigned int newSize);
	unsigned int size();
    
    template <typename T> int find(T key, unsigned int start=0) {
        int ret = -1;
        unsigned int len = buf.size();
        for(unsigned int i = start; i < len; i++) {
            T data = read<T>(i);
            if((key != 0) && (data == 0))
                break;
            
            if(data == key) {
                ret = i;
                break;
            }
        }
        return ret;
    }
    
    void replace(byte key, byte rep, unsigned int start = 0, bool firstOccuranceOnly=false);
	

	byte peek(); 
	byte get(); 
	byte get(unsigned int index); 
	void getBytes(byte* buf, unsigned int len); 
	char getChar(); // Relative
	char getChar(unsigned int index); 
	double getDouble();
	double getDouble(unsigned int index);
	float getFloat();
	float getFloat(unsigned int index);
	int getInt();
	int getInt(unsigned int index);
	long getLong();
	long getLong(unsigned int index);
	short getShort();
	short getShort(unsigned int index);

	void put(ByteBuffer* src); 
	void put(byte b); 
	void put(byte b, unsigned int index);
	void putBytes(byte* b, unsigned int len); 
	void putBytes(byte* b, unsigned int len, unsigned int index); 
	void putChar(char value); 
	void putChar(char value, unsigned int index);
	void putDouble(double value);
	void putDouble(double value, unsigned int index);
	void putFloat(float value);
	void putFloat(float value, unsigned int index);
	void putInt(int value);
	void putInt(int value, unsigned int index);
	void putLong(long value);
	void putLong(long value, unsigned int index);
	void putShort(short value);
	void putShort(short value, unsigned int index);


	void setReadPos(unsigned int r) {
		rpos = r;
	}

	int getReadPos() {
		return rpos;
	}

	void setWritePos(unsigned int w) {
		wpos = w;
	}

	int getWritePos() {
		return wpos;
	}
#ifdef BB_UTILITY
void setName(std::string n);
std::string getName();
void printInfo();	
void printAH();
void printAscii();
void printHex();
void printPosition();
#endif
};

#endif
