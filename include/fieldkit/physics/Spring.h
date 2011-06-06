/*                                                                           
 *      _____  __  _____  __     ____                                   
 *     / ___/ / / /____/ / /    /    \   FieldKit
 *    / ___/ /_/ /____/ / /__  /  /  /   (c) 2010, FIELD. All rights reserved.              
 *   /_/        /____/ /____/ /_____/    http://www.field.io           
 *   
 *	 Created by Marcus Wendt on 27/05/2010.
 */

#pragma once

#include "fieldkit/physics/Particle.h"

namespace fieldkit { namespace physics {

	class Spring {
	public:
		Particle* a;
		Particle* b;
		
		// Flag to allow us to de-activate springs after their creation
		bool isAlive;

		// Spring rest length to which it always wants to return too
		float restLength;
		
		// Spring strength, possible value range depends on engine configuration
		float strength;
		
		// Flag, if either particle is locked in space
		bool isALocked;
		bool isBLocked;

		// 
		int id;
		
		Spring();
		Spring(Particle* a, Particle* b, float restLength, float strength);
		virtual ~Spring() {};
		
		virtual void init(Particle* a, Particle* b, float restLength, float strength);
		virtual void update();
		
		virtual void reset();
	};

} } // namespace fieldkit::physics