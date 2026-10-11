#include "PhysicsCommon.h"
#include "Particle.h"

namespace Physics
{

Particle::Particle( const std::string& id ) : id( id )
{
	size = 0.01f;
	maxEnergy = 100;
	energy = maxEnergy;
	startTimer = rand() % ( maxEnergy * 10 );
	
	_transform = std::make_shared<Transform>();
	_transform->SetScale( 0.5f, 0.5f, 0.5f );
	_transform->SetInitialPosition( 0.0f, 0.5f, 5.0f );
	_particleModel = std::make_shared<ParticleModel>( _transform );
	_particleModel->SetIsParticle( true );
}

void Particle::Update( const float dt )
{
	if ( startTimer < 0 )
	{
		_particleModel->Update( dt );
		_transform->SetScale( v3df( energy * size, energy * size, energy * size ) );
	}

	if ( energy <= 0 && startTimer < 0 )
		RespawnParticle();
	
	energy -= 1; // original: int -= 0.1f, which truncates to a decrement of 1
	startTimer -= 1;
}

void Particle::RespawnParticle()
{
	energy = maxEnergy;
	_transform->SetPosition( _transform->GetInitialPosition() );
}
}
