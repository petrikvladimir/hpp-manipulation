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

#ifndef HPP_MANIPULATION_GRAPH_GRAPH_HH
# define HPP_MANIPULATION_GRAPH_GRAPH_HH

# include "hpp/manipulation/config.hh"
# include "hpp/manipulation/fwd.hh"
# include "hpp/manipulation/graph/fwd.hh"
# include "hpp/manipulation/graph/graph-component.hh"

namespace hpp {
  namespace manipulation {
    namespace graph {
      /// Description of the constraint graph
      /// This class contains a graph representing a robot with several
      /// end-effectors.
      /// 
      /// One must make sure not to create loop with shared pointers.
      /// To ensure that, the classes are defined as follow:
      /// - A Graph owns (i.e. has a shared pointer to) the NodeSelector s
      /// - A NodeSelector owns the Node s related to one gripper.
      /// - A Node owns its outgoing Edge s.
      /// - An Edge does not own anything.
      class HPP_MANIPULATION_DLLAPI Graph : public GraphComponent
      {
        public:
          /// Create a new Graph.
          static GraphPtr_t create(RobotPtr_t robot);

          /// Create and insert a NodeSelector inside the graph.
          NodeSelectorPtr_t createNodeSelector();

          /// Returns the states of a configuration.
          virtual Nodes_t getNode(const Configuration_t config);

          /// Select randomly outgoing edges of the given nodes.
          virtual Edges_t chooseEdge(const Nodes_t& node);

          /// Constraint to project onto the Nodes_t.
          /// \param the Nodes_t on which to project.
          /// \return The initialized projector.
          virtual ConstraintPtr_t configConstraint (const Nodes_t& nodes);

          /// Constraint to project onto the same leaf as config.
          /// \param edges a list of edges defining the foliation.
          /// \param config Configuration that will initialize the projector.
          /// \return The initialized projector.
          virtual ConstraintPtr_t configConstraint (const Edges_t& edges, ConfigurationIn_t config);

          /// Constraint to project a path.
          /// \param edges a list of edges defining the foliation.
          /// \param config Configuration that will initialize the constraint.
          /// \return The initialized constraint.
          virtual ConstraintPtr_t pathConstraint (const Edges_t& edges, ConfigurationIn_t config);

          /// Return the NodeSelector with the given name if any,
          /// NULL pointer if not found.
          NodeSelectorPtr_t getNodeSelectorByName (const std::string& name);

          /// Print the object in a stream.
          std::ostream& print (std::ostream& os) const;

          /// Set maximal number of iterations
          void maxIterations (size_type iterations);

          /// Get maximal number of iterations in config projector
          size_type maxIterations () const;

          /// Set error threshold
          void errorThreshold (const value_type& threshold);

          /// Get errorimal number of threshold in config projector
          value_type errorThreshold () const;

          /// Get the robot.
          const RobotPtr_t& robot () const;

        protected:
          /// Initialization of the object.
          void init (const GraphWkPtr_t& weak, RobotPtr_t robot);

          /// Constructor
          Graph ()
          {}

        private:
          /// This list contains a node selector for each end-effector.
          NodeSelectors_t nodeSelectors_;

          /// A set of constraints that will always be used, for example
          /// stability constraints.
          ConstraintPtr_t constraints_;

          /// Keep a pointer to the composite robot.
          RobotPtr_t robot_;

          /// Weak pointer to itself.
          GraphWkPtr_t wkPtr_;

          value_type errorThreshold_;
          size_type maxIterations_;
      }; // Class Graph
    } // namespace graph
  } // namespace manipulation

} // namespace hpp

#endif // HPP_MANIPULATION_GRAPH_GRAPH_HH