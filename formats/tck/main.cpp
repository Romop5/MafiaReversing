#include "tck.hpp"
using namespace TckFile;
int main(int argc, char** argv)
{
    Loader loader;
    if(argc < 2)
    {
	    std::cerr << "USAGE: pathToRecordFile.rec" << std::endl;
	    return 0;
    }
    auto rep = loader.loadFile(argv[1]);
}
