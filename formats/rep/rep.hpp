#include <string>
#include <fstream>
#include <iostream>
#include <vector>

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

const uint32_t magicByteConstant = 0x1631;

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

struct AnimationBlock
{
    uint32_t animationID;
    char animationName[48];
};

struct PostanimationBlock 
{
    char modelName[36];
    char frameName[36];
    unsigned char unk[36];
};

#pragma pack(pop)

class File
{
	public:
	std::vector<AnimationBlock> animationBlocks;
	std::vector<PostanimationBlock> postanimationBlocks;

};


#define READ(var) read(reinterpret_cast<char*>(&var), sizeof(var)/sizeof(char))
class Loader
{
    private:
	friend File;
	Header fileHeader;
	File currentFile;
	void readAnimations(std::ifstream& stream)
	{
		std::cerr << "AnimationStart" << std::endl;
		for(size_t i = 0; i < fileHeader.countOfAnimations; i++)
		{
			AnimationBlock animationBlock;
			memset(&animationBlock,0,52);
			stream.READ(animationBlock);
			currentFile.animationBlocks.push_back(animationBlock);
			std::cerr << "[Animation Block] " << animationBlock.animationID << " - " << animationBlock.animationName << std::endl;
		}
	}

	void readFrameSequences(std::ifstream& stream)
	{
		std::cerr << "FrameSequence" << std::endl;
		for(size_t i = 0; i < fileHeader.frameSequenceBlocks; i++)
		{
			PostanimationBlock postanimationBlock;
			memset(&postanimationBlock,0,108);
			stream.READ(postanimationBlock);
			currentFile.postanimationBlocks.push_back(postanimationBlock);
			std::cerr << "[FrameSequence Block] " << postanimationBlock.frameName << " - " << postanimationBlock.modelName << std::endl;
		}

	}
    
    public:
        File loadFile(std::string fileName)
        {
	
		std::ifstream inputFile;
		inputFile.open(fileName, std::ifstream::binary);
		if(inputFile.is_open())
		{
			inputFile.READ(fileHeader);
			std::cout << "Magic Byte: " << std::hex << fileHeader.magicByte << std::dec << std::endl;
			std::cout << "Anim block size: " << fileHeader.animationBlockSize << std::endl;
			std::cout << "Count of anims: " << fileHeader.countOfAnimations<< std::endl;
			if(fileHeader.magicByte != magicByteConstant)
			{
				std::cerr << "[Err] Invalid magic byte ...\n" << std::endl;
				return File();
			}
			
			readAnimations(inputFile);
			readFrameSequences(inputFile);
		} else {
			std::cerr << "[Err] Failed to open file " << fileName  << std::endl;
		}
		return currentFile;
        }
};
}
