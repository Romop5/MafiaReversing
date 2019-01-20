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

/* 
 * \brief Contains animations 
 *
 * PROBABLY a way how to preload animations and usem them in the sequence of object state
 * transitions
 */
struct AnimationBlock
{
    uint32_t animationID;
    char animationName[48];
};

/*
 * \brief Describes animated objects
 *
 * For each object in cutscene, an actor (or specialized object) is assigned to scene's frame
 * (frameName) and additional meta information are enclosed such as size of animation blocks in
 * animation transformation stream
 *
 */
struct AnimatedObjectDefinitions 
{
    char frameName[36];
    char actorName[36];
    uint32_t sizeOfBlocks[4];   // size of different blocks
    unsigned char unk[12];
    uint32_t sizeOfStreamSection; // size of all blocks for this object together
    uint32_t unk2;
};

struct TransformationHeader
{
    uint32_t timestamp; // the beggining of transformation
    uint32_t type; // 1 or 2
};

// TODO-rename
struct FollowingStructHeader
{
	uint32_t unkZero;
	uint32_t unkTimestamp;
	uint32_t unkTimestamp2; // increasing
	uint32_t probablyType; // usually 1 or 2
};

struct TransformPayload
{
	float position[3];
	float rotation[4]; // maybe rotation in angles
        uint32_t auxiliary; // first 10 bits = animation ID
        size_t getAnimationID() { return this->auxiliary & 0x3FF; }
};

#pragma pack(pop)

class File
{
	public:
	std::vector<AnimationBlock> animationBlocks;
	std::vector<AnimatedObjectDefinitions> postanimationBlocks;

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
			AnimatedObjectDefinitions postanimationBlock;
			memset(&postanimationBlock,0,108);
			stream.READ(postanimationBlock);
			currentFile.postanimationBlocks.push_back(postanimationBlock);
			std::cerr << "[FrameSequence Block] " << postanimationBlock.frameName << " - " << postanimationBlock.actorName<< " - " << std::hex << postanimationBlock.sizeOfStreamSection << std::dec << std::endl;
                        std::cerr << "[FS] ";
                        for(int i = 0; i < 4; i++)
                        {
                            std::cerr << postanimationBlock.sizeOfBlocks[i] << " ";
                        }
                        std::cerr << std::endl;
		}

	}

	void readTransformation(std::ifstream& stream)
	{
            size_t endPointer= 0;
            size_t currentPointer = 0;
            // for each animated object 
            for(size_t i = 0; i < fileHeader.frameSequenceBlocks; i++)
            {
                std::cerr << "Reading object: " << i << std::endl;
                endPointer = currentPointer + currentFile.postanimationBlocks[i].sizeOfStreamSection;
                uint32_t zeroUnkBlock;
                // Skip leading 8 bytes
                stream.READ(zeroUnkBlock);
                stream.READ(zeroUnkBlock);
                currentPointer += 8;

                // for all blocks for current animated object
                while(endPointer > currentPointer)
                {
                    std::cerr << "Position: " << std::hex << currentPointer << std::dec << std::endl;
                    // get header with timestamp / type
                    TransformationHeader header;
                    stream.READ(header);
                    currentPointer += 8;

                    // get payload size (-8 is for header)
                    size_t payloadLength = currentFile.postanimationBlocks[i].sizeOfBlocks[header.type]-8;
                    currentPointer += payloadLength;
                    // read payload
                    unsigned char payload[512];
                    stream.read(reinterpret_cast<char*>(&payload), payloadLength);
                    std::cerr << "[TransformSequence] Timestamp: " << std::hex << header.timestamp << std::dec << " Type: " << header.type << " Read chunk with size: " << payloadLength << std::endl;

                    TransformPayload *body = reinterpret_cast<TransformPayload*>(&payload);
                    std::cerr << "[TransformSequence] Transform payload ["<< body->position[0] << "," << body->position[1] << "," << body->position[2] << "] ["
                    << body->rotation[0] << "," << body->rotation[1] << "," << body->rotation[2] << "," << body->rotation[3] << "] AnimID: " << body->getAnimationID() << " " << std::hex << body->auxiliary << "\n";
                }

                
            }
            /*
		std::cerr << "TransformSequence" << std::endl;
		uint32_t startunk1;
		uint32_t startunk2;
		stream.READ(startunk1);
		stream.READ(startunk2);
		for(size_t i = 8; i < fileHeader.followingSequenceSize; )
		{
			uint32_t unkWhatever;
			uint32_t type;
			stream.READ(unkWhatever);
			stream.READ(type);

			if(type == 0)
			{
				float unk7;
				stream.READ(unk7);
				stream.READ(type);
			}
			FollowingStructTransform body;
			stream.READ(body);
			float unk6;
			stream.READ(unk6);
			i += 40;
			if(type == 2)
			{
				float unk;
				stream.READ(unk);
				i += 4;
			}
			//currentFile.postanimationBlocks.push_back(postanimationBlock);
			std::cerr << "[TransformSequence] Whatever: " << std::hex << unkWhatever << " Position: " << std::hex << i << std::dec << "\t Type: " << type << " ["<< body.position[0] << "," << body.position[1] << "," << body.position[2] << "] ["
				<< body.rotation[0] << "," << body.rotation[1] << "," << body.rotation[2] << "," << body.rotation[3] << "]\n";
		}
                */

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
