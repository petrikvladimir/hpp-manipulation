// Copyright (c) 2014, LAAS-CNRS
// Authors: Joseph Mirabel (joseph.mirabel@laas.fr)
//
// This file is part of hpp-manipulation.
// hpp-manipulation is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// hpp-manipulation is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.  You should have
// received a copy of the GNU Lesser General Public License along with
// hpp-manipulation. If not, see <http://www.gnu.org/licenses/>.

#include "hpp/manipulation/graph-path-validation.hh"

namespace hpp {
  namespace manipulation {
    GraphPathValidationPtr_t GraphPathValidation::create (const PathValidationPtr_t& pathValidation)
    {
      GraphPathValidation* p = new GraphPathValidation (pathValidation);
      return GraphPathValidationPtr_t (p);
    }

    GraphPathValidation::GraphPathValidation (const PathValidationPtr_t& pathValidation) :
      pathValidation_ (pathValidation), constraintGraph_ ()
    {}

    bool GraphPathValidation::validate (
          const PathPtr_t& path, bool reverse, PathPtr_t& validPart)
    {
      assert (path);
      bool success = impl_validate (path, reverse, validPart);
      assert (constraintGraph_);
      assert (constraintGraph_->getNode ((*validPart) (validPart->timeRange ().first)));
      assert (constraintGraph_->getNode ((*validPart) (validPart->timeRange ().second)));
      return success;
    }

    bool GraphPathValidation::validate
    (const PathPtr_t& path, bool reverse, PathPtr_t& validPart,
     ValidationReport&)
    {
      assert (path);
      return impl_validate (path, reverse, validPart);
    }

    bool GraphPathValidation::validate (const PathPtr_t& path, bool reverse,
					PathPtr_t& validPart,
					PathValidationReportPtr_t&)
    {
      assert (path);
      return impl_validate (path, reverse, validPart);
    }

    bool GraphPathValidation::impl_validate (
        const PathVectorPtr_t& path, bool reverse, PathPtr_t& validPart)
    {
      PathPtr_t validSubPart;
      if (reverse) {
        // TODO: This has never been tested.
        assert (!reverse && "This has never been tested with reverse path");
        for (long int i = path->numberPaths () - 1; i >= 0; i--) {
          // We should stop at the first non valid subpath.
          if (!impl_validate (path->pathAtRank (i), true, validSubPart)) {
            PathVectorPtr_t p = PathVector::create
	      (path->outputSize(), path->outputDerivativeSize());
            for (long int v = path->numberPaths () - 1; v > i; v--)
              p->appendPath (path->pathAtRank(i)->copy());
            // TODO: Make sure this subpart is generated by the steering method.
            p->appendPath (validSubPart);
            validPart = p;
            return false;
          }
        }
      } else {
        for (size_t i = 0; i != path->numberPaths (); i++) {
          // We should stop at the first non valid subpath.
          if (!impl_validate (path->pathAtRank (i), false, validSubPart)) {
            PathVectorPtr_t p = PathVector::create
	      (path->outputSize(), path->outputDerivativeSize());
            for (size_t v = 0; v < i; v++)
              p->appendPath (path->pathAtRank(v)->copy());
            // TODO: Make sure this subpart is generated by the steering method.
            p->appendPath (validSubPart);
            validPart = p;
            return false;
          }
        }
      }
      // Here, every subpath is valid.
      validPart = path;
      return true;
    }

    bool GraphPathValidation::impl_validate (
        const PathPtr_t& path, bool reverse, PathPtr_t& validPart)
    {
      PathVectorPtr_t pathVector = HPP_DYNAMIC_PTR_CAST(PathVector, path);
      if (pathVector)
        return impl_validate (pathVector, reverse, validPart);

      PathPtr_t pathNoCollision;
      PathValidationReportPtr_t report;
      if (pathValidation_->validate (path, reverse, pathNoCollision, report)) {
        validPart = path;
        return true;
      }
      const Path& newPath (*pathNoCollision);
      const Path& oldPath (*path);
      const core::interval_t& newTR = newPath.timeRange (),
                              oldTR = oldPath.timeRange ();
      Configuration_t q (newPath.outputSize());
      if (!newPath (q, newTR.first))
        throw std::logic_error ("Initial configuration of the valid part cannot be projected.");
      const graph::NodePtr_t& origNode = constraintGraph_->getNode (q);
      if (!newPath (q, newTR.second))
        throw std::logic_error ("End configuration of the valid part cannot be projected.");
      const graph::NodePtr_t& destNode = constraintGraph_->getNode (q);
      if (!oldPath (q, oldTR.first))
        throw std::logic_error ("Initial configuration of the path to be validated cannot be projected.");
      const graph::NodePtr_t& oldOnode = constraintGraph_->getNode (q);
      if (!oldPath (q, oldTR.second))
        throw std::logic_error ("End configuration of the path to be validated cannot be projected.");
      const graph::NodePtr_t& oldDnode = constraintGraph_->getNode (q);

      if (origNode == oldOnode && destNode == oldDnode) {
        validPart = pathNoCollision;
        return false;
      }

      // Here, the full path is not valid. Moreover, it does not correspond to the same
      // edge of the constraint graph. Two option are possible:
      // - Use the steering method to create a new path and validate it.
      // - Return a null path.
      validPart = path->extract (std::make_pair (oldTR.first,oldTR.first));
      return false;
    }

    void GraphPathValidation::addObstacle (const hpp::core::CollisionObjectPtr_t& collisionObject)
    {
      pathValidation_->addObstacle (collisionObject);
    }
  } // namespace manipulation
} // namespace hpp
