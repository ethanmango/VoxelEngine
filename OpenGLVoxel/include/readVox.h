//
// Created by ethanmoran on 12/24/22.
//

#ifndef OPENGLVOXEL_READVOX_H
#define OPENGLVOXEL_READVOX_H

#include <vector>

class readVox {

public:
    struct model {
        int xSize{};
        int ySize{};
        int zSize{};
        //For now, stores a -1 if voxel doesn't exist, and index into color array if it exists
        std::vector<int> voxelArray;
    };
    struct voxInfo {

        int numModels{};
        std::vector<struct model> allModelsInFile;
    };

    voxInfo readFromFile(const char * vertex_file_path);
};

#endif //OPENGLVOXEL_READVOX_H
