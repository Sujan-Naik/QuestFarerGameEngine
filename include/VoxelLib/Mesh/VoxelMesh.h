#ifndef VOXEL_MESH_H
#define VOXEL_MESH_H

#include <vector>
#include <array>
#include "Vector3F.h"
#include "Vector2F.h"

namespace VoxelLib::Mesh {

/**
 * Inspired by https://learnopengl.com/Getting-started/Hello-Triangle
 */
struct VoxelMesh
{
    std::vector<Vector3F> newVertices;
    std::vector<Vector2F> newUV;
    std::vector<int> newTriangles;

    VoxelMesh(bool top, bool back, bool left, bool front, bool right, bool bottom)
    {
        std::vector<Vector3F> vertices;
        std::vector<Vector2F> uvs;
        std::vector<int> indices;

        if (top)
        {
            int baseIndex = vertices.size();

            vertices.insert(vertices.end(), {
                Vector3F(1.0f, 1.0f, 1.0f), // top right
                Vector3F(1.0f, 1.0f, 0.0f), // bottom right
                Vector3F(0.0f, 1.0f, 0.0f), // bottom left
                Vector3F(0.0f, 1.0f, 1.0f)  // top left
            });

            uvs.insert(uvs.end(), {
                Vector2F(1.0f, 1.0f),
                Vector2F(1.0f, 0.0f),
                Vector2F(0.0f, 0.0f),
                Vector2F(0.0f, 1.0f)
            });

            indices.insert(indices.end(), {
                baseIndex + 0, baseIndex + 1, baseIndex + 3,
                baseIndex + 1, baseIndex + 2, baseIndex + 3
            });
        }

        if (back)
        {
            int baseIndex = vertices.size();

            vertices.insert(vertices.end(), {
                Vector3F(1.0f, 1.0f, 0.0f), // top right
                Vector3F(1.0f, 0.0f, 0.0f), // bottom right
                Vector3F(0.0f, 0.0f, 0.0f), // bottom left
                Vector3F(0.0f, 1.0f, 0.0f)  // top left
            });

            uvs.insert(uvs.end(), {
                Vector2F(1.0f, 1.0f),
                Vector2F(1.0f, 0.0f),
                Vector2F(0.0f, 0.0f),
                Vector2F(0.0f, 1.0f)
            });

            indices.insert(indices.end(), {
                baseIndex + 0, baseIndex + 1, baseIndex + 3,
                baseIndex + 1, baseIndex + 2, baseIndex + 3
            });
        }

        if (left)
        {
            int baseIndex = vertices.size();

            vertices.insert(vertices.end(), {
                Vector3F(0.0f, 1.0f, 0.0f), // top right
                Vector3F(0.0f, 0.0f, 0.0f), // bottom right
                Vector3F(0.0f, 0.0f, 1.0f), // bottom left
                Vector3F(0.0f, 1.0f, 1.0f)  // top left
            });

            uvs.insert(uvs.end(), {
                Vector2F(1.0f, 0.0f),
                Vector2F(0.0f, 0.0f),
                Vector2F(0.0f, 1.0f),
                Vector2F(1.0f, 1.0f)
            });

            indices.insert(indices.end(), {
                baseIndex + 0, baseIndex + 1, baseIndex + 3,
                baseIndex + 1, baseIndex + 2, baseIndex + 3
            });
        }

        if (front)
        {
            int baseIndex = vertices.size();

            vertices.insert(vertices.end(), {
                Vector3F(0.0f, 1.0f, 1.0f), // top right
                Vector3F(0.0f, 0.0f, 1.0f), // bottom right
                Vector3F(1.0f, 0.0f, 1.0f), // bottom left
                Vector3F(1.0f, 1.0f, 1.0f)  // top left
            });

            uvs.insert(uvs.end(), {
                Vector2F(0.0f, 1.0f),
                Vector2F(0.0f, 0.0f),
                Vector2F(1.0f, 0.0f),
                Vector2F(1.0f, 1.0f)
            });

            indices.insert(indices.end(), {
                baseIndex + 0, baseIndex + 1, baseIndex + 3,
                baseIndex + 1, baseIndex + 2, baseIndex + 3
            });
        }

        if (right)
        {
            int baseIndex = vertices.size();

            vertices.insert(vertices.end(), {
                Vector3F(1.0f, 1.0f, 1.0f), // top right
                Vector3F(1.0f, 0.0f, 1.0f), // bottom right
                Vector3F(1.0f, 0.0f, 0.0f), // bottom left
                Vector3F(1.0f, 1.0f, 0.0f)  // top left
            });

            uvs.insert(uvs.end(), {
                Vector2F(1.0f, 1.0f),
                Vector2F(0.0f, 1.0f),
                Vector2F(0.0f, 0.0f),
                Vector2F(1.0f, 0.0f)
            });

            indices.insert(indices.end(), {
                baseIndex + 0, baseIndex + 1, baseIndex + 3,
                baseIndex + 1, baseIndex + 2, baseIndex + 3
            });
        }

        if (bottom)
        {
            int baseIndex = vertices.size();

            vertices.insert(vertices.end(), {
                Vector3F(1.0f, 0.0f, 1.0f), // top right
                Vector3F(1.0f, 0.0f, 0.0f), // bottom right
                Vector3F(0.0f, 0.0f, 0.0f), // bottom left
                Vector3F(0.0f, 0.0f, 1.0f)  // top left
            });

            uvs.insert(uvs.end(), {
                Vector2F(1.0f, 1.0f),
                Vector2F(1.0f, 0.0f),
                Vector2F(0.0f, 0.0f),
                Vector2F(0.0f, 1.0f)
            });

            indices.insert(indices.end(), {
                baseIndex + 3, baseIndex + 1, baseIndex + 0,
                baseIndex + 3, baseIndex + 2, baseIndex + 1
            });
        }

        newVertices = vertices;
        newUV = uvs;
        newTriangles = indices;
    }
};

}

#endif // VOXEL_MESH_H