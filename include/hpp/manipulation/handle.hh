///
/// Copyright (c) 2014 CNRS
/// Authors: Florent Lamiraux
///
///
// This file is part of hpp-manipulation.
// hpp-manipulation is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// hpp-manipulation is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Lesser Public License for more details. You should have
// received a copy of the GNU Lesser General Public License along with
// hpp-manipulation. If not, see
// <http://www.gnu.org/licenses/>.

#ifndef HPP_MANIPULATION_HANDLE_HH
# define HPP_MANIPULATION_HANDLE_HH

# include <fcl/math/transform.h>
# include <hpp/manipulation/config.hh>
# include <hpp/manipulation/fwd.hh>

namespace hpp {
  namespace manipulation {
    /// Part of an object that is aimed at being grasped
    class Handle
    {
    public:
      /// Create constraint corresponding to a gripper grasping this object
      /// \param robot the robot that grasps the handle,
      /// \param grasp object containing the grasp information
      /// \return the constraint of relative position between the handle and
      ///         the gripper.
      static HandlePtr_t create (const std::string& name,
				 const Transform3f& localPosition,
				 const JointPtr_t& joint)
      {
	Handle* ptr = new Handle (name, localPosition, joint);
	HandlePtr_t shPtr (ptr);
	ptr->init (shPtr);
	return shPtr;
      }
      /// Return a pointer to the copy of this
      virtual HandlePtr_t clone () const;

      /// \name Name
      /// \{

      /// Get name
      const std::string& name () const
      {
	return name_;
      }
      /// Set name
      void name (const std::string& n)
      {
	name_ = n;
      }
      /// \}

      /// \name Joint
      /// \{

      /// Get joint to which the handle is linked
      const JointPtr_t& joint () const
      {
	return joint_;
      }
      /// Set joint to which the handle is linked
      void joint (const JointPtr_t& joint)
      {
	joint_ = joint;
      }
      /// \}

      /// Get local position in joint frame
      const Transform3f& localPosition () const
      {
	return localPosition_;
      }

      /// Create constraint corresponding to a gripper grasping this object
      /// \param gripper object containing the gripper information
      /// \return the constraint of relative transformation between the handle and
      ///         the gripper.
      /// \note The 6 DOFs of the relative transformation are constrained.
      virtual DifferentiableFunctionPtr_t createGrasp
      (const GripperPtr_t& gripper) const;

      /// Create constraint corresponding to a gripper grasping this object
      /// \param gripper object containing the gripper information
      /// \return the constraint of relative position between the handle and
      ///         the gripper.
      /// \note Only 5 DOFs of the relative transformation between the handle and the gripper
      ///       are constrained. The translation along x-axis is not constrained.
      virtual DifferentiableFunctionPtr_t createPreGrasp
      (const GripperPtr_t& gripper) const;

      /// Create constraint that acts on the non-constrained axis of the
      /// constraint generated by Handle::createPreGrasp.
      /// \param gripper object containing the gripper information
      /// \param shift the target value along the x-axis
      /// \return the constraint of relative position between the handle and
      ///         the gripper.
      /// \note Only the x-axis of the relative transformation between the handle and the gripper
      ///       is constrained.
      virtual DifferentiableFunctionPtr_t createPreGraspComplement
        (const GripperPtr_t& gripper, const value_type& shift) const;

      static DifferentiableFunctionPtr_t createGrasp
      (const GripperPtr_t& gripper,const HandlePtr_t& handle)
      {
        return handle->createGrasp(gripper);
      }

    protected:
      /// Constructor
      /// \param robot the robot that grasps the handle,
      /// \param grasp object containing the grasp information
      /// \return the constraint of relative position between the handle and
      ///         the gripper.
      Handle (const std::string& name, const Transform3f& localPosition,
	      const JointPtr_t& joint) : name_ (name),
					 localPosition_ (localPosition),
					 joint_ (joint), weakPtr_ ()
      {
      }
      void init (HandleWkPtr_t weakPtr)
      {
	weakPtr_ = weakPtr;
      }

      virtual std::ostream& print (std::ostream& os) const;

    private:
      std::string name_;
      /// Position of the handle in the joint frame.
      Transform3f localPosition_;
      /// Joint to which the handle is linked.
      JointPtr_t joint_;
      /// Weak pointer to itself
      HandleWkPtr_t weakPtr_;

      friend std::ostream& operator<< (std::ostream&, const Handle&);
    }; // class Handle

    std::ostream& operator<< (std::ostream& os, const Handle& handle);
  } // namespace manipulation
} // namespace hpp

#endif // HPP_MANIPULATION_HANDLE_HH
