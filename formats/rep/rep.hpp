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
	uint32_t followingSequenceSize; // contains positions (with some auxiliary info) = probably camera tracks ?
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

// TODO-rename
struct PostanimationBlock 
{
    char modelName[36];
    char frameName[36];
    unsigned char unk[36];
};

// TODO-rename
struct FollowingStructHeader
{
	uint32_t unkZero;
	uint32_t unkTimestamp;
	uint32_t unkTimestamp2; // increasing
	uint32_t probablyType; // usually 1 or 2
};

struct FollowingStructTransform
{
	float position[3];
	float rotation[3]; // maybe rotation in angles
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

	void readTransformation(std::ifstream& stream)
	{
		std::cerr << "TransformSequence" << std::endl;
		for(size_t i = 0; i < fileHeader.followingSequenceSize; )
		{
			FollowingStructHeader header;
			memset(&header,0,16);
			stream.READ(header);
			FollowingStructTransform body;
			switch(header.probablyType)
			{
				case 1:
					{
						stream.READ(body);
						i += 24;
					}
					break;
				case 2: {
						stream.READ(body);
						float unk;
						stream.READ(unk);
						i += 24+4;
					}
					break;
				case 0:
					{
						uint32_t unk1;
						uint32_t realType;
						stream.READ(unk1);
						stream.READ(realType);
						stream.READ(body);
						uint32_t unk;
						if(realType == 2)
						{
							stream.READ(unk);
							i += 4;
						}
						i += 24;
					}
					break;

			}
			//currentFile.postanimationBlocks.push_back(postanimationBlock);
			std::cerr << "[TransformSequence] " << "Type: " << header.probablyType  << " ["<< body.position[0] << "," << body.position[1] << "," << body.position[2] << "] ["
				<< body.rotation[0] << "," << body.rotation[1] << "," << body.rotation[2] << "]\n";
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
			readTransformation(inputFile);
		} else {
			std::cerr << "[Err] Failed to open file " << fileName  << std::endl;
		}
		return currentFile;
        }
};
}
