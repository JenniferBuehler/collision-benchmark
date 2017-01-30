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


#include <collision_benchmark/GazeboWorldLoader.hh>
#include <collision_benchmark/GazeboWorldState.hh>
#include <collision_benchmark/GazeboHelpers.hh>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

bool collision_benchmark::WaitForNamespace(std::string worldNamespace, float maxWaitTime, float sleepTime)
{
  gazebo::transport::TopicManager * topicManager = gazebo::transport::TopicManager::Instance();
  if (!topicManager)
  {
    std::cerr << "No topic manager instance" << std::endl;
    return false;
  }

  gazebo::common::Timer timer;
  timer.Start();

  std::cout << "Waiting for namespace '" << worldNamespace << " 'to be loaded." << std::endl;

  bool found = false;
  while (!found && (timer.GetElapsed().Float() < maxWaitTime))
  {
    // found at most 1 second for namespaces to appear.
    if (gazebo::transport::waitForNamespaces(gazebo::common::Time(sleepTime, 0)))
    {
      std::list<std::string> namespaces;
      topicManager->GetTopicNamespaces(namespaces);

      /*std::cout<<"Namespaces:"<<std::endl;
      for (std::list<std::string>::iterator it=namespaces.begin(); it!=namespaces.end(); ++it)
        std::cout<<*it<<std::endl;*/

      std::list<std::string>::iterator ns = std::find(namespaces.begin(), namespaces.end(), worldNamespace);
      bool loaded = (ns != namespaces.end()) && (*ns == worldNamespace);
      if (loaded)
      {
        std::cout << "Namespace '" << worldNamespace << "' received." << std::endl;
        found = true;
      }
      /*else
      {
        std::cout << "Namespace '" << worldNamespace << "' not received yet." << std::endl;
      }*/
    }
    /*else
    {
      std::cout << "No namespaces received yet.\n";
    }*/
  }
  if (!found)
    std::cerr<<"Unsuccessful wait for namespace "<<worldNamespace<<"."<<std::endl;

  return found;
}

sdf::ElementPtr collision_benchmark::GetSDFElementFromFile(const std::string& filename,
                                                           const std::string& elemName,
                                                           const std::string& name)
{
  sdf::ElementPtr sdfRoot;

  // Load the world file
  sdf::SDFPtr sdf(new sdf::SDF);
  if (!sdf::init(sdf))
  {
    std::cerr << "Unable to initialize sdf\n";
    return sdfRoot;
  }

  // Find the file.
  std::string fullFile;
  try
  {
   fullFile = gazebo::common::find_file(filename);
  }
  catch (gazebo::common::Exception& e)
  {
    std::cerr << "File "<<filename<<" not found. " << e.GetErrorStr() << std::endl;
    return sdf::ElementPtr();
  }


  if (fullFile.empty())
  {
    std::cerr << "Unable to find file[" << filename << "]\n";
    return sdfRoot;
  }

  std::cout << "File "<<fullFile<<" (from "<<filename<<") found. " << std::endl;

  try
  {
    if (!sdf::readFile(fullFile, sdf))
    {
      std::cerr << "Unable to read sdf file[" << filename << "]\n";
      return sdfRoot;
    }
  }
  catch (...)
  {
    std::cerr << "Unable to read sdf file[" << filename << "]\n";
    return sdfRoot;
  }

  sdfRoot = sdf->Root()->GetElement(elemName);

  if (!name.empty())
  {
    sdf::ParamPtr sdfElemName = sdfRoot->GetAttribute("name");
    // std::cout << "Replacing SDF name: '" << sdfElemName->GetAsString() << "' with '" << name << "'" << std::endl;
    sdfElemName->SetFromString(name);
  }

  return sdfRoot;
}

sdf::ElementPtr collision_benchmark::GetSDFElementFromString(const std::string& xmlString,
                                                             const std::string& elemName,
                                                             const std::string& name)
{
  sdf::ElementPtr sdfRoot;

  // Load the world file
  sdf::SDFPtr sdf(new sdf::SDF);
  if (!sdf::init(sdf))
  {
    std::cerr << "Unable to initialize sdf\n";
    return sdfRoot;
  }

  if (!sdf::readString(xmlString, sdf))
  {
    std::cerr << "Unable to read sdf string: " << std::endl << xmlString << std::endl;
    return sdfRoot;
  }

  sdfRoot = sdf->Root()->GetElement(elemName);

  if (!name.empty())
  {
    sdf::ParamPtr sdfElemName = sdfRoot->GetAttribute("name");
    std::cout << "Replacing world name: '" << sdfElemName->GetAsString() << "' with '" << name << "'" << std::endl;
    sdfElemName->SetFromString(name);
  }

  return sdfRoot;
}

sdf::ElementPtr collision_benchmark::GetPhysicsFromSDF(const std::string& filename)
{
  sdf::ElementPtr sdfRoot;

  // Load the world file
  sdf::SDFPtr sdf(new sdf::SDF);
  if (!sdf::init(sdf))
  {
    std::cerr << "Unable to initialize sdf\n";
    return sdfRoot;
  }

  // Find the file.
  std::string fullFile = gazebo::common::find_file(filename);

  if (fullFile.empty())
  {
    std::cerr << "Unable to find file[" << filename << "]\n";
    return sdfRoot;
  }

  if (!sdf::readFile(fullFile, sdf))
  {
    std::cerr << "Unable to read sdf file[" << filename << "]\n";
    return sdfRoot;
  }

  sdfRoot = sdf->Root()->GetElement("world");
  if (!sdfRoot)
  {
      std::cerr<<"No <world> tag exits in SDF "<<filename<<std::endl;
      return sdfRoot;
  }

  sdfRoot = sdfRoot->GetElement("physics");

  if (!sdfRoot)
    std::cerr<<"No <physics> tag under <world>"<<std::endl;

  return sdfRoot;
}

gazebo::physics::WorldPtr collision_benchmark::LoadWorldFromSDF(const sdf::ElementPtr& sdfRoot, const std::string& name)
{
  if (sdfRoot->GetName() != "world")
  {
    std::cerr<<"SDF must be a 'world' element"<<std::endl;
    std::cerr<<sdfRoot->ToString("")<<std::endl;
    return gazebo::physics::WorldPtr();
  }

  gazebo::physics::WorldPtr world;
  try
  {
    // XXX this just creates a new world object, all worls objects are
    // kept in a static list and can be retrieved. No Physics engine
    // is created yet - this is done in World::Load (line 257), called
    // from load_world: The physics engine specified **in the SDF** is
    // loaded.
    world = gazebo::physics::create_world(name);

    if (!name.empty())
    {
      sdf::ParamPtr sdfWorldName = sdfRoot->GetAttribute("name");
      if (sdfWorldName->GetAsString() != name)
      {
        std::cout << "INFO: Need to replace world name in SDF: '" << sdfWorldName->GetAsString() << "' with '" << name << "'" << std::endl;
        sdfWorldName->SetFromString(name);
      }
    }
    std::cout<<"Loading world..."<<std::endl;
    if (world) gazebo::physics::load_world(world, sdfRoot);

    std::cout<<"Initializing world..."<<std::endl;
    // call to world->init
    if (world) gazebo::physics::init_world(world);
  }
  catch (gazebo::common::Exception& e)
  {
    std::cerr << " Exception ocurred when loading world. " << e.GetErrorStr() << std::endl;
    return gazebo::physics::WorldPtr();
  }
  assert(world);
  std::cout<<"World loaded."<<std::endl;

  return world;
}

gazebo::physics::ModelPtr collision_benchmark::LoadModelFromSDF(const sdf::ElementPtr& sdfRoot,
                                                                const gazebo::physics::WorldPtr& world,
                                                                const std::string& name)
{
  GZ_ASSERT(world, "Can't load a model without a world");

  if (sdfRoot->GetName() != "model")
  {
    std::cerr<<"SDF must be a 'model' element"<<std::endl;
    std::cerr<<sdfRoot->ToString("")<<std::endl;
    return gazebo::physics::ModelPtr();
  }

  sdf::ParamPtr sdfModelName = sdfRoot->GetAttribute("name");
  std::string modelName = sdfModelName->GetAsString();

  // first, fix up the name in the SDF
  if (!name.empty())
  {
    if (modelName != name)
    {
      std::cout << "INFO: Replacing name in SDF: '" << sdfModelName->GetAsString() << "' with '" << name << "'" << std::endl;
      sdfModelName->SetFromString(name);
      modelName = name;
    }
  }

  // load the model
  bool pausedState=world->IsPaused();
  world->SetPaused(true);

  // The current interface of the world only allows addition of models via SetState() in the insertions.
  gazebo::physics::WorldState wstate(world); // get current state as we need the time values
  gazebo::physics::WorldState tempState;
  tempState.SetSimTime(wstate.GetSimTime());
  tempState.SetRealTime(wstate.GetRealTime());
  tempState.SetWallTime(wstate.GetWallTime());
  tempState.SetIterations(wstate.GetIterations());

  std::string ins=sdfRoot->ToString("");
  // the SDF has to be fixed up, otherwise gazebo::physics::World::SetState is not successful
  collision_benchmark::wrapSDF(ins);

  std::vector<std::string> insertions;
  insertions.push_back(ins);
  tempState.SetInsertions(insertions);

  world->SetState(tempState);

  gazebo::physics::ModelPtr m = world->ModelByName(modelName);
  if (!m)
  {
    std::cerr<<"Could not load model "<<modelName<<std::endl;
  }

  world->SetPaused(pausedState);

  return m;
}

gazebo::physics::ModelPtr collision_benchmark::LoadModelFromSDFString(const std::string& sdfString,
                                                                      const gazebo::physics::WorldPtr& world,
                                                                      const std::string& name)
{
  GZ_ASSERT(world, "Can't load a model without a world");

  sdf::ElementPtr sdfRoot = collision_benchmark::GetSDFElementFromString(sdfString, "model", name);
  if (!sdfRoot)
  {
    std::cerr<<"SDF has no tag named 'model' in the root"<<std::endl;
  }
  return LoadModelFromSDF(sdfRoot, world, name);
}

// Helper function which interprets the string \e str as file if \e isFile is true,
// and as xml string  otherwise
gazebo::physics::WorldPtr LoadWorld_helper(const std::string &str,
                                                   const bool isFile,
                                                   const std::string& name,
                                                   const sdf::ElementPtr& overridePhysics)
{
  gazebo::physics::WorldPtr world;
  sdf::ElementPtr sdfRoot;
  try
  {
    if (isFile) sdfRoot = collision_benchmark::GetSDFElementFromFile(str, "world", name);
    else sdfRoot = collision_benchmark::GetSDFElementFromString(str, "world", name);
  }
  catch (gazebo::common::Exception& e)
  {
    std::cerr << " Exception ocurred when loading world. " << e.GetErrorStr() << std::endl;
    return gazebo::physics::WorldPtr();
  }
  if (!sdfRoot)
  {
    std::cerr << "Could not load world" << std::endl;
    return world;
  }
  if (overridePhysics)
  {
    if (!sdfRoot->HasElement("physics"))
    {
#ifdef DEBUG
      std::cout<<"World in did not have physics, so adding the override physics: "
        <<std::endl<<overridePhysics->ToString("")<<std::endl;
#endif
      sdf::ElementPtr physics = overridePhysics->Clone();
      physics->SetParent(sdfRoot);
      sdfRoot->InsertElement(physics);
    }
    else
    {
      sdf::ElementPtr sdfPhysics = sdfRoot->GetElement("physics");
      // std::cout<<"World in "<<worldfile<<" has physics: "<<std::endl<<
      //      sdfPhysics->ToString("")<<"overriding with: "<<std::endl<<overridePhysics->ToString("")<<std::endl;
      sdfPhysics->Copy(overridePhysics);
    }
  }
  return collision_benchmark::LoadWorldFromSDF(sdfRoot, name);
}

gazebo::physics::WorldPtr collision_benchmark::LoadWorldFromFile(const std::string &worldfile,
                                                                 const std::string& name,
                                                                 const sdf::ElementPtr& overridePhysics)
{
  std::cout << "Loading world from file " << worldfile;
  if (!name.empty()) std::cout << " (to be named '" << name << "')";
  std::cout << std::endl;
  return LoadWorld_helper(worldfile, true, name, overridePhysics);
}

gazebo::physics::WorldPtr collision_benchmark::LoadWorldFromSDFString(const std::string &xmlStr,
                                                                   const std::string& name,
                                                                   const sdf::ElementPtr& overridePhysics)
{
  std::cout << "Loading world XML string. ";
  if (!name.empty()) std::cout << " (to be named '" << name << "')";
  std::cout << std::endl;
  return LoadWorld_helper(xmlStr, false, name, overridePhysics);
}


gazebo::physics::WorldPtr collision_benchmark::LoadWorld(const std::string& worldfile,
                                                         const std::string& name,
                                                         const sdf::ElementPtr& overridePhysics)
{
  gazebo::physics::WorldPtr world = LoadWorldFromFile(worldfile, name, overridePhysics);
  if (!world)
  {
    std::cerr << "Could not load world." << std::endl;
    return gazebo::physics::WorldPtr();
  }

  std::string worldNamespace = world->Name();

  // wait for namespace to be loaded, to make sure the order of namespaces maintained
  // in the transport system eventually will correspond to the same order of the worlds
  if (!WaitForNamespace(worldNamespace))
  {
    std::cerr << "Namespace of world '" << worldNamespace << "' was not loaded" << std::endl;
    return gazebo::physics::WorldPtr();
  }
  return world;
}


std::vector<gazebo::physics::WorldPtr> collision_benchmark::LoadWorlds(const std::vector<Worldfile>& worldfiles)
{
  std::vector<gazebo::physics::WorldPtr> worlds;
  // -- load all worlds --
  for (std::vector<Worldfile>::const_iterator w = worldfiles.begin();
      w != worldfiles.end(); ++w)
  {
    std::cout << "Loading world " << w->filename << " (named as '" << w->worldname << "')" << std::endl;
    gazebo::physics::WorldPtr world = LoadWorld(w->filename, w->worldname);
    if (!world)
    {
      std::cerr << "Could not load world " << w->filename << std::endl;
      worlds.clear();
      return worlds;
    }
    worlds.push_back(world);
  }
  return worlds;
}