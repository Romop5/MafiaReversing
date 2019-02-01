/*
 * .rep file printer
 * Author: Roman Romop5 Dobias
 * Purpose: print outs content of .rep file
 * Credits also go to guys from Lost Heaven Modding's wiki, who provided basics
 * of .rep structure
 */

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <string>
#include <vector>

/* \brief Stores cutscene directives
 *
 * .rep files contains directives for performing cutscenes.
 * Each object which is going to be animated is defined here,
 * and subsequently described by sequence of timed transformations chunks.
 *
 * Morever, camera has separeted stream of transformations which additionally
 * contains positions at which the camera looks at during the cutscene.
 *
 * Eventually, a stream of dialog speeches is stored in a single stream for each
 * human speaking during the cutscene.
 *
 */
namespace RepFile {

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

#pragma pack(push, 1)
struct Header
{
  uint32_t magicByte;              // 0x31 16 00 00 - determines .rep file
  uint32_t sizeOfAnimationSection; // the size (in bytes) of all animation
                                   // blocks together
  uint32_t
    sizeOfObjectDefinitionsSection; // contains positions (with some auxiliary
                                    // info) = probably camera tracks ?
  uint32_t countOfObjectDefinitionBlocks; // *108 to get size in bytes
  uint32_t fixedCCSequence;               // always contains 0xCC CC CC CC
  uint32_t countOfCameraChunks;           // each chunk has 64 bytes
  uint32_t countOfCameraFocusChunks;      // each chunk has 56 bytes
  uint32_t sizeOfScriptEventsSequence;    // together with sounds events
  uint32_t
    sizeOfDialogSection;     // together with DialogHeader + some unknown block
  unsigned char padding[60]; // 00s
  // Start of animation block
  uint32_t
    countOfAnimationBlocks;   // the number of animation blocks following header
  uint32_t AlwaysContainsOne; // usually set to 00 01 00 00
};

/*
 * \brief Contains animation used during cutscene
 *
 * Each animation can be referenced in Transformation Stream (by animationID)
 */
struct AnimationBlock
{
  uint32_t animationID; // actually, only lower 10bits are used in
                        // transformation stream
  char
    animationName[48]; // NULL-terminated name of animation (ending with .i3d)
};

/*
 * \brief Type of animated object
 *
 * Type determines how the object is handled during cutscenes. Only some of
 * objects can have dialog animation applied on.
 */
enum AnimatedObjectDefinitionType : uint32_t
{
  OBJECT_FRAME = 1,
  OBJECT_HUMAN = 2,
  OBJECT_CAR = 4,
  OBJECT_BOX = 9, // pickable object ?
  OBJECT_EFFECT =
    21 // usually stands for muzzle / cigarrette => particle effect
};

/*
 * \brief Describes animated objects
 *
 * For each object in cutscene, an actor (or specialized object) is assigned to
 * scene's frame (frameName) and additional meta information are enclosed such
 * as size of animation blocks in animation transformation stream
 *
 */
struct AnimatedObjectDefinitions
{
  char frameName[36];       // frame name as defined in scene file
  char actorName[36];       // actor named, which is used for referencing in
                            // MafiaScripts
  uint32_t sizeOfBlocks[4]; // size of chunks which store different
                            // transformation data in Transformation Section
  uint32_t activationTime;  // at least I guessed so, all objects has this field
                            // set to 0
  uint32_t deactivationTime; // verified, after deactivationTime the object
                             // gonna stuck (till the end of cutscene)
  AnimatedObjectDefinitionType type; // 2 = human (actor), 1 = frame, 4 = car, 6
                                     // = unk, 9 = box, 21 = particle effect?
  uint32_t sizeOfStreamSection; // size of all blocks for this object, stored in
                                // TransformationStream, together
  uint32_t
    positionOfTheBeginning; // the offset since the start of transformation
                            // section at which the ObjectDefinition block
                            // starts (and last for sizeOfStreamSection bytes)

  std::string getTypeString() const
  {
    switch (this->type) {
      case OBJECT_FRAME:
        return "OBJECT_FRAME";
      case OBJECT_HUMAN:
        return "OBJECT_HUMAN";
      case OBJECT_CAR:
        return "OBJECT_CAR";
      case OBJECT_EFFECT:
        return "OBJECT_EFFECT";
      default:
        return "OBJECT_UNKNOWN";
    }
  };
};

/* \brief Determines type of the following transformation chunk
 *
 * Each chunk starts with this header.
 */
struct TransformationHeader
{
  uint32_t timestamp; // the beggining of transformation
  uint32_t type; // chunk type, 1, 2, 3, the size of chunk can be determined in
                 // sizeOfBlocks and type as index into the array
};


enum TransformAnimationFlags : uint32_t
{
    ANIMATION_HAS_ID = 0x400,
    ANIMATION_SHOULD_REPEAT = 0x2000,
    ANIMATION_SHOULD_INTERPOLATE = 0x4000
};
/*
 * \brief Most frequently used transformation payload
 *
 * For given timestamp, it contains world-space position + rotation of object  +
 * animation
 */
struct TransformPayload
{
  float position[3];  // world space
  float rotation[4];  // rotation quat
  uint32_t auxiliary; // first 10 bits = animation ID + the rest bits = bitmap with flags (must
                      // have 0x400 to animate) // TODO
  uint32_t
    animationStartOffset; // last 0xFFF goes for animation frame offset (ahead) * 40 to get time in miliseconds

  size_t getAnimationID() { return this->auxiliary & 0x3FF; }
  bool hasAnimationID() const { return this->auxiliary & ANIMATION_HAS_ID; }
  bool shouldInterpolate() const { return this->auxiliary & ANIMATION_SHOULD_INTERPOLATE; }
  bool shouldRepeat() const { return this->auxiliary & ANIMATION_SHOULD_REPEAT; }

  size_t getFlags() { return this->auxiliary >> 12; }
  std::string getFlagsAsString() const {
      std::string flags;
      if(shouldRepeat())
      {
        flags += "ANIMATION_SHOULD_REPEAT |";
      }
      if(shouldInterpolate())
      {
        flags += "ANIMATION_SHOULD_INTERPOLATE |";
      }
      if(hasAnimationID())
      {
        flags += "ANIMATION_HAS_ID |";
      }
      return flags;
  };
  size_t getAnimationOffset() { return this->animationStartOffset & 0xFFF; } 
};

/* \brief Describes camera position, rotation and FOV
 */
struct CameraTransformationChunk
{
  uint32_t timestamp;
  uint32_t unk1;
  uint32_t type;     // GUESS: probably defines how the position is interpolated
  float position[3]; // position which the camera is placed at
  float unkVector[3]; // somehow controls how much the camera "hangs" from side
                      // to side during the movement
  float unkVectorSecond[3];
  uint32_t unk2;
  float fov; // field of view - camera perspective angle
  uint32_t unk3[2];
};

/* \brief Describes where the camera looks at
 *
 */
struct CameraFocusChunk
{
  uint32_t timestamp;
  uint32_t timestamp2;
  uint32_t type;
  float
    position[3]; // determines the position at which the camera is focused at
  float unkVector[3];
  float unkVectorSecond[3];
  uint32_t unk1;
  uint32_t unk2;
};

struct ScriptsAndSoundsHeader
{
  uint32_t sizeOfPostheaderData; // post header data
  uint32_t sizeOfFadeSection; // each FadeChunk has size of 0x20 
  uint32_t sizeOfScriptSection; // size of all ScriptChunk together
  uint32_t sizeOfSoundSection;  // size of all SoundChunk together
};

struct FadeChunk
{
    uint32_t compressedStartWithFlags; // defines the time of end ? + some additional flags that controls fading ?
    uint32_t timeAheadOfDarkening; // how many ms before the end of darkening should it start ?
    float timeOfDarkening; // how long the darkening last
    uint32_t unk;
    uint32_t fixedCCSequence[3]; // filled with CC 
    uint32_t fixedSequence; // CC CC CC 00

    uint32_t getStartOfFadeEvent() const { return this->compressedStartWithFlags & 0xFFFFFF; }
    uint32_t getFlags() const { return this->compressedStartWithFlags >> 24; }
    bool isFadeIn() const { return this->getFlags() & 0x80; }
    bool isFadeOut() const { return this->getFlags() & 0x50; }
    std::string getFlagsAsString() const {
        std::string flags = "";
        if(isFadeIn())
            flags += "FADE_IN ";
        if(isFadeOut())
            flags += "FADE_OUT";
        return flags;
    }
};

/* \brief Determines the script which is called at specified time during the
 * cutscene
 *
 * Note: scripts can be dynamically loaded into the scene using .diff files
 */
struct ScriptChunk
{
  uint32_t timestamp;
  char scriptName[36];
};

enum SoundChunkType : uint32_t
{
  SOUND_START = 0,
  SOUND_END = 1,
};

/* \brief Determines the name of sound frame which is (de)activated at given
 * time
 */
struct SoundChunk
{
  uint32_t timestamp;
  SoundChunkType type; // 0 = start, 1 = end
  char soundName[32];

  std::string getTypeStringRepresentation() const
  {
    switch (this->type) {
      case SOUND_START:
        return "SOUND_START";
      case SOUND_END:
        return "SOUND_END";
      default:
        return "UNKOWN SOUND TYPE";
    }
  }
};

/* \brief Defines the number of dialog chunks
 *
 * Note: occurs at the beginning of the dialog section
 */
struct DialogHeader
{
  uint32_t countOfDialogs;
  uint32_t countOfNarratorChunks;
  uint32_t unk2;
};

struct DialogChunk
{
  uint32_t timestamp;
  uint32_t channelID; // each channelID stands for different human giving speech
  uint32_t dialogID;  // 0 = unk, 1 = bind frame to dialog, FFFFFFFF unk, rest =
                      // dialogID which can be found in sounds/
  char framename[24]; // identifies the human which speaks, contains 0 if
                      // dialogID is not 1
};

/* \brief Defines a background speech given at specific time
 *
 */
struct NarratorChunk
{
  uint32_t timestamp; 
  uint32_t unk2;      // probably duration ?
  uint32_t speechID; 
};

struct UnkChunk
{
  uint32_t timestamp; 
  uint32_t unk1;      
  char frameName[32];
  uint32_t unk2;
};


#pragma pack(pop)

class File
{
public:
  std::vector<AnimationBlock> animationBlocks;
  std::vector<AnimatedObjectDefinitions> animatedObjects;
  // Transformation payload TODO
  std::vector<CameraTransformationChunk> cameraPositionChunks;
  std::vector<CameraFocusChunk> camerafocusChunks;
  std::vector<FadeChunk> fadeChunks;
  std::vector<ScriptChunk> scriptChunks;
  std::vector<SoundChunk> soundChunks;
  std::vector<DialogChunk> dialogChunks;
};

#define GETPOS(stream)                                                         \
  {                                                                            \
    std::cerr << "Stream position at: 0x" << std::hex << stream.tellg()          \
              << std::dec << std::endl;                                        \
  }
#define READ(var)                                                              \
  read(reinterpret_cast<char*>(&var), sizeof(var) / sizeof(char))
class Loader
{
private:
  friend File;
  Header fileHeader;
  File currentFile;
  void readAnimations(std::ifstream& stream)
  {
    std::cerr << "AnimationStart" << std::endl;
    for (size_t i = 0; i < fileHeader.countOfAnimationBlocks; i++) {
      AnimationBlock animationBlock;
      memset(&animationBlock, 0, 52);
      stream.READ(animationBlock);
      currentFile.animationBlocks.push_back(animationBlock);
      std::cerr << "[Animation Block] " << animationBlock.animationID << " - "
                << animationBlock.animationName << std::endl;
    }
  }

  void readObjectDefinitions(std::ifstream& stream)
  {
    std::cerr << "FrameSequence" << std::endl;
    for (size_t i = 0; i < fileHeader.countOfObjectDefinitionBlocks; i++) {
      AnimatedObjectDefinitions postanimationBlock;
      memset(&postanimationBlock, 0, 108);
      stream.READ(postanimationBlock);
      currentFile.animatedObjects.push_back(postanimationBlock);
      std::cerr << "[FrameSequence Block] " << postanimationBlock.frameName
                << " - " << postanimationBlock.actorName
                << " - Size: 0x" << std::hex
                << postanimationBlock.sizeOfStreamSection
                << "Start: " << postanimationBlock.positionOfTheBeginning
                << std::dec << std::endl;
      std::cerr << "[FS] ";
      for (int i = 0; i < 4; i++) {
        std::cerr << std::setw(6) << postanimationBlock.sizeOfBlocks[i] << " ";
      }

      std::cerr << "Time: " << std::setw(6) << std::hex
                << postanimationBlock.activationTime << std::dec << " ";
      std::cerr << std::setw(6) << std::hex
                << postanimationBlock.deactivationTime << std::dec << " ";
      std::cerr << " Type: " << postanimationBlock.getTypeString() << "["
                << std::hex << postanimationBlock.type << "]" << std::dec;
      std::cerr << std::endl;
    }
  }

  void readTransformation(std::ifstream& stream)
  {
    size_t endPointer = 0;
    size_t currentPointer = 0;
    // for each animated object
    for (size_t i = 0; i < fileHeader.countOfObjectDefinitionBlocks; i++) {
      GETPOS(stream);
      auto& animatedObject = currentFile.animatedObjects[i];
      std::cerr << "Reading object: " << animatedObject.actorName << "[ "
                << animatedObject.frameName << " ] [" << i << "]" << std::endl;
      endPointer = currentPointer + animatedObject.sizeOfStreamSection;
      uint32_t zeroUnkBlock;
      // Skip leading 8 bytes
      stream.READ(zeroUnkBlock);
      stream.READ(zeroUnkBlock);
      currentPointer += 8;

      // for all blocks for current animated object
      while (endPointer > currentPointer) {
        std::cerr << "Position: 0x" << std::hex << currentPointer << std::dec
                  << std::endl;
        // get header with timestamp / type
        TransformationHeader header;
        stream.READ(header);
        currentPointer += 8;

        // get payload size (-8 is for header)
        size_t payloadLength = animatedObject.sizeOfBlocks[header.type] - 8;
        currentPointer += payloadLength;
        // read payload
        unsigned char payload[512];
        stream.read(reinterpret_cast<char*>(&payload), payloadLength);
        std::cerr << "[TransformSequence] Timestamp: 0x" << std::hex
                  << header.timestamp << std::dec << " Type: " << header.type
                  << " Read chunk with size(without header): " << payloadLength << std::endl;

        TransformPayload* body = reinterpret_cast<TransformPayload*>(&payload);
        std::cerr << "[TransformSequence] Transform payload ["
                  << body->position[0] << "," << body->position[1] << ","
                  << body->position[2] << "] [" << body->rotation[0] << ","
                  << body->rotation[1] << "," << body->rotation[2] << ","
                  << body->rotation[3] << "] AnimID: " << body->getAnimationID()
                  << " Flags:" << std::hex << body->getFlags() << std::dec << " - "
                  << " Flags:" << body->getFlagsAsString() << " - "
                  << " Offset:" << body->getAnimationOffset() << " - "
                  << std::hex << body->auxiliary << std::dec << "\n";
      }
    }
  }

  void readCameraSection(std::ifstream& stream)
  { 
    GETPOS(stream);
    std::cerr << "[Camera Section] Count of camera chunks: " << fileHeader.countOfCameraChunks << std::endl;
    std::cerr << "[Camera Section] Count of camera focus chunks: " << fileHeader.countOfCameraFocusChunks << std::endl;
    for (size_t i = 0; i < fileHeader.countOfCameraChunks; i++) {
      CameraTransformationChunk chunk;
      stream.READ(chunk);
      std::cerr << "Camera Section: Time: " << chunk.timestamp
                << " Type: " << chunk.type << " [" << chunk.position[0] << ", "
                << chunk.position[1] << ", " << chunk.position[2] << "]"
                << " FOV: " << chunk.fov;
      std::cerr << " [" << chunk.unkVector[0] << ", " << chunk.unkVector[1]
                << ", " << chunk.unkVector[2] << "]"
                << " - ";
      std::cerr << " [" << chunk.unkVectorSecond[0] << ", "
                << chunk.unkVectorSecond[1] << ", " << chunk.unkVectorSecond[2]
                << "]" << std::endl;
      currentFile.cameraPositionChunks.push_back(chunk);
    }

    GETPOS(stream);
    for (size_t i = 0; i < fileHeader.countOfCameraFocusChunks; i++) {
      CameraFocusChunk chunk;
      stream.READ(chunk);
      std::cerr << "Camera Focus: Time: " << chunk.timestamp
                << " Type: " << chunk.type << " [" << chunk.position[0] << ", "
                << chunk.position[1] << ", " << chunk.position[2] << "]";
      std::cerr << " [" << chunk.unkVector[0] << ", " << chunk.unkVector[1]
                << ", " << chunk.unkVector[2] << "]"
                << " - ";
      std::cerr << " [" << chunk.unkVectorSecond[0] << ", "
                << chunk.unkVectorSecond[1] << ", " << chunk.unkVectorSecond[2]
                << "]" << std::endl;
      currentFile.camerafocusChunks.push_back(chunk);
    }
  }

  void readScriptEvents(std::ifstream& stream)
  {
    GETPOS(stream);
    ScriptsAndSoundsHeader header;
    stream.READ(header);
    std::cerr << "[EventsHeader] Size of post header section: 0x" << std::hex << header.sizeOfPostheaderData << std::dec << std::endl;
    std::cerr << "[EventsHeader] Size of Fade section: 0x" << std::hex << header.sizeOfFadeSection << std::dec << std::endl;

    stream.seekg(header.sizeOfPostheaderData, std::ifstream::cur);

    uint32_t countOfFadeSection = header.sizeOfFadeSection/ 32;
    uint32_t countOfScriptSection = header.sizeOfScriptSection / 40;
    uint32_t countOfSoundSection = header.sizeOfSoundSection / 40;
    for (size_t i = 0; i < countOfFadeSection; i++) {
      FadeChunk chunk;
      stream.READ(chunk);
      std::cerr << "Fade chunk: Time: " << chunk.getStartOfFadeEvent()
                << " Flags: 0x" << std::hex << chunk.getFlags() <<std::dec << " - " << chunk.getFlagsAsString() 
                << " Duration: " << chunk.timeOfDarkening << std::endl;
      currentFile.fadeChunks.push_back(chunk);
    }

    for (size_t i = 0; i < countOfScriptSection; i++) {
      ScriptChunk chunk;
      stream.READ(chunk);
      std::cerr << "Script chunk: " << chunk.timestamp
                << " Name: " << chunk.scriptName << std::endl;
      currentFile.scriptChunks.push_back(chunk);
    }

    for (size_t i = 0; i < countOfSoundSection; i++) {
      SoundChunk chunk;
      stream.READ(chunk);
      std::cerr << "Sound chunk: " << chunk.timestamp
                << " Type: " << chunk.getTypeStringRepresentation() << " ("
                << chunk.type << ") Name: " << chunk.soundName << std::endl;
      currentFile.soundChunks.push_back(chunk);
    }
  }

  void readDialogs(std::ifstream& stream)
  {
    DialogHeader header;
    stream.READ(header);
    std::cerr << "[DialogSection] Count of dialog chunks: " << header.countOfDialogs << std::endl;
    std::cerr << "[DialogSection] Unk: " << header.countOfNarratorChunks<< std::endl;
    std::cerr << "[DialogSection] Unk2: " << header.unk2<< std::endl;

    for (size_t i = 0; i < header.countOfDialogs; i++) {
      DialogChunk chunk;
      stream.READ(chunk);
      std::cerr << "Dialog chunk: stamp: " << chunk.timestamp
                << " ID: " << chunk.channelID << " animation ID: " << std::hex
                << chunk.dialogID << std::dec;
                if(chunk.dialogID == 1)
                    std::cerr << " Name: " << chunk.framename;
                std::cerr << std::endl;
      currentFile.dialogChunks.push_back(chunk);
    }

    for (size_t i = 0; i < header.countOfNarratorChunks; i++) {
      NarratorChunk chunk;
      stream.READ(chunk);
      std::cerr << "Narrator chunk: timestamp: " << chunk.timestamp 
                << " unk: " << chunk.unk2 
                << " speechID: " << std::hex << chunk.speechID << std::dec
                << std::endl;
    }

    for (size_t i = 0; i < header.unk2; i++) {
      UnkChunk chunk;
      stream.READ(chunk);
      std::cerr << "Unk chunk: timestamp: " << chunk.timestamp 
                << " unk: " << chunk.unk1 
                << " frameName: " << chunk.frameName
                << " unk: " << chunk.unk2 
                << std::endl;
    }



  }

public:
  File loadFile(std::string fileName)
  {

    std::cerr << "[RepParser] Parsing file: " << fileName << std::endl;
    std::ifstream inputFile;
    inputFile.open(fileName, std::ifstream::binary);
    if (inputFile.is_open()) {
      inputFile.READ(fileHeader);
      std::cout << "Magic Byte: " << std::hex << fileHeader.magicByte
                << std::dec << std::endl;
      std::cout << "Anim block size: " << fileHeader.sizeOfAnimationSection
                << std::endl;
      std::cout << "Count of anims: " << fileHeader.countOfAnimationBlocks
                << std::endl;
      if (fileHeader.magicByte != magicByteConstant) {
        std::cerr << "[Err] Invalid magic byte ...\n" << std::endl;
        return File();
      }

      readAnimations(inputFile);
      readObjectDefinitions(inputFile);
      readTransformation(inputFile);
      readCameraSection(inputFile);
      readScriptEvents(inputFile);
      readDialogs(inputFile);
    } else {
      std::cerr << "[Err] Failed to open file " << fileName << std::endl;
    }
    return currentFile;
  }
};
} // namespace RepFile
