#pragma once
#ifndef PHYSICSFRAMEWORK_H
#define PHYSICSFRAMEWORK_H

// Umbrella header for the Physics module (ported from the "physics-framework" repo).
//
// Everything lives in namespace Physics and depends only on the C++ standard
// library and DirectXMath - no other part of the engine is needed.
//
// Add to the project (all paths relative to Mark_1_DX3D\Engine\Physics):
//   ParticleModel.cpp  Particle.cpp  RigidBody.cpp
//   Collisions\CollisionDetector.cpp  Collisions\Contact.cpp
//   Collisions\ContactResolver.cpp    Collisions\IntersectionTests.cpp
// and include this header:  #include "Physics/PhysicsFramework.h"
//
// Typical rigid body use:
//   auto transform = std::make_shared<Physics::Transform>();
//   auto body      = std::make_shared<Physics::RigidBody>( transform );
//   body->SetPosition( 0.0f, 5.0f, 0.0f );
//   body->AddForce( Physics::v3df( 0.0f, -9.81f * body->GetMass(), 0.0f ) );
//   body->Update( dt );                       // integrate one step
//   Physics::v3df p = body->GetPosition();    // feed this to your renderer
//
// Collisions: fill a CollisionData with a Contact array, call the
// CollisionDetector::* functions for each pair of CollisionSphere/CollisionCube/
// CollisionPlane (set _body and call CalculateInternals() first), then pass the
// contacts to ContactResolver::ResolveContacts( contacts, count, dt ).

#include "Math/Vector.h"
#include "Math/Quaternion.h"
#include "Math/Matrix3.h"
#include "Math/Matrix4.h"
#include "Transform.h"
#include "ParticleModel.h"
#include "RigidBody.h"
#include "Particle.h"
#include "Collisions/CollisionPrimitives.h"
#include "Collisions/CollisionData.h"
#include "Collisions/Contact.h"
#include "Collisions/ContactResolver.h"
#include "Collisions/CollisionDetector.h"
#include "Collisions/IntersectionTests.h"

#endif
