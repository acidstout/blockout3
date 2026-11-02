/*
  File:        PolyCubeData.cpp
  Description: The 41 polycubes of the game
  Program:     Block Out III
  Based on:    BlockOut II 2.5 by Jean-Luc PONS

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/
#include "PolyCube.h"

namespace bo3 {

void InitAllPolyCubes(PolyCube *allPolyCube) {

  // Distinct orientations of each polycube (needed by the AI player)
#include "BlockOrientation.h"


  allPolyCube[0].AddCube(0,0,0);
  allPolyCube[0].SetInfo(156,14,true,false);

  allPolyCube[1].AddCube(0,0,0);
  allPolyCube[1].AddCube(0,1,0);
  allPolyCube[1].SetInfo(156,14,true,false);

  allPolyCube[2].AddCube(0,0,0);
  allPolyCube[2].AddCube(0,1,0);
  allPolyCube[2].AddCube(0,2,0);
  allPolyCube[2].SetInfo(307,27,true,false);

  allPolyCube[3].AddCube(0,0,0);
  allPolyCube[3].AddCube(0,1,0);
  allPolyCube[3].AddCube(0,2,0);
  allPolyCube[3].AddCube(0,3,0);
  allPolyCube[3].SetInfo(307,27,false,false);

  allPolyCube[4].AddCube(0,0,0);
  allPolyCube[4].AddCube(0,1,0);
  allPolyCube[4].AddCube(0,2,0);
  allPolyCube[4].AddCube(0,3,0);
  allPolyCube[4].AddCube(0,4,0);
  allPolyCube[4].SetInfo(624,53,false,false);

  allPolyCube[5].AddCube(0,0,0);
  allPolyCube[5].AddCube(1,0,0);
  allPolyCube[5].AddCube(1,1,0);
  allPolyCube[5].SetInfo(307,27,true,true);

  allPolyCube[6].AddCube(0,0,0);
  allPolyCube[6].AddCube(1,0,0);
  allPolyCube[6].AddCube(0,1,0);
  allPolyCube[6].AddCube(1,1,0);
  allPolyCube[6].SetInfo(156,14,true,false);

  allPolyCube[7].AddCube(0,0,0);
  allPolyCube[7].AddCube(1,0,0);
  allPolyCube[7].AddCube(2,0,0);
  allPolyCube[7].AddCube(1,1,0);
  allPolyCube[7].SetInfo(461,40,true,true);

  allPolyCube[8].AddCube(1,0,0);
  allPolyCube[8].AddCube(2,0,0);
  allPolyCube[8].AddCube(0,1,0);
  allPolyCube[8].AddCube(1,1,0);
  allPolyCube[8].SetInfo(461,40,true,true);

  allPolyCube[9].AddCube(0,0,0);
  allPolyCube[9].AddCube(1,0,0);
  allPolyCube[9].AddCube(2,0,0);
  allPolyCube[9].AddCube(2,1,0);
  allPolyCube[9].SetInfo(307,27,true,true);

  allPolyCube[10].AddCube(0,0,0);
  allPolyCube[10].AddCube(1,0,0);
  allPolyCube[10].AddCube(2,0,0);
  allPolyCube[10].AddCube(2,1,0);
  allPolyCube[10].AddCube(2,2,0);
  allPolyCube[10].SetInfo(921,79,false,false);

  allPolyCube[11].AddCube(2,0,0);
  allPolyCube[11].AddCube(0,1,0);
  allPolyCube[11].AddCube(1,1,0);
  allPolyCube[11].AddCube(2,1,0);
  allPolyCube[11].AddCube(2,2,0);
  allPolyCube[11].SetInfo(921,79,false,false);

  allPolyCube[12].AddCube(2,0,0);
  allPolyCube[12].AddCube(0,1,0);
  allPolyCube[12].AddCube(1,1,0);
  allPolyCube[12].AddCube(2,1,0);
  allPolyCube[12].AddCube(1,2,0);
  allPolyCube[12].SetInfo(921,79,false,false);

  allPolyCube[13].AddCube(1,0,0);
  allPolyCube[13].AddCube(2,0,0);
  allPolyCube[13].AddCube(1,1,0);
  allPolyCube[13].AddCube(0,2,0);
  allPolyCube[13].AddCube(1,2,0);
  allPolyCube[13].SetInfo(921,79,false,false);

  allPolyCube[14].AddCube(0,0,0);
  allPolyCube[14].AddCube(1,0,0);
  allPolyCube[14].AddCube(1,1,0);
  allPolyCube[14].AddCube(2,1,0);
  allPolyCube[14].AddCube(2,2,0);
  allPolyCube[14].SetInfo(780,66,false,false);

  allPolyCube[15].AddCube(0,0,0);
  allPolyCube[15].AddCube(1,0,0);
  allPolyCube[15].AddCube(1,1,0);
  allPolyCube[15].AddCube(1,2,0);
  allPolyCube[15].AddCube(1,3,0);
  allPolyCube[15].SetInfo(780,66,false,false);

  allPolyCube[16].AddCube(1,0,0);
  allPolyCube[16].AddCube(0,1,0);
  allPolyCube[16].AddCube(1,1,0);
  allPolyCube[16].AddCube(1,2,0);
  allPolyCube[16].AddCube(1,3,0);
  allPolyCube[16].SetInfo(921,79,false,false);

  allPolyCube[17].AddCube(0,0,0);
  allPolyCube[17].AddCube(1,0,0);
  allPolyCube[17].AddCube(1,1,0);
  allPolyCube[17].AddCube(1,2,0);
  allPolyCube[17].AddCube(0,2,0);
  allPolyCube[17].SetInfo(1248,105,false,false);

  allPolyCube[18].AddCube(0,0,0);
  allPolyCube[18].AddCube(1,0,0);
  allPolyCube[18].AddCube(0,1,0);
  allPolyCube[18].AddCube(1,1,0);
  allPolyCube[18].AddCube(1,2,0);
  allPolyCube[18].SetInfo(461,40,false,false);

  allPolyCube[19].AddCube(1,0,0);
  allPolyCube[19].AddCube(1,1,0);
  allPolyCube[19].AddCube(0,1,0);
  allPolyCube[19].AddCube(0,2,0);
  allPolyCube[19].AddCube(0,3,0);
  allPolyCube[19].SetInfo(921,79,false,false);

  allPolyCube[20].AddCube(1,0,0);
  allPolyCube[20].AddCube(0,1,0);
  allPolyCube[20].AddCube(1,1,0);
  allPolyCube[20].AddCube(2,1,0);
  allPolyCube[20].AddCube(1,2,0);
  allPolyCube[20].SetInfo(1402,118,false,false);

  allPolyCube[21].AddCube(0,0,0);
  allPolyCube[21].AddCube(0,0,1);
  allPolyCube[21].AddCube(1,0,1);
  allPolyCube[21].AddCube(1,1,1);
  allPolyCube[21].AddCube(1,2,1);
  allPolyCube[21].SetInfo(1379,131,false,false);

  allPolyCube[22].AddCube(1,0,0);
  allPolyCube[22].AddCube(0,0,1);
  allPolyCube[22].AddCube(1,0,1);
  allPolyCube[22].AddCube(0,1,1);
  allPolyCube[22].AddCube(0,2,1);
  allPolyCube[22].SetInfo(1379,131,false,false);

  allPolyCube[23].AddCube(0,1,0);
  allPolyCube[23].AddCube(0,1,1);
  allPolyCube[23].AddCube(1,0,1);
  allPolyCube[23].AddCube(1,1,1);
  allPolyCube[23].AddCube(1,2,1);
  allPolyCube[23].SetInfo(965,92,false,false);

  allPolyCube[24].AddCube(1,1,0);
  allPolyCube[24].AddCube(1,0,1);
  allPolyCube[24].AddCube(1,1,1);
  allPolyCube[24].AddCube(1,2,1);
  allPolyCube[24].AddCube(0,2,1);
  allPolyCube[24].SetInfo(965,92,false,false);

  allPolyCube[25].AddCube(1,1,0);
  allPolyCube[25].AddCube(0,0,1);
  allPolyCube[25].AddCube(1,0,1);
  allPolyCube[25].AddCube(1,1,1);
  allPolyCube[25].AddCube(1,2,1);
  allPolyCube[25].SetInfo(965,92,false,false);

  allPolyCube[26].AddCube(1,0,0);
  allPolyCube[26].AddCube(1,0,1);
  allPolyCube[26].AddCube(1,1,1);
  allPolyCube[26].AddCube(0,1,1);
  allPolyCube[26].AddCube(0,2,1);
  allPolyCube[26].SetInfo(1379,131,false,false);

  allPolyCube[27].AddCube(0,0,0);
  allPolyCube[27].AddCube(0,0,1);
  allPolyCube[27].AddCube(0,1,1);
  allPolyCube[27].AddCube(1,1,1);
  allPolyCube[27].AddCube(1,2,1);
  allPolyCube[27].SetInfo(1379,131,false,false);

  allPolyCube[28].AddCube(1,1,0);
  allPolyCube[28].AddCube(1,2,0);
  allPolyCube[28].AddCube(0,0,1);
  allPolyCube[28].AddCube(0,1,1);
  allPolyCube[28].AddCube(1,1,1);
  allPolyCube[28].SetInfo(1103,105,false,false);

  allPolyCube[29].AddCube(0,1,0);
  allPolyCube[29].AddCube(0,2,0);
  allPolyCube[29].AddCube(1,0,1);
  allPolyCube[29].AddCube(1,1,1);
  allPolyCube[29].AddCube(0,1,1);
  allPolyCube[29].SetInfo(1103,105,false,false);

  allPolyCube[30].AddCube(1,0,0);
  allPolyCube[30].AddCube(1,0,1);
  allPolyCube[30].AddCube(1,1,1);
  allPolyCube[30].AddCube(1,2,1);
  allPolyCube[30].AddCube(0,2,1);
  allPolyCube[30].SetInfo(1379,131,false,false);

  allPolyCube[31].AddCube(0,0,0);
  allPolyCube[31].AddCube(0,0,1);
  allPolyCube[31].AddCube(0,1,1);
  allPolyCube[31].AddCube(0,2,1);
  allPolyCube[31].AddCube(1,2,1);
  allPolyCube[31].SetInfo(1379,131,false,false);

  allPolyCube[32].AddCube(1,0,0);
  allPolyCube[32].AddCube(1,0,1);
  allPolyCube[32].AddCube(0,0,1);
  allPolyCube[32].AddCube(0,1,1);
  allPolyCube[32].SetInfo(552,53,false,true);

  allPolyCube[33].AddCube(1,0,0);
  allPolyCube[33].AddCube(1,0,1);
  allPolyCube[33].AddCube(0,1,1);
  allPolyCube[33].AddCube(1,1,1);
  allPolyCube[33].SetInfo(552,53,false,true);

  allPolyCube[34].AddCube(1,0,0);
  allPolyCube[34].AddCube(0,0,1);
  allPolyCube[34].AddCube(1,0,1);
  allPolyCube[34].AddCube(1,1,1);
  allPolyCube[34].SetInfo(552,53,false,true);

  allPolyCube[35].AddCube(1,1,0);
  allPolyCube[35].AddCube(0,1,1);
  allPolyCube[35].AddCube(1,1,1);
  allPolyCube[35].AddCube(1,0,1);
  allPolyCube[35].AddCube(1,2,1);
  allPolyCube[35].SetInfo(827,79,false,false);

  allPolyCube[36].AddCube(1,0,0);
  allPolyCube[36].AddCube(0,0,1);
  allPolyCube[36].AddCube(1,0,1);
  allPolyCube[36].AddCube(0,1,1);
  allPolyCube[36].AddCube(1,1,1);
  allPolyCube[36].SetInfo(461,40,false,false);

  allPolyCube[37].AddCube(1,0,0);
  allPolyCube[37].AddCube(0,0,1);
  allPolyCube[37].AddCube(1,0,1);
  allPolyCube[37].AddCube(1,1,1);
  allPolyCube[37].AddCube(1,2,1);
  allPolyCube[37].SetInfo(1103,105,false,false);

  allPolyCube[38].AddCube(1,0,0);
  allPolyCube[38].AddCube(1,1,0);
  allPolyCube[38].AddCube(0,1,1);
  allPolyCube[38].AddCube(1,1,1);
  allPolyCube[38].AddCube(1,2,1);
  allPolyCube[38].SetInfo(1103,105,false,false);

  allPolyCube[39].AddCube(1,1,0);
  allPolyCube[39].AddCube(1,2,0);
  allPolyCube[39].AddCube(1,0,1);
  allPolyCube[39].AddCube(1,1,1);
  allPolyCube[39].AddCube(0,1,1);
  allPolyCube[39].SetInfo(1103,105,false,false);

  allPolyCube[40].AddCube(0,0,0);
  allPolyCube[40].AddCube(1,1,0);
  allPolyCube[40].AddCube(0,0,1);
  allPolyCube[40].AddCube(0,1,1);
  allPolyCube[40].AddCube(1,1,1);
  allPolyCube[40].SetInfo(1379,131,false,false);

  for(int i=0;i<NB_POLYCUBE;i++)
    allPolyCube[i].InitRotationCenter();

}

} // namespace bo3
