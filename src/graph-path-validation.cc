// Copyright (c) 2014, LAAS-CNRS
// Authors: Joseph Mirabel (joseph.mirabel@laas.fr)
//

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.

#include "hpp/manipulation/graph-path-validation.hh"

#include <hpp/pinocchio/configuration.hh>

#include <hpp/core/path.hh>
#include <hpp/core/path-vector.hh>

#include <hpp/manipulation/graph/state.hh>
#include <hpp/manipulation/graph/edge.hh>
#include <hpp/manipulation/graph/graph.hh>
#include <hpp/manipulation/constraint-set.hh>

#ifdef HPP_DEBUG
# include <hpp/manipulation/graph/state.hh>
#endif

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

    bool GraphPathValidation::validate (const PathPtr_t& path, bool reverse,
					PathPtr_t& validPart,
					PathValidationReportPtr_t& report)
    {
      assert (path);
      bool success = impl_validate (path, reverse, validPart, report);
      assert (constraintGraph_);
      assert (constraintGraph_->getState (validPart->initial ()));
      assert (constraintGraph_->getState (validPart->end     ()));
      return success;
    }

    bool GraphPathValidation::impl_validate (const PathVectorPtr_t& path,
        bool reverse, PathPtr_t& validPart, PathValidationReportPtr_t& report)
    {
      PathPtr_t validSubPart;
      if (reverse) {
        // TODO: This has never been tested.
        assert (!reverse && "This has never been tested with reverse path");
        for (long int i = path->numberPaths () - 1; i >= 0; i--) {
          // We should stop at the first non valid subpath.
          if (!impl_validate (path->pathAtRank (i), true, validSubPart, report)) {
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
          if (!impl_validate (path->pathAtRank (i), false, validSubPart, report)) {
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

    bool GraphPathValidation::impl_validate (const PathPtr_t& path,
        bool reverse, PathPtr_t& validPart, PathValidationReportPtr_t& report)
    {
#ifndef NDEBUG
      bool success;
      Configuration_t q0 = path->eval(path->timeRange ().second, success);
      assert (success);
      assert (!path->constraints () || path->constraints ()->isSatisfied (q0));
#endif
      using pinocchio::displayConfig;
      PathVectorPtr_t pathVector = HPP_DYNAMIC_PTR_CAST(PathVector, path);
      if (pathVector)
        return impl_validate (pathVector, reverse, validPart, report);

      PathPtr_t pathNoCollision;
      ConstraintSetPtr_t c = HPP_DYNAMIC_PTR_CAST(ConstraintSet, path->constraints());
      hppDout(info, (c?"Using edge path validation":"Using default path validation"));
      PathValidationPtr_t validation (c
          ? c->edge()->pathValidation()
          : pathValidation_);

      if (validation->validate (path, reverse, pathNoCollision, report)) {
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
      const graph::StatePtr_t& origState = constraintGraph_->getState (q);
      if (!newPath (q, newTR.second))
        throw std::logic_error ("End configuration of the valid part cannot be projected.");
      // This may throw in the following case:
      // - state constraints: object_placement + other_function
      // - path constraints: other_function, object_lock
      // This is semantically correct but for a path going from q0 to q1,
      // we ensure that
      // - object_placement (q0) = eps_place0
      // - other_function (q0) = eps_other0
      // - eps_place0 + eps_other0 < eps
      // - other_function (q(s)) < eps
      // - object_placement (q(s)) = object_placement (q0) # thanks to the object_lock
      // So we only have:
      // - other_function (q(s)) + object_placement (q(s)) < eps + eps_place0
      // And not:
      // - other_function (q(s)) + object_placement (q(s)) < eps
      // In this case, there is no good way to recover. Just return failure.
      graph::StatePtr_t destState;
      try {
        destState = constraintGraph_->getState (q);
      } catch (const std::logic_error& e) {
        ConstraintSetPtr_t c = HPP_DYNAMIC_PTR_CAST(ConstraintSet, path->constraints());
        hppDout (error, "Edge " << c->edge()->name()
            << " generated an error: " << e.what());
        hppDout (error, "Likely, the constraints for paths are relaxed. If "
            "this problem occurs often, you may want to use the same "
            "constraints for state and paths in " << c->edge()->state()->name());
        validPart = path->extract (std::make_pair (oldTR.first,oldTR.first));
        return false;
      }
      if (!oldPath (q, oldTR.first)) {
        std::stringstream oss;
        oss << "Initial configuration of the path to be validated failed to"
          " be projected. After maximal number of iterations, q="
            << displayConfig (q) << "; error=";
        vector_t error;
        oldPath.constraints ()->isSatisfied (q, error);
        oss << displayConfig (error) << ".";
        throw std::logic_error (oss.str ().c_str ());
      }
      const graph::StatePtr_t& oldOstate = constraintGraph_->getState (q);
      if (!oldPath (q, oldTR.second)) {
        std::stringstream oss;
        oss << "End configuration of the path to be validated failed to"
          " be projected. After maximal number of iterations, q="
            << displayConfig (q) << "; error=";
        vector_t error;
        oldPath.constraints ()->isSatisfied (q, error);
        oss << displayConfig (error) << ".";
        throw std::logic_error (oss.str ().c_str ());
      }
      const graph::StatePtr_t& oldDstate = constraintGraph_->getState (q);

      if (origState == oldOstate && destState == oldDstate) {
        validPart = pathNoCollision;
        return false;
      }

      // Here, the full path is not valid. Moreover, it does not correspond to the same
      // edge of the constraint graph. Two option are possible:
      // - Use the steering method to create a new path and validate it.
      // - Return a null path.
      // validPart = path->extract (std::make_pair (oldTR.first,oldTR.first));
      validPart = pathNoCollision;
      return false;
    }
  } // namespace manipulation
} // namespace hpp
