#pragma once
#ifndef PHYSICS_TRANSFORM_H
#define PHYSICS_TRANSFORM_H

#include "PhysicsCommon.h"

namespace Physics
{
	// Minimal position/scale/rotation holder the physics bodies read from and
	// write to. Copy its values into your own scene object each frame after
	// stepping the simulation.
	class Transform
	{
	public:
		Transform() :
			_scale( 1.0f, 1.0f, 1.0f ),
			_rotation( 0.0f, 0.0f, 0.0f ),
			_position( 0.0f, 0.0f, 0.0f ),
			_initialPosition( 0.0f, 0.0f, 0.0f )
		{}

		void SetInitialPosition( v3df initialPosition ) { _initialPosition = initialPosition; _position = _initialPosition; }
		void SetInitialPosition( float x, float y, float z ) { _initialPosition = { x, y, z }; _position = _initialPosition; }
		v3df GetInitialPosition() const noexcept { return _initialPosition; }

		void SetPosition( v3df position ) { _position = position; }
		void SetPosition( float x, float y, float z ) { _position = { x, y, z }; }
		v3df GetPosition() const noexcept { return _position; }
		void ResetPosition() { _position = _initialPosition; }

		void SetScale( const v3df& scale ) { _scale = scale; }
		void SetScale( float x, float y, float z ) { _scale = { x, y, z }; }
		v3df GetScale() const noexcept { return _scale; }

		void SetRotation( v3df rotation ) { _rotation = rotation; }
		void SetRotation( float x, float y, float z ) { _rotation = { x, y, z }; }
		v3df GetRotation() const noexcept { return _rotation; }
	private:
		v3df _scale;
		v3df _rotation;
		v3df _position;
		v3df _initialPosition;
	};
}

#endif
