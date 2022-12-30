//
// Created by ethanmoran on 12/24/22.
//
#include "readVox.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

//TODO: Figure out what to do with max size stuff. In magicavoxel this is technically 256*256*256, but we need to decide
//how we want to restrict clump size in this program anyway which determines how we import something that is potentially
//bigger than this max size with multiple models. For now though, if we don't restirct max size, we can simply use the
//transform information to piece back together the split up larger models. Ideally the way they are constructed
//outside of magicavoxel vs imported is treated the same.. but also the voxels will have attributes like stickyness
//or stuff so maybe we just encourage making everything separately in small segemnts and only allow imports of certain siszes..
//idk problem for later for sure
readVox::voxInfo readVox::readFromFile(const char *vox_file_path) {

    voxInfo newVoxInfo = voxInfo();
    //find a better way to throw away bytes in rust
    char* throwaway = new char[5000];
    std::ifstream voxfile;
    voxfile.open(vox_file_path);

    char* title = new char[5];
    voxfile.read(title, 4);
    title[4] = '\0';
//    std::cout << title << "hello";
    uint32_t versionNumber;
    voxfile.read((char*)&versionNumber, 4);


    char* firstChunkID = new char[5];
    voxfile.read(firstChunkID, 4);
    firstChunkID[4] = '\0';

    uint32_t numBytesContent;
    voxfile.read((char*)&numBytesContent, 4);

    uint32_t numChildrenBytes;
    voxfile.read((char*)&numChildrenBytes, 4);

    if (std::strcmp(firstChunkID, "MAIN") != 0){
        //In rust make this an actual error.
        std::cout << "ERROR: First chunk is not of type MAIN";

    }
    //Main chunk has no content, so next byte is ID of next chunk
    bool packExists = false;
    char* secondChunkID = new char[5];
    voxfile.read(secondChunkID, 4);
    secondChunkID[4] = '\0';

    uint32_t numModels = 1;
    uint32_t PACKnumBytesContent = 0;
    uint32_t PACKnumChildrenBytes = 0;

    if (std::strcmp(firstChunkID, "PACK") == 0){
        voxfile.read((char*)&PACKnumBytesContent, 4);
        voxfile.read((char*)&PACKnumChildrenBytes, 4);
        voxfile.read((char*)&numModels, 4);
        packExists = true;
    }


    for (int i = 0; i < numModels; i++){
        model thisModel;

        uint32_t SIZEnumBytesContent = 0;
        uint32_t SIZEnumChildrenBytes = 0;
        //MagicaVoxel has Z in the Y direction, so we just get the variables in xzy order here.
        uint32_t xSize = 0;
        uint32_t zSize = 0;
        uint32_t ySize = 0;

        if (packExists) {
            //Next will be SIZE (check this is rust anyway)
            voxfile.read(throwaway, 4);
        }
        voxfile.read((char*)&SIZEnumBytesContent, 4);
        voxfile.read((char*)&SIZEnumChildrenBytes, 4);
        voxfile.read((char*)&xSize, 4);
        voxfile.read((char*)&zSize, 4);
        voxfile.read((char*)&ySize, 4);

        int BBSize = xSize * ySize * zSize;
        std::vector<int> voxelExistenceArray(BBSize);
        std::fill(voxelExistenceArray.begin(), voxelExistenceArray.end(), -1);

        //Next is the XYZI chunk (check this in rust)
        char* XYZIChunkID = new char[5];
        voxfile.read(XYZIChunkID, 4);
        XYZIChunkID[4] = '\0';
        uint32_t XYZInumBytesContent = 0;
        uint32_t XYZInumChildrenBytes = 0;
        uint32_t numVoxels;

        voxfile.read((char*)&XYZInumBytesContent, 4);
        voxfile.read((char*)&XYZInumChildrenBytes, 4);
        voxfile.read((char*)&numVoxels, 4);

        for (int x = 0; x < numVoxels; x++) {
            unsigned char xVal;
            unsigned char zVal;
            unsigned char yVal;
            unsigned char colorIndex;
            voxfile.read((char *) &xVal, 1);
            voxfile.read((char *) &zVal, 1);
            voxfile.read((char *) &yVal, 1);
            voxfile.read((char *) &colorIndex, 1);
            //Right now it goes right into color index, in the future it should map into an additional array that
            //gives the index in every information array (like material and such).
            //So mapping to 5 would give 5 -> [1,2,3] for color, material, shininess.
            //Can create some sort of hash on the CPU when consrtucting to avoid numVoxels size for this additional
            //table, as multiple voxels can share the exact same properties so if it exists it should go to the same index

            voxelExistenceArray[zVal * int(ySize) * int(xSize) + yVal * int(xSize) + xVal] = (int) colorIndex - 1;
        }


        //Next is the nTRN chunk, nGRP, nSHP, MTL, LAYR, rOBJ, rCAM, NOTE, IMAP (check this in rust)
        //FOr now we skip these , but maybe we can still use this information when importing
        //multiple models to retain this additional information

        char* chunkID = new char[5];
        voxfile.read(chunkID, 4);
        chunkID[4] = '\0';
        //So we skip every chunk until we reach RGBA.
        while (std::strcmp(chunkID, "RGBA") != 0){
            uint32_t numBytesContent = 0;
            uint32_t numChildrenBytes = 0;
            voxfile.read((char*)&numBytesContent, 4);
            voxfile.read((char*)&numChildrenBytes, 4);
            voxfile.read(throwaway, numBytesContent);
            voxfile.read(throwaway, numChildrenBytes);

            voxfile.read(chunkID, 4);
            chunkID[4] = '\0';
        }

        //Now we should be at the RGBA chunk


        uint32_t RGBAnumBytesContent = 0;
        uint32_t RGBAChildrenBytes = 0;
        voxfile.read((char*)&RGBAnumBytesContent, 4);
        voxfile.read((char*)&RGBAChildrenBytes, 4);


        voxfile.read((char*)thisModel.colors,RGBAnumBytesContent);

        thisModel.xSize = (int)xSize;
        thisModel.ySize = (int)ySize;
        thisModel.zSize = (int)zSize;
        thisModel.voxelArray = voxelExistenceArray;
        newVoxInfo.allModelsInFile.push_back(thisModel);
    }
    newVoxInfo.numModels = numModels;
    return newVoxInfo;
}
