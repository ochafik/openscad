/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *  Copyright (C) 2021      Konstantin Podsvirov <konstantin@podsvirov.pro>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "export.h"

#include "PolySetUtils.h"
#include "PolySet.h"
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>

void export_glb(const std::map<Color4f, std::shared_ptr<const Geometry>>& geoms, std::ostream& output)
{
  // FIXME: In lazy union mode, should we export multiple objects?
  output << "# OpenSCAD assimp exporter\n";
  
  std::vector<aiMesh> meshes;
  std::vector<aiMaterial> materials;

  for (const auto & [color, geom] : geoms) {
    std::shared_ptr<const PolySet> out = PolySetUtils::getGeometryAsPolySet(geom);
    if (Feature::ExperimentalPredictibleOutput.is_enabled()) {
        out = createSortedPolySet(*out);
    }
    auto numVertices = out->vertices.size();
    auto numTriangles = out->indices.size();

    meshes.resize(meshes.size() + 1);
    aiMesh * mesh = &meshes.back();
    // auto mesh = new aiMesh();
    mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
    mesh->mNumVertices = numVertices;
    mesh->mNumFaces = numTriangles;
    mesh->mVertices = new aiVector3D[numVertices];
    for (unsigned int i = 0; i < numVertices; i++) {
        mesh->mVertices[i] = aiVector3D(out->vertices[i][0], out->vertices[i][1], out->vertices[i][2]);
    }

    mesh->mFaces = new aiFace[numTriangles];
    for (unsigned int i = 0; i < numTriangles; i++) {
        aiFace& face = mesh->mFaces[i];
        face.mNumIndices = 3;
        face.mIndices = new unsigned int[3];
        face.mIndices[0] = out->indices[i][0];
        face.mIndices[1] = out->indices[i][1];
        face.mIndices[2] = out->indices[i][2];
    }

    materials.resize(materials.size() + 1);
    aiMaterial * material = &materials.back();
    // aiMaterial* material = new aiMaterial();
    aiColor4D color(color.r, color.g, color.b, color.a);
    material->AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

  }

    auto scene = std::make_unique<aiScene>();
    scene->mNumMeshes = meshes.size();
    scene->mMeshes = meshes.data();

    scene->mNumMaterials = materials.size();
    scene->mMaterials = materials.data();

    scene->mRootNode = new aiNode();
    scene->mRootNode->mNumMeshes = 1;
    scene->mRootNode->mMeshes = new unsigned int[meshes.size()];
    for (unsigned int i = 0; i < meshes.size(); i++) {
        scene->mRootNode->mMeshes[i] = i;
    }
    Assimp::Exporter exporter;
    // exporter.Export(scene, "glb", "output.glb");
    
    const aiExportDataBlob* blob = exporter.ExportToBlob(scene, "glb");
    if (blob == nullptr) {
        throw std::runtime_error("Failed to export the scene to GLB format.");
    }
    std::ostringstream oss;
    oss.write(reinterpret_cast<const char*>(blob->data), blob->size);

    std::string exportedScene = oss.str();
    output.write(exportedScene.data(), exportedScene.size());

    scene->mMeshes = nullptr;
    scene->mMaterials = nullptr;
}
