/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
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

#include "Context.h"
#include "function.h"
#include "printutils.h"
#include "Expression.h"
#include "Geometry.h"
#include "GeometryEvaluator.h"
#include "PolySet.h"
#include "PolySetUtils.h"
#include "Value.h"
#include "Tree.h"

CallableFunction wrapModuleAsFunction(const InstantiableModule& module, const Location& loc) {
  return std::make_shared<const BuiltinFunction>([module, loc] (const std::shared_ptr<const Context>& context, const FunctionCall *call) {
    auto inst = std::make_unique<ModuleInstantiation>(call->name, call->arguments, loc);
    auto node = module.module->instantiate(module.defining_context, inst.get(), context);
    if (!node) {
      throw std::runtime_error("ModuleFunctionAdapter: failed to instantiate module");
    }
    Tree tree(node);
    GeometryEvaluator evaluator(tree);
    auto geom = evaluator.evaluateGeometry(*node, /* allownef= */ false);
    auto ps = PolySetUtils::getGeometryAsPolySet(geom);
    if (ps) {
      ps = PolySetUtils::tessellate_faces(*ps);
    }
    if (!ps) {
      throw std::runtime_error("ModuleFunctionAdapter: failed to evaluate geometry");
    }

    VectorType vertices(context->session());
    vertices.reserve(ps->vertices.size());
    for (const auto & v : ps->vertices) {
      VectorType vertex(context->session());
      vertex.reserve(3);
      vertex.emplace_back(v[0]);
      vertex.emplace_back(v[1]);
      vertex.emplace_back(v[2]);
      vertices.emplace_back(std::move(vertex));
    }
    
    VectorType faces(context->session());
    faces.reserve(ps->indices.size());
    for (const auto & f : ps->indices) {
      VectorType face(context->session());
      face.reserve(3);
      face.emplace_back(f[0]);
      face.emplace_back(f[1]);
      face.emplace_back(f[2]);
      faces.emplace_back(std::move(face));
    }

    const Color4f invalid_color;
    VectorType colors(context->session());
    colors.reserve(ps->color_indices.size());
    for (const auto & ci : ps->color_indices) {
      const auto & color = ci >= 0 && ci < ps->colors.size() ? ps->colors[ci] : invalid_color;
      VectorType colorVec(context->session());
      colorVec.reserve(4);
      colorVec.emplace_back(color[0]);
      colorVec.emplace_back(color[1]);
      colorVec.emplace_back(color[2]);
      colorVec.emplace_back(color[3]);
      colors.emplace_back(std::move(colorVec));
    }

    auto make_pair = [&](const char* name, VectorType && v) {
      VectorType pair(context->session());
      pair.reserve(2);
      pair.emplace_back(std::string(name));
      pair.emplace_back(std::move(v));
      return std::move(pair);
    };
    VectorType ret(context->session());
    ret.reserve(3);
    ret.emplace_back(std::move(make_pair("vertices", std::move(vertices))));
    ret.emplace_back(std::move(make_pair("faces", std::move(faces))));
    if (!colors.empty()) {
      ret.emplace_back(std::move(make_pair("colors", std::move(colors))));
    }

    return std::move(ret);
  });
}
