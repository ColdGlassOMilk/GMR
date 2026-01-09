#pragma once

#include "raylib.h"
#include <cstdlib>

namespace gmr {
namespace bindings {

// Storage data file name
#define GMR_STORAGE_DATA_FILE "storage.data"

// Implementation of raylib's removed storage functions
// Based on: https://github.com/raysan5/raylib/blob/master/examples/core/core_storage_values.c

inline bool SaveStorageValue(unsigned int position, int value) {
    bool success = false;
    int dataSize = 0;
    int newDataSize = 0;
    unsigned char* fileData = LoadFileData(GMR_STORAGE_DATA_FILE, &dataSize);
    unsigned char* newFileData = nullptr;

    if (fileData != nullptr) {
        if (dataSize <= (int)(position * sizeof(int))) {
            // Need to expand file
            newDataSize = (position + 1) * sizeof(int);
            newFileData = (unsigned char*)RL_REALLOC(fileData, newDataSize);

            if (newFileData != nullptr) {
                // Zero out new memory
                for (int i = dataSize; i < newDataSize; i++) {
                    newFileData[i] = 0;
                }
                int* dataPtr = (int*)newFileData;
                dataPtr[position] = value;
            } else {
                // Realloc failed, use original data
                newFileData = fileData;
                newDataSize = dataSize;
            }
        } else {
            // File is large enough
            newFileData = fileData;
            newDataSize = dataSize;
            int* dataPtr = (int*)newFileData;
            dataPtr[position] = value;
        }

        success = SaveFileData(GMR_STORAGE_DATA_FILE, newFileData, newDataSize);
        RL_FREE(newFileData);
    } else {
        // File doesn't exist, create it
        dataSize = (position + 1) * sizeof(int);
        fileData = (unsigned char*)RL_MALLOC(dataSize);

        // Zero out memory
        for (int i = 0; i < dataSize; i++) {
            fileData[i] = 0;
        }

        int* dataPtr = (int*)fileData;
        dataPtr[position] = value;

        success = SaveFileData(GMR_STORAGE_DATA_FILE, fileData, dataSize);
        UnloadFileData(fileData);
    }

    return success;
}

inline int LoadStorageValue(unsigned int position) {
    int value = 0;
    int dataSize = 0;
    unsigned char* fileData = LoadFileData(GMR_STORAGE_DATA_FILE, &dataSize);

    if (fileData != nullptr) {
        if (dataSize < (int)(position * sizeof(int) + sizeof(int))) {
            // Position out of bounds, return 0
            value = 0;
        } else {
            int* dataPtr = (int*)fileData;
            value = dataPtr[position];
        }

        UnloadFileData(fileData);
    }

    return value;
}

}  // namespace bindings
}  // namespace gmr
