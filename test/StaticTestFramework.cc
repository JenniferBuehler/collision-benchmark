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
#include <test/StaticTestFramework.hh>

#include <collision_benchmark/PrimitiveShape.hh>
#include <collision_benchmark/SimpleTriMeshShape.hh>
#include <collision_benchmark/BasicTypes.hh>
#include <collision_benchmark/Helpers.hh>

#include <ignition/math/Vector3.hh>

#include <gazebo/gazebo.hh>
#include <gazebo/msgs/msgs.hh>


#include <sstream>
#include <thread>
#include <atomic>

using collision_benchmark::Shape;
using collision_benchmark::BasicState;
using collision_benchmark::Vector3;
using collision_benchmark::Quaternion;
using collision_benchmark::PhysicsWorldBaseInterface;

////////////////////////////////////////////////////////////////
void StaticTestFramework::AABBTestWorldsAgreement(const std::string &modelName1,
                                   const std::string &modelName2,
                                   const float cellSizeFactor,
                                   const double minAgree,
                                   const double bbTol,
                                   const double zeroDepthTol,
                                   const bool interactive,
                                   const std::string &outputBasePath,
                                   const std::string &outputSubdir)
{
  ASSERT_GT(cellSizeFactor, 1e-07) << "Cell size factor too small";

  GzMultipleWorldsServer::Ptr mServer = GetServer();
  ASSERT_NE(mServer.get(), nullptr) << "Could not create and start server";
  GzWorldManager::Ptr worldManager = mServer->GetWorldManager();
  ASSERT_NE(worldManager.get(), nullptr) << "No valid world manager created";

  worldManager->SetDynamicsEnabled(false);
  worldManager->SetPaused(false);

  int numWorlds = worldManager->GetNumWorlds();

  // set models to their initial pose

  // First, place models at the origin in default orientation
  // (in case they were loaded form SDF they may have a pose different
  // to the origin, but here we want to start them at the origin).
  BasicState originPose;
  originPose.SetPosition(Vector3(0, 0, 0));
  originPose.SetRotation(Quaternion(0, 0, 0, 1));
  int cnt1 = worldManager->SetBasicModelState(modelName1, originPose);
  ASSERT_EQ(cnt1, numWorlds) << "All worlds should have been updated";
  int cnt2 = worldManager->SetBasicModelState(modelName2, originPose);
  ASSERT_EQ(cnt2, numWorlds) << "All worlds should have been updated";

  // now, get the model AABBs and displace model 2 relative to model 1
  collision_benchmark::GzAABB aabb1, aabb2;
  ASSERT_TRUE(GetAABBs(modelName1, modelName2, bbTol, aabb1, aabb2));

  // std::cout << "Got AABB 1: " <<  aabb1.min << ", "
  //           << aabb1.max << std::endl;
  // std::cout << "Got AABB 2: " <<  aabb2.min << ", "
  //           << aabb2.max << std::endl;

  collision_benchmark::GzAABB grid = aabb1;
  grid.min -= aabb2.size() / 2;
  grid.max += aabb2.size() / 2;

  // place model 2 at start position
  BasicState bstate2;
  bstate2.SetPosition(Vector3(grid.min.X(), grid.min.Y(), grid.min.Z()));
  int cnt = worldManager->SetBasicModelState(modelName2, bstate2);
  ASSERT_EQ(cnt, numWorlds) << "All worlds should have been updated";

  const float cellSizeX = grid.size().X() * cellSizeFactor;
  const float cellSizeY = grid.size().Y() * cellSizeFactor;
  const float cellSizeZ = grid.size().Z() * cellSizeFactor;
  /* std::cout << "GRID : " <<  grid.min << ", " << grid.max << std::endl;
  std::cout << "cell size : " <<  cellSizeX << ", " <<cellSizeY << ", "
            << cellSizeZ << std::endl; */

  if (interactive)
  {
    std::cout << "Check that gzclient is up and then press [Enter] to continue." << std::endl;
    getchar();
  }

  // start the update loop
  std::cout << "Now starting to update worlds." << std::endl;

  int msSleep = 0;  // delay for running the test
  double eps = 1e-07;
  unsigned int itCnt = 0;
  unsigned int failCnt = 0;
  for (double x = grid.min.X(); x < grid.max.X()+eps; x += cellSizeX)
  for (double y = grid.min.Y(); y < grid.max.Y()+eps; y += cellSizeY)
  for (double z = grid.min.Z(); z < grid.max.Z()+eps; z += cellSizeZ)
  {
    ++itCnt;
    // std::cout << "Placing model 2 at " << x<<", " << y<<", " << z<<std::endl;
    bstate2.position.x = x;
    bstate2.position.y = y;
    bstate2.position.z = z;
    cnt = worldManager->SetBasicModelState(modelName2, bstate2);
    ASSERT_EQ(cnt, numWorlds) << "All worlds should have been updated";

    int numSteps = 1;
    worldManager->Update(numSteps);
    if (msSleep > 0) gazebo::common::Time::MSleep(msSleep);

    std::vector<std::string> colliding, notColliding;
    double maxContactDepth;
    ASSERT_TRUE(collision_benchmark::CollisionState(modelName1, modelName2,
                                                    worldManager, colliding,
                                                    notColliding,
                                                    maxContactDepth));
# if 0
    // For TESTING: stop at every colliding state
    int stopX = 5;
    if (!colliding.empty()&& ((itCnt % stopX) == 0))
    {
      std::stringstream str;
      str << std::endl << "Colliding: " << std::endl << " ------ " << std::endl;
      for (std::vector<std::string>::iterator it = colliding.begin();
           it != colliding.end(); ++it)
      {
        if (it != colliding.begin()) str << std::endl;
        std::vector<GzContactInfoPtr> contacts =
          collision_benchmark::GetContactInfo(modelName1, modelName2,
                                              *it, worldManager);
        str << *it << ": " << VectorPtrToString(contacts);
      }
      RefreshClient(5);
      collision_benchmark::UpdateUntilEnter(worldManager);
    }
#endif

    if (!colliding.empty() && (fabs(maxContactDepth) < zeroDepthTol))
    {
      // if contacts were found but they are just surface contacts,
      // skip this because engines are actually allowed to disagree.
      // std::cout << "DEBUG-INFO: Not considering case of maximum depth 0 "
      //          << "because this is a borderline case" << std::endl;
      continue;
    }

    size_t total = colliding.size() + notColliding.size();

    ASSERT_EQ(numWorlds, total) << "All worlds must have voted";
    ASSERT_GT(total, 0) << "This should have been caught before";

    double negative = notColliding.size() / static_cast<double>(total);
    double positive= colliding.size() / static_cast<double>(total);

    if (((positive > negative) && (positive < minAgree)) ||
        ((positive <= negative) && (negative < minAgree)))
    {
      std::stringstream str;
      std::cout << "FAIL " << failCnt << ": Minimum agreement not reached. "
                << "Agreement: " << positive << ", " << negative << std::endl;

      // str << " Collision: "<< VectorToString(colliding) << ", no collision: "
      //     << VectorToString(notColliding) << ".";

      str << "------ " << std::endl;
      str << "Colliding: " << std::endl
          << "------ " << std::endl;
      for (std::vector<std::string>::iterator it = colliding.begin();
           it != colliding.end(); ++it)
      {
        if (it != colliding.begin()) str << std::endl;
        std::vector<GzContactInfoPtr> contacts =
          collision_benchmark::GetContactInfo(modelName1, modelName2,
                                              *it, worldManager);
        str << *it << ": " << VectorPtrToString(contacts);
      }

      str << std::endl;
      str << "------ " << std::endl;
      str << "Not colliding: " << std::endl
          << "------ " << std::endl;
      for (std::vector<std::string>::iterator it = notColliding.begin();
           it != notColliding.end(); ++it)
      {
        if (it != notColliding.begin()) str << std::endl;
        std::vector<GzContactInfoPtr> contacts =
          collision_benchmark::GetContactInfo(modelName1, modelName2,
                                              *it, worldManager);
        str << *it << ": " << VectorPtrToString(contacts);
      }
      str << std::endl;

      if (!outputBasePath.empty() &&
          collision_benchmark::makeDirectoryIfNeeded(outputBasePath+
                                                     "/"+outputSubdir))
      {
        std::stringstream namePrefix;
        namePrefix << "STest_fail_" << failCnt << "_";
        int nFails = worldManager->SaveAllWorlds(outputBasePath,
                                                 outputSubdir,
                                                 namePrefix.str(),
                                                 "world", true);
        std::cout << "Worlds written to " << outputBasePath
                  << "/" << outputSubdir
                  << " (failed: "<< nFails << ")" <<std::endl;
      }

      if (interactive)
      {
        std::cout << str.str() << std::endl
                  << "Press [Enter] to continue." << std::endl;
        RefreshClient(5);
        collision_benchmark::UpdateUntilEnter(worldManager);
      }
      else
      {
        // trigger a test failure
        EXPECT_TRUE(false) << str.str();
      }
      ++failCnt;
    }
  }
  std::cout << "TwoModels test finished. " << std::endl;
}
