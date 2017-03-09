#include <collision_benchmark/WorldManager.hh>
#include <collision_benchmark/PrimitiveShape.hh>
#include <collision_benchmark/SimpleTriMeshShape.hh>
#include <collision_benchmark/BasicTypes.hh>
#include <gazebo/gazebo.hh>

#include "MultipleWorldsTestFramework.hh"

using collision_benchmark::Shape;
using collision_benchmark::PrimitiveShape;
using collision_benchmark::SimpleTriMeshShape;
using collision_benchmark::BasicState;
using collision_benchmark::Vector3;
using collision_benchmark::Quaternion;

class StaticTest : public MultipleWorldsTestFramework {};

// Loads a world with different physics engines (only the ones implemented
// in Gazebo) and does the static test in which two models are generated at
// various poses and all engines have to agree on the
// collision state (boolean collision).
TEST_F(StaticTest, TwoSpheres)
{
  std::vector<std::string> selectedEngines;
  selectedEngines.push_back("bullet");
  selectedEngines.push_back("ode");
  selectedEngines.push_back("dart");

  /*std::set<std::string> engines =
    collision_benchmark::GetSupportedPhysicsEngines();
  // run test on all engines
  selectedEngines.insert(selectedEngines.end(), engines.begin(), engines.end());*/

  // world to load
  std::string worldfile = "worlds/empty.world";

  GzMultipleWorldsServer::Ptr mServer = GetServer();
  ASSERT_NE(mServer.get(), nullptr) << "Could not create and start server";

  bool loadMirror = true;
  std::string mirrorName = "";
  if (loadMirror) mirrorName = "mirror";
  // with the tests, the mirror can be used to watch the test,
  // but not to manipulate the worlds.
  bool allowControlViaMirror = false;
  int numWorlds = mServer->Load(worldfile, selectedEngines,
                               mirrorName, allowControlViaMirror);

  GzWorldManager::Ptr worldManager = mServer->GetWorldManager();
  assert(worldManager);

  ASSERT_EQ(numWorlds, worldManager->GetNumWorlds())
    << "Inconsistency: Server returned different number of worlds";

/*  GzWorldManager::ControlServerPtr controlServer =
    worldManager->GetControlServer();

  if (controlServer)
  {
    controlServer->RegisterPauseCallback(std::bind(pauseCallback,
                                                   std::placeholders::_1));
  }*/

  //Shape::Ptr shape(PrimitiveShape::CreateSphere(2));

  // create simple mesh for testing
/*  SimpleTriMeshShape::MeshDataPtr meshData(new SimpleTriMeshShape::MeshDataT());
  typedef SimpleTriMeshShape::Vertex Vertex;
  typedef SimpleTriMeshShape::Face Face;
  std::vector<Vertex>& vertices=meshData->GetVertices();
  std::vector<Face>& triangles=meshData->GetFaces();
  vertices.push_back(Vertex(-1,0,0));
  vertices.push_back(Vertex(0,0,-1));
  vertices.push_back(Vertex(1,0,0));
  vertices.push_back(Vertex(0,1,0));
  triangles.push_back(Face(0,1,2));
  triangles.push_back(Face(0,2,3));
  Shape::Ptr shape(new SimpleTriMeshShape(meshData, "test_mesh"));*/

  // shape->SetPose(Shape::Pose3(2,2,2,0,0,0));

  // Load model 1
  std::string modelName1 = "model1";
  Shape::Ptr shape1(PrimitiveShape::CreateBox(2,2,2));
  typedef GzWorldManager::ModelLoadResult ModelLoadResult;
  std::vector<ModelLoadResult> res1
    = worldManager->AddModelFromShape(modelName1, shape1, shape1);
  ASSERT_EQ(res1.size(), numWorlds)
    << "Model must have been loaded in all worlds";
  for (std::vector<ModelLoadResult>::iterator it = res1.begin();
       it != res1.end(); ++it)
  {
    const ModelLoadResult& mlRes=*it;
    ASSERT_EQ(mlRes.opResult, collision_benchmark::SUCCESS)
      << "Could not load model";
    ASSERT_EQ(mlRes.modelID, modelName1)
      << "Model names should be equal";
  }

  // Load model 2
  std::string modelName2 = "model2";
  Shape::Ptr shape2(PrimitiveShape::CreateCylinder(1,3));
  std::vector<ModelLoadResult> res2
    = worldManager->AddModelFromShape(modelName2, shape2, shape2);
  ASSERT_EQ(res2.size(), numWorlds)
    << "Model must have been loaded in all worlds";
  for (std::vector<ModelLoadResult>::iterator it = res2.begin();
       it != res2.end(); ++it)
  {
    const ModelLoadResult& mlRes=*it;
    ASSERT_EQ(mlRes.opResult, collision_benchmark::SUCCESS)
      << "Could not load model";
    ASSERT_EQ(mlRes.modelID, modelName2)
      << "Model names should be equal";
  }

  worldManager->SetDynamicsEnabled(false);
  worldManager->SetPaused(true);

  std::cout << "Now start gzclient if you would like "
            << "to view the test. "<<std::endl;
  std::cout << "Press [Enter] to continue without gzclient or hit "
            << "the play button in gzclient."<<std::endl;
  getchar();

  worldManager->SetPaused(false);

  // set models to their initial pose
  BasicState bstate1, bstate2;
  bstate1.SetPosition(Vector3(1,0,0));
  bstate2.SetPosition(Vector3(-1,0,0));
  int cnt1 = worldManager->SetBasicModelState(modelName1, bstate1);
  int cnt2 = worldManager->SetBasicModelState(modelName2, bstate2);
  ASSERT_EQ(cnt1, numWorlds) << "All worlds should have been updated";
  ASSERT_EQ(cnt2, numWorlds) << "All worlds should have been updated";

  // start the update loop
  std::cout << "Now starting to update worlds."<<std::endl;
  int msSleep = 1000;  // delay for running the test
  while(true)
  {
    std::cout<<"UPDATE"<<std::endl;
    int numSteps=1;
    worldManager->Update(numSteps);
    if (msSleep > 0) gazebo::common::Time::MSleep(msSleep);

    bstate1.position.x-=0.1;
    bstate2.position.x+=0.1;
    cnt1 = worldManager->SetBasicModelState(modelName1, bstate1);
    cnt2 = worldManager->SetBasicModelState(modelName2, bstate2);
    ASSERT_EQ(cnt1, numWorlds) << "All worlds should have been updated";
    ASSERT_EQ(cnt2, numWorlds) << "All worlds should have been updated";
  }
}

int main(int argc, char**argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
