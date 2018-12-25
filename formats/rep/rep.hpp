#include <string>
#include <fstream>
#include <iostream>

namespace RepFile
{

/*
 * Simplified file structure
 *  1. RepFile header
 *  2. for N where N = header.countOfAnimations
 *      2. animation block
 *  3. frame sequences = overall size is frameSequenceBlocks*108
 *  4. followingSequenceSize (unk)
 *
 */

#pragma pack(push,1)
struct Header
{
	uint32_t magicByte;	// 0x31 16 00 00
	uint32_t animationBlockSize; // the size (in bytes) of all animation blocks together 
	uint32_t followingSequenceSize;
	uint32_t frameSequenceBlocks; // *108 to get size in bytes
	uint32_t unkCCSequence; // CC CC CC CC 
	uint32_t unk3;
	uint32_t unk4;
	uint32_t unk5;
	uint32_t unk6;
    unsigned char padding[60];
    // Start of animation block
    uint32_t countOfAnimations; // the number of animation blocks following header
    uint32_t unknown;
};
#pragma pop()

class AnimationBlock
{
    uint32_t animationID;
    
};

class File
{

};


#define READ(var) read(reinterpret_cast<char*>(&var), sizeof(var)/sizeof(char))
class Loader
{
    private:
    
    public:
        File loadFile(std::string fileName)
        {
	
		std::ifstream inputFile;
		inputFile.open(fileName, std::ifstream::binary);
		if(inputFile.is_open())
		{
			uint32_t magicByte;
			Header fileHeader;
			inputFile.READ(fileHeader);
			std::cout << "Magic Byte: " << fileHeader.magicByte << std::endl;
			std::cout << "Anim block size: " << fileHeader.animationBlockSize << std::endl;
			std::cout << "Count of anims: " << fileHeader.countOfAnimations<< std::endl;
		}
        }
};
}
