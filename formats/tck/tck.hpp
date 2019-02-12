/*
 * .tck file printer
 * Author: Roman Romop5 Dobias
 * Purpose: print outs content of .rep file
 * Credits: djbozkosz for RE the .tck format
 */

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <string>
#include <vector>

/* \brief Stores cutscene directives
 *
 * .tck files store position of animated object for each animation frame
 * where as .5ds files store relative transformation of different parts (bones)
 * for each frame of animation
 */
namespace TckFile {

const uint32_t magicByteConstant = 0x4;

#pragma pack(push, 1)
struct Header
{
  uint32_t magicByte;              // 0x4 - determines .tck file
  float startPosition[3];           // TODO: find out what it stands for - maybe a bounding box ?
  float endPosition[3];
  uint32_t lengthOfAnimation;
  uint32_t milisecondsPerFrame;      // frequency ?
  uint32_t countOfPositionBlocks;    // verified 
};

struct PositionBlock
{
  float position[3];
};
#pragma pack(pop)

class Loader;

class File
{
    friend Loader;
    Header header;
    std::vector<PositionBlock> positionBlocks;
public:
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
  Header fileHeader;
  File currentFile;

  bool readPositionBlocks(std::ifstream& inputFile)
  {
    PositionBlock block;
    for(size_t i = 0; i < fileHeader.countOfPositionBlocks; i++)
    {
        inputFile.READ(block);
        // Note: ignore first position block as it contains zeros and according to the duration of anim
        // it shouldn't be used
        if(i == 0)
            continue;
        std::cerr << "Position: " << block.position[0] << ", " << block.position[1] << ", " << block.position[2] << std::endl;
        currentFile.positionBlocks.push_back(block);
    }
    return true;
  }
public:
  File loadFile(std::string fileName)
  {
    std::cerr << "[TckParser] Parsing file: " << fileName << std::endl;
    std::ifstream inputFile;
    inputFile.open(fileName, std::ifstream::binary);
    if (inputFile.is_open()) {
      inputFile.READ(fileHeader);
      std::cout << "Magic byte: " << std::hex << fileHeader.magicByte << std::dec << std::endl;
      std::cout << "Start pos: " << " [" << fileHeader.startPosition[0] << ", " << fileHeader.startPosition[1] 
          << ", " <<fileHeader.startPosition[2] << "] " <<  std::endl;
      std::cout << "End pos: " << " [" << fileHeader.endPosition[0] << ", " << fileHeader.endPosition[1] 
          << ", " <<fileHeader.endPosition[2] << "] " <<  std::endl;
      std::cout << "Duration: " << fileHeader.lengthOfAnimation << std::endl;
      std::cout << "Miliseconds per frame: " << fileHeader.milisecondsPerFrame << std::endl;
      std::cout << "Count of position blocks: " << fileHeader.countOfPositionBlocks << std::endl;
      if (fileHeader.magicByte != magicByteConstant) {
        std::cerr << "[Err] Invalid magic byte ...\n" << std::endl;
        return File();
      }
      readPositionBlocks(inputFile);
      inputFile.close();
    } else {
      std::cerr << "[Err] Failed to open file " << fileName << std::endl;
    }
    return currentFile;
  }
  bool storeFile(File file, std::string fileName)
  {
      return false;
  }
};
} // namespace TckFile
