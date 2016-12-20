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

#include <collision_benchmark/WorldLoader.hh>
#include <collision_benchmark/GazeboWorldState.hh>
#include <collision_benchmark/GazeboMirrorWorld.hh>
#include <collision_benchmark/GazeboPhysicsWorld.hh>
#include <collision_benchmark/boost_std_conversion.hh>

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

using collision_benchmark::GazeboMirrorWorld;
using collision_benchmark::GazeboPhysicsWorld;

/**
 * Convenience function to call gazebo::runWorld() on several worlds \e iter times
 * \param iter number of iterations (world updates)
 * \param worlds all worlds that have to be updated
 * \param mirrorWorld optional: if not NULL, the method GazeboMirrorWorld::Sync() is called at each iteration
 */
void RunWorlds(int iter, const std::vector<gazebo::physics::WorldPtr>& worlds, const GazeboMirrorWorld::Ptr& mirrorWorld=GazeboMirrorWorld::Ptr(NULL))
{
  for (unsigned int i = 0; i < iter; ++i)
  {
    static const int steps = 1;
    // std::cout << "##### Running world(s), iter=" << i << std::endl;
    for (std::vector<gazebo::physics::WorldPtr>::const_iterator w = worlds.begin();
        w != worlds.end(); ++w)
    {
      gazebo::physics::WorldPtr world = *w;
      std::cout<<"Running "<<steps<<" steps for world "<<world->Name()<<", physics engine: "<<world->Physics()->GetType()<<std::endl;
      // Run simulation for given number of steps.
      // This method calls world->RunBlocking();
      gazebo::runWorld(world, steps);
    }

    if (mirrorWorld)
    {
      mirrorWorld->Sync();
      // std::cout<<"Running mirror world. Paused? "<<mirrorWorld->GetMirrorWorld()->IsPaused()<<std::endl;
      gazebo::runWorld(mirrorWorld->GetMirrorWorld(), steps);
    }
  }
}

// loads the mirror world. This should be loaded before all other Gazebo worlds, so that
// gzclient connects to this one.
GazeboMirrorWorld::Ptr setupMirrorWorld()
{
    std::cout << "Setting up mirror world..." << std::endl;
    std::string mirrorName = "mirror_world";
    gazebo::physics::WorldPtr _mirrorWorld = collision_benchmark::LoadWorld("worlds/empty.world", mirrorName);
    if (!_mirrorWorld)
    {
        std::cerr<<"Could not load mirror world"<<mirrorName<<std::endl;
        return GazeboMirrorWorld::Ptr();
    }

    std::cout<<"Creating mirror world object."<<std::endl;
    GazeboMirrorWorld::Ptr mirrorWorld(new GazeboMirrorWorld(_mirrorWorld));
    return mirrorWorld;
}


// Main method to play the test, later to be replaced by a dedicated structure (without command line arument params)
bool PlayTest(int argc, char **argv)
{
  if (argc < 3)
  {
    std::cerr<<"Usage: "<<argv[0]<<" <number iterations> <list of world filenames>"<<std::endl;
    return false;
  }

  // list of worlds to be loaded
  std::vector<collision_benchmark::Worldfile> worldsToLoad;

  int numIters = atoi(argv[1]);

  for (int i = 2; i < argc; ++i)
  {
    std::string worldfile = std::string(argv[i]);
    std::stringstream worldname;
    worldname << "world_" << i - 1;
    worldsToLoad.push_back(collision_benchmark::Worldfile(worldfile,worldname.str()));
  }

  // first, load the mirror world (it has to be loaded first for gzclient to connect to it)
  GazeboMirrorWorld::Ptr mirrorWorld = setupMirrorWorld();
  if(!mirrorWorld)
  {
    std::cerr<<"Could not load mirror world."<<std::endl;
    return false;
  }

  std::cout << "Loading worlds..." << std::endl;
  std::vector<gazebo::physics::WorldPtr> gzworlds = collision_benchmark::LoadWorlds(worldsToLoad);
  if (gzworlds.size()!=worldsToLoad.size())
  {
    std::cerr << "Could not load worlds." << std::endl;
    return false;
  }

  // the worlds wrapped in the PhysicsWorld interface. Same size as gzworlds.
  // XXX TODO Eventually gzworlds is going to become obsolete and exchanged by objects of PhysicsWorld,
  // it is only used still in this test program.
  std::vector<GazeboPhysicsWorld::Ptr> worlds;
  for (int i = 0; i < gzworlds.size(); ++i)
  {
    GazeboPhysicsWorld::Ptr gzPhysicsWorld(new GazeboPhysicsWorld());
    gzPhysicsWorld->SetWorld(collision_benchmark::to_std_ptr<gazebo::physics::World>(gzworlds[i]));
    worlds.push_back(gzPhysicsWorld);
  }
  assert(worlds.size() == gzworlds.size());

  // Go through all worlds and print info
  bool printState = false;
  if (printState)
  {
    std::cout << "##### States of all worlds before running:" << std::endl;
    collision_benchmark::PrintWorldStates(gzworlds);
  }

  std::cout << "Now start gzclient if you would like to view the test. Press any key to continue."<<std::endl;
  getchar();

  // Go through all worlds, mirroring each for the given number of iterations.
  for (int i = 0; i < worlds.size(); ++i)
  {
      std::cout << "+++++++++++++++++++++++++++++++++++++++++" << std::endl;
      std::cout << "Now mirroring " << gzworlds[i]->Name() << std::endl;
      std::cout << "+++++++++++++++++++++++++++++++++++++++++" << std::endl;
      mirrorWorld->SetOriginalWorld(worlds[i]);
      RunWorlds(numIters, gzworlds, mirrorWorld);
  }

  if (printState)
  {
    std::cout << "##### States of all worlds after running:" << std::endl;
    collision_benchmark::PrintWorldStates(gzworlds);
  }

  return true;
}


/////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Initialize gazebo.
  gazebo::setupServer(argc, argv);

  PlayTest(argc, argv);

  gazebo::shutdown();

  std::cout << "Test ended." << std::endl;
}
