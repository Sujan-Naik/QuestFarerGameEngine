#ifndef VOXEL_GRID_H
#define VOXEL_GRID_H

#include <vector>
#include <algorithm>
#include <cmath>

namespace VoxelLib {

class VoxelGrid
{
private:
    std::vector<bool> voxels;

    /**
     * Round up to the next highest power of 2
     * https://graphics.stanford.edu/~seander/bithacks.html
     */
    static int NextPowerOfTwo(int v)
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }

    /**
     * https://www.forceflow.be/2013/10/07/morton-encodingdecoding-through-bit-interleaving-implementations/
     */
    static int Interleave(int x)
    {
        x &= 0x3ff;
        x = (x | (x << 16)) & 0x30000ff;
        x = (x | (x << 8)) & 0x0300f00f;
        x = (x | (x << 4)) & 0x30c30c3;
        x = (x | (x << 2)) & 0x9249249;
        return x;
    }

public:
    int maxX, maxY, maxZ;
    int requiredStorageSize;

    VoxelGrid(int maxX, int maxY, int maxZ)
        : maxX(maxX), maxY(maxY), maxZ(maxZ)
    {
        int tightestFit = NextPowerOfTwo(std::max(maxX, std::max(maxY, maxZ)));
        requiredStorageSize = tightestFit * tightestFit * tightestFit;
        voxels.resize(requiredStorageSize, false);
    }

    virtual ~VoxelGrid() = default;

    int GetRequiredStorageSize() const
    {
        return requiredStorageSize;
    }

    void AddVoxels(const std::vector<bool>& newVoxels)
    {
        if (newVoxels.size() != voxels.size())
        {
            return; // Size mismatch, operation skipped
        }

        for (std::size_t i = 0; i < voxels.size(); ++i)
        {
            voxels[i] = voxels[i] | newVoxels[i];
        }
    }

    void SetVoxels(const std::vector<bool>& newVoxels)
    {
        voxels = newVoxels;
    }

    int Flatten(int x, int y, int z) const
    {
        return Interleave(x) | (Interleave(y) << 1) | (Interleave(z) << 2);
    }

    bool GetVoxel(int x, int y, int z) const
    {
        if (x < 0 || x >= maxX) return false;
        if (z < 0 || z >= maxZ) return false;
        if (y < 0 || y >= maxY) return false;

        int index = Flatten(x, y, z);
        if (index < 0 || index >= static_cast<int>(voxels.size()))
        {
            return false;
        }

        return voxels[index];
    }
};

}

#endif // VOXEL_GRID_H