/*
 * Copyright (C) 2012-2016 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
/*
 * Author: Jennifer Buehler
 * Date: December 2016
 */
#include <collision_benchmark/GazeboMirrorWorld.hh>
#include <collision_benchmark/GazeboStateCompare.hh>
#include <collision_benchmark/GazeboWorldState.hh>
#include <collision_benchmark/Exception.hh>

using collision_benchmark::GazeboMirrorWorld;

GazeboMirrorWorld::GazeboMirrorWorld(gazebo::physics::WorldPtr& mirrorWorld_):
  mirrorWorld(mirrorWorld_)
{
  assert(mirrorWorld);
  mirrorWorld->SetPhysicsEnabled(false);
}

GazeboMirrorWorld::~GazeboMirrorWorld()
{
}

gazebo::physics::WorldPtr GazeboMirrorWorld::GetMirrorWorld() const
{
  return mirrorWorld;
}

void GazeboMirrorWorld::NotifyOriginalWorldChange(const OriginalWorldPtr &_newWorld)
{
  GzPhysicsWorld::Ptr gzNewWorld =
    std::dynamic_pointer_cast<GzPhysicsWorld>(_newWorld);
  if (!gzNewWorld)
  {
    THROW_EXCEPTION("Only Gazebo original worlds supported");
  }
}

void GazeboMirrorWorld::Sync()
{
  GzPhysicsWorld::Ptr origWorld =
    std::dynamic_pointer_cast<GzPhysicsWorld>(GetOriginalWorld());
  // this should always cast successfully as we've checked it in NotifyOriginalWorldChange
  assert(origWorld);

/*
  XXX TODO support Pause and Step functions, though this will have to be done for all worlds,
  so probably rather in WorldManager with a common interface... not of high priority now though.
  Best is probably to re-write mirror world and receive all the Gui commands from a dedicated interface,
  instead of having the whole cloned world.
  GazeboPhysicsWorld::Ptr gzOrigWorld = std::dynamic_pointer_cast<GazeboPhysicsWorld>(origWorld);
  bool isPaused=false;
  if (gzOrigWorld)
  {
    // set mirror world to paused if the original world is paused too. Only supported for other gazebo worlds.
    bool isPaused = gzOrigWorld->GetWorld()->IsPaused();
    std::cout<<"Paused state: "<<gzOrigWorld->GetWorld()->IsPaused()<<std::endl;;
    mirrorWorld->SetPaused(isPaused);
  }
  if (isPaused) return;*/

  gazebo::physics::WorldState origState = origWorld->GetWorldState();
  collision_benchmark::SetWorldState(mirrorWorld, origState);
#ifdef DEBUG
  gazebo::physics::WorldState _currentState(mirrorWorld);
  GazeboStateCompare::Tolerances t=GazeboStateCompare::Tolerances::CreateDefault(1e-03);
  t.CheckDynamics=false; // don't check dynamics because the physics engine of the mirror world is disabled
  if (!GazeboStateCompare::Equal(_currentState, origState, t))
  {
    std::cerr<<"Target state was not set as supposed to!!"<<std::endl;
  }
#endif
  // update the mirror world
  gazebo::runWorld(mirrorWorld, 1);
}