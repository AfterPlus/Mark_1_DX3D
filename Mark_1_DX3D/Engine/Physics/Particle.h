#pragma once
#ifndef PARTICLE_H
#define PARTICLE_H

#include "Transform.h"
#include "ParticleModel.h"

namespace Physics
{
	// Simulation-only particle. The original framework also owned a render
	// Appearance and a Draw() method; rendering is left to the host application,
	// which can read GetTransform()->GetPosition()/GetScale() each frame.
	class Particle
	{
	public:
		Particle( const std::string& id );
		void Update( const float dt );
		void RespawnParticle();

		float GetMaxSize() const noexcept { return size; };
		void SetMaxSize( float newSize ) { size = newSize; };

		int GetStartTimer() const noexcept { return startTimer; };
		// true once the start delay has elapsed and the particle should be drawn
		bool IsVisible() const noexcept { return startTimer < -1; }
		const std::string& GetID() const noexcept { return id; };

		std::shared_ptr<Transform> GetTransform() const noexcept { return _transform; };
		std::shared_ptr<ParticleModel> GetParticleModel() const noexcept { return _particleModel; };
	private:
		// Local Variables
		float size;
		int energy;
		int maxEnergy;
		int startTimer;
		std::string id;

		// Class References
		std::shared_ptr<Transform> _transform;
		std::shared_ptr<ParticleModel> _particleModel;
	};
}

#endif
