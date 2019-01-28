/*
 * .rep file printer
 * Author: Roman Romop5 Dobias
 * Purpose: print outs content of .rep file
 * Credits also go to guys from Lost Heaven Modding's wiki, who provided basics of .rep structure
 */

#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>

namespace RepFile
{

/*
 * Simplified file structure
 *  1. RepFile header
 *  2. for N where N = header.countOfAnimationBlocks
 *      2. animation block (with ID / name)
 *  3. for N in objectDefinitionBlocks
 *      4. object definition:
 *          frameName = binds object to frame with this name
 *          actorName = assigns actor to frameName
 *          size of transform stream chunks (for type 1 and 2)
 *  5. for N in objectDefinitionBlocks
 *      while readBytes < objectDefinition.sizeOfStream
 *          6. read transformation chunk 
 *              with timestamp, type and payload
 */

const uint32_t magicByteConstant = 0x1631;

#pragma pack(push,1)
struct Header
{
	uint32_t magicByte;	// 0x31 16 00 00
	uint32_t sizeOfAnimationSection; // the size (in bytes) of all animation blocks together 
	uint32_t sizeOfObjectDefinitionsSection; // contains positions (with some auxiliary info) = probably camera tracks ?
	uint32_t countOfObjectDefinitionBlocks; // *108 to get size in bytes
	uint32_t fixedCCSequence; // CC CC CC CC 
	uint32_t countOfCameraChunks; // each chunk has 64 bytes
	uint32_t countOfCameraFocusChunks; // each chunk has 56 bytes
	uint32_t sizeOfScriptEventsSequence; // together with sounds
	uint32_t sizeOfDialogSection; // unknown at the moment
    unsigned char padding[60]; // 00s
    // Start of animation block
    uint32_t countOfAnimationBlocks; // the number of animation blocks following header
    uint32_t AlwaysContainsOne; // usually set to 00 01 00 00

};

/* 
 * \brief Contains animations 
 *
 * Each animation can be referenced in Transformation Stream (by ID) 
 */
struct AnimationBlock
{
    uint32_t animationID;
    char animationName[48];
};

enum AnimatedObjectDefinitionType : uint32_t
{
    OBJECT_FRAME = 1,
    OBJECT_HUMAN = 2,
    OBJECT_CAR = 4,
    OBJECT_MUZZLE = 21
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
    uint32_t activationTime; // at least I guessed so, all objects has this field set to 0
    uint32_t deactivationTime; // verified, after deactivationTime the object gonna stuck (till the end of cutscene)
    AnimatedObjectDefinitionType type; // 2 = human (actor), 1 = frame, 4 = car, 21 = muzzle ?
    uint32_t sizeOfStreamSection; // size of all blocks for this object together
    uint32_t positionOfTheBeginning;  // the offset since the start of transformation section at which the ObjectDefinition block starts (and last for sizeOfStreamSection bytes)
};

struct TransformationHeader
{
    uint32_t timestamp; // the beggining of transformation
    uint32_t type; // 1 or 2
};

/*
 * \brief Most frequently used transformation payload
 *
 * For given timestamp, it contains world-space position + rotation of object  + animation
 */
struct TransformPayload
{
	float position[3]; // world space
	float rotation[4]; // rotation quat
        uint32_t auxiliary; // first 10 bits = animation ID + bitmap with flags (must have 0x400 to animate) // TODO
        uint32_t offset; // last 0xFFF goes for animation frame offset (ahead)

        size_t getAnimationID() { return this->auxiliary & 0x3FF; }
        size_t getAnimationOffset() { return this->offset & 0xFFF; }
};


struct AnimationChunk
{
        uint32_t timestamp;
        uint32_t unk1;
        uint32_t type;
        float position[3];
        float unkVector[3];
        float unkVectorSecond[3];
        uint32_t unk2;
        float fov;
        uint32_t unk3[2];
};

struct CameraFocusChunk
{
        uint32_t timestamp;
        uint32_t timestamp2;
        uint32_t type;
        float position[3];
        float unkVector[3];
        float unkVectorSecond[3];
        uint32_t unk1;
        uint32_t unk2;
};

struct ScriptsAndSoundsHeader
{
        uint32_t unk;
        uint32_t sizeOfFollowingData; // unk, TODO
        uint32_t sizeOfScriptSection;
        uint32_t sizeOfSoundSection;
};

struct ScriptChunk
{
    uint32_t timestamp;
    char scriptName[36];
};

struct SoundChunk
{
    uint32_t timestamp;
    uint32_t type; // 0 = start, 1 = end
    char soundName[32];
};

struct DialogHeader
{
    uint32_t countOfDialogs;
    uint32_t unk;
    uint32_t unk2;
};

struct DialogChunk
{
    uint32_t timestamp;
    uint32_t channelID;
    uint32_t dialogID; // 0 = unk, 1 = bind frame to dialog, FFFFFFFF unk, rest = dialogID which can be found in sounds/
    char framename[24];
};

#pragma pack(pop)

class File
{
	public:
	std::vector<AnimationBlock> animationBlocks;
	std::vector<AnimatedObjectDefinitions> postanimationBlocks;

};


#define GETPOS(stream) { std::cerr << "Stream position at: " << std::hex << stream.tellg() << std::dec << std::endl;}
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
		for(size_t i = 0; i < fileHeader.countOfAnimationBlocks; i++)
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
		for(size_t i = 0; i < fileHeader.countOfObjectDefinitionBlocks; i++)
		{
			AnimatedObjectDefinitions postanimationBlock;
			memset(&postanimationBlock,0,108);
			stream.READ(postanimationBlock);
			currentFile.postanimationBlocks.push_back(postanimationBlock);
			std::cerr << "[FrameSequence Block] " << postanimationBlock.frameName << " - " << postanimationBlock.actorName<< " - Size: " << std::hex << postanimationBlock.sizeOfStreamSection << "Start: " << postanimationBlock.positionOfTheBeginning << std::dec << std::endl;
                        std::cerr << "[FS] ";
                        for(int i = 0; i < 4; i++)
                        {
                            std::cerr << std::setw(6) << postanimationBlock.sizeOfBlocks[i] << " ";
                        }

                        std::cerr << "Time: " << std::setw(6) << std::hex << postanimationBlock.activationTime << std::dec << " ";
                        std::cerr << std::setw(6) << std::hex << postanimationBlock.deactivationTime << std::dec << " ";
                        std::cerr << " Type: " << std::hex << postanimationBlock.type << std::dec;
                        std::cerr << std::endl;
		}

	}

	void readTransformation(std::ifstream& stream)
	{
            size_t endPointer= 0;
            size_t currentPointer = 0;
            // for each animated object 
            for(size_t i = 0; i < fileHeader.countOfObjectDefinitionBlocks; i++)
            {
                auto& animatedObject = currentFile.postanimationBlocks[i];
                std::cerr << "Reading object: " << animatedObject.actorName << "[ " << animatedObject.frameName << " ] [" << i << "]" << std::endl;
                endPointer = currentPointer + animatedObject.sizeOfStreamSection;
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
                    size_t payloadLength = animatedObject.sizeOfBlocks[header.type]-8;
                    currentPointer += payloadLength;
                    // read payload
                    unsigned char payload[512];
                    stream.read(reinterpret_cast<char*>(&payload), payloadLength);
                    std::cerr << "[TransformSequence] Timestamp: " << std::hex << header.timestamp << std::dec << " Type: " << header.type << " Read chunk with size: " << payloadLength << std::endl;

                    TransformPayload *body = reinterpret_cast<TransformPayload*>(&payload);
                    std::cerr << "[TransformSequence] Transform payload ["<< body->position[0] << "," << body->position[1] << "," << body->position[2] << "] ["
                    << body->rotation[0] << "," << body->rotation[1] << "," << body->rotation[2] << "," << body->rotation[3] << "] AnimID: " << body->getAnimationID() << " Offset:" << body->getAnimationOffset() << " - "<< std::hex << body->auxiliary << std::dec << "\n";
                }

                
            }
	}

	void readCameraSection(std::ifstream& stream)
	{
            for(size_t i = 0; i < fileHeader.countOfCameraChunks; i++)
            {
                    AnimationChunk chunk;
                    stream.READ(chunk);
                    std::cerr << "Camera Section: Time: " << chunk.timestamp << " Type: " << chunk.type << " [" << chunk.position[0] << ", " << chunk.position[1] << ", " << chunk.position[2] << "]" << " FOV: " << chunk.fov;
                    std::cerr << " [" << chunk.unkVector[0] << ", " << chunk.unkVector[1] << ", " << chunk.unkVector[2] << "]" <<  " - " ;
                    std::cerr << " [" << chunk.unkVectorSecond[0] << ", " << chunk.unkVectorSecond[1] << ", " << chunk.unkVectorSecond[2] << "]" <<  std::endl;
                
            }

            GETPOS(stream);
            for(size_t i = 0; i < fileHeader.countOfCameraFocusChunks; i++)
            {
                    CameraFocusChunk chunk;
                    stream.READ(chunk);
                    std::cerr << "Camera Focus: Time: " << chunk.timestamp << " Type: " << chunk.type << " [" << chunk.position[0] << ", " << chunk.position[1] << ", " << chunk.position[2] << "]";
                    std::cerr << " [" << chunk.unkVector[0] << ", " << chunk.unkVector[1] << ", " << chunk.unkVector[2] << "]" <<  " - ";
                    std::cerr << " [" << chunk.unkVectorSecond[0] << ", " << chunk.unkVectorSecond[1] << ", " << chunk.unkVectorSecond[2] << "]" <<  std::endl;
            }
        }

	void readScriptEvents(std::ifstream& stream)
	{
            ScriptsAndSoundsHeader header;
            stream.READ(header);

            stream.seekg(header.sizeOfFollowingData, std::ifstream::cur);

            uint32_t countOFFirstSection = header.sizeOfScriptSection / 40;
            uint32_t countOFSecondSection = header.sizeOfSoundSection / 40;
            for(size_t i = 0; i < countOFFirstSection; i++)
            {
                    ScriptChunk chunk;
                    stream.READ(chunk);
                    std::cerr << "Script chunk: " << chunk.timestamp << " Name: " << chunk.scriptName << std::endl;               
            }

            for(size_t i = 0; i < countOFSecondSection; i++)
            {
                    SoundChunk chunk;
                    stream.READ(chunk);
                    std::cerr << "Sound chunk: " << chunk.timestamp << " Type: " << chunk.type << " Name: " << chunk.soundName << std::endl;               
            }

        }

	void readDialogs(std::ifstream& stream)
	{
            DialogHeader header;
            stream.READ(header);

            for(size_t i = 0; i < header.countOfDialogs; i++)
            {
                    DialogChunk chunk;
                    stream.READ(chunk);
                    std::cerr << "Dialog chunk: stamp: " << chunk.timestamp << " ID: " << chunk.channelID << " animation ID: " << std::hex << chunk.dialogID << std::dec << " Name: " << chunk.framename << std::endl;               
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
			std::cout << "Anim block size: " << fileHeader.sizeOfAnimationSection << std::endl;
			std::cout << "Count of anims: " << fileHeader.countOfAnimationBlocks<< std::endl;
			if(fileHeader.magicByte != magicByteConstant)
			{
				std::cerr << "[Err] Invalid magic byte ...\n" << std::endl;
				return File();
			}
			
			readAnimations(inputFile);
			readFrameSequences(inputFile);
			readTransformation(inputFile);
                        readCameraSection(inputFile);
                        readScriptEvents(inputFile);
                        readDialogs(inputFile);
		} else {
			std::cerr << "[Err] Failed to open file " << fileName  << std::endl;
		}
		return currentFile;
        }
};
}
